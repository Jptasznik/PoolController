#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
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
// SYSTEM STATE STRUCTURE (INTERNAL)
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
// STATE MANAGER CLASS
// ============================================

class StateManager {
public:
  // Singleton accessor
  static StateManager& getInstance();

  // Initialization
  void init();

  // === TEMPERATURE READINGS ===
  float getInletTemp();
  void setInletTemp(float temp);
  bool isInletTempValid();
  void setInletTempValid(bool valid);

  float getHeaterTemp();
  void setHeaterTemp(float temp);
  bool isHeaterTempValid();
  void setHeaterTempValid(bool valid);

  float getAmbientTemp();
  void setAmbientTemp(float temp);
  bool isAmbientTempValid();
  void setAmbientTempValid(bool valid);

  float getTargetTemp();
  // Note: Use setTargetTemperature() for full validation and flash save

  // === CURRENT MONITORING ===
  float getPumpCurrent();
  void setPumpCurrent(float current);
  float getCurrentOffset();
  void setCurrentOffset(float offset);
  bool isCurrentSensorValid();
  void setCurrentSensorValid(bool valid);
  bool isPumpFailureAlert();
  void setPumpFailureAlert(bool alert);
  bool isOvercurrentAlert();
  void setOvercurrentAlert(bool alert);

  // === CONTROL STATES ===
  bool isHeaterEnabled();
  // Note: Use setHeaterEnabledState() for full safety checks and flash save

  HeaterStatus getHeaterStatus();
  void setHeaterStatus(HeaterStatus status);

  PumpState getPumpState();
  void setPumpStateInternal(PumpState state);  // Internal use only
  // Note: Use setPumpState() for full safety checks and flash save

  // === COOLDOWN TRACKING ===
  bool isInCooldown();
  void setInCooldown(bool inCooldown);
  unsigned long getCooldownStartTime();
  void setCooldownStartTime(unsigned long time);
  PumpState getPumpStateBeforeCooldown();
  void setPumpStateBeforeCooldown(PumpState state);

  // === RUNTIME TRACKING ===
  unsigned long getPumpRuntimeToday();
  void incrementPumpRuntimeToday(unsigned long elapsed);
  unsigned long getLastRuntimeUpdate();
  void setLastRuntimeUpdate(unsigned long time);
  int getLastResetDay();
  void setLastResetDay(int day);
  void setPumpRuntimeToday(unsigned long runtime);

  // === SETTINGS ===
  float getMaxRuntimeHours();
  void setMaxRuntimeHoursInternal(float hours);  // Internal use only
  int getHeaterStartHour();
  void setHeaterStartHourInternal(int hour);  // Internal use only
  int getHeaterEndHour();
  void setHeaterEndHourInternal(int hour);  // Internal use only
  float getMinHeaterTemp();
  void setMinHeaterTempInternal(float temp);  // Internal use only
  float getMaxHeaterTemp();
  void setMaxHeaterTempInternal(float temp);  // Internal use only
  void setTargetTempInternal(float temp);  // Internal use only

  // === CONNECTION STATUS ===
  bool isWiFiConnected();
  void setWiFiConnected(bool connected);
  bool isBlynkConnected();
  void setBlynkConnected(bool connected);
  bool isSinricConnected();
  void setSinricConnected(bool connected);

  // === WIFI RUNTIME TRACKING ===
  bool isProvisioningActive();
  void setProvisioningActive(bool active);
  unsigned long getProvisioningStartTime();
  void setProvisioningStartTime(unsigned long time);
  bool areCredentialsReceived();
  void setCredentialsReceived(bool received);
  unsigned long getCredReceivedTime();
  void setCredReceivedTime(unsigned long time);
  unsigned long getDisconnectTimestamp(int index);
  void setDisconnectTimestamp(int index, unsigned long time);
  int getDisconnectCount();
  void setDisconnectCount(int count);

  // === EMERGENCY/FAILSAFE ===
  bool isEmergencyShutdown();
  void setEmergencyShutdown(bool shutdown);
  bool isInFailsafeMode();
  void setInFailsafeMode(bool inFailsafe);
  const char* getFailsafeReason();
  void setFailsafeReason(const char* reason);
  unsigned long getFailsafeEnterTime();
  void setFailsafeEnterTime(unsigned long time);
  bool isOverheatLockout();
  void setOverheatLockout(bool lockout);
  unsigned long getOverheatDetectedTime();
  void setOverheatDetectedTime(unsigned long time);

