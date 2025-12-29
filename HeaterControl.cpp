#define LOG_MODULE "HEATER"

#include "HeaterControl.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"
#include "SettingsManager.h"
#include "PumpControl.h"

// ============================================
// INITIALIZATION
// ============================================

void initHeaterControl() {
#ifdef ENABLE_HEATER
  LOG_INFO("Initializing heater control...");

  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);

  LOG_INFO("Heater control initialized");
#else
  LOG_WARN("Heater control DISABLED (ENABLE_HEATER not defined)");
#endif
}

// ============================================
// CONTROL SYSTEM LOGIC
// ============================================

void runControlSystem() {
  // CRITICAL SAFETY: Check for overheat condition FIRST
  checkEmergencyOverheat();

  // Check if heater should be on (this function handles mutex internally)
  bool shouldBeOn = shouldHeaterBeOn();

  // Get current state using StateManager API
  StateManager& stateMgr = StateManager::getInstance();

  HeaterStatus previousStatus = stateMgr.getHeaterStatus();
  bool heaterEnabled = stateMgr.isHeaterEnabled();
  PumpState currentPumpState = stateMgr.getPumpState();
  bool inCooldown = stateMgr.isInCooldown();
  float inletTemp = stateMgr.getInletTemp();
  float targetTemp = stateMgr.getTargetTemp();

  // Determine new status
  HeaterStatus newStatus;
  if (shouldBeOn) {
    newStatus = HEATER_ON;
  } else {
    newStatus = heaterEnabled ? HEATER_ENABLED : HEATER_OFF;
  }

  // Update heater status
  stateMgr.setHeaterStatus(newStatus);

  // CRITICAL SAFETY: Ensure pump runs when heater is on
  if (newStatus == HEATER_ON && currentPumpState == PUMP_OFF) {
    LOG_WARN("SAFETY: Starting pump (LOW) because heater is ON");
    setPumpState(PUMP_LOW);  // This function handles mutex internally
  }

  // Apply heater state to hardware
  applyHeaterState();

  // Handle transitions
  if (previousStatus == HEATER_ON && newStatus != HEATER_ON) {
    LOG_INFO("Heater turned OFF - starting cooldown");
    startCooldown();
  }

  // Check cooldown completion
  if (inCooldown && isCooldownComplete()) {
    endCooldown();
  }

  // Log state changes
#ifndef NO_HARDWARE
  if (previousStatus != newStatus) {
    LOG_INFOF("Heater status changed: %s (Inlet: %.1f°F, Target: %.1f°F)",
              getHeaterStatusString(), inletTemp, targetTemp);
  }
#endif
}

// ============================================
// HEATER STATE APPLICATION
// ============================================

void applyHeaterState() {
#ifdef ENABLE_HEATER
  StateManager& stateMgr = StateManager::getInstance();
  HeaterStatus currentStatus = stateMgr.getHeaterStatus();

  if (currentStatus == HEATER_ON) {
    digitalWrite(HEATER_PIN, HIGH);
  } else {
    digitalWrite(HEATER_PIN, LOW);
  }
#else
  // Heater disabled - do nothing
#endif
}

// ============================================
// HEATER DECISION LOGIC
// ============================================

