#define LOG_MODULE "STATE"

#include "StateManager.h"
#include "Logger.h"
#include "Config.h"
#include "SettingsManager.h"
#include "LCDDisplay.h"
#include "SinricIntegration.h"
#include "PumpControl.h"
#include "MessageQueue.h"
#include <time.h>

// ============================================
// SINGLETON IMPLEMENTATION
// ============================================

StateManager& StateManager::getInstance() {
  static StateManager instance;
  return instance;
}

StateManager::StateManager() : mutex(NULL) {
  // Private constructor - initialize will be called via init()
}

// ============================================
// MUTEX HELPERS
// ============================================

bool StateManager::lock(uint32_t timeout_ms) {
  if (mutex == NULL) {
    LOG_ERROR("State mutex not initialized!");
    return false;
  }

  TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
  if (xSemaphoreTake(mutex, timeout_ticks) == pdTRUE) {
    return true;
  } else {
    LOG_WARN("State mutex lock timeout!");
    return false;
  }
}

void StateManager::unlock() {
  if (mutex == NULL) {
    LOG_ERROR("State mutex not initialized!");
    return;
  }
  xSemaphoreGive(mutex);
}

// ============================================
// INITIALIZATION
// ============================================

void StateManager::init() {
  // Create mutex
  if (mutex == NULL) {
    mutex = xSemaphoreCreateMutex();
    if (mutex == NULL) {
      LOG_ERROR("Failed to create state mutex!");
    } else {
      LOG_INFO("State mutex created successfully");
    }
  }

  // Initialize all state fields
  state.inletTemp = 0.0;
  state.heaterTemp = 0.0;
  state.ambientTemp = 0.0;
  state.targetTemp = DEFAULT_TARGET_TEMP;
  state.inletTempValid = false;
  state.heaterTempValid = false;
  state.ambientTempValid = false;

  // Initialize current monitoring
  state.pumpCurrent = 0.0;
  state.currentOffset = 0.0;
  state.currentSensorValid = false;
  state.pumpFailureAlert = false;
  state.overcurrentAlert = false;

  state.heaterEnabled = false;
  state.heaterStatus = HEATER_OFF;
  state.pumpState = PUMP_OFF;

  state.inCooldown = false;
  state.cooldownStartTime = 0;
  state.pumpStateBeforeCooldown = PUMP_OFF;

  state.pumpRuntimeToday = 0;
  state.lastRuntimeUpdate = millis();
  state.lastResetDay = -1;

  state.maxRuntimeHours = DEFAULT_MAX_RUNTIME_HOURS;
  state.heaterStartHour = DEFAULT_HEATER_START_HOUR;
  state.heaterEndHour = DEFAULT_HEATER_END_HOUR;
  state.minHeaterTemp = DEFAULT_MIN_HEATER_TEMP;
  state.maxHeaterTemp = DEFAULT_MAX_HEATER_TEMP;

  state.wifiConnected = false;
  state.blynkConnected = false;
  state.sinricConnected = false;

  // Initialize WiFi runtime failure tracking
  state.provisioningActive = false;
  state.provisioningStartTime = 0;
  state.credentialsReceived = false;
  state.credReceivedTime = 0;
  state.disconnectCount = 0;
  for (int i = 0; i < WIFI_MAX_DISCONNECTS; i++) {
    state.disconnectTimestamps[i] = 0;
  }

  state.emergencyShutdown = false;

  // Initialize failsafe mode
  state.inFailsafeMode = false;
  state.failsafeEnterTime = 0;
  state.failsafeReason[0] = '\0';

  // Initialize overheat protection
  state.overheatLockout = false;
  state.overheatDetectedTime = 0;

  // Initialize runtime state tracking
  state.bootCount = 0;
  state.crashCount = 0;
  state.totalPumpRuntime = 0;

  // Initialize OTA update state
  state.otaInProgress = false;
  state.otaStartTime = 0;
  state.otaVersion[0] = '\0';
}

// ============================================
// TEMPERATURE GETTERS/SETTERS
// ============================================

float StateManager::getInletTemp() {
  if (!lock()) return 0.0;
  float temp = state.inletTemp;
  unlock();
  return temp;
}

void StateManager::setInletTemp(float temp) {
  if (!lock()) return;
  state.inletTemp = temp;
  unlock();
}

bool StateManager::isInletTempValid() {
  if (!lock()) return false;
  bool valid = state.inletTempValid;
  unlock();
  return valid;
}

void StateManager::setInletTempValid(bool valid) {
  if (!lock()) return;
  state.inletTempValid = valid;
  unlock();
}

float StateManager::getHeaterTemp() {
  if (!lock()) return 0.0;
  float temp = state.heaterTemp;
  unlock();
  return temp;
}

void StateManager::setHeaterTemp(float temp) {
  if (!lock()) return;
  state.heaterTemp = temp;
  unlock();
}

bool StateManager::isHeaterTempValid() {
  if (!lock()) return false;
  bool valid = state.heaterTempValid;
  unlock();
  return valid;
}

void StateManager::setHeaterTempValid(bool valid) {
  if (!lock()) return;
  state.heaterTempValid = valid;
  unlock();
}

