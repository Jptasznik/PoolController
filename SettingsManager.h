#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include "StateManager.h"

// ============================================
// SETTINGS PERSISTENCE (ESP32 Preferences)
// ============================================

// Initialize preferences system
void initSettings();

// Load all settings from flash
void loadAllSettings();

// Save individual settings
void saveTargetTemp(float temp);
void saveHeaterEnabled(bool enabled);
void savePumpState(PumpState state);
void saveMaxRuntimeHours(float hours);
void saveHeaterStartHour(int hour);
void saveHeaterEndHour(int hour);
void saveMinHeaterTemp(float temp);
void saveMaxHeaterTemp(float temp);

// Load individual settings
float loadTargetTemp();
bool loadHeaterEnabled();
PumpState loadPumpState();
float loadMaxRuntimeHours();
int loadHeaterStartHour();
int loadHeaterEndHour();
float loadMinHeaterTemp();
float loadMaxHeaterTemp();

// Factory reset
void factoryResetSettings();

// WiFi failure tracking
int loadWiFiFailureCount();
void saveWiFiFailureCount(int count);
void incrementWiFiFailureCount();
void resetWiFiFailureCount();

// Current sensor calibration
float loadCurrentCalibration();
void saveCurrentCalibration(float offset);

// Runtime state persistence (for crash recovery)
void saveRuntimeState();
void loadRuntimeState();

#endif // SETTINGS_MANAGER_H
