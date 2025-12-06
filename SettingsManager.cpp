#include "SettingsManager.h"
#include "Config.h"
#include <Preferences.h>

Preferences preferences;

// ============================================
// INITIALIZATION
// ============================================

void initSettings() {
  preferences.begin("pool-ctrl", false);  // RW mode
  Serial.println("Settings manager initialized");
}

// ============================================
// LOAD ALL SETTINGS
// ============================================

void loadAllSettings() {
  Serial.println("Loading settings from flash...");
  
  state.targetTemp = loadTargetTemp();
  state.heaterEnabled = loadHeaterEnabled();
  state.pumpState = loadPumpState();
  state.maxRuntimeHours = loadMaxRuntimeHours();
  state.heaterStartHour = loadHeaterStartHour();
  state.heaterEndHour = loadHeaterEndHour();
  
  Serial.println("Settings loaded:");
  Serial.print("  Target temp: ");
  Serial.println(state.targetTemp, 1);
  Serial.print("  Heater enabled: ");
  Serial.println(state.heaterEnabled);
  Serial.print("  Pump state: ");
  Serial.println((int)state.pumpState);
  Serial.print("  Max runtime: ");
  Serial.println(state.maxRuntimeHours, 1);
  Serial.print("  Heater hours: ");
  Serial.print(state.heaterStartHour);
  Serial.print(" - ");
  Serial.println(state.heaterEndHour);
}

// ============================================
// SAVE FUNCTIONS
// ============================================

void saveTargetTemp(float temp) {
  preferences.putFloat("targetTemp", temp);
  Serial.print("Saved target temp: ");
  Serial.println(temp, 1);
}

void saveHeaterEnabled(bool enabled) {
  preferences.putBool("heaterEn", enabled);
  Serial.print("Saved heater enabled: ");
  Serial.println(enabled);
}

void savePumpState(PumpState state) {
  preferences.putUChar("pumpState", (uint8_t)state);
  Serial.print("Saved pump state: ");
  Serial.println((int)state);
}

void saveMaxRuntimeHours(float hours) {
  preferences.putFloat("maxRuntime", hours);
  Serial.print("Saved max runtime: ");
  Serial.println(hours, 1);
}

void saveHeaterStartHour(int hour) {
  preferences.putInt("heatStart", hour);
  Serial.print("Saved heater start hour: ");
  Serial.println(hour);
}

void saveHeaterEndHour(int hour) {
  preferences.putInt("heatEnd", hour);
  Serial.print("Saved heater end hour: ");
  Serial.println(hour);
}

// ============================================
// LOAD FUNCTIONS
// ============================================

float loadTargetTemp() {
  return preferences.getFloat("targetTemp", DEFAULT_TARGET_TEMP);
}

bool loadHeaterEnabled() {
  return preferences.getBool("heaterEn", false);
}

PumpState loadPumpState() {
  uint8_t val = preferences.getUChar("pumpState", (uint8_t)PUMP_OFF);
  return (PumpState)val;
}

float loadMaxRuntimeHours() {
  return preferences.getFloat("maxRuntime", DEFAULT_MAX_RUNTIME_HOURS);
}

int loadHeaterStartHour() {
  return preferences.getInt("heatStart", DEFAULT_HEATER_START_HOUR);
}

int loadHeaterEndHour() {
  return preferences.getInt("heatEnd", DEFAULT_HEATER_END_HOUR);
}

// ============================================
// FACTORY RESET
// ============================================

void factoryResetSettings() {
  Serial.println("Performing factory reset...");
  
  preferences.clear();  // Clear all preferences
  
  // Reset to defaults
  state.targetTemp = DEFAULT_TARGET_TEMP;
  state.heaterEnabled = false;
  state.pumpState = PUMP_OFF;
  state.maxRuntimeHours = DEFAULT_MAX_RUNTIME_HOURS;
  state.heaterStartHour = DEFAULT_HEATER_START_HOUR;
  state.heaterEndHour = DEFAULT_HEATER_END_HOUR;
  
  Serial.println("Factory reset complete");
}
