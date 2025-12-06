#ifndef SINRIC_INTEGRATION_H
#define SINRIC_INTEGRATION_H

#include <Arduino.h>

// ============================================
// SINRIC PRO / ALEXA INTEGRATION
// ============================================

// Initialize Sinric Pro connection
void initSinric();

// Handle Sinric Pro events (call in loop)
void handleSinricEvents();

// Sync current state to Alexa
void syncHeaterToSinric();
void syncPumpToSinric();
void syncTargetTempToSinric();

// Callback handlers (implemented in .cpp)
// These handle commands from Alexa

#endif // SINRIC_INTEGRATION_H
