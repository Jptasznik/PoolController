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
// OTA support is provided by SinricPro library
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
// OTA UPDATE CALLBACK
// ============================================

#if ENABLE_SINRIC_OTA

// OTA Update callback - called when SinricPro sends OTA command
void onOTAUpdate() {
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

  // Save runtime state before OTA
  saveRuntimeState();

  LOG_INFO("Hardware secured - ready for OTA update");
  LOG_INFO("SinricPro will now handle the update process");
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

  // Connect to Sinric Pro
  SinricPro.begin(getSinricAppKey(), getSinricAppSecret());

  LOG_INFO("Sinric Pro initialized");

#if ENABLE_SINRIC_OTA
  // Register OTA update callback with SinricPro
  SinricPro.onOTAUpdate(onOTAUpdate);
  LOG_INFO("SinricPro OTA update callback registered");
#else
  LOG_WARN("SinricPro OTA updates disabled (ENABLE_SINRIC_OTA=0)");
#endif
}

// ============================================
// EVENT HANDLING
// ============================================

void handleSinricEvents() {
  SinricPro.handle();

  // Update connection status
  state.sinricConnected = SinricPro.isConnected();
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
