#ifndef PUMP_CONTROL_H
#define PUMP_CONTROL_H

#include <Arduino.h>

// ============================================
// PUMP CONTROL
// ============================================

// Initialize pump control outputs
void initPumpControl();

// Apply physical pump state to hardware
void applyPumpState();

// Check if daily runtime limit exceeded
bool isRuntimeLimitExceeded();

#endif // PUMP_CONTROL_H
