#define LOG_MODULE "SETTINGS"

#include "SettingsManager.h"
#include "Logger.h"
#include "Config.h"
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

Preferences preferences;

// Preferences mutex for thread-safe flash access
static SemaphoreHandle_t preferencesMutex = NULL;

// NVS configuration
#define SETTINGS_NVS_NAMESPACE "pool"           // NVS namespace for settings
#define SETTINGS_MUTEX_TIMEOUT_MS 1000          // Timeout for acquiring preferences mutex (ms)

// ============================================
// INITIALIZATION
// ============================================

void initSettings() {
  // Create preferences mutex for thread-safe flash access
  preferencesMutex = xSemaphoreCreateMutex();
  if (preferencesMutex == NULL) {
    LOG_ERROR("Failed to create preferences mutex!");
  } else {
    LOG_DEBUG("Preferences mutex created successfully");
  }

  // Settings manager ready - preferences opened/closed per transaction
  LOG_INFO("Settings manager initialized");
}

// ============================================
// LOAD ALL SETTINGS
// ============================================

void loadAllSettings() {
  LOG_INFO("Loading settings from flash...");

  state.targetTemp = loadTargetTemp();
  state.heaterEnabled = loadHeaterEnabled();
  state.pumpState = loadPumpState();
  state.maxRuntimeHours = loadMaxRuntimeHours();
  state.heaterStartHour = loadHeaterStartHour();
  state.heaterEndHour = loadHeaterEndHour();
  state.minHeaterTemp = loadMinHeaterTemp();
  state.maxHeaterTemp = loadMaxHeaterTemp();
  state.currentOffset = loadCurrentCalibration();

  LOG_INFO("Settings loaded:");
  LOG_INFOF("  Target temp: %.1f", state.targetTemp);
  LOG_INFOF("  Heater enabled: %d", state.heaterEnabled);
  LOG_INFOF("  Pump state: %d", (int)state.pumpState);
  LOG_INFOF("  Max runtime: %.1f", state.maxRuntimeHours);
  LOG_INFOF("  Heater hours: %d - %d", state.heaterStartHour, state.heaterEndHour);
  LOG_INFOF("  Min heater temp: %.1f", state.minHeaterTemp);
  LOG_INFOF("  Max heater temp: %.1f", state.maxHeaterTemp);
  LOG_INFOF("  Current calibration: %.3f", state.currentOffset);
}

// ============================================
// SAVE FUNCTIONS
// ============================================

void saveTargetTemp(float temp) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);
    preferences.putFloat("targetTemp", temp);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    LOG_DEBUGF("Saved target temp: %.1f", temp);
  } else {
    LOG_ERROR("Preferences mutex timeout in saveTargetTemp");
  }
}

void saveHeaterEnabled(bool enabled) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);
    preferences.putBool("heaterEn", enabled);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    LOG_DEBUGF("Saved heater enabled: %d", enabled);
  } else {
    LOG_ERROR("Preferences mutex timeout in saveHeaterEnabled");
  }
}

void savePumpState(PumpState state) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);
    preferences.putUChar("pumpState", (uint8_t)state);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    LOG_DEBUGF("Saved pump state: %d", (int)state);
  } else {
    LOG_ERROR("Preferences mutex timeout in savePumpState");
  }
}

void saveMaxRuntimeHours(float hours) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);
    preferences.putFloat("maxRuntime", hours);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    LOG_DEBUGF("Saved max runtime: %.1f", hours);
  } else {
    LOG_ERROR("Preferences mutex timeout in saveMaxRuntimeHours");
  }
}

void saveHeaterStartHour(int hour) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);
    preferences.putInt("heatStart", hour);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    LOG_DEBUGF("Saved heater start hour: %d", hour);
  } else {
    LOG_ERROR("Preferences mutex timeout in saveHeaterStartHour");
  }
}

void saveHeaterEndHour(int hour) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);
    preferences.putInt("heatEnd", hour);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    LOG_DEBUGF("Saved heater end hour: %d", hour);
  } else {
    LOG_ERROR("Preferences mutex timeout in saveHeaterEndHour");
  }
}

void saveMinHeaterTemp(float temp) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);
    preferences.putFloat("minHeatTemp", temp);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    LOG_DEBUGF("Saved minimum heater temp: %.1f", temp);
  } else {
    LOG_ERROR("Preferences mutex timeout in saveMinHeaterTemp");
  }
}