float StateManager::getAmbientTemp() {
  if (!lock()) return 0.0;
  float temp = state.ambientTemp;
  unlock();
  return temp;
}

void StateManager::setAmbientTemp(float temp) {
  if (!lock()) return;
  state.ambientTemp = temp;
  unlock();
}

bool StateManager::isAmbientTempValid() {
  if (!lock()) return false;
  bool valid = state.ambientTempValid;
  unlock();
  return valid;
}

void StateManager::setAmbientTempValid(bool valid) {
  if (!lock()) return;
  state.ambientTempValid = valid;
  unlock();
}

float StateManager::getTargetTemp() {
  if (!lock()) return 0.0;
  float temp = state.targetTemp;
  unlock();
  return temp;
}

void StateManager::setTargetTempInternal(float temp) {
  if (!lock()) return;
  state.targetTemp = temp;
  unlock();
}

// ============================================
// CURRENT MONITORING GETTERS/SETTERS
// ============================================

float StateManager::getPumpCurrent() {
  if (!lock()) return 0.0;
  float current = state.pumpCurrent;
  unlock();
  return current;
}

void StateManager::setPumpCurrent(float current) {
  if (!lock()) return;
  state.pumpCurrent = current;
  unlock();
}

float StateManager::getCurrentOffset() {
  if (!lock()) return 0.0;
  float offset = state.currentOffset;
  unlock();
  return offset;
}

void StateManager::setCurrentOffset(float offset) {
  if (!lock()) return;
  state.currentOffset = offset;
  unlock();
}

bool StateManager::isCurrentSensorValid() {
  if (!lock()) return false;
  bool valid = state.currentSensorValid;
  unlock();
  return valid;
}

void StateManager::setCurrentSensorValid(bool valid) {
  if (!lock()) return;
  state.currentSensorValid = valid;
  unlock();
}

bool StateManager::isPumpFailureAlert() {
  if (!lock()) return false;
  bool alert = state.pumpFailureAlert;
  unlock();
  return alert;
}

void StateManager::setPumpFailureAlert(bool alert) {
  if (!lock()) return;
  state.pumpFailureAlert = alert;
  unlock();
}

bool StateManager::isOvercurrentAlert() {
  if (!lock()) return false;
  bool alert = state.overcurrentAlert;
  unlock();
  return alert;
}

void StateManager::setOvercurrentAlert(bool alert) {
  if (!lock()) return;
  state.overcurrentAlert = alert;
  unlock();
}

// ============================================
// CONTROL STATE GETTERS/SETTERS
// ============================================

bool StateManager::isHeaterEnabled() {
  if (!lock()) return false;
  bool enabled = state.heaterEnabled;
  unlock();
  return enabled;
}

void StateManager::setHeaterEnabledInternal(bool enabled) {
  if (!lock()) return;
  state.heaterEnabled = enabled;
  unlock();
}

HeaterStatus StateManager::getHeaterStatus() {
  if (!lock()) return HEATER_OFF;
  HeaterStatus status = state.heaterStatus;
  unlock();
  return status;
}

void StateManager::setHeaterStatus(HeaterStatus status) {
  if (!lock()) return;
  state.heaterStatus = status;
  unlock();
}

PumpState StateManager::getPumpState() {
  if (!lock()) return PUMP_OFF;
  PumpState pstate = state.pumpState;
  unlock();
  return pstate;
}

// ============================================
// COOLDOWN GETTERS/SETTERS
// ============================================

bool StateManager::isInCooldown() {
  if (!lock()) return false;
  bool cooldown = state.inCooldown;
  unlock();
  return cooldown;
}

void StateManager::setInCooldown(bool inCooldown) {
  if (!lock()) return;
  state.inCooldown = inCooldown;
  unlock();
}

unsigned long StateManager::getCooldownStartTime() {
  if (!lock()) return 0;
  unsigned long time = state.cooldownStartTime;
  unlock();
  return time;
}

void StateManager::setCooldownStartTime(unsigned long time) {
  if (!lock()) return;
  state.cooldownStartTime = time;
  unlock();
}

PumpState StateManager::getPumpStateBeforeCooldown() {
  if (!lock()) return PUMP_OFF;
  PumpState pstate = state.pumpStateBeforeCooldown;
  unlock();
  return pstate;
}

void StateManager::setPumpStateBeforeCooldown(PumpState pstate) {
  if (!lock()) return;
  state.pumpStateBeforeCooldown = pstate;
  unlock();
}

// ============================================
// RUNTIME TRACKING GETTERS/SETTERS
// ============================================

unsigned long StateManager::getPumpRuntimeToday() {
  if (!lock()) return 0;
  unsigned long runtime = state.pumpRuntimeToday;
  unlock();
  return runtime;
}

void StateManager::setPumpRuntimeToday(unsigned long runtime) {
  if (!lock()) return;
  state.pumpRuntimeToday = runtime;
  unlock();
}

void StateManager::incrementPumpRuntimeToday(unsigned long elapsed) {
  if (!lock()) return;
  state.pumpRuntimeToday += elapsed;
  unlock();
}

unsigned long StateManager::getLastRuntimeUpdate() {
  if (!lock()) return 0;
  unsigned long time = state.lastRuntimeUpdate;
  unlock();
  return time;
}

