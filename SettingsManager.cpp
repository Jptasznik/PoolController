#include "SettingsManager.h"
#include "Config.h"
#include <Preferences.h>

Preferences preferences;

// ============================================
// INITIALIZATION
// ============================================

void initSettings() {
  // Settings manager ready - preferences opened/closed per transaction
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
  state.currentOffset = loadCurrentCalibration();

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
  Serial.print("  Current calibration: ");
  Serial.println(state.currentOffset, 3);
}

// ============================================
// SAVE FUNCTIONS
// ============================================

void saveTargetTemp(float temp) {
  preferences.begin("pool", false);
  preferences.putFloat("targetTemp", temp);
  preferences.end();
  Serial.print("Saved target temp: ");
  Serial.println(temp, 1);
}

void saveHeaterEnabled(bool enabled) {
  preferences.begin("pool", false);
  preferences.putBool("heaterEn", enabled);
  preferences.end();
  Serial.print("Saved heater enabled: ");
  Serial.println(enabled);
}

void savePumpState(PumpState state) {
  preferences.begin("pool", false);
  preferences.putUChar("pumpState", (uint8_t)state);
  preferences.end();
  Serial.print("Saved pump state: ");
  Serial.println((int)state);
}

void saveMaxRuntimeHours(float hours) {
  preferences.begin("pool", false);
  preferences.putFloat("maxRuntime", hours);
  preferences.end();
  Serial.print("Saved max runtime: ");
  Serial.println(hours, 1);
}

void saveHeaterStartHour(int hour) {
  preferences.begin("pool", false);
  preferences.putInt("heatStart", hour);
  preferences.end();
  Serial.print("Saved heater start hour: ");
  Serial.println(hour);
}

void saveHeaterEndHour(int hour) {
  preferences.begin("pool", false);
  preferences.putInt("heatEnd", hour);
  preferences.end();
  Serial.print("Saved heater end hour: ");
  Serial.println(hour);
}

// ============================================
// LOAD FUNCTIONS
// ============================================

float loadTargetTemp() {
  preferences.begin("pool", true);  // Read-only
  float value = preferences.getFloat("targetTemp", DEFAULT_TARGET_TEMP);
  preferences.end();
  return value;
}

bool loadHeaterEnabled() {
  preferences.begin("pool", true);  // Read-only
  bool value = preferences.getBool("heaterEn", false);
  preferences.end();
  return value;
}

PumpState loadPumpState() {
  preferences.begin("pool", true);  // Read-only
  uint8_t val = preferences.getUChar("pumpState", (uint8_t)PUMP_OFF);
  preferences.end();
  return (PumpState)val;
}

float loadMaxRuntimeHours() {
  preferences.begin("pool", true);  // Read-only
  float value = preferences.getFloat("maxRuntime", DEFAULT_MAX_RUNTIME_HOURS);
  preferences.end();
  return value;
}

int loadHeaterStartHour() {
  preferences.begin("pool", true);  // Read-only
  int value = preferences.getInt("heatStart", DEFAULT_HEATER_START_HOUR);
  preferences.end();
  return value;
}

int loadHeaterEndHour() {
  preferences.begin("pool", true);  // Read-only
  int value = preferences.getInt("heatEnd", DEFAULT_HEATER_END_HOUR);
  preferences.end();
  return value;
}

// ============================================
// FACTORY RESET
// ============================================

void factoryResetSettings() {
  Serial.println("Performing factory reset...");

  preferences.begin("pool", false);
  preferences.clear();  // Clear all preferences
  preferences.end();

  // Reset to defaults
  state.targetTemp = DEFAULT_TARGET_TEMP;
  state.heaterEnabled = false;
  state.pumpState = PUMP_OFF;
  state.maxRuntimeHours = DEFAULT_MAX_RUNTIME_HOURS;
  state.heaterStartHour = DEFAULT_HEATER_START_HOUR;
  state.heaterEndHour = DEFAULT_HEATER_END_HOUR;

  Serial.println("Factory reset complete");
}

// ============================================
// WIFI FAILURE TRACKING
// ============================================

int loadWiFiFailureCount() {
  preferences.begin("pool", true);  // Read-only
  int count = preferences.getInt("wifiFailCnt", 0);
  preferences.end();
  return count;
}

void saveWiFiFailureCount(int count) {
  preferences.begin("pool", false);
  preferences.putInt("wifiFailCnt", count);
  preferences.end();
  Serial.print("Saved WiFi failure count: ");
  Serial.println(count);
}

void incrementWiFiFailureCount() {
  int count = loadWiFiFailureCount();
  count++;
  saveWiFiFailureCount(count);
}

void resetWiFiFailureCount() {
  saveWiFiFailureCount(0);
}

// ============================================
// CURRENT SENSOR CALIBRATION
// ============================================

float loadCurrentCalibration() {
  preferences.begin("pool", true);  // Read-only
  float offset = preferences.getFloat("currentCal", 0.0);
  preferences.end();
  return offset;
}

void saveCurrentCalibration(float offset) {
  preferences.begin("pool", false);
  preferences.putFloat("currentCal", offset);
  preferences.end();
  Serial.print("Saved current calibration: ");
  Serial.println(offset, 3);
}
