#include "SettingsManager.h"
#include "Config.h"
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

Preferences preferences;

// Preferences mutex for thread-safe flash access
static SemaphoreHandle_t preferencesMutex = NULL;

// ============================================
// INITIALIZATION
// ============================================

void initSettings() {
  // Create preferences mutex for thread-safe flash access
  preferencesMutex = xSemaphoreCreateMutex();
  if (preferencesMutex == NULL) {
    Serial.println("ERROR: Failed to create preferences mutex!");
  } else {
    Serial.println("Preferences mutex created successfully");
  }

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
  state.minHeaterTemp = loadMinHeaterTemp();
  state.maxHeaterTemp = loadMaxHeaterTemp();
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
  Serial.print("  Min heater temp: ");
  Serial.println(state.minHeaterTemp, 1);
  Serial.print("  Max heater temp: ");
  Serial.println(state.maxHeaterTemp, 1);
  Serial.print("  Current calibration: ");
  Serial.println(state.currentOffset, 3);
}

// ============================================
// SAVE FUNCTIONS
// ============================================

void saveTargetTemp(float temp) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", false);
    preferences.putFloat("targetTemp", temp);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    Serial.print("Saved target temp: ");
    Serial.println(temp, 1);
  } else {
    Serial.println("ERROR: Preferences mutex timeout in saveTargetTemp");
  }
}

void saveHeaterEnabled(bool enabled) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", false);
    preferences.putBool("heaterEn", enabled);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    Serial.print("Saved heater enabled: ");
    Serial.println(enabled);
  } else {
    Serial.println("ERROR: Preferences mutex timeout in saveHeaterEnabled");
  }
}

void savePumpState(PumpState state) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", false);
    preferences.putUChar("pumpState", (uint8_t)state);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    Serial.print("Saved pump state: ");
    Serial.println((int)state);
  } else {
    Serial.println("ERROR: Preferences mutex timeout in savePumpState");
  }
}

void saveMaxRuntimeHours(float hours) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", false);
    preferences.putFloat("maxRuntime", hours);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    Serial.print("Saved max runtime: ");
    Serial.println(hours, 1);
  } else {
    Serial.println("ERROR: Preferences mutex timeout in saveMaxRuntimeHours");
  }
}

void saveHeaterStartHour(int hour) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", false);
    preferences.putInt("heatStart", hour);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    Serial.print("Saved heater start hour: ");
    Serial.println(hour);
  } else {
    Serial.println("ERROR: Preferences mutex timeout in saveHeaterStartHour");
  }
}

void saveHeaterEndHour(int hour) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", false);
    preferences.putInt("heatEnd", hour);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    Serial.print("Saved heater end hour: ");
    Serial.println(hour);
  } else {
    Serial.println("ERROR: Preferences mutex timeout in saveHeaterEndHour");
  }
}

void saveMinHeaterTemp(float temp) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", false);
    preferences.putFloat("minHeatTemp", temp);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    Serial.print("Saved minimum heater temp: ");
    Serial.println(temp, 1);
  } else {
    Serial.println("ERROR: Preferences mutex timeout in saveMinHeaterTemp");
  }
}

void saveMaxHeaterTemp(float temp) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", false);
    preferences.putFloat("maxHeatTemp", temp);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    Serial.print("Saved maximum heater temp: ");
    Serial.println(temp, 1);
  } else {
    Serial.println("ERROR: Preferences mutex timeout in saveMaxHeaterTemp");
  }
}

// ============================================
// LOAD FUNCTIONS
// ============================================

float loadTargetTemp() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", true);  // Read-only
    float value = preferences.getFloat("targetTemp", DEFAULT_TARGET_TEMP);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    Serial.println("ERROR: Preferences mutex timeout in loadTargetTemp");
    return DEFAULT_TARGET_TEMP;
  }
}

bool loadHeaterEnabled() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", true);  // Read-only
    bool value = preferences.getBool("heaterEn", false);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    Serial.println("ERROR: Preferences mutex timeout in loadHeaterEnabled");
    return false;
  }
}