void StateManager::setLastRuntimeUpdate(unsigned long time) {
  if (!lock()) return;
  state.lastRuntimeUpdate = time;
  unlock();
}

int StateManager::getLastResetDay() {
  if (!lock()) return -1;
  int day = state.lastResetDay;
  unlock();
  return day;
}

void StateManager::setLastResetDay(int day) {
  if (!lock()) return;
  state.lastResetDay = day;
  unlock();
}

// ============================================
// SETTINGS GETTERS/SETTERS
// ============================================

float StateManager::getMaxRuntimeHours() {
  if (!lock()) return 0.0;
  float hours = state.maxRuntimeHours;
  unlock();
  return hours;
}

void StateManager::setMaxRuntimeHoursInternal(float hours) {
  if (!lock()) return;
  state.maxRuntimeHours = hours;
  unlock();
}

int StateManager::getHeaterStartHour() {
  if (!lock()) return 0;
  int hour = state.heaterStartHour;
  unlock();
  return hour;
}

void StateManager::setHeaterStartHourInternal(int hour) {
  if (!lock()) return;
  state.heaterStartHour = hour;
  unlock();
}

int StateManager::getHeaterEndHour() {
  if (!lock()) return 0;
  int hour = state.heaterEndHour;
  unlock();
  return hour;
}

void StateManager::setHeaterEndHourInternal(int hour) {
  if (!lock()) return;
  state.heaterEndHour = hour;
  unlock();
}

float StateManager::getMinHeaterTemp() {
  if (!lock()) return 0.0;
  float temp = state.minHeaterTemp;
  unlock();
  return temp;
}

void StateManager::setMinHeaterTempInternal(float temp) {
  if (!lock()) return;
  state.minHeaterTemp = temp;
  unlock();
}

float StateManager::getMaxHeaterTemp() {
  if (!lock()) return 0.0;
  float temp = state.maxHeaterTemp;
  unlock();
  return temp;
}

void StateManager::setMaxHeaterTempInternal(float temp) {
  if (!lock()) return;
  state.maxHeaterTemp = temp;
  unlock();
}

// ============================================
// CONNECTION STATUS GETTERS/SETTERS
// ============================================

bool StateManager::isWiFiConnected() {
  if (!lock()) return false;
  bool connected = state.wifiConnected;
  unlock();
  return connected;
}

void StateManager::setWiFiConnected(bool connected) {
  if (!lock()) return;
  state.wifiConnected = connected;
  unlock();
}

bool StateManager::isBlynkConnected() {
  if (!lock()) return false;
  bool connected = state.blynkConnected;
  unlock();
  return connected;
}

void StateManager::setBlynkConnected(bool connected) {
  if (!lock()) return;
  state.blynkConnected = connected;
  unlock();
}

bool StateManager::isSinricConnected() {
  if (!lock()) return false;
  bool connected = state.sinricConnected;
  unlock();
  return connected;
}

void StateManager::setSinricConnected(bool connected) {
  if (!lock()) return;
  state.sinricConnected = connected;
  unlock();
}

// ============================================
// WIFI RUNTIME TRACKING GETTERS/SETTERS
// ============================================

bool StateManager::isProvisioningActive() {
  if (!lock()) return false;
  bool active = state.provisioningActive;
  unlock();
  return active;
}

void StateManager::setProvisioningActive(bool active) {
  if (!lock()) return;
  state.provisioningActive = active;
  unlock();
}

unsigned long StateManager::getProvisioningStartTime() {
  if (!lock()) return 0;
  unsigned long time = state.provisioningStartTime;
  unlock();
  return time;
}

void StateManager::setProvisioningStartTime(unsigned long time) {
  if (!lock()) return;
  state.provisioningStartTime = time;
  unlock();
}

bool StateManager::areCredentialsReceived() {
  if (!lock()) return false;
  bool received = state.credentialsReceived;
  unlock();
  return received;
}

void StateManager::setCredentialsReceived(bool received) {
  if (!lock()) return;
  state.credentialsReceived = received;
  unlock();
}

unsigned long StateManager::getCredReceivedTime() {
  if (!lock()) return 0;
  unsigned long time = state.credReceivedTime;
  unlock();
  return time;
}

void StateManager::setCredReceivedTime(unsigned long time) {
  if (!lock()) return;
  state.credReceivedTime = time;
  unlock();
}

unsigned long StateManager::getDisconnectTimestamp(int index) {
  if (index < 0 || index >= WIFI_MAX_DISCONNECTS) return 0;
  if (!lock()) return 0;
  unsigned long time = state.disconnectTimestamps[index];
  unlock();
  return time;
}

void StateManager::setDisconnectTimestamp(int index, unsigned long time) {
  if (index < 0 || index >= WIFI_MAX_DISCONNECTS) return;
  if (!lock()) return;
  state.disconnectTimestamps[index] = time;
  unlock();
}

int StateManager::getDisconnectCount() {
  if (!lock()) return 0;
  int count = state.disconnectCount;
  unlock();
  return count;
}

void StateManager::setDisconnectCount(int count) {
  if (!lock()) return;
  state.disconnectCount = count;
  unlock();
}

// ============================================
// EMERGENCY/FAILSAFE GETTERS/SETTERS
// ============================================

