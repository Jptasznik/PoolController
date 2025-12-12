#include "StateManager.h"
#include "Config.h"
#include "StateMutex.h"
#include "SettingsManager.h"
#include "LCDDisplay.h"
#include "SinricIntegration.h"
#include "PumpControl.h"
#include <time.h>

// Forward declarations for Blynk sync functions (implemented in PoolController3.ino)
void syncHeaterToBlynk();
void syncPumpToBlynk();
void syncTargetTempToBlynk();

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
  state.disconnectCount = 0;
  for (int i = 0; i < WIFI_MAX_DISCONNECTS; i++) {
    state.disconnectTimestamps[i] = 0;
  }

  state.emergencyShutdown = false;
}

// ============================================
// CENTRALIZED STATE SETTERS
// ============================================

void setHeaterEnabledState(bool enabled) {
  if (state.emergencyShutdown && enabled) {
    Serial.println("Cannot enable heater during emergency shutdown");
    return;
  }
  
  state.heaterEnabled = enabled;
  saveHeaterEnabled(enabled);
  
  Serial.print("Heater enabled state: ");
  Serial.println(enabled ? "ENABLED" : "DISABLED");
  
  // Update all interfaces
  updateLCDDisplay();
  syncHeaterToBlynk();
  syncHeaterToSinric();
}

void setPumpState(PumpState newState) {
  if (state.emergencyShutdown && newState != PUMP_OFF) {
    Serial.println("Cannot enable pump during emergency shutdown");
    return;
  }
  
  // Safety check: prevent turning off pump if heater is on
  if (newState == PUMP_OFF && state.heaterStatus == HEATER_ON) {
    Serial.println("Cannot turn pump OFF while heater is ON (safety interlock)");
    return;
  }
  
  // Safety check: prevent turning off pump during cooldown
  if (newState == PUMP_OFF && state.inCooldown) {
    Serial.println("Cannot turn pump OFF during cooldown period");
    return;
  }
  
  state.pumpState = newState;
  savePumpState(newState);
  
  Serial.print("Pump state: ");
  Serial.println(getPumpStateString());
  
  // Update all interfaces
  updateLCDDisplay();
  syncPumpToBlynk();
  syncPumpToSinric();
}

void setTargetTemperature(float newTemp) {
  // Enforce limits
  if (newTemp < MIN_TARGET_TEMP) newTemp = MIN_TARGET_TEMP;
  if (newTemp > MAX_TARGET_TEMP) newTemp = MAX_TARGET_TEMP;
  
  state.targetTemp = newTemp;
  saveTargetTemp(newTemp);
  
  Serial.print("Target temperature: ");
  Serial.print(newTemp, 1);
  Serial.println("°F");
  
  // Update all interfaces
  updateLCDDisplay();
  syncTargetTempToBlynk();
  syncTargetTempToSinric();
}

void setMaxRuntimeHours(float hours) {
  if (hours < 0) hours = 0;
  if (hours > 24) hours = 24;
  
  state.maxRuntimeHours = hours;
  saveMaxRuntimeHours(hours);
  
  Serial.print("Max runtime hours: ");
  Serial.println(hours, 1);
}

void setHeaterStartHour(int hour) {
  if (hour < 0) hour = 0;
  if (hour > 23) hour = 23;
  
  state.heaterStartHour = hour;
  saveHeaterStartHour(hour);
  
  Serial.print("Heater start hour: ");
  Serial.println(hour);
}

void setHeaterEndHour(int hour) {
  if (hour < 0) hour = 0;
  if (hour > 23) hour = 23;

  state.heaterEndHour = hour;
  saveHeaterEndHour(hour);

  Serial.print("Heater end hour: ");
  Serial.println(hour);
}

void setMinHeaterTemp(float temp) {
  // Enforce safety limits
  if (temp < INVALID_TEMP_LOW) temp = INVALID_TEMP_LOW;
  if (temp > MAX_TARGET_TEMP) temp = MAX_TARGET_TEMP;

  state.minHeaterTemp = temp;
  saveMinHeaterTemp(temp);

  Serial.print("Minimum heater temp: ");
  Serial.print(temp, 1);
  Serial.println("°F");
}

void setMaxHeaterTemp(float temp) {
  // Enforce safety limits
  if (temp < MIN_TARGET_TEMP) temp = MIN_TARGET_TEMP;
  if (temp > MAX_HEATER_TEMP) temp = MAX_HEATER_TEMP;

  state.maxHeaterTemp = temp;
  saveMaxHeaterTemp(temp);

  Serial.print("Maximum heater temp: ");
  Serial.print(temp, 1);
  Serial.println("°F");
}

// ============================================
// EMERGENCY CONTROLS
// ============================================

void triggerEmergencyShutdown() {
  Serial.println("!!! EMERGENCY SHUTDOWN TRIGGERED !!!");

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

  // Update all interfaces
  displayEmergencyShutdown();
  syncHeaterToBlynk();
  syncPumpToBlynk();
  syncHeaterToSinric();
  syncPumpToSinric();
}