  // === CRASH TRACKING ===
  uint32_t getBootCount();
  void setBootCount(uint32_t count);
  uint32_t getCrashCount();
  void setCrashCount(uint32_t count);
  unsigned long getTotalPumpRuntime();
  void setTotalPumpRuntime(unsigned long runtime);

  // === OTA ===
  bool isOTAInProgress();
  void setOTAInProgress(bool inProgress);
  unsigned long getOTAStartTime();
  void setOTAStartTime(unsigned long time);
  const char* getOTAVersion();
  void setOTAVersion(const char* version);
  void setHeaterEnabledInternal(bool enabled);  // Internal use only

  // === HIGH-LEVEL STATE MANAGEMENT FUNCTIONS ===
  // These provide validation, safety checks, and flash persistence

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

private:
  StateManager();  // Private constructor for singleton
  StateManager(const StateManager&) = delete;
  StateManager& operator=(const StateManager&) = delete;

  SystemState state;  // NOW PRIVATE - encapsulated!
  SemaphoreHandle_t mutex;

  // Mutex helpers
  bool lock(uint32_t timeout_ms = 100);
  void unlock();
};

// ============================================
// BACKWARD COMPATIBILITY FUNCTIONS
// ============================================
// These functions maintain backward compatibility with code that used global functions.
// They simply delegate to the singleton instance.

inline void initSystemState() { StateManager::getInstance().init(); }
inline void setHeaterEnabledState(bool enabled) { StateManager::getInstance().setHeaterEnabledState(enabled); }
inline void setPumpState(PumpState newState) { StateManager::getInstance().setPumpState(newState); }
inline void setTargetTemperature(float newTemp) { StateManager::getInstance().setTargetTemperature(newTemp); }
inline void setMaxRuntimeHours(float hours) { StateManager::getInstance().setMaxRuntimeHours(hours); }
inline void setHeaterStartHour(int hour) { StateManager::getInstance().setHeaterStartHour(hour); }
inline void setHeaterEndHour(int hour) { StateManager::getInstance().setHeaterEndHour(hour); }
inline void setMinHeaterTemp(float temp) { StateManager::getInstance().setMinHeaterTemp(temp); }
inline void setMaxHeaterTemp(float temp) { StateManager::getInstance().setMaxHeaterTemp(temp); }
inline void triggerEmergencyShutdown() { StateManager::getInstance().triggerEmergencyShutdown(); }
inline void clearEmergencyShutdown() { StateManager::getInstance().clearEmergencyShutdown(); }
inline void enterFailsafeMode(const char* reason) { StateManager::getInstance().enterFailsafeMode(reason); }
inline void clearFailsafeMode() { StateManager::getInstance().clearFailsafeMode(); }
inline void clearOverheatLockout() { StateManager::getInstance().clearOverheatLockout(); }
inline void startCooldown() { StateManager::getInstance().startCooldown(); }
inline void endCooldown() { StateManager::getInstance().endCooldown(); }
inline bool isCooldownComplete() { return StateManager::getInstance().isCooldownComplete(); }
inline void updatePumpRuntime() { StateManager::getInstance().updatePumpRuntime(); }
inline void resetDailyRuntime() { StateManager::getInstance().resetDailyRuntime(); }
inline float getPumpRuntimeHours() { return StateManager::getInstance().getPumpRuntimeHours(); }
inline bool isWithinOperatingHours() { return StateManager::getInstance().isWithinOperatingHours(); }
inline bool canPumpTurnOff() { return StateManager::getInstance().canPumpTurnOff(); }
inline const char* getPumpStateString() { return StateManager::getInstance().getPumpStateString(); }
inline const char* getHeaterStatusString() { return StateManager::getInstance().getHeaterStatusString(); }
inline void recordWiFiDisconnect() { StateManager::getInstance().recordWiFiDisconnect(); }
inline int getRecentDisconnectCount() { return StateManager::getInstance().getRecentDisconnectCount(); }
inline bool isProvisioningHung() { return StateManager::getInstance().isProvisioningHung(); }
inline void clearWiFiRuntimeTracking() { StateManager::getInstance().clearWiFiRuntimeTracking(); }

#endif // STATE_MANAGER_H
