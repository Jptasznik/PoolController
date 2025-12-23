#define LOG_MODULE "STATE"

#include "StateManager.h"
#include "Logger.h"
#include "Config.h"
#include "StateMutex.h"
#include "SettingsManager.h"
#include "LCDDisplay.h"
#include "SinricIntegration.h"
#include "PumpControl.h"
#include "MessageQueue.h"
#include <time.h>

// Global state instance
SystemState state;

// ============================================
// INITIALIZATION
// ============================================

void initSystemState() {
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
// CENTRALIZED STATE SETTERS
// ============================================

void setHeaterEnabledState(bool enabled) {
  // CRITICAL: Acquire state mutex for thread-safe access
  StateScopedLock stateLock;
  if (!stateLock.isLocked()) {
    LOG_ERROR("Failed to acquire state lock in setHeaterEnabledState");
    return;
  }

  if (state.emergencyShutdown && enabled) {
    LOG_WARN("Cannot enable heater during emergency shutdown");
    return;
  }

  if (state.inFailsafeMode && enabled) {
    LOG_ERROR("Cannot enable heater while in FAILSAFE MODE");
    return;
  }

  if (state.overheatLockout && enabled) {
    LOG_ERROR("Cannot enable heater - OVERHEAT LOCKOUT active");
    LOG_ERROR("Clear lockout manually after verifying system safety");
    return;
  }

  state.heaterEnabled = enabled;
  saveHeaterEnabled(enabled);

#ifndef NO_HARDWARE
  LOG_INFOF("Heater enabled state: %s", enabled ? "ENABLED" : "DISABLED");
#endif

  // Queue network sync message (non-blocking, handled by network tasks)
  Message msg = createSyncHeaterMessage();
  sendToNetworkSyncQueue(&msg);
}

void setPumpState(PumpState newState) {
  // CRITICAL: Acquire state mutex for thread-safe access
  StateScopedLock stateLock;
  if (!stateLock.isLocked()) {
    LOG_ERROR("Failed to acquire state lock in setPumpState");
    return;
  }

  if (state.emergencyShutdown && newState != PUMP_OFF) {
    LOG_WARN("Cannot enable pump during emergency shutdown");
    return;
  }

  // Safety check: prevent turning off pump if heater is on
  if (newState == PUMP_OFF && state.heaterStatus == HEATER_ON) {
    LOG_WARN("Cannot turn pump OFF while heater is ON (safety interlock)");
    return;
  }

  // Safety check: prevent turning off pump during cooldown
  if (newState == PUMP_OFF && state.inCooldown) {
    LOG_WARN("Cannot turn pump OFF during cooldown period");
    return;
  }

  state.pumpState = newState;
  savePumpState(newState);

#ifndef NO_HARDWARE
  LOG_INFOF("Pump state: %s", getPumpStateString());
#endif

  // Queue network sync message (non-blocking, handled by network tasks)
  Message msg = createSyncPumpMessage();
  sendToNetworkSyncQueue(&msg);
}

void setTargetTemperature(float newTemp) {
  // Enforce limits
  if (newTemp < MIN_TARGET_TEMP) newTemp = MIN_TARGET_TEMP;
  if (newTemp > MAX_TARGET_TEMP) newTemp = MAX_TARGET_TEMP;

  // CRITICAL: Acquire state mutex for thread-safe access
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to acquire state lock in setTargetTemperature");
      return;
    }

    state.targetTemp = newTemp;
  }

  saveTargetTemp(newTemp);

#ifndef NO_HARDWARE
  LOG_INFOF("Target temperature: %.1f°F", newTemp);
#endif

  // Queue network sync message (non-blocking, handled by network tasks)
  Message msg = createSyncTargetTempMessage();
  sendToNetworkSyncQueue(&msg);
}

void setMaxRuntimeHours(float hours) {
  if (hours < 0) hours = 0;
  if (hours > 24) hours = 24;

  // CRITICAL: Acquire state mutex for thread-safe access
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to acquire state lock in setMaxRuntimeHours");
      return;
    }

    state.maxRuntimeHours = hours;
  }

  saveMaxRuntimeHours(hours);

  LOG_INFOF("Max runtime hours: %.1f", hours);
}

