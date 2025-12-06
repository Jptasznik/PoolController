#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>

// ============================================
// ENUMERATIONS
// ============================================

enum PumpState {
  PUMP_OFF = 0,
  PUMP_LOW = 1,
  PUMP_HIGH = 2
};

enum HeaterStatus {
  HEATER_OFF = 0,
  HEATER_ENABLED = 1,  // Enabled but not actively heating
  HEATER_ON = 2        // Actively heating
};

// ============================================
// SYSTEM STATE STRUCTURE
// ============================================

struct SystemState {
  // Temperature readings
  float inletTemp;
  float heaterTemp;
  float ambientTemp;         // 3rd temperature sensor
  float targetTemp;
  bool inletTempValid;
  bool heaterTempValid;
  bool ambientTempValid;     // 3rd sensor validity
  
  // Control states
  bool heaterEnabled;
  HeaterStatus heaterStatus;
  PumpState pumpState;
  
  // Cooldown tracking
  bool inCooldown;
  unsigned long cooldownStartTime;
  PumpState pumpStateBeforeCooldown;
  
  // Runtime tracking
  unsigned long pumpRuntimeToday;  // milliseconds
  unsigned long lastRuntimeUpdate;
  int lastResetDay;
  
  // Settings
  float maxRuntimeHours;
  int heaterStartHour;
  int heaterEndHour;
  
  // Connection status
  bool wifiConnected;
  bool blynkConnected;
  bool sinricConnected;
  
  // Emergency state
  bool emergencyShutdown;
};

// ============================================
// GLOBAL STATE INSTANCE
// ============================================
extern SystemState state;

// ============================================
// CENTRALIZED STATE MANAGEMENT FUNCTIONS
// ============================================

// Initialize system state to default values
void initSystemState();

// Centralized state setters (update all interfaces)
void setHeaterEnabledState(bool enabled);
void setPumpState(PumpState newState);
void setTargetTemperature(float newTemp);
void setMaxRuntimeHours(float hours);
void setHeaterStartHour(int hour);
void setHeaterEndHour(int hour);

// Emergency controls
void triggerEmergencyShutdown();
void clearEmergencyShutdown();

// Cooldown management
void startCooldown();
void endCooldown();
bool isCooldownComplete();

// Runtime tracking
void updatePumpRuntime();
void resetDailyRuntime();
float getPumpRuntimeHours();

// Status queries
bool isWithinOperatingHours();
bool canPumpTurnOff();
const char* getPumpStateString();
const char* getHeaterStatusString();

#endif // STATE_MANAGER_H
