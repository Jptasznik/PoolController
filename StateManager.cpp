#include "StateManager.h"
#include "Config.h"
#include "SettingsManager.h"
#include "LCDDisplay.h"
#include "BlynkIntegration.h"
#include "SinricIntegration.h"
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
  
  state.wifiConnected = false;
  state.blynkConnected = false;
  state.sinricConnected = false;
  
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

// ============================================
// EMERGENCY CONTROLS
// ============================================

void triggerEmergencyShutdown() {
  Serial.println("!!! EMERGENCY SHUTDOWN TRIGGERED !!!");
  
  state.emergencyShutdown = true;
  
  // Immediately disable both heater and pump
  digitalWrite(HEATER_PIN, LOW);
  digitalWrite(PUMP_ENABLE_PIN, LOW);
  
  state.heaterEnabled = false;
  state.heaterStatus = HEATER_OFF;
  state.pumpState = PUMP_OFF;
  state.inCooldown = false;
  
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
  state.emergencyShutdown = false;
  Serial.println("Emergency shutdown cleared");
  updateLCDDisplay();
}

// ============================================
// COOLDOWN MANAGEMENT
// ============================================

void startCooldown() {
  if (!state.inCooldown) {
    Serial.println("Starting 15-minute cooldown period");
    state.inCooldown = true;
    state.cooldownStartTime = millis();
    state.pumpStateBeforeCooldown = state.pumpState;
    
    // Ensure pump runs at minimum LOW speed
    if (state.pumpState == PUMP_OFF) {
      state.pumpState = PUMP_LOW;
      digitalWrite(PUMP_ENABLE_PIN, HIGH);
      digitalWrite(PUMP_SPEED_PIN, LOW);
    }
    
    updateLCDDisplay();
  }
}

void endCooldown() {
  if (state.inCooldown) {
    Serial.println("Cooldown period ended");
    state.inCooldown = false;
    
    // Return pump to user's preferred setting
    setPumpState(state.pumpStateBeforeCooldown);
  }
}

bool isCooldownComplete() {
  if (!state.inCooldown) return true;
  
  unsigned long elapsed = millis() - state.cooldownStartTime;
  
  // Check if 15 minutes elapsed
  if (elapsed >= COOLDOWN_DURATION) {
    return true;
  }
  
  // Check if temperatures equalized (within 2°F)
  if (state.inletTempValid && state.heaterTempValid) {
    float tempDiff = abs(state.heaterTemp - state.inletTemp);
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

String getPumpStateString() {
  if (state.inCooldown) return "COOLDOWN";
  
  switch (state.pumpState) {
    case PUMP_OFF: return "OFF";
    case PUMP_LOW: return "LOW";
    case PUMP_HIGH: return "HIGH";
    default: return "UNKNOWN";
  }
}

String getHeaterStatusString() {
  switch (state.heaterStatus) {
    case HEATER_OFF: return "OFF";
    case HEATER_ENABLED: return "ENABLED";
    case HEATER_ON: return "ON";
    default: return "UNKNOWN";
  }
}