bool StateManager::isEmergencyShutdown() {
  if (!lock()) return false;
  bool shutdown = state.emergencyShutdown;
  unlock();
  return shutdown;
}

void StateManager::setEmergencyShutdown(bool shutdown) {
  if (!lock()) return;
  state.emergencyShutdown = shutdown;
  unlock();
}

bool StateManager::isInFailsafeMode() {
  if (!lock()) return false;
  bool failsafe = state.inFailsafeMode;
  unlock();
  return failsafe;
}

void StateManager::setInFailsafeMode(bool inFailsafe) {
  if (!lock()) return;
  state.inFailsafeMode = inFailsafe;
  unlock();
}

const char* StateManager::getFailsafeReason() {
  // Note: Returning pointer to internal buffer - caller should copy if needed
  if (!lock()) return "";
  const char* reason = state.failsafeReason;
  unlock();
  return reason;
}

void StateManager::setFailsafeReason(const char* reason) {
  if (!lock()) return;
  strncpy(state.failsafeReason, reason, sizeof(state.failsafeReason) - 1);
  state.failsafeReason[sizeof(state.failsafeReason) - 1] = '\0';
  unlock();
}

unsigned long StateManager::getFailsafeEnterTime() {
  if (!lock()) return 0;
  unsigned long time = state.failsafeEnterTime;
  unlock();
  return time;
}

void StateManager::setFailsafeEnterTime(unsigned long time) {
  if (!lock()) return;
  state.failsafeEnterTime = time;
  unlock();
}

bool StateManager::isOverheatLockout() {
  if (!lock()) return false;
  bool lockout = state.overheatLockout;
  unlock();
  return lockout;
}

void StateManager::setOverheatLockout(bool lockout) {
  if (!lock()) return;
  state.overheatLockout = lockout;
  unlock();
}

unsigned long StateManager::getOverheatDetectedTime() {
  if (!lock()) return 0;
  unsigned long time = state.overheatDetectedTime;
  unlock();
  return time;
}

void StateManager::setOverheatDetectedTime(unsigned long time) {
  if (!lock()) return;
  state.overheatDetectedTime = time;
  unlock();
}

// ============================================
// CRASH TRACKING GETTERS/SETTERS
// ============================================

uint32_t StateManager::getBootCount() {
  if (!lock()) return 0;
  uint32_t count = state.bootCount;
  unlock();
  return count;
}

void StateManager::setBootCount(uint32_t count) {
  if (!lock()) return;
  state.bootCount = count;
  unlock();
}

uint32_t StateManager::getCrashCount() {
  if (!lock()) return 0;
  uint32_t count = state.crashCount;
  unlock();
  return count;
}

void StateManager::setCrashCount(uint32_t count) {
  if (!lock()) return;
  state.crashCount = count;
  unlock();
}

unsigned long StateManager::getTotalPumpRuntime() {
  if (!lock()) return 0;
  unsigned long runtime = state.totalPumpRuntime;
  unlock();
  return runtime;
}

void StateManager::setTotalPumpRuntime(unsigned long runtime) {
  if (!lock()) return;
  state.totalPumpRuntime = runtime;
  unlock();
}

// ============================================
// OTA GETTERS/SETTERS
// ============================================

bool StateManager::isOTAInProgress() {
  if (!lock()) return false;
  bool inProgress = state.otaInProgress;
  unlock();
  return inProgress;
}

void StateManager::setOTAInProgress(bool inProgress) {
  if (!lock()) return;
  state.otaInProgress = inProgress;
  unlock();
}

unsigned long StateManager::getOTAStartTime() {
  if (!lock()) return 0;
  unsigned long time = state.otaStartTime;
  unlock();
  return time;
}

void StateManager::setOTAStartTime(unsigned long time) {
  if (!lock()) return;
  state.otaStartTime = time;
  unlock();
}

const char* StateManager::getOTAVersion() {
  // Note: Returning pointer to internal buffer - caller should copy if needed
  if (!lock()) return "";
  const char* version = state.otaVersion;
  unlock();
  return version;
}

void StateManager::setOTAVersion(const char* version) {
  if (!lock()) return;
  strncpy(state.otaVersion, version, sizeof(state.otaVersion) - 1);
  state.otaVersion[sizeof(state.otaVersion) - 1] = '\0';
  unlock();
}

// ============================================
// HIGH-LEVEL STATE MANAGEMENT FUNCTIONS
// ============================================
// The rest of the file contains the existing high-level functions,
// adapted to use internal mutex locking instead of StateScopedLock

void StateManager::setHeaterEnabledState(bool enabled) {
  // Check safety conditions with lock
  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in setHeaterEnabledState");
      return;
    }

    if (state.emergencyShutdown && enabled) {
      unlock();
      LOG_WARN("Cannot enable heater during emergency shutdown");
      return;
    }

    if (state.inFailsafeMode && enabled) {
      unlock();
      LOG_ERROR("Cannot enable heater while in FAILSAFE MODE");
      return;
    }

    if (state.overheatLockout && enabled) {
      unlock();
      LOG_ERROR("Cannot enable heater - OVERHEAT LOCKOUT active");
      LOG_ERROR("Clear lockout manually after verifying system safety");
      return;
    }

    state.heaterEnabled = enabled;
    unlock();
  }

  // Save to flash (outside mutex to avoid blocking other tasks)
  saveHeaterEnabled(enabled);

