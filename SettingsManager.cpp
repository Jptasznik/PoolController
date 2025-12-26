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
// SETTINGS VALIDATION
// ============================================

void validateAllSettings() {
  LOG_DEBUG("Validating all settings...");
  bool corrected = false;

  // Validate target temperature
  if (state.targetTemp < MIN_TARGET_TEMP || state.targetTemp > MAX_TARGET_TEMP) {
    LOG_WARNF("Target temp out of range (%.1f) - correcting to default", state.targetTemp);
    state.targetTemp = DEFAULT_TARGET_TEMP;
    saveTargetTemp(state.targetTemp);
    corrected = true;
  }

  // Validate max runtime hours (reasonable range: 0.1 to 24 hours)
  if (state.maxRuntimeHours < 0.1 || state.maxRuntimeHours > 24.0) {
    LOG_WARNF("Max runtime hours out of range (%.1f) - correcting to default", state.maxRuntimeHours);
    state.maxRuntimeHours = DEFAULT_MAX_RUNTIME_HOURS;
    saveMaxRuntimeHours(state.maxRuntimeHours);
    corrected = true;
  }

  // Validate heater start hour (0-23)
  if (state.heaterStartHour < 0 || state.heaterStartHour > 23) {
    LOG_WARNF("Heater start hour out of range (%d) - correcting to default", state.heaterStartHour);
    state.heaterStartHour = DEFAULT_HEATER_START_HOUR;
    saveHeaterStartHour(state.heaterStartHour);
    corrected = true;
  }

  // Validate heater end hour (0-23)
  if (state.heaterEndHour < 0 || state.heaterEndHour > 23) {
    LOG_WARNF("Heater end hour out of range (%d) - correcting to default", state.heaterEndHour);
    state.heaterEndHour = DEFAULT_HEATER_END_HOUR;
    saveHeaterEndHour(state.heaterEndHour);
    corrected = true;
  }

  // Validate min heater temp
  if (state.minHeaterTemp < INVALID_TEMP_LOW || state.minHeaterTemp > MAX_HEATER_TEMP) {
    LOG_WARNF("Min heater temp out of range (%.1f) - correcting to default", state.minHeaterTemp);
    state.minHeaterTemp = DEFAULT_MIN_HEATER_TEMP;
    saveMinHeaterTemp(state.minHeaterTemp);
    corrected = true;
  }

  // Validate max heater temp
  if (state.maxHeaterTemp < state.minHeaterTemp || state.maxHeaterTemp > MAX_HEATER_TEMP) {
    LOG_WARNF("Max heater temp out of range (%.1f) - correcting to default", state.maxHeaterTemp);
    state.maxHeaterTemp = DEFAULT_MAX_HEATER_TEMP;
    saveMaxHeaterTemp(state.maxHeaterTemp);
    corrected = true;
  }

  // Validate pump state (ensure it's a valid enum value)
  if (state.pumpState < PUMP_OFF || state.pumpState > PUMP_HIGH) {
    LOG_WARNF("Pump state invalid (%d) - correcting to PUMP_OFF", (int)state.pumpState);
    state.pumpState = PUMP_OFF;
    savePumpState(state.pumpState);
    corrected = true;
  }

  if (corrected) {
    LOG_WARN("Settings validation completed with corrections");
  } else {
    LOG_DEBUG("All settings valid");
  }
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

// ============================================
// RUNTIME STATE PERSISTENCE (CRASH RECOVERY)
// ============================================

void saveRuntimeState() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);

    // Save boot and crash counts
    preferences.putUInt("bootCount", state.bootCount);
    preferences.putUInt("crashCount", state.crashCount);

    // Save cooldown state
    preferences.putBool("inCooldown", state.inCooldown);
    preferences.putULong("cooldownStart", state.cooldownStartTime);

    // Save total runtime
    preferences.putULong("totalRuntime", state.totalPumpRuntime);

    preferences.end();
    xSemaphoreGive(preferencesMutex);

    LOG_DEBUG("Runtime state saved to NVS");
  } else {
    LOG_ERROR("Preferences mutex timeout in saveRuntimeState");
  }
}

void loadRuntimeState() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only

    // Load boot and crash counts
    state.bootCount = preferences.getUInt("bootCount", 0);
    state.crashCount = preferences.getUInt("crashCount", 0);

    // Load cooldown state
    state.inCooldown = preferences.getBool("inCooldown", false);
    state.cooldownStartTime = preferences.getULong("cooldownStart", 0);

    // Load total runtime
    state.totalPumpRuntime = preferences.getULong("totalRuntime", 0);

    preferences.end();
    xSemaphoreGive(preferencesMutex);

    LOG_INFO("Runtime state loaded from NVS:");
    LOG_INFOF("  Boot count: %u", state.bootCount);
    LOG_INFOF("  Crash count: %u", state.crashCount);
    LOG_INFOF("  In cooldown: %d", state.inCooldown);
    LOG_INFOF("  Total runtime: %lu ms", state.totalPumpRuntime);
  } else {
    LOG_ERROR("Preferences mutex timeout in loadRuntimeState");
  }

  // Increment boot count and save
  state.bootCount++;

  // Check ESP32 reset reason
  esp_reset_reason_t reset_reason = esp_reset_reason();

  switch (reset_reason) {
    case ESP_RST_POWERON:
      LOG_INFO("Boot reason: Power-on reset");
      break;

    case ESP_RST_SW:
      LOG_INFO("Boot reason: Software reset (ESP.restart())");
      break;

    case ESP_RST_PANIC:
      LOG_ERROR("Boot reason: Exception/panic (CRASH!)");
      state.crashCount++;
      LOG_ERRORF("Total crashes detected: %u", state.crashCount);
      break;

    case ESP_RST_INT_WDT:
      LOG_ERROR("Boot reason: Interrupt watchdog timeout (CRASH!)");
      state.crashCount++;
      LOG_ERRORF("Total crashes detected: %u", state.crashCount);
      break;

    case ESP_RST_TASK_WDT:
      LOG_ERROR("Boot reason: Task watchdog timeout (CRASH!)");
      state.crashCount++;
      LOG_ERRORF("Total crashes detected: %u", state.crashCount);
      break;

    case ESP_RST_WDT:
      LOG_ERROR("Boot reason: Other watchdog reset (CRASH!)");
      state.crashCount++;
      LOG_ERRORF("Total crashes detected: %u", state.crashCount);
      break;

    case ESP_RST_BROWNOUT:
      LOG_ERROR("Boot reason: Brownout detector reset (POWER ISSUE!)");
      LOG_WARN("Check power supply - voltage may be insufficient");
      break;

    default:
      LOG_WARNF("Boot reason: Unknown (%d)", reset_reason);
      break;
  }

  // Save updated counts
  saveRuntimeState();
}
