#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include "config.h"

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

  // Current monitoring
  float pumpCurrent;         // Current reading in amps (RMS)
  float currentOffset;       // Calibration offset
  bool currentSensorValid;   // Current sensor status
  bool pumpFailureAlert;     // Pump failure detected
  bool overcurrentAlert;     // Overcurrent detected

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
  float minHeaterTemp;  // Minimum water temp to allow heater ON (°F)
  float maxHeaterTemp;  // Maximum water temp to allow heater ON (°F)
  
  // Connection status
  bool wifiConnected;
  bool blynkConnected;
  bool sinricConnected;

  // WiFi runtime failure tracking
  bool provisioningActive;
  unsigned long provisioningStartTime;
  bool credentialsReceived;           // Credentials received, waiting for validation
  unsigned long credReceivedTime;     // When credentials were received
  unsigned long disconnectTimestamps[WIFI_MAX_DISCONNECTS];
  int disconnectCount;

  // Emergency state
  bool emergencyShutdown;

  // Failsafe mode (critical sensor failure protection)
  bool inFailsafeMode;
  unsigned long failsafeEnterTime;
  char failsafeReason[32];

  // Overheat protection
  bool overheatLockout;
  unsigned long overheatDetectedTime;

  // Runtime state tracking (for crash recovery)
  uint32_t bootCount;
  uint32_t crashCount;
  unsigned long totalPumpRuntime;  // Total lifetime runtime in milliseconds

  // OTA update state
  bool otaInProgress;
  unsigned long otaStartTime;
  char otaVersion[32];
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
void setMinHeaterTemp(float temp);
void setMaxHeaterTemp(float temp);

// Emergency controls
void triggerEmergencyShutdown();
void clearEmergencyShutdown();

// Failsafe mode
void enterFailsafeMode(const char* reason);
void clearFailsafeMode();

// Overheat protection
void clearOverheatLockout();

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

// WiFi runtime failure tracking
void recordWiFiDisconnect();
int getRecentDisconnectCount();
bool isProvisioningHung();
void clearWiFiRuntimeTracking();

#endif // STATE_MANAGER_H