#ifndef NO_HARDWARE
  LOG_INFOF("Heater enabled state: %s", enabled ? "ENABLED" : "DISABLED");
#endif

  // Queue network sync message (non-blocking, handled by network tasks)
  Message msg = createSyncHeaterMessage();
  sendToNetworkSyncQueue(&msg);
}

void StateManager::setPumpState(PumpState newState) {
  // Check safety conditions with lock
  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in setPumpState");
      return;
    }

    if (state.emergencyShutdown && newState != PUMP_OFF) {
      unlock();
      LOG_WARN("Cannot enable pump during emergency shutdown");
      return;
    }

    // Safety check: prevent turning off pump if heater is on
    if (newState == PUMP_OFF && state.heaterStatus == HEATER_ON) {
      unlock();
      LOG_WARN("Cannot turn pump OFF while heater is ON (safety interlock)");
      return;
    }

    // Safety check: prevent turning off pump during cooldown
    if (newState == PUMP_OFF && state.inCooldown) {
      unlock();
      LOG_WARN("Cannot turn pump OFF during cooldown period");
      return;
    }

    state.pumpState = newState;
    unlock();
  }

  // Save to flash (outside mutex to avoid blocking other tasks)
  savePumpState(newState);

#ifndef NO_HARDWARE
  LOG_INFOF("Pump state: %s", getPumpStateString());
#endif

  // Queue network sync message (non-blocking, handled by network tasks)
  Message msg = createSyncPumpMessage();
  sendToNetworkSyncQueue(&msg);
}

void StateManager::setTargetTemperature(float newTemp) {
  // Enforce limits
  if (newTemp < MIN_TARGET_TEMP) newTemp = MIN_TARGET_TEMP;
  if (newTemp > MAX_TARGET_TEMP) newTemp = MAX_TARGET_TEMP;

  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in setTargetTemperature");
      return;
    }

    state.targetTemp = newTemp;
    unlock();
  }

  saveTargetTemp(newTemp);

#ifndef NO_HARDWARE
  LOG_INFOF("Target temperature: %.1f°F", newTemp);
#endif

  // Queue network sync message (non-blocking, handled by network tasks)
  Message msg = createSyncTargetTempMessage();
  sendToNetworkSyncQueue(&msg);
}

void StateManager::setMaxRuntimeHours(float hours) {
  if (hours < 0) hours = 0;
  if (hours > 24) hours = 24;

  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in setMaxRuntimeHours");
      return;
    }

    state.maxRuntimeHours = hours;
    unlock();
  }

  saveMaxRuntimeHours(hours);

  LOG_INFOF("Max runtime hours: %.1f", hours);
}

void StateManager::setHeaterStartHour(int hour) {
  if (hour < 0) hour = 0;
  if (hour > 23) hour = 23;

  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in setHeaterStartHour");
      return;
    }

    state.heaterStartHour = hour;
    unlock();
  }

  saveHeaterStartHour(hour);

  LOG_INFOF("Heater start hour: %d", hour);
}

void StateManager::setHeaterEndHour(int hour) {
  if (hour < 0) hour = 0;
  if (hour > 23) hour = 23;

  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in setHeaterEndHour");
      return;
    }

    state.heaterEndHour = hour;
    unlock();
  }

  saveHeaterEndHour(hour);

  LOG_INFOF("Heater end hour: %d", hour);
}

void StateManager::setMinHeaterTemp(float temp) {
  // Enforce safety limits
  if (temp < INVALID_TEMP_LOW) temp = INVALID_TEMP_LOW;
  if (temp > MAX_TARGET_TEMP) temp = MAX_TARGET_TEMP;

  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in setMinHeaterTemp");
      return;
    }

    state.minHeaterTemp = temp;
    unlock();
  }

  saveMinHeaterTemp(temp);

  LOG_INFOF("Minimum heater temp: %.1f°F", temp);
}

void StateManager::setMaxHeaterTemp(float temp) {
  // Enforce safety limits
  if (temp < MIN_TARGET_TEMP) temp = MIN_TARGET_TEMP;
  if (temp > MAX_HEATER_TEMP) temp = MAX_HEATER_TEMP;

  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in setMaxHeaterTemp");
      return;
    }

    state.maxHeaterTemp = temp;
    unlock();
  }

  saveMaxHeaterTemp(temp);

  LOG_INFOF("Maximum heater temp: %.1f°F", temp);
}

// ============================================
// EMERGENCY CONTROLS
// ============================================

