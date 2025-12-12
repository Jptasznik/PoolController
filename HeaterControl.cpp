#include "HeaterControl.h"
#include "Config.h"
#include "StateManager.h"
#include "StateMutex.h"
#include "PumpControl.h"

// ============================================
// INITIALIZATION
// ============================================

void initHeaterControl() {
  Serial.println("Initializing heater control...");
  
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);
  
  Serial.println("Heater control initialized");
}

// ============================================
// CONTROL SYSTEM LOGIC
// ============================================

void runControlSystem() {
  // Check if heater should be on (this function handles mutex internally)
  bool shouldBeOn = shouldHeaterBeOn();

  HeaterStatus previousStatus, newStatus;
  PumpState currentPumpState;
  bool heaterEnabled, inCooldown;
  float inletTemp, targetTemp;

  // Read current state with mutex
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      Serial.println("ERROR: Control system mutex timeout");
      return;
    }

    previousStatus = state.heaterStatus;
    heaterEnabled = state.heaterEnabled;
    currentPumpState = state.pumpState;
    inCooldown = state.inCooldown;
    inletTemp = state.inletTemp;
    targetTemp = state.targetTemp;

    // Determine new status
    if (shouldBeOn) {
      newStatus = HEATER_ON;
    } else {
      newStatus = heaterEnabled ? HEATER_ENABLED : HEATER_OFF;
    }

    // Update heater status
    state.heaterStatus = newStatus;
  }

  // CRITICAL SAFETY: Ensure pump runs when heater is on
  if (newStatus == HEATER_ON && currentPumpState == PUMP_OFF) {
    Serial.println("SAFETY: Starting pump (LOW) because heater is ON");
    setPumpState(PUMP_LOW);  // This function handles mutex internally
  }

  // Apply heater state to hardware
  applyHeaterState();

  // Handle transitions
  if (previousStatus == HEATER_ON && newStatus != HEATER_ON) {
    Serial.println("Heater turned OFF - starting cooldown");
    startCooldown();
  }

  // Check cooldown completion
  if (inCooldown && isCooldownComplete()) {
    endCooldown();
  }

  // Log state changes
  if (previousStatus != newStatus) {
    Serial.print("Heater status changed: ");
    Serial.print(getHeaterStatusString());
    Serial.print(" (Inlet: ");
    Serial.print(inletTemp, 1);
    Serial.print("°F, Target: ");
    Serial.print(targetTemp, 1);
    Serial.println("°F)");
  }
}

// ============================================
// HEATER STATE APPLICATION
// ============================================

void applyHeaterState() {
  HeaterStatus currentStatus;

  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      Serial.println("ERROR: Failed to read heater status for hardware apply");
      return;
    }
    currentStatus = state.heaterStatus;
  }

  if (currentStatus == HEATER_ON) {
    digitalWrite(HEATER_PIN, HIGH);
  } else {
    digitalWrite(HEATER_PIN, LOW);
  }
}

// ============================================
// HEATER DECISION LOGIC
// ============================================

bool shouldHeaterBeOn() {
  // Read all state values with mutex protection
  bool heaterEnabled, emergencyShutdown, inletTempValid, heaterTempValid;
  float inletTemp, heaterTemp, minHeaterTemp, maxHeaterTemp, targetTemp;

  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      Serial.println("ERROR: Failed to read state in shouldHeaterBeOn()");
      return false;  // Fail safe - don't turn on heater
    }

    heaterEnabled = state.heaterEnabled;
    emergencyShutdown = state.emergencyShutdown;
    inletTempValid = state.inletTempValid;
    heaterTempValid = state.heaterTempValid;
    inletTemp = state.inletTemp;
    heaterTemp = state.heaterTemp;
    minHeaterTemp = state.minHeaterTemp;
    maxHeaterTemp = state.maxHeaterTemp;
    targetTemp = state.targetTemp;
  }

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
    Serial.println("!!! HEATER OVERHEAT DETECTED !!!");
    Serial.print("Heater temperature: ");
    Serial.print(heaterTemp, 1);
    Serial.println("°F");
    triggerEmergencyShutdown();
    return false;
  }

  // 4. Within operating hours
  if (!isWithinOperatingHours()) {
    return false;
  }

  // 5. Valid temperature reading
  if (!inletTempValid) {
    Serial.println("Heater OFF: Invalid temperature sensor");
    return false;
  }

  // 6. Temperature check (hard safety limits)
  if (inletTemp < INVALID_TEMP_LOW || inletTemp > INVALID_TEMP_HIGH) {
    Serial.println("Heater OFF: Temperature out of safe range");
    return false;
  }

  // 7. User-configurable minimum heater temperature
  if (inletTemp < minHeaterTemp) {
    Serial.print("Heater OFF: Water temp ");
    Serial.print(inletTemp, 1);
    Serial.print("°F below minimum ");
    Serial.print(minHeaterTemp, 1);
    Serial.println("°F");
    return false;
  }

  // 8. User-configurable maximum heater temperature
  if (inletTemp >= maxHeaterTemp) {
    Serial.print("Heater OFF: Water temp ");
    Serial.print(inletTemp, 1);
    Serial.print("°F at or above maximum ");
    Serial.print(maxHeaterTemp, 1);
    Serial.println("°F");
    return false;
  }

  // 9. Runtime limit check
  if (isRuntimeLimitExceeded()) {
    Serial.println("Heater OFF: Daily runtime limit exceeded");
    return false;
  }

  // 10. Inlet temperature < Target temperature
  if (inletTemp >= targetTemp) {
    return false;  // Target reached or exceeded
  }

  // All conditions met - heater should be ON
  return true;
}