void setHeaterStartHour(int hour) {
  if (hour < 0) hour = 0;
  if (hour > 23) hour = 23;

  // CRITICAL: Acquire state mutex for thread-safe access
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to acquire state lock in setHeaterStartHour");
      return;
    }

    state.heaterStartHour = hour;
  }

  saveHeaterStartHour(hour);

  LOG_INFOF("Heater start hour: %d", hour);
}

void setHeaterEndHour(int hour) {
  if (hour < 0) hour = 0;
  if (hour > 23) hour = 23;

  // CRITICAL: Acquire state mutex for thread-safe access
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to acquire state lock in setHeaterEndHour");
      return;
    }

    state.heaterEndHour = hour;
  }

  saveHeaterEndHour(hour);

  LOG_INFOF("Heater end hour: %d", hour);
}

void setMinHeaterTemp(float temp) {
  // Enforce safety limits
  if (temp < INVALID_TEMP_LOW) temp = INVALID_TEMP_LOW;
  if (temp > MAX_TARGET_TEMP) temp = MAX_TARGET_TEMP;

  // CRITICAL: Acquire state mutex for thread-safe access
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to acquire state lock in setMinHeaterTemp");
      return;
    }

    state.minHeaterTemp = temp;
  }

  saveMinHeaterTemp(temp);

  LOG_INFOF("Minimum heater temp: %.1f°F", temp);
}

void setMaxHeaterTemp(float temp) {
  // Enforce safety limits
  if (temp < MIN_TARGET_TEMP) temp = MIN_TARGET_TEMP;
  if (temp > MAX_HEATER_TEMP) temp = MAX_HEATER_TEMP;

  // CRITICAL: Acquire state mutex for thread-safe access
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to acquire state lock in setMaxHeaterTemp");
      return;
    }

    state.maxHeaterTemp = temp;
  }

  saveMaxHeaterTemp(temp);

  LOG_INFOF("Maximum heater temp: %.1f°F", temp);
}

// ============================================
// EMERGENCY CONTROLS
// ============================================

void triggerEmergencyShutdown() {
  LOG_ERROR("!!! EMERGENCY SHUTDOWN TRIGGERED !!!");

  // CRITICAL: Immediately disable hardware FIRST (no mutex delay for safety)
  digitalWrite(HEATER_PIN, LOW);
  digitalWrite(PUMP_ENABLE_PIN, LOW);

  // Now update state with mutex protection
  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      state.emergencyShutdown = true;
      state.heaterEnabled = false;
      state.heaterStatus = HEATER_OFF;
      state.pumpState = PUMP_OFF;
      state.inCooldown = false;
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

void clearEmergencyShutdown() {
  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      state.emergencyShutdown = false;
    }
  }
  LOG_INFO("Emergency shutdown cleared");

  // NOTE: Network sync calls removed to prevent watchdog timeouts when called from FreeRTOS tasks
  // LCD task will update display on next cycle
}

// ============================================
// FAILSAFE MODE
// ============================================

