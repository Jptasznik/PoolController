#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

// ============================================
// BUTTON HANDLING (MCP23008)
// ============================================

// Initialize MCP23008 and configure buttons
void initButtons();

// Scan all buttons and handle presses
void scanButtons();

// Handle individual button actions
void handleHeaterButton();
void handlePumpButton();
void handleTempUpButton();
void handleTempDownButton();
void handleResetButton();

#endif // BUTTON_HANDLER_H
