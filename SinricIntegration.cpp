#define LOG_MODULE "SINRIC"

#include "SinricIntegration.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"
#include "CredentialsManager.h"
#include "SettingsManager.h"
#include "LCDDisplay.h"
#include <SinricPro.h>
#include <SinricProThermostat.h>
#include <SinricProDimSwitch.h>

#if ENABLE_SINRIC_OTA
#include "ESP32OTAHelper.h"
#include "SemVer.h"
#endif

#ifdef ENABLE_SINRIC

// ============================================
// ALEXA THERMOSTAT CALLBACKS
// ============================================

bool onPowerStateThermostat(const String &deviceId, bool &state) {
  LOG_INFOF("Alexa: Thermostat power %s", state ? "ON" : "OFF");

  setHeaterEnabledState(state);
  return true;
}

bool onTargetTemperature(const String &deviceId, float &temperature) {
  LOG_INFOF("Alexa: Set target temperature %.1f", temperature);

  setTargetTemperature(temperature);
  return true;
}

bool onAdjustTargetTemperature(const String &deviceId, float &temperatureDelta) {
  LOG_INFOF("Alexa: Adjust temperature %.1f", temperatureDelta);

  setTargetTemperature(state.targetTemp + temperatureDelta);
  return true;
}

bool onThermostatMode(const String &deviceId, String &mode) {
  LOG_INFOF("Alexa: Thermostat mode %s", mode.c_str());

  if (mode == "HEAT") {
    setHeaterEnabledState(true);
  } else if (mode == "OFF") {
    setHeaterEnabledState(false);
  }

  return true;
}

// ============================================
// ALEXA PUMP (DIMSWITCH) CALLBACKS
// ============================================

bool onPowerStatePump(const String &deviceId, bool &state) {
  LOG_INFOF("Alexa: Pump power %s", state ? "ON" : "OFF");

  if (state) {
    setPumpState(PUMP_LOW);
  } else {
    if (canPumpTurnOff()) {
      setPumpState(PUMP_OFF);
    } else {
      LOG_WARN("Cannot turn pump OFF (safety interlock)");
      return false;
    }
  }

  return true;
}

bool onPowerLevelPump(const String &deviceId, int &powerLevel) {
  LOG_INFOF("Alexa: Pump power level %d", powerLevel);

  if (powerLevel == 0) {
    if (canPumpTurnOff()) {
      setPumpState(PUMP_OFF);
    } else {
      LOG_WARN("Cannot turn pump OFF (safety interlock)");
      return false;
    }
  } else if (powerLevel <= 50) {
    setPumpState(PUMP_LOW);
  } else {
    setPumpState(PUMP_HIGH);
  }

  return true;
}

bool onAdjustPowerLevelPump(const String &deviceId, int &powerLevelDelta) {
  LOG_INFOF("Alexa: Adjust pump power level %d", powerLevelDelta);

  // Convert current state to power level
  int currentLevel = 0;
  if (state.pumpState == PUMP_LOW) currentLevel = 25;
  if (state.pumpState == PUMP_HIGH) currentLevel = 75;

  int newLevel = currentLevel + powerLevelDelta;
  if (newLevel < 0) newLevel = 0;
  if (newLevel > 100) newLevel = 100;

  return onPowerLevelPump(deviceId, newLevel);
}

// ============================================
// OTA UPDATE CALLBACK
// ============================================

#if ENABLE_SINRIC_OTA

// OTA Update callback - called when SinricPro sends OTA command
// Returns: true if update initiated successfully, false otherwise
bool handleOTAUpdate(const String& url, int major, int minor, int patch, bool forceUpdate) {
  LOG_INFO("========================================");
  LOG_INFO("OTA UPDATE REQUEST RECEIVED");
  LOG_INFO("========================================");

  // Version comparison
  Version currentVersion = Version(FIRMWARE_VERSION);
  Version newVersion = Version(String(major) + "." + String(minor) + "." + String(patch));
  bool updateAvailable = newVersion > currentVersion;

  LOG_INFOF("URL: %s", url.c_str());
  LOG_INFOF("Current version: %s", currentVersion.toString().c_str());
  LOG_INFOF("New version: %s", newVersion.toString().c_str());
  if (forceUpdate) LOG_INFO("Force update flag is SET");

  // Check if update should proceed
  if (!forceUpdate && !updateAvailable) {
    String result = "Current version is up to date";
    SinricPro.setResponseMessage(std::move(result));
    LOG_INFO(result.c_str());
    return false;
  }

  if (updateAvailable) {
    LOG_INFO("Update available - proceeding with OTA");
  } else {
    LOG_INFO("Forced update - proceeding with OTA");
  }

  // Set OTA in progress flag
  state.otaInProgress = true;
  state.otaStartTime = millis();
  snprintf(state.otaVersion, sizeof(state.otaVersion), "%d.%d.%d", major, minor, patch);

  // CRITICAL SAFETY: Disable heater immediately
  LOG_WARN("SAFETY: Disabling heater for OTA update");
  setHeaterEnabledState(false);
  digitalWrite(HEATER_PIN, LOW);
  saveHeaterEnabled(false);

  // Force pump to OFF if safe
  if (canPumpTurnOff()) {
    LOG_INFO("SAFETY: Turning pump OFF for OTA update");
    setPumpState(PUMP_OFF);
  } else {
    LOG_WARN("Pump remains ON during OTA (safety interlock)");
  }

  // Display OTA update on LCD
  displayOTAUpdate();

  // Small delay for hardware to stabilize
  delay(OTA_SAFETY_SHUTDOWN_DELAY);

  // Save runtime state before OTA
  saveRuntimeState();

  LOG_INFO("Hardware secured - starting OTA download");

  // Perform the OTA update
  String result = startOtaUpdate(url);

  if (!result.isEmpty()) {
    // Update failed
    LOG_ERRORF("OTA update failed: %s", result.c_str());
    SinricPro.setResponseMessage(std::move(result));

    // Clear OTA in progress flag
    state.otaInProgress = false;

    return false;
  }

  // If we reach here, update succeeded and device will restart
  // This line should never be reached because startOtaUpdate calls ESP.restart()
  return true;
}