void enterFailsafeMode(const char* reason) {
  LOG_ERROR("!!! ENTERING FAILSAFE MODE !!!");
  LOG_ERRORF("Reason: %s", reason);

  // CRITICAL: Immediately disable heater hardware FIRST (no mutex delay for safety)
  digitalWrite(HEATER_PIN, LOW);

  // Force pump to LOW speed for water circulation (safety)
  digitalWrite(PUMP_ENABLE_PIN, HIGH);
  digitalWrite(PUMP_SPEED_PIN, LOW);

  // Update state with mutex protection
  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
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

void clearFailsafeMode() {
  bool wasInFailsafe;

  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      wasInFailsafe = state.inFailsafeMode;
      state.inFailsafeMode = false;
      state.failsafeEnterTime = 0;
      state.failsafeReason[0] = '\0';
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

void clearOverheatLockout() {
  bool wasInLockout;

  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      wasInLockout = state.overheatLockout;
      state.overheatLockout = false;
      state.overheatDetectedTime = 0;
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

void startCooldown() {
  bool inCooldown;
  PumpState currentPumpState;

  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to lock state for cooldown start");
      return;
    }

    inCooldown = state.inCooldown;
    currentPumpState = state.pumpState;
  }

  if (!inCooldown) {
    LOG_INFO("Starting 15-minute cooldown period");

    {
      StateScopedLock stateLock;
      if (stateLock.isLocked()) {
        state.inCooldown = true;
        state.cooldownStartTime = millis();
        state.pumpStateBeforeCooldown = currentPumpState;
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

void endCooldown() {
  bool inCooldown;
  PumpState pumpStateBeforeCooldown;

  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to lock state for cooldown end");
      return;
    }

    inCooldown = state.inCooldown;
    pumpStateBeforeCooldown = state.pumpStateBeforeCooldown;
  }

  if (inCooldown) {
    LOG_INFO("Cooldown period ended");

    {
      StateScopedLock stateLock;
      if (stateLock.isLocked()) {
        state.inCooldown = false;
      }
    }

    // Return pump to user's preferred setting
    setPumpState(pumpStateBeforeCooldown);  // Handles mutex internally
  }
}

bool isCooldownComplete() {
  bool inCooldown, inletTempValid, heaterTempValid;
  unsigned long cooldownStartTime;
  float inletTemp, heaterTemp;

  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      // If we can't get the lock, assume cooldown is not complete (fail safe)
      return false;
    }

    inCooldown = state.inCooldown;
    if (!inCooldown) return true;

    cooldownStartTime = state.cooldownStartTime;
    inletTempValid = state.inletTempValid;
    heaterTempValid = state.heaterTempValid;
    inletTemp = state.inletTemp;
    heaterTemp = state.heaterTemp;
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

void updatePumpRuntime() {
  unsigned long now = millis();
  unsigned long elapsed;
  PumpState currentPumpState;
  unsigned long lastRuntimeUpdate;
  int lastResetDay;

  // CRITICAL: Acquire state mutex for thread-safe access
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
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
      return;
    }

    // Sanity check: if elapsed time is impossibly large (> 1 hour),
    // assume system suspension or corruption occurred
    // In this case, don't accumulate runtime to avoid corruption
    if (elapsed > COOLDOWN_SANITY_CHECK) {  // 1 hour
      LOG_WARNF("Runtime tracking anomaly detected: %lu ms elapsed (> 1 hour) - skipping accumulation",
                elapsed);
      state.lastRuntimeUpdate = now;
      return;
    }

    // Only count runtime if pump is actually running
    currentPumpState = state.pumpState;
    if (currentPumpState != PUMP_OFF) {
      state.pumpRuntimeToday += elapsed;
    }

    state.lastRuntimeUpdate = now;
    lastResetDay = state.lastResetDay;
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
      StateScopedLock stateLock;
      if (stateLock.isLocked()) {
        state.lastResetDay = timeinfo.tm_mday;
      }
    }
  }
}

void resetDailyRuntime() {
  LOG_INFO("Resetting daily pump runtime counter");

  // CRITICAL: Acquire state mutex for thread-safe access
  StateScopedLock stateLock;
  if (!stateLock.isLocked()) {
    LOG_ERROR("Failed to acquire state lock in resetDailyRuntime");
    return;
  }

  state.pumpRuntimeToday = 0;
  state.lastRuntimeUpdate = millis();
}

float getPumpRuntimeHours() {
  // CRITICAL: Acquire state mutex for thread-safe access
  StateScopedLock stateLock;
  if (!stateLock.isLocked()) {
    LOG_ERROR("Failed to acquire state lock in getPumpRuntimeHours");
    return 0.0;
  }

  return state.pumpRuntimeToday / 3600000.0;  // Convert ms to hours
}

// ============================================
// STATUS QUERIES
// ============================================

