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

  StateManager& stateMgr = StateManager::getInstance();

  stateMgr.setTargetTemp(loadTargetTemp());
  stateMgr.setHeaterEnabled(loadHeaterEnabled());
  stateMgr.setPumpStateInternal(loadPumpState());  // Internal setter to avoid saving back to flash
  stateMgr.setMaxRuntimeHours(loadMaxRuntimeHours());
  stateMgr.setHeaterStartHour(loadHeaterStartHour());
  stateMgr.setHeaterEndHour(loadHeaterEndHour());
  stateMgr.setMinHeaterTemp(loadMinHeaterTemp());
  stateMgr.setMaxHeaterTemp(loadMaxHeaterTemp());
  stateMgr.setCurrentOffset(loadCurrentCalibration());

  LOG_INFO("Settings loaded:");
  LOG_INFOF("  Target temp: %.1f", stateMgr.getTargetTemp());
  LOG_INFOF("  Heater enabled: %d", stateMgr.isHeaterEnabled());
  LOG_INFOF("  Pump state: %d", (int)stateMgr.getPumpState());
  LOG_INFOF("  Max runtime: %.1f", stateMgr.getMaxRuntimeHours());
  LOG_INFOF("  Heater hours: %d - %d", stateMgr.getHeaterStartHour(), stateMgr.getHeaterEndHour());
  LOG_INFOF("  Min heater temp: %.1f", stateMgr.getMinHeaterTemp());
  LOG_INFOF("  Max heater temp: %.1f", stateMgr.getMaxHeaterTemp());
  LOG_INFOF("  Current calibration: %.3f", stateMgr.getCurrentOffset());
}

// ============================================
// SETTINGS VALIDATION
// ============================================

void validateAllSettings() {
  LOG_DEBUG("Validating all settings...");
  bool corrected = false;

  StateManager& stateMgr = StateManager::getInstance();

  // Validate target temperature
  float targetTemp = stateMgr.getTargetTemp();
  if (targetTemp < MIN_TARGET_TEMP || targetTemp > MAX_TARGET_TEMP) {
    LOG_WARNF("Target temp out of range (%.1f) - correcting to default", targetTemp);
    stateMgr.setTargetTemp(DEFAULT_TARGET_TEMP);
    saveTargetTemp(DEFAULT_TARGET_TEMP);
    corrected = true;
  }

  // Validate max runtime hours (reasonable range: 0.1 to 24 hours)
  float maxRuntimeHours = stateMgr.getMaxRuntimeHours();
  if (maxRuntimeHours < 0.1 || maxRuntimeHours > 24.0) {
    LOG_WARNF("Max runtime hours out of range (%.1f) - correcting to default", maxRuntimeHours);
    stateMgr.setMaxRuntimeHours(DEFAULT_MAX_RUNTIME_HOURS);
    saveMaxRuntimeHours(DEFAULT_MAX_RUNTIME_HOURS);
    corrected = true;
  }

  // Validate heater start hour (0-23)
  int heaterStartHour = stateMgr.getHeaterStartHour();
  if (heaterStartHour < 0 || heaterStartHour > 23) {
    LOG_WARNF("Heater start hour out of range (%d) - correcting to default", heaterStartHour);
    stateMgr.setHeaterStartHour(DEFAULT_HEATER_START_HOUR);
    saveHeaterStartHour(DEFAULT_HEATER_START_HOUR);
    corrected = true;
  }

  // Validate heater end hour (0-23)
  int heaterEndHour = stateMgr.getHeaterEndHour();
  if (heaterEndHour < 0 || heaterEndHour > 23) {
    LOG_WARNF("Heater end hour out of range (%d) - correcting to default", heaterEndHour);
    stateMgr.setHeaterEndHour(DEFAULT_HEATER_END_HOUR);
    saveHeaterEndHour(DEFAULT_HEATER_END_HOUR);
    corrected = true;
  }

  // Validate min heater temp
  float minHeaterTemp = stateMgr.getMinHeaterTemp();
  if (minHeaterTemp < INVALID_TEMP_LOW || minHeaterTemp > MAX_HEATER_TEMP) {
    LOG_WARNF("Min heater temp out of range (%.1f) - correcting to default", minHeaterTemp);
    stateMgr.setMinHeaterTemp(DEFAULT_MIN_HEATER_TEMP);
    saveMinHeaterTemp(DEFAULT_MIN_HEATER_TEMP);
    corrected = true;
  }

  // Validate max heater temp
  float maxHeaterTemp = stateMgr.getMaxHeaterTemp();
  if (maxHeaterTemp < minHeaterTemp || maxHeaterTemp > MAX_HEATER_TEMP) {
    LOG_WARNF("Max heater temp out of range (%.1f) - correcting to default", maxHeaterTemp);
    stateMgr.setMaxHeaterTemp(DEFAULT_MAX_HEATER_TEMP);
    saveMaxHeaterTemp(DEFAULT_MAX_HEATER_TEMP);
    corrected = true;
  }

  // Validate pump state (ensure it's a valid enum value)
  PumpState pumpState = stateMgr.getPumpState();
  if (pumpState < PUMP_OFF || pumpState > PUMP_HIGH) {
    LOG_WARNF("Pump state invalid (%d) - correcting to PUMP_OFF", (int)pumpState);
    stateMgr.setPumpStateInternal(PUMP_OFF);
    savePumpState(PUMP_OFF);
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
  StateManager& stateMgr = StateManager::getInstance();
  stateMgr.setTargetTemp(DEFAULT_TARGET_TEMP);
  stateMgr.setHeaterEnabled(false);
  stateMgr.setPumpStateInternal(PUMP_OFF);
  stateMgr.setMaxRuntimeHours(DEFAULT_MAX_RUNTIME_HOURS);
  stateMgr.setHeaterStartHour(DEFAULT_HEATER_START_HOUR);
  stateMgr.setHeaterEndHour(DEFAULT_HEATER_END_HOUR);
  stateMgr.setMinHeaterTemp(DEFAULT_MIN_HEATER_TEMP);
  stateMgr.setMaxHeaterTemp(DEFAULT_MAX_HEATER_TEMP);

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
  StateManager& stateMgr = StateManager::getInstance();

  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, false);

    // Save boot and crash counts
    preferences.putUInt("bootCount", stateMgr.getBootCount());
    preferences.putUInt("crashCount", stateMgr.getCrashCount());

    // Save cooldown state
    preferences.putBool("inCooldown", stateMgr.isInCooldown());
    preferences.putULong("cooldownStart", stateMgr.getCooldownStartTime());

    // Save total runtime
    preferences.putULong("totalRuntime", stateMgr.getTotalPumpRuntime());

    preferences.end();
    xSemaphoreGive(preferencesMutex);

    LOG_DEBUG("Runtime state saved to NVS");
  } else {
    LOG_ERROR("Preferences mutex timeout in saveRuntimeState");
  }
}