void saveMaxHeaterTemp(float temp) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);
    preferences.putFloat("maxHeatTemp", temp);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    LOG_DEBUGF("Saved maximum heater temp: %.1f", temp);
  } else {
    LOG_ERROR("Preferences mutex timeout in saveMaxHeaterTemp");
  }
}

// ============================================
// LOAD FUNCTIONS
// ============================================

float loadTargetTemp() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only
    float value = preferences.getFloat("targetTemp", DEFAULT_TARGET_TEMP);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    LOG_ERROR("Preferences mutex timeout in loadTargetTemp");
    return DEFAULT_TARGET_TEMP;
  }
}

bool loadHeaterEnabled() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only
    bool value = preferences.getBool("heaterEn", false);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    LOG_ERROR("Preferences mutex timeout in loadHeaterEnabled");
    return false;
  }
}

PumpState loadPumpState() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only
    uint8_t val = preferences.getUChar("pumpState", (uint8_t)PUMP_OFF);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return (PumpState)val;
  } else {
    LOG_ERROR("Preferences mutex timeout in loadPumpState");
    return PUMP_OFF;
  }
}

float loadMaxRuntimeHours() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only
    float value = preferences.getFloat("maxRuntime", DEFAULT_MAX_RUNTIME_HOURS);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    LOG_ERROR("Preferences mutex timeout in loadMaxRuntimeHours");
    return DEFAULT_MAX_RUNTIME_HOURS;
  }
}

int loadHeaterStartHour() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only
    int value = preferences.getInt("heatStart", DEFAULT_HEATER_START_HOUR);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    LOG_ERROR("Preferences mutex timeout in loadHeaterStartHour");
    return DEFAULT_HEATER_START_HOUR;
  }
}

int loadHeaterEndHour() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only
    int value = preferences.getInt("heatEnd", DEFAULT_HEATER_END_HOUR);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    LOG_ERROR("Preferences mutex timeout in loadHeaterEndHour");
    return DEFAULT_HEATER_END_HOUR;
  }
}

float loadMinHeaterTemp() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only
    float value = preferences.getFloat("minHeatTemp", DEFAULT_MIN_HEATER_TEMP);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    LOG_ERROR("Preferences mutex timeout in loadMinHeaterTemp");
    return DEFAULT_MIN_HEATER_TEMP;
  }
}

float loadMaxHeaterTemp() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only
    float value = preferences.getFloat("maxHeatTemp", DEFAULT_MAX_HEATER_TEMP);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return value;
  } else {
    LOG_ERROR("Preferences mutex timeout in loadMaxHeaterTemp");
    return DEFAULT_MAX_HEATER_TEMP;
  }
}

// ============================================
// FACTORY RESET
// ============================================

void factoryResetSettings() {
  LOG_WARN("Performing factory reset...");

  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);
    preferences.clear();  // Clear all preferences
    preferences.end();
    xSemaphoreGive(preferencesMutex);
  } else {
    LOG_ERROR("Preferences mutex timeout in factoryResetSettings");
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

  LOG_INFO("Factory reset complete");
}

// ============================================
// WIFI FAILURE TRACKING
// ============================================

int loadWiFiFailureCount() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only
    int count = preferences.getInt("wifiFailCnt", 0);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return count;
  } else {
    LOG_ERROR("Preferences mutex timeout in loadWiFiFailureCount");
    return 0;
  }
}

void saveWiFiFailureCount(int count) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);
    preferences.putInt("wifiFailCnt", count);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    LOG_DEBUGF("Saved WiFi failure count: %d", count);
  } else {
    LOG_ERROR("Preferences mutex timeout in saveWiFiFailureCount");
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
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only
    float offset = preferences.getFloat("currentCal", 0.0);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    return offset;
  } else {
    LOG_ERROR("Preferences mutex timeout in loadCurrentCalibration");
    return 0.0;
  }
}

void saveCurrentCalibration(float offset) {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);
    preferences.putFloat("currentCal", offset);
    preferences.end();
    xSemaphoreGive(preferencesMutex);
    LOG_DEBUGF("Saved current calibration: %.3f", offset);
  } else {
    LOG_ERROR("Preferences mutex timeout in saveCurrentCalibration");
  }
}