bool isWithinOperatingHours() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);

  int currentHour = timeinfo.tm_hour;
  int heaterStartHour, heaterEndHour;

  // CRITICAL: Acquire state mutex for thread-safe access
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to acquire state lock in isWithinOperatingHours");
      return false;  // Fail-safe: assume outside operating hours
    }

    heaterStartHour = state.heaterStartHour;
    heaterEndHour = state.heaterEndHour;
  }

  if (heaterStartHour <= heaterEndHour) {
    // Normal case: 6 AM to 10 PM
    return (currentHour >= heaterStartHour && currentHour < heaterEndHour);
  } else {
    // Wrap case: 10 PM to 6 AM (overnight)
    return (currentHour >= heaterStartHour || currentHour < heaterEndHour);
  }
}

bool canPumpTurnOff() {
  // CRITICAL: Acquire state mutex for thread-safe access
  StateScopedLock stateLock;
  if (!stateLock.isLocked()) {
    LOG_ERROR("Failed to acquire state lock in canPumpTurnOff");
    return false;  // Fail-safe: cannot turn off
  }

  // Pump cannot turn off if heater is on
  if (state.heaterStatus == HEATER_ON) return false;

  // Pump cannot turn off during cooldown
  if (state.inCooldown) return false;

  return true;
}

const char* getPumpStateString() {
  // CRITICAL: Acquire state mutex for thread-safe access
  StateScopedLock stateLock;
  if (!stateLock.isLocked()) {
    return "UNKNOWN";  // Safe fallback
  }

  if (state.inCooldown) return "COOLDOWN";

  switch (state.pumpState) {
    case PUMP_OFF: return "OFF";
    case PUMP_LOW: return "LOW";
    case PUMP_HIGH: return "HIGH";
    default: return "UNKNOWN";
  }
}

const char* getHeaterStatusString() {
  // CRITICAL: Acquire state mutex for thread-safe access
  StateScopedLock stateLock;
  if (!stateLock.isLocked()) {
    return "UNKNOWN";  // Safe fallback
  }

  switch (state.heaterStatus) {
    case HEATER_OFF: return "OFF";
    case HEATER_ENABLED: return "ENABLED";
    case HEATER_ON: return "ON";
    default: return "UNKNOWN";
  }
}

// ============================================
// WIFI RUNTIME FAILURE TRACKING
// ============================================

void recordWiFiDisconnect() {
  unsigned long now = millis();

  // CRITICAL: Acquire state mutex for thread-safe access
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to acquire state lock in recordWiFiDisconnect");
      return;
    }

    // Add new disconnect timestamp to circular buffer
    state.disconnectTimestamps[state.disconnectCount % WIFI_MAX_DISCONNECTS] = now;
    state.disconnectCount++;
  }

  LOG_INFOF("WiFi disconnect recorded. Recent count: %d", getRecentDisconnectCount());
}

int getRecentDisconnectCount() {
  unsigned long now = millis();
  int recentCount = 0;
  unsigned long timestamps[WIFI_MAX_DISCONNECTS];

  // CRITICAL: Acquire state mutex for thread-safe access
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to acquire state lock in getRecentDisconnectCount");
      return 0;  // Safe fallback
    }

    // Copy timestamps under mutex protection
    for (int i = 0; i < WIFI_MAX_DISCONNECTS; i++) {
      timestamps[i] = state.disconnectTimestamps[i];
    }
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

bool isProvisioningHung() {
  bool provisioningActive;
  unsigned long provisioningStartTime;

  // CRITICAL: Acquire state mutex for thread-safe access
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      LOG_ERROR("Failed to acquire state lock in isProvisioningHung");
      return false;  // Safe fallback
    }

    provisioningActive = state.provisioningActive;
    if (!provisioningActive) {
      return false;
    }

    provisioningStartTime = state.provisioningStartTime;
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

void clearWiFiRuntimeTracking() {
  // CRITICAL: Acquire state mutex for thread-safe access
  StateScopedLock stateLock;
  if (!stateLock.isLocked()) {
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
  LOG_INFO("WiFi runtime tracking cleared");
}