void StateManager::triggerEmergencyShutdown() {
  LOG_ERROR("!!! EMERGENCY SHUTDOWN TRIGGERED !!!");

  // CRITICAL: Immediately disable hardware FIRST (no mutex delay for safety)
  digitalWrite(HEATER_PIN, LOW);
  digitalWrite(PUMP_ENABLE_PIN, LOW);

  // Now update state with mutex protection
  {
    if (lock()) {
      state.emergencyShutdown = true;
      state.heaterEnabled = false;
      state.heaterStatus = HEATER_OFF;
      state.pumpState = PUMP_OFF;
      state.inCooldown = false;
      unlock();
    } else {
      // If we can't get the lock, still set these directly for safety
      // This is an emergency - hardware is already off
      state.emergencyShutdown = true;
      state.heaterEnabled = false;
      state.heaterStatus = HEATER_OFF;
      state.pumpState = PUMP_OFF;
      state.inCooldown = false;
    }
  }

  // Save disabled states
  saveHeaterEnabled(false);
  savePumpState(PUMP_OFF);

  // Update all interfaces (display only - network sync via queue)
  displayEmergencyShutdown();

  // Queue network sync messages for both heater and pump (non-blocking)
  Message heaterMsg = createSyncHeaterMessage();
  sendToNetworkSyncQueue(&heaterMsg);
  Message pumpMsg = createSyncPumpMessage();
  sendToNetworkSyncQueue(&pumpMsg);
}

void StateManager::clearEmergencyShutdown() {
  {
    if (lock()) {
      state.emergencyShutdown = false;
      unlock();
    }
  }
  LOG_INFO("Emergency shutdown cleared");

  // NOTE: Network sync calls removed to prevent watchdog timeouts when called from FreeRTOS tasks
  // LCD task will update display on next cycle
}

// ============================================
// FAILSAFE MODE
// ============================================

void StateManager::enterFailsafeMode(const char* reason) {
  LOG_ERROR("!!! ENTERING FAILSAFE MODE !!!");
  LOG_ERRORF("Reason: %s", reason);

  // CRITICAL: Immediately disable heater hardware FIRST (no mutex delay for safety)
  digitalWrite(HEATER_PIN, LOW);

  // Force pump to LOW speed for water circulation (safety)
  digitalWrite(PUMP_ENABLE_PIN, HIGH);
  digitalWrite(PUMP_SPEED_PIN, LOW);

  // Update state with mutex protection
  {
    if (lock()) {
      state.inFailsafeMode = true;
      state.failsafeEnterTime = millis();
      strncpy(state.failsafeReason, reason, sizeof(state.failsafeReason) - 1);
      state.failsafeReason[sizeof(state.failsafeReason) - 1] = '\0';  // Ensure null termination

      // Force heater OFF
      state.heaterEnabled = false;
      state.heaterStatus = HEATER_OFF;

      // Force pump to LOW speed (minimum circulation)
      state.pumpState = PUMP_LOW;

      // End any cooldown (failsafe takes priority)
      state.inCooldown = false;
      unlock();
    } else {
      // If we can't get the lock, still set these directly for safety
      state.inFailsafeMode = true;
      state.failsafeEnterTime = millis();
      strncpy(state.failsafeReason, reason, sizeof(state.failsafeReason) - 1);
      state.failsafeReason[sizeof(state.failsafeReason) - 1] = '\0';
      state.heaterEnabled = false;
      state.heaterStatus = HEATER_OFF;
      state.pumpState = PUMP_LOW;
      state.inCooldown = false;
    }
  }

  // Save disabled heater state to NVS
  saveHeaterEnabled(false);

  // Log to NVS for post-mortem analysis
  LOG_ERROR("Failsafe event logged to NVS");

  // Queue network sync messages for heater and pump (non-blocking)
  Message heaterMsg = createSyncHeaterMessage();
  sendToNetworkSyncQueue(&heaterMsg);
  Message pumpMsg = createSyncPumpMessage();
  sendToNetworkSyncQueue(&pumpMsg);

  LOG_WARN("System in FAILSAFE MODE - manual intervention required");
}

void StateManager::clearFailsafeMode() {
  bool wasInFailsafe;

  {
    if (lock()) {
      wasInFailsafe = state.inFailsafeMode;
      state.inFailsafeMode = false;
      state.failsafeEnterTime = 0;
      state.failsafeReason[0] = '\0';
      unlock();
    } else {
      return;
    }
  }

  if (wasInFailsafe) {
    LOG_INFO("Failsafe mode cleared - system ready for operation");
    // NOTE: LCD update removed to prevent watchdog timeouts
    // LCD task will update display on next cycle
  }
}

// ============================================
// OVERHEAT PROTECTION
// ============================================

void StateManager::clearOverheatLockout() {
  bool wasInLockout;

  {
    if (lock()) {
      wasInLockout = state.overheatLockout;
      state.overheatLockout = false;
      state.overheatDetectedTime = 0;
      unlock();
    } else {
      return;
    }
  }

  if (wasInLockout) {
    LOG_INFO("Overheat lockout cleared - heater may be enabled");
    // NOTE: LCD update removed to prevent watchdog timeouts
    // LCD task will update display on next cycle
  }
}

// ============================================
// COOLDOWN MANAGEMENT
// ============================================

void StateManager::startCooldown() {
  bool inCooldown;
  PumpState currentPumpState;

  {
    if (!lock()) {
      LOG_ERROR("Failed to lock state for cooldown start");
      return;
    }

    inCooldown = state.inCooldown;
    currentPumpState = state.pumpState;
    unlock();
  }

  if (!inCooldown) {
    LOG_INFO("Starting 15-minute cooldown period");

    {
      if (lock()) {
        state.inCooldown = true;
        state.cooldownStartTime = millis();
        state.pumpStateBeforeCooldown = currentPumpState;
        unlock();
      }
    }

    // Ensure pump runs at minimum LOW speed
    if (currentPumpState == PUMP_OFF) {
      setPumpState(PUMP_LOW);  // Use centralized pump control (handles mutex)
    }

    // NOTE: LCD update removed to prevent watchdog timeouts
    // LCD task will update display on next cycle
  }
}

