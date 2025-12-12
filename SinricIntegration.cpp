#include "SinricIntegration.h"
#include "Config.h"
#include "StateManager.h"
#include "CredentialsManager.h"
#include <SinricPro.h>
#include <SinricProThermostat.h>
#include <SinricProDimSwitch.h>

// ============================================
// ALEXA THERMOSTAT CALLBACKS
// ============================================

bool onPowerStateThermostat(const String &deviceId, bool &state) {
  Serial.print("Alexa: Thermostat power ");
  Serial.println(state ? "ON" : "OFF");
  
  setHeaterEnabledState(state);
  return true;
}

bool onTargetTemperature(const String &deviceId, float &temperature) {
  Serial.print("Alexa: Set target temperature ");
  Serial.println(temperature);
  
  setTargetTemperature(temperature);
  return true;
}

bool onAdjustTargetTemperature(const String &deviceId, float &temperatureDelta) {
  Serial.print("Alexa: Adjust temperature ");
  Serial.println(temperatureDelta);
  
  setTargetTemperature(state.targetTemp + temperatureDelta);
  return true;
}

bool onThermostatMode(const String &deviceId, String &mode) {
  Serial.print("Alexa: Thermostat mode ");
  Serial.println(mode);
  
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
  Serial.print("Alexa: Pump power ");
  Serial.println(state ? "ON" : "OFF");
  
  if (state) {
    setPumpState(PUMP_LOW);
  } else {
    if (canPumpTurnOff()) {
      setPumpState(PUMP_OFF);
    } else {
      Serial.println("Cannot turn pump OFF (safety interlock)");
      return false;
    }
  }
  
  return true;
}

bool onPowerLevelPump(const String &deviceId, int &powerLevel) {
  Serial.print("Alexa: Pump power level ");
  Serial.println(powerLevel);
  
  if (powerLevel == 0) {
    if (canPumpTurnOff()) {
      setPumpState(PUMP_OFF);
    } else {
      Serial.println("Cannot turn pump OFF (safety interlock)");
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
  Serial.print("Alexa: Adjust pump power level ");
  Serial.println(powerLevelDelta);
  
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
// INITIALIZATION
// ============================================

void initSinric() {
  if (!hasSinricCredentials()) {
    Serial.println("WARNING: Sinric Pro credentials not configured, skipping Sinric initialization");
    return;
  }

  Serial.println("Initializing Sinric Pro...");

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

  Serial.println("Sinric Pro initialized");
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