void clearEmergencyShutdown() {
  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      state.emergencyShutdown = false;
    }
  }
  Serial.println("Emergency shutdown cleared");
  updateLCDDisplay();
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
      Serial.println("ERROR: Failed to lock state for cooldown start");
      return;
    }

    inCooldown = state.inCooldown;
    currentPumpState = state.pumpState;
  }

  if (!inCooldown) {
    Serial.println("Starting 15-minute cooldown period");

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

    updateLCDDisplay();
  }
}

void endCooldown() {
  bool inCooldown;
  PumpState pumpStateBeforeCooldown;

  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      Serial.println("ERROR: Failed to lock state for cooldown end");
      return;
    }

    inCooldown = state.inCooldown;
    pumpStateBeforeCooldown = state.pumpStateBeforeCooldown;
  }

  if (inCooldown) {
    Serial.println("Cooldown period ended");

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
    Serial.println("WARNING: Cooldown timer anomaly detected (possible millis overflow), ending cooldown");
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
  unsigned long elapsed = now - state.lastRuntimeUpdate;

  // Sanity check: if elapsed time is impossibly large (> 1 hour),
  // assume millis() overflow or system suspension occurred
  // In this case, don't accumulate runtime to avoid corruption
  if (elapsed > 3600000) {  // 1 hour
    Serial.println("WARNING: Runtime tracking anomaly detected, skipping accumulation");
    state.lastRuntimeUpdate = now;
    return;
  }

  // Only count runtime if pump is actually running
  if (state.pumpState != PUMP_OFF) {
    state.pumpRuntimeToday += elapsed;
  }

  state.lastRuntimeUpdate = now;

  // Check if we need to reset daily runtime
  time_t now_t;
  struct tm timeinfo;
  time(&now_t);
  localtime_r(&now_t, &timeinfo);

  if (state.lastResetDay != timeinfo.tm_mday) {
    resetDailyRuntime();
    state.lastResetDay = timeinfo.tm_mday;
  }
}

void resetDailyRuntime() {
  Serial.println("Resetting daily pump runtime counter");
  state.pumpRuntimeToday = 0;
  state.lastRuntimeUpdate = millis();
}

float getPumpRuntimeHours() {
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
  
  if (state.heaterStartHour <= state.heaterEndHour) {
    // Normal case: 6 AM to 10 PM
    return (currentHour >= state.heaterStartHour && currentHour < state.heaterEndHour);
  } else {
    // Wrap case: 10 PM to 6 AM (overnight)
    return (currentHour >= state.heaterStartHour || currentHour < state.heaterEndHour);
  }
}

bool canPumpTurnOff() {
  // Pump cannot turn off if heater is on
  if (state.heaterStatus == HEATER_ON) return false;
  
  // Pump cannot turn off during cooldown
  if (state.inCooldown) return false;
  
  return true;
}

const char* getPumpStateString() {
  if (state.inCooldown) return "COOLDOWN";

  switch (state.pumpState) {
    case PUMP_OFF: return "OFF";
    case PUMP_LOW: return "LOW";
    case PUMP_HIGH: return "HIGH";
    default: return "UNKNOWN";
  }
}

const char* getHeaterStatusString() {
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

  // Add new disconnect timestamp to circular buffer
  state.disconnectTimestamps[state.disconnectCount % WIFI_MAX_DISCONNECTS] = now;
  state.disconnectCount++;

  Serial.print("WiFi disconnect recorded. Recent count: ");
  Serial.println(getRecentDisconnectCount());
}

int getRecentDisconnectCount() {
  unsigned long now = millis();
  int recentCount = 0;

  // Count disconnects within the time window
  for (int i = 0; i < WIFI_MAX_DISCONNECTS; i++) {
    if (state.disconnectTimestamps[i] > 0) {
      unsigned long age = now - state.disconnectTimestamps[i];

      // Handle millis() overflow (happens every ~50 days)
      if (age < WIFI_DISCONNECT_WINDOW || age > MILLIS_OVERFLOW_THRESHOLD) {
        recentCount++;
      }
    }
  }

  return recentCount;
}

bool isProvisioningHung() {
  if (!state.provisioningActive) {
    return false;
  }

  unsigned long now = millis();
  unsigned long elapsed = now - state.provisioningStartTime;

  // Handle millis() overflow
  if (elapsed > MILLIS_OVERFLOW_THRESHOLD) {
    // Overflow detected, reset tracking
    return false;
  }

  return elapsed >= WIFI_PROVISIONING_TIMEOUT;
}

void clearWiFiRuntimeTracking() {
  state.provisioningActive = false;
  state.provisioningStartTime = 0;
  state.disconnectCount = 0;
  for (int i = 0; i < WIFI_MAX_DISCONNECTS; i++) {
    state.disconnectTimestamps[i] = 0;
  }
  Serial.println("WiFi runtime tracking cleared");
}