void StateManager::endCooldown() {
  bool inCooldown;
  PumpState pumpStateBeforeCooldown;

  {
    if (!lock()) {
      LOG_ERROR("Failed to lock state for cooldown end");
      return;
    }

    inCooldown = state.inCooldown;
    pumpStateBeforeCooldown = state.pumpStateBeforeCooldown;
    unlock();
  }

  if (inCooldown) {
    LOG_INFO("Cooldown period ended");

    {
      if (lock()) {
        state.inCooldown = false;
        unlock();
      }
    }

    // Return pump to user's preferred setting
    setPumpState(pumpStateBeforeCooldown);  // Handles mutex internally
  }
}

bool StateManager::isCooldownComplete() {
  bool inCooldown, inletTempValid, heaterTempValid;
  unsigned long cooldownStartTime;
  float inletTemp, heaterTemp;

  {
    if (!lock()) {
      // If we can't get the lock, assume cooldown is not complete (fail safe)
      return false;
    }

    inCooldown = state.inCooldown;
    if (!inCooldown) {
      unlock();
      return true;
    }

    cooldownStartTime = state.cooldownStartTime;
    inletTempValid = state.inletTempValid;
    heaterTempValid = state.heaterTempValid;
    inletTemp = state.inletTemp;
    heaterTemp = state.heaterTemp;
    unlock();
  }

  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - cooldownStartTime;

  // Sanity check: if elapsed time is impossibly large (> 1 hour),
  // assume overflow or error and end cooldown safely
  if (elapsed > COOLDOWN_SANITY_CHECK) {
    LOG_WARN("Cooldown timer anomaly detected (possible millis overflow), ending cooldown");
    return true;
  }

  // Check if 15 minutes elapsed
  if (elapsed >= COOLDOWN_DURATION) {
    return true;
  }

  // Check if temperatures equalized (within 2°F)
  if (inletTempValid && heaterTempValid) {
    float tempDiff = abs(heaterTemp - inletTemp);
    if (tempDiff <= TEMP_DIFFERENTIAL_THRESHOLD) {
      return true;
    }
  }

  return false;
}

// ============================================
// RUNTIME TRACKING
// ============================================

void StateManager::updatePumpRuntime() {
  unsigned long now = millis();
  unsigned long elapsed;
  PumpState currentPumpState;
  unsigned long lastRuntimeUpdate;
  int lastResetDay;

  // Acquire state mutex for thread-safe access
  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in updatePumpRuntime");
      return;
    }

    lastRuntimeUpdate = state.lastRuntimeUpdate;
    elapsed = now - lastRuntimeUpdate;

    // Enhanced overflow detection:
    // 1. Check for millis() overflow (now wrapped around to small value)
    // 2. Check for impossibly large elapsed times (> 1 hour)

    // Detect millis() overflow: if lastRuntimeUpdate is near max uint32 and now is small
    if (lastRuntimeUpdate > MILLIS_OVERFLOW_THRESHOLD && now < 60000) {
      LOG_WARNF("millis() overflow detected (last: %lu, now: %lu) - resetting runtime tracking",
                lastRuntimeUpdate, now);
      state.lastRuntimeUpdate = now;
      unlock();
      return;
    }

    // Sanity check: if elapsed time is impossibly large (> 1 hour),
    // assume system suspension or corruption occurred
    // In this case, don't accumulate runtime to avoid corruption
    if (elapsed > COOLDOWN_SANITY_CHECK) {  // 1 hour
      LOG_WARNF("Runtime tracking anomaly detected: %lu ms elapsed (> 1 hour) - skipping accumulation",
                elapsed);
      state.lastRuntimeUpdate = now;
      unlock();
      return;
    }

    // Only count runtime if pump is actually running
    currentPumpState = state.pumpState;
    if (currentPumpState != PUMP_OFF) {
      state.pumpRuntimeToday += elapsed;
    }

    state.lastRuntimeUpdate = now;
    lastResetDay = state.lastResetDay;
    unlock();
  }

  // Check if we need to reset daily runtime
  time_t now_t;
  struct tm timeinfo;
  time(&now_t);
  localtime_r(&now_t, &timeinfo);

  if (lastResetDay != timeinfo.tm_mday) {
    resetDailyRuntime();

    // Update lastResetDay with mutex protection
    {
      if (lock()) {
        state.lastResetDay = timeinfo.tm_mday;
        unlock();
      }
    }
  }
}

void StateManager::resetDailyRuntime() {
  LOG_INFO("Resetting daily pump runtime counter");

  if (!lock()) {
    LOG_ERROR("Failed to acquire state lock in resetDailyRuntime");
    return;
  }

  state.pumpRuntimeToday = 0;
  state.lastRuntimeUpdate = millis();
  unlock();
}

float StateManager::getPumpRuntimeHours() {
  if (!lock()) {
    LOG_ERROR("Failed to acquire state lock in getPumpRuntimeHours");
    return 0.0;
  }

  float hours = state.pumpRuntimeToday / 3600000.0;  // Convert ms to hours
  unlock();
  return hours;
}

