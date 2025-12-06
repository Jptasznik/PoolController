#include "HeaterControl.h"
#include "Config.h"
#include "StateManager.h"
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
  // Check if heater should be on
  bool shouldBeOn = shouldHeaterBeOn();
  
  // Determine actual heater status
  HeaterStatus previousStatus = state.heaterStatus;
  
  if (shouldBeOn) {
    state.heaterStatus = HEATER_ON;
    
    // CRITICAL SAFETY: Ensure pump runs when heater is on
    if (state.pumpState == PUMP_OFF) {
      Serial.println("SAFETY: Starting pump (LOW) because heater is ON");
      state.pumpState = PUMP_LOW;
      applyPumpState();
    }
  } else {
    if (state.heaterEnabled) {
      state.heaterStatus = HEATER_ENABLED;
    } else {
      state.heaterStatus = HEATER_OFF;
    }
  }
  
  // Apply heater state to hardware
  applyHeaterState();
  
  // Handle transitions
  if (previousStatus == HEATER_ON && state.heaterStatus != HEATER_ON) {
    // Heater just turned off - start cooldown
    Serial.println("Heater turned OFF - starting cooldown");
    startCooldown();
  }
  
  // Check cooldown completion
  if (state.inCooldown && isCooldownComplete()) {
    endCooldown();
  }
  
  // Log state changes
  if (previousStatus != state.heaterStatus) {
    Serial.print("Heater status changed: ");
    Serial.print(getHeaterStatusString());
    Serial.print(" (Inlet: ");
    Serial.print(state.inletTemp, 1);
    Serial.print("°F, Target: ");
    Serial.print(state.targetTemp, 1);
    Serial.println("°F)");
  }
}

// ============================================
// HEATER STATE APPLICATION
// ============================================

void applyHeaterState() {
  if (state.heaterStatus == HEATER_ON) {
    digitalWrite(HEATER_PIN, HIGH);
  } else {
    digitalWrite(HEATER_PIN, LOW);
  }
}

// ============================================
// HEATER DECISION LOGIC
// ============================================

bool shouldHeaterBeOn() {
  // Heater turns ON when ALL conditions met:
  // 1. Heater enabled by user
  if (!state.heaterEnabled) {
    return false;
  }
  
  // 2. Emergency shutdown not active
  if (state.emergencyShutdown) {
    return false;
  }
  
  // 3. Within operating hours
  if (!isWithinOperatingHours()) {
    return false;
  }
  
  // 4. Valid temperature reading
  if (!state.inletTempValid) {
    Serial.println("Heater OFF: Invalid temperature sensor");
    return false;
  }
  
  // 5. Temperature check
  if (state.inletTemp < INVALID_TEMP_LOW || state.inletTemp > INVALID_TEMP_HIGH) {
    Serial.println("Heater OFF: Temperature out of safe range");
    return false;
  }
  
  // 6. Inlet temperature < Target temperature
  if (state.inletTemp >= state.targetTemp) {
    return false;  // Target reached or exceeded
  }
  
  // All conditions met - heater should be ON
  return true;
}
