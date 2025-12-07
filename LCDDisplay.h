#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <Arduino.h>

// ============================================
// LCD DISPLAY MANAGEMENT
// ============================================

// Initialize LCD display
void initLCD();

// Update LCD with current system state
void updateLCDDisplay();

// Display special messages
void displayEmergencyShutdown();
void displayFactoryReset();
void displayWiFiProvisioning();
void displayWiFiReset();

#endif // LCD_DISPLAY_H