// ============================================
// STATUS QUERIES
// ============================================

bool StateManager::isWithinOperatingHours() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  int currentHour = timeinfo.tm_hour;
  int heaterStartHour, heaterEndHour;

  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in isWithinOperatingHours");
      return false;  // Fail-safe: assume outside operating hours
    }

    heaterStartHour = state.heaterStartHour;
    heaterEndHour = state.heaterEndHour;
    unlock();
  }

  if (heaterStartHour <= heaterEndHour) {
    // Normal case: 6 AM to 10 PM
    return (currentHour >= heaterStartHour && currentHour < heaterEndHour);
  } else {
    // Wrap case: 10 PM to 6 AM (overnight)
    return (currentHour >= heaterStartHour || currentHour < heaterEndHour);
  }
}

bool StateManager::canPumpTurnOff() {
  if (!lock()) {
    LOG_ERROR("Failed to acquire state lock in canPumpTurnOff");
    return false;  // Fail-safe: cannot turn off
  }

  // Pump cannot turn off if heater is on
  if (state.heaterStatus == HEATER_ON) {
    unlock();
    return false;
  }

  // Pump cannot turn off during cooldown
  if (state.inCooldown) {
    unlock();
    return false;
  }

  unlock();
  return true;
}

const char* StateManager::getPumpStateString() {
  if (!lock()) {
    return "UNKNOWN";  // Safe fallback
  }

  if (state.inCooldown) {
    unlock();
    return "COOLDOWN";
  }

  const char* str;
  switch (state.pumpState) {
    case PUMP_OFF: str = "OFF"; break;
    case PUMP_LOW: str = "LOW"; break;
    case PUMP_HIGH: str = "HIGH"; break;
    default: str = "UNKNOWN"; break;
  }

  unlock();
  return str;
}

const char* StateManager::getHeaterStatusString() {
  if (!lock()) {
    return "UNKNOWN";  // Safe fallback
  }

  const char* str;
  switch (state.heaterStatus) {
    case HEATER_OFF: str = "OFF"; break;
    case HEATER_ENABLED: str = "ENABLED"; break;
    case HEATER_ON: str = "ON"; break;
    default: str = "UNKNOWN"; break;
  }

  unlock();
  return str;
}

// ============================================
// WIFI RUNTIME FAILURE TRACKING
// ============================================

void StateManager::recordWiFiDisconnect() {
  unsigned long now = millis();

  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in recordWiFiDisconnect");
      return;
    }

    // Add new disconnect timestamp to circular buffer
    state.disconnectTimestamps[state.disconnectCount % WIFI_MAX_DISCONNECTS] = now;
    state.disconnectCount++;
    unlock();
  }

  LOG_INFOF("WiFi disconnect recorded. Recent count: %d", getRecentDisconnectCount());
}

int StateManager::getRecentDisconnectCount() {
  unsigned long now = millis();
  int recentCount = 0;
  unsigned long timestamps[WIFI_MAX_DISCONNECTS];

  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in getRecentDisconnectCount");
      return 0;  // Safe fallback
    }

    // Copy timestamps under mutex protection
    for (int i = 0; i < WIFI_MAX_DISCONNECTS; i++) {
      timestamps[i] = state.disconnectTimestamps[i];
    }
    unlock();
  }

  // Count disconnects within the time window (outside mutex to reduce lock time)
  for (int i = 0; i < WIFI_MAX_DISCONNECTS; i++) {
    if (timestamps[i] > 0) {
      unsigned long age = now - timestamps[i];

      // Handle millis() overflow (happens every ~50 days)
      if (age < WIFI_DISCONNECT_WINDOW || age > MILLIS_OVERFLOW_THRESHOLD) {
        recentCount++;
      }
    }
  }

  return recentCount;
}

bool StateManager::isProvisioningHung() {
  bool provisioningActive;
  unsigned long provisioningStartTime;

  {
    if (!lock()) {
      LOG_ERROR("Failed to acquire state lock in isProvisioningHung");
      return false;  // Safe fallback
    }

    provisioningActive = state.provisioningActive;
    if (!provisioningActive) {
      unlock();
      return false;
    }

    provisioningStartTime = state.provisioningStartTime;
    unlock();
  }

  unsigned long now = millis();
  unsigned long elapsed = now - provisioningStartTime;

  // Handle millis() overflow
  if (elapsed > MILLIS_OVERFLOW_THRESHOLD) {
    // Overflow detected, reset tracking
    return false;
  }

  return elapsed >= WIFI_PROVISIONING_TIMEOUT;
}

void StateManager::clearWiFiRuntimeTracking() {
  if (!lock()) {
    LOG_ERROR("Failed to acquire state lock in clearWiFiRuntimeTracking");
    return;
  }

  state.provisioningActive = false;
  state.provisioningStartTime = 0;
  state.credentialsReceived = false;
  state.credReceivedTime = 0;
  state.disconnectCount = 0;
  for (int i = 0; i < WIFI_MAX_DISCONNECTS; i++) {
    state.disconnectTimestamps[i] = 0;
  }
  unlock();

  LOG_INFO("WiFi runtime tracking cleared");
}