PumpState loadPumpState() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", true);  // Read-only
    uint8_t val = preferences.getUChar("pumpState", (uint8_t)PUMP_OFF);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return (PumpState)val;
  } else {
    Serial.println("ERROR: Preferences mutex timeout in loadPumpState");
    return PUMP_OFF;
  }
}

float loadMaxRuntimeHours() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", true);  // Read-only
    float value = preferences.getFloat("maxRuntime", DEFAULT_MAX_RUNTIME_HOURS);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    Serial.println("ERROR: Preferences mutex timeout in loadMaxRuntimeHours");
    return DEFAULT_MAX_RUNTIME_HOURS;
  }
}

int loadHeaterStartHour() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", true);  // Read-only
    int value = preferences.getInt("heatStart", DEFAULT_HEATER_START_HOUR);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    Serial.println("ERROR: Preferences mutex timeout in loadHeaterStartHour");
    return DEFAULT_HEATER_START_HOUR;
  }
}

int loadHeaterEndHour() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", true);  // Read-only
    int value = preferences.getInt("heatEnd", DEFAULT_HEATER_END_HOUR);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    Serial.println("ERROR: Preferences mutex timeout in loadHeaterEndHour");
    return DEFAULT_HEATER_END_HOUR;
  }
}

float loadMinHeaterTemp() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", true);  // Read-only
    float value = preferences.getFloat("minHeatTemp", DEFAULT_MIN_HEATER_TEMP);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    Serial.println("ERROR: Preferences mutex timeout in loadMinHeaterTemp");
    return DEFAULT_MIN_HEATER_TEMP;
  }
}

float loadMaxHeaterTemp() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", true);  // Read-only
    float value = preferences.getFloat("maxHeatTemp", DEFAULT_MAX_HEATER_TEMP);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    Serial.println("ERROR: Preferences mutex timeout in loadMaxHeaterTemp");
    return DEFAULT_MAX_HEATER_TEMP;
  }
}

// ============================================
// FACTORY RESET
// ============================================

void factoryResetSettings() {
  Serial.println("Performing factory reset...");

  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", false);
    preferences.clear();  // Clear all preferences
    preferences.end();
    xSemaphoreGive(preferencesMutex);
  } else {
    Serial.println("ERROR: Preferences mutex timeout in factoryResetSettings");
  }

  // Reset to defaults
  state.targetTemp = DEFAULT_TARGET_TEMP;
  state.heaterEnabled = false;
  state.pumpState = PUMP_OFF;
  state.maxRuntimeHours = DEFAULT_MAX_RUNTIME_HOURS;
  state.heaterStartHour = DEFAULT_HEATER_START_HOUR;
  state.heaterEndHour = DEFAULT_HEATER_END_HOUR;
  state.minHeaterTemp = DEFAULT_MIN_HEATER_TEMP;
  state.maxHeaterTemp = DEFAULT_MAX_HEATER_TEMP;

  Serial.println("Factory reset complete");
}

// ============================================
// WIFI FAILURE TRACKING
// ============================================

int loadWiFiFailureCount() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", true);  // Read-only
    int count = preferences.getInt("wifiFailCnt", 0);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return count;
  } else {
    Serial.println("ERROR: Preferences mutex timeout in loadWiFiFailureCount");
    return 0;
  }
}

void saveWiFiFailureCount(int count) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", false);
    preferences.putInt("wifiFailCnt", count);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    Serial.print("Saved WiFi failure count: ");
    Serial.println(count);
  } else {
    Serial.println("ERROR: Preferences mutex timeout in saveWiFiFailureCount");
  }
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
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", true);  // Read-only
    float offset = preferences.getFloat("currentCal", 0.0);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return offset;
  } else {
    Serial.println("ERROR: Preferences mutex timeout in loadCurrentCalibration");
    return 0.0;
  }
}

void saveCurrentCalibration(float offset) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    preferences.begin("pool", false);
    preferences.putFloat("currentCal", offset);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    Serial.print("Saved current calibration: ");
    Serial.println(offset, 3);
  } else {
    Serial.println("ERROR: Preferences mutex timeout in saveCurrentCalibration");
  }
}
