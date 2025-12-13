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
#include <Update.h>
#endif

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
// OTA UPDATE CALLBACKS
// ============================================

#if ENABLE_SINRIC_OTA

// OTA Start callback - called when OTA update begins
void onOTAStart() {
  LOG_INFO("========================================");
  LOG_INFO("OTA UPDATE STARTING");
  LOG_INFO("========================================");

  // Set OTA in progress flag
  state.otaInProgress = true;
  state.otaStartTime = millis();

  // CRITICAL SAFETY: Disable heater immediately
  LOG_WARN("SAFETY: Disabling heater for OTA update");
  setHeaterEnabledState(false);
  digitalWrite(HEATER_PIN, LOW);

  // Save heater disabled state
  saveHeaterEnabled(false);

  // Force pump to OFF if safe (OTA should be quick)
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

  LOG_INFO("Hardware secured - ready for OTA update");
}

// OTA Progress callback - called periodically during update
void onOTAProgress(unsigned int progress, unsigned int total) {
  static unsigned int lastPercent = 0;
  unsigned int percent = (progress * 100) / total;

  // Only log every 10%
  if (percent != lastPercent && percent % 10 == 0) {
    LOG_INFOF("OTA Progress: %u%%", percent);
    lastPercent = percent;
  }

  // Check for timeout
  unsigned long elapsed = millis() - state.otaStartTime;
  if (elapsed > OTA_UPDATE_TIMEOUT) {
    LOG_ERROR("OTA update timeout - aborting");
    Update.abort();
  }
}

// OTA End callback - called when OTA update completes
void onOTAEnd(bool success) {
  state.otaInProgress = false;

  if (success) {
    LOG_INFO("========================================");
    LOG_INFO("OTA UPDATE SUCCESSFUL!");
    LOG_INFO("System will restart in 3 seconds...");
    LOG_INFO("========================================");

    // Save runtime state before restart
    saveRuntimeState();

    // Wait for user to see message
    delay(3000);

    // Restart ESP32
    ESP.restart();
  } else {
    LOG_ERROR("========================================");
    LOG_ERROR("OTA UPDATE FAILED!");
    LOG_ERRORF("Error: %s", Update.errorString());
    LOG_ERROR("System returning to normal operation");
    LOG_ERROR("========================================");

    // Update LCD to show failure
    updateLCDDisplay();

    // System will continue with old firmware
    // User can retry OTA update
  }
}

// OTA Error callback - called if OTA update encounters an error
void onOTAError(int error) {
  LOG_ERRORF("OTA Error Code: %d", error);
  LOG_ERRORF("Error Description: %s", Update.errorString());

  state.otaInProgress = false;
  updateLCDDisplay();
}

#endif // ENABLE_SINRIC_OTA

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

#if ENABLE_SINRIC_OTA
  // Setup OTA update callbacks
  LOG_INFO("Enabling SinricPro OTA updates");
  SinricPro.onOTAStart(onOTAStart);
  SinricPro.onOTAProgress(onOTAProgress);
  SinricPro.onOTAEnd(onOTAEnd);
  SinricPro.onOTAError(onOTAError);
  LOG_INFO("OTA callbacks registered");
#else
  LOG_WARN("SinricPro OTA updates disabled (ENABLE_SINRIC_OTA=0)");
#endif

  // Connect to Sinric Pro
  SinricPro.begin(getSinricAppKey(), getSinricAppSecret());

  LOG_INFO("Sinric Pro initialized");
}

// ============================================
// EVENT HANDLING
// ============================================

void handleSinricEvents() {
  SinricPro.handle();
  
  // Update connection status
  state.sinricConnected = SinricPro.isConnected();
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
