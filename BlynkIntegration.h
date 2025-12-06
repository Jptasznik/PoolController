#ifndef BLYNK_INTEGRATION_H
#define BLYNK_INTEGRATION_H

#include <Arduino.h>

// ============================================
// BLYNK CLOUD INTEGRATION
// ============================================

// Initialize Blynk connection
void initBlynk();

// Update all Blynk virtual pins with current state
void syncAllToBlynk();

// Sync individual values to Blynk
void syncHeaterToBlynk();
void syncPumpToBlynk();
void syncTargetTempToBlynk();
void syncTemperaturesToBlynk();
void syncPumpRuntimeToBlynk();

// Blynk callback handlers (defined in main .ino file)
// These are declared here but implemented inline with BLYNK_WRITE macros

#endif // BLYNK_INTEGRATION_H