void loadRuntimeState() {
  StateManager& stateMgr = StateManager::getInstance();

  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(SETTINGS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(SETTINGS_NVS_NAMESPACE, true);  // Read-only

    // Load boot and crash counts
    stateMgr.setBootCount(preferences.getUInt("bootCount", 0));
    stateMgr.setCrashCount(preferences.getUInt("crashCount", 0));

    // Load cooldown state
    stateMgr.setInCooldown(preferences.getBool("inCooldown", false));
    stateMgr.setCooldownStartTime(preferences.getULong("cooldownStart", 0));

    // Load total runtime
    stateMgr.setTotalPumpRuntime(preferences.getULong("totalRuntime", 0));

    preferences.end();
    xSemaphoreGive(preferencesMutex);

    LOG_INFO("Runtime state loaded from NVS:");
    LOG_INFOF("  Boot count: %u", stateMgr.getBootCount());
    LOG_INFOF("  Crash count: %u", stateMgr.getCrashCount());
    LOG_INFOF("  In cooldown: %d", stateMgr.isInCooldown());
    LOG_INFOF("  Total runtime: %lu ms", stateMgr.getTotalPumpRuntime());
  } else {
    LOG_ERROR("Preferences mutex timeout in loadRuntimeState");
  }

  // Increment boot count and save
  stateMgr.setBootCount(stateMgr.getBootCount() + 1);

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
      stateMgr.setCrashCount(stateMgr.getCrashCount() + 1);
      LOG_ERRORF("Total crashes detected: %u", stateMgr.getCrashCount());
      break;

    case ESP_RST_INT_WDT:
      LOG_ERROR("Boot reason: Interrupt watchdog timeout (CRASH!)");
      stateMgr.setCrashCount(stateMgr.getCrashCount() + 1);
      LOG_ERRORF("Total crashes detected: %u", stateMgr.getCrashCount());
      break;

    case ESP_RST_TASK_WDT:
      LOG_ERROR("Boot reason: Task watchdog timeout (CRASH!)");
      stateMgr.setCrashCount(stateMgr.getCrashCount() + 1);
      LOG_ERRORF("Total crashes detected: %u", stateMgr.getCrashCount());
      break;

    case ESP_RST_WDT:
      LOG_ERROR("Boot reason: Other watchdog reset (CRASH!)");
      stateMgr.setCrashCount(stateMgr.getCrashCount() + 1);
      LOG_ERRORF("Total crashes detected: %u", stateMgr.getCrashCount());
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