#endif // ENABLE_SINRIC_OTA

// ============================================
// CONNECTION CALLBACKS
// ============================================

void onSinricConnected() {
  LOG_INFO("========================================");
  LOG_INFO("Sinric Pro Connected!");
  LOG_INFO("========================================");
  state.sinricConnected = true;
}

void onSinricDisconnected() {
  LOG_WARN("========================================");
  LOG_WARN("Sinric Pro Disconnected!");
  LOG_WARN("========================================");
  state.sinricConnected = false;
}

// ============================================
// INITIALIZATION
// ============================================

void initSinric() {
  if (!hasSinricCredentials()) {
    LOG_WARN("Sinric Pro credentials not configured, skipping Sinric initialization");
    return;
  }

  LOG_INFO("Initializing Sinric Pro...");

  // Setup thermostat device
  SinricProThermostat &thermostat = SinricPro[getSinricThermostatId()];
  thermostat.onPowerState(onPowerStateThermostat);
  thermostat.onTargetTemperature(onTargetTemperature);
  thermostat.onAdjustTargetTemperature(onAdjustTargetTemperature);
  thermostat.onThermostatMode(onThermostatMode);

  // Setup pump device (dimmer switch)
  SinricProDimSwitch &pump = SinricPro[getSinricPumpId()];
  pump.onPowerState(onPowerStatePump);
  pump.onPowerLevel(onPowerLevelPump);
  pump.onAdjustPowerLevel(onAdjustPowerLevelPump);

  // Register connection callbacks
  SinricPro.onConnected(onSinricConnected);
  SinricPro.onDisconnected(onSinricDisconnected);

  // Connect to Sinric Pro
  SinricPro.begin(getSinricAppKey(), getSinricAppSecret());

  // Restore device states from cloud (required for proper connection)
  SinricPro.restoreDeviceStates(true);

  LOG_INFO("Sinric Pro initialized");
  LOG_INFO("Waiting for Sinric Pro connection...");

#if ENABLE_SINRIC_OTA
  // Register OTA update callback with SinricPro
  SinricPro.onOTAUpdate(handleOTAUpdate);
  LOG_INFOF("SinricPro OTA update callback registered (version: %s)", FIRMWARE_VERSION);
#else
  LOG_WARN("SinricPro OTA updates disabled (ENABLE_SINRIC_OTA=0)");
#endif
}

// ============================================
// EVENT HANDLING
// ============================================

void handleSinricEvents() {
  SinricPro.handle();

  // Update connection status and log changes
  bool wasConnected = state.sinricConnected;
  state.sinricConnected = SinricPro.isConnected();

  if (state.sinricConnected && !wasConnected) {
    LOG_INFO("Sinric Pro connected successfully!");
  } else if (!state.sinricConnected && wasConnected) {
    LOG_WARN("Sinric Pro disconnected!");
  }
}

void handleOTA() {
#if ENABLE_SINRIC_OTA
  // SinricPro handles OTA internally after onOTAUpdate callback
  // No additional handling needed in loop - SinricPro.handle() manages OTA
#endif
}

// ============================================
// SYNC TO SINRIC/ALEXA
// ============================================

void syncHeaterToSinric() {
  if (!SinricPro.isConnected()) return;
  if (!hasSinricCredentials()) return;

  SinricProThermostat &thermostat = SinricPro[getSinricThermostatId()];
  thermostat.sendPowerStateEvent(state.heaterEnabled);
  thermostat.sendThermostatModeEvent(state.heaterEnabled ? "HEAT" : "OFF");
}

void syncPumpToSinric() {
  if (!SinricPro.isConnected()) return;
  if (!hasSinricCredentials()) return;

  SinricProDimSwitch &pump = SinricPro[getSinricPumpId()];

  bool powerState = (state.pumpState != PUMP_OFF);
  int powerLevel = 0;
  if (state.pumpState == PUMP_LOW) powerLevel = 25;
  if (state.pumpState == PUMP_HIGH) powerLevel = 75;

  pump.sendPowerStateEvent(powerState);
  pump.sendPowerLevelEvent(powerLevel);
}

void syncTargetTempToSinric() {
  if (!SinricPro.isConnected()) return;
  if (!hasSinricCredentials()) return;

  SinricProThermostat &thermostat = SinricPro[getSinricThermostatId()];
  thermostat.sendTargetTemperatureEvent(state.targetTemp);

  if (state.inletTempValid) {
    thermostat.sendTemperatureEvent(state.inletTemp);
  }
}

#else // ENABLE_SINRIC not defined - provide stub implementations

void initSinric() {
  LOG_WARN("Sinric DISABLED (ENABLE_SINRIC not defined)");
}

void handleSinricEvents() {
  // Sinric disabled - do nothing
}

void handleOTA() {
  // Sinric disabled - do nothing
}

void syncHeaterToSinric() {
  // Sinric disabled - do nothing
}

void syncPumpToSinric() {
  // Sinric disabled - do nothing
}

void syncTargetTempToSinric() {
  // Sinric disabled - do nothing
}

#endif // ENABLE_SINRIC