bool shouldHeaterBeOn() {
  // Read all state values using StateManager API
  StateManager& stateMgr = StateManager::getInstance();

  bool heaterEnabled = stateMgr.isHeaterEnabled();
  bool emergencyShutdown = stateMgr.isEmergencyShutdown();
  bool inletTempValid = stateMgr.isInletTempValid();
  bool heaterTempValid = stateMgr.isHeaterTempValid();
  float inletTemp = stateMgr.getInletTemp();
  float heaterTemp = stateMgr.getHeaterTemp();
  float minHeaterTemp = stateMgr.getMinHeaterTemp();
  float maxHeaterTemp = stateMgr.getMaxHeaterTemp();
  float targetTemp = stateMgr.getTargetTemp();

  // Heater turns ON when ALL conditions met:
  // 1. Heater enabled by user
  if (!heaterEnabled) {
    return false;
  }

  // 2. Emergency shutdown not active
  if (emergencyShutdown) {
    return false;
  }

  // 3. SAFETY: Check heater outlet temperature for overheat
  if (heaterTempValid && heaterTemp > MAX_HEATER_TEMP) {
    LOG_ERROR("!!! HEATER OVERHEAT DETECTED !!!");
#ifndef NO_HARDWARE
    LOG_ERRORF("Heater temperature: %.1f°F", heaterTemp);
#endif
    triggerEmergencyShutdown();
    return false;
  }

  // 4. Within operating hours
  if (!isWithinOperatingHours()) {
    return false;
  }

  // 5. Valid temperature reading
  if (!inletTempValid) {
    LOG_WARN("Heater OFF: Invalid temperature sensor");
    return false;
  }

  // 6. Temperature check (hard safety limits)
  if (inletTemp < INVALID_TEMP_LOW || inletTemp > INVALID_TEMP_HIGH) {
    LOG_WARN("Heater OFF: Temperature out of safe range");
    return false;
  }

  // 7. User-configurable minimum heater temperature
  if (inletTemp < minHeaterTemp) {
#ifndef NO_HARDWARE
    LOG_WARNF("Heater OFF: Water temp %.1f°F below minimum %.1f°F", inletTemp, minHeaterTemp);
#endif
    return false;
  }

  // 8. User-configurable maximum heater temperature
  if (inletTemp >= maxHeaterTemp) {
#ifndef NO_HARDWARE
    LOG_WARNF("Heater OFF: Water temp %.1f°F at or above maximum %.1f°F", inletTemp, maxHeaterTemp);
#endif
    return false;
  }

  // 9. Runtime limit check
  if (isRuntimeLimitExceeded()) {
    LOG_WARN("Heater OFF: Daily runtime limit exceeded");
    return false;
  }

  // 10. Inlet temperature < Target temperature
  if (inletTemp >= targetTemp) {
    return false;  // Target reached or exceeded
  }

  // All conditions met - heater should be ON
  return true;
}

// ============================================
// EMERGENCY OVERHEAT PROTECTION
// ============================================

void checkEmergencyOverheat() {
  // Read critical state using StateManager API
  StateManager& stateMgr = StateManager::getInstance();

  bool heaterTempValid = stateMgr.isHeaterTempValid();
  float heaterTemp = stateMgr.getHeaterTemp();
  bool inFailsafeMode = stateMgr.isInFailsafeMode();
  bool overheatLockout = stateMgr.isOverheatLockout();

  // Only check if we have valid temperature and not already in failsafe/lockout
  if (!heaterTempValid || inFailsafeMode || overheatLockout) {
    return;
  }

  // CRITICAL: Check for overheat condition
  if (heaterTemp > MAX_HEATER_TEMP) {
    LOG_ERROR("!!! EMERGENCY: HEATER OVERHEAT DETECTED !!!");
#ifndef NO_HARDWARE
    LOG_ERRORF("Heater temp: %.1f°F (max: %.1f°F)", heaterTemp, MAX_HEATER_TEMP);
#endif

#ifndef NO_HARDWARE
    // Immediate hardware shutdown (critical safety)
    digitalWrite(HEATER_PIN, LOW);

    // Set overheat lockout state using StateManager API
    StateManager& stateMgr = StateManager::getInstance();
    stateMgr.setOverheatLockout(true);
    stateMgr.setOverheatDetectedTime(millis());

    // Enter failsafe mode
    enterFailsafeMode("HEATER_OVERHEAT");

    // Disable heater in settings (requires manual re-enable)
    saveHeaterEnabled(false);

    LOG_ERROR("OVERHEAT LOCKOUT active - manual intervention required!");
    LOG_ERROR("System will NOT automatically recover");
#else
    LOG_WARN("NO_HARDWARE mode: Skipping failsafe trigger for overheat");
#endif
  }
}
