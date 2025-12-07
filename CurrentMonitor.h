#ifndef CURRENT_MONITOR_H
#define CURRENT_MONITOR_H

#include <Arduino.h>

// ============================================
// CURRENT MONITORING FUNCTIONS
// ============================================

// Initialize ADS1115 and current monitoring
void initCurrentMonitor();

// Read current from pump CT sensor
// Returns current in amps (RMS)
void readPumpCurrent();

// Check if current reading indicates pump is running
bool isPumpCurrentDetected();

// Check for pump failure conditions
// Returns true if pump should be running but no current detected
bool isPumpFailureDetected();

// Check for overcurrent condition
bool isOvercurrentDetected();

// Calibrate current sensor
// Call this with pump OFF to measure zero offset
void calibrateCurrentSensor();

#endif // CURRENT_MONITOR_H
