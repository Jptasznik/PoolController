#ifndef HEATER_CONTROL_H
#define HEATER_CONTROL_H

#include <Arduino.h>

// ============================================
// HEATER CONTROL
// ============================================

// Initialize heater control outputs
void initHeaterControl();

// Main control system logic (called periodically)
void runControlSystem();

// Apply physical heater state to hardware
void applyHeaterState();

// Determine if heater should be on based on all conditions
bool shouldHeaterBeOn();

#endif // HEATER_CONTROL_H
