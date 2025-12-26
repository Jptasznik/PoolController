#define LOG_MODULE "BUTTONS"

#include "ButtonHandler.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"
#include "StateMutex.h"
#include "I2CMutex.h"
#include "SettingsManager.h"
#include "LCDDisplay.h"
#include <Adafruit_MCP23X08.h>
#include <WiFi.h>
#include <WiFiProv.h>

#ifdef ENABLE_BUTTONS
// MCP23008 instance
Adafruit_MCP23X08 mcp;

// Button state tracking
struct ButtonState {
  bool pressed;
  bool lastState;
  unsigned long pressStartTime;
  unsigned long lastDebounceTime;
};

ButtonState buttons[5];  // 5 buttons on GP0-GP4

// Timing variables
unsigned long lastButtonScan = 0;
unsigned long lastHoldCheck = 0;
#endif

// ============================================
// INITIALIZATION
// ============================================

void initButtons() {
#ifdef ENABLE_BUTTONS
  LOG_INFO("Initializing MCP23008 button handler...");

  // Lock I2C bus for MCP23008 initialization
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
#ifndef NO_HARDWARE
    LOG_ERROR("Failed to lock I2C for MCP23008 init!");
#endif
    return;
  }

  if (!mcp.begin_I2C(MCP23008_ADDRESS)) {
#ifndef NO_HARDWARE
    LOG_ERROR("MCP23008 not found!");
#endif
    return;
  }

  LOG_INFO("MCP23008 found at address 0x20");

  // Configure all button pins as inputs with pull-ups
  for (int i = 0; i <= 4; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
    buttons[i].pressed = false;
    buttons[i].lastState = true;  // Pull-up means HIGH when not pressed
    buttons[i].pressStartTime = 0;
    buttons[i].lastDebounceTime = 0;
  }

  LOG_INFO("Button handler initialized");
#else
  LOG_WARN("Button handler DISABLED (ENABLE_BUTTONS not defined)");
#endif
}

// ============================================
// BUTTON SCANNING
// ============================================

void scanButtons() {
#ifdef ENABLE_BUTTONS
  unsigned long now = millis();

  // Scan buttons at regular interval
  if (now - lastButtonScan < BUTTON_SCAN_INTERVAL) {
    return;
  }
  lastButtonScan = now;

  // Read all button states with I2C lock
  uint8_t gpio;
  {
    I2CScopedLock i2cLock;
    if (!i2cLock.isLocked()) {
      return;  // Can't access I2C, skip this scan
    }
    gpio = mcp.readGPIO();
  }  // Release I2C lock

  // Check each button
  for (int i = 0; i <= 4; i++) {
    bool currentState = !(gpio & (1 << i));  // Invert because pull-up (LOW = pressed)

    // Debouncing
    if (currentState != buttons[i].lastState) {
      buttons[i].lastDebounceTime = now;
    }

    if ((now - buttons[i].lastDebounceTime) > BUTTON_DEBOUNCE_TIME) {
      // Button state is stable
      if (currentState && !buttons[i].pressed) {
        // Button just pressed - record press start time
        buttons[i].pressed = true;
        buttons[i].pressStartTime = now;
      } else if (!currentState && buttons[i].pressed) {
        // Button just released - handle short press
        unsigned long pressDuration = now - buttons[i].pressStartTime;
        buttons[i].pressed = false;

        // Handle short press (not a hold action)
        if (pressDuration < BUTTON_SHORT_PRESS_TIME) {
          switch (i) {
            case BUTTON_HEATER:
              handleHeaterButton();
              break;
            case BUTTON_PUMP:
              handlePumpButton();
              break;
            case BUTTON_TEMP_UP:
              handleTempUpButton();
              break;
            case BUTTON_TEMP_DOWN:
              handleTempDownButton();
              break;
            case BUTTON_RESET:
              // Don't handle short press for reset (only hold action)
              break;
          }
        }
      }
    }

    buttons[i].lastState = currentState;
  }
  
  // Check for hold actions
  if (now - lastHoldCheck >= BUTTON_HOLD_CHECK_INTERVAL) {
    lastHoldCheck = now;
    
    // Emergency shutdown (Button 2 - Pump button)
    if (buttons[BUTTON_PUMP].pressed) {
      unsigned long holdTime = now - buttons[BUTTON_PUMP].pressStartTime;
      if (holdTime >= EMERGENCY_SHUTDOWN_HOLD_TIME) {
        LOG_WARN("Emergency shutdown hold detected!");
        triggerEmergencyShutdown();
        buttons[BUTTON_PUMP].pressed = false;  // Prevent repeat
      }
    }

    // Factory reset (Button 5)
    if (buttons[BUTTON_RESET].pressed) {
      unsigned long holdTime = now - buttons[BUTTON_RESET].pressStartTime;
      if (holdTime >= FACTORY_RESET_HOLD_TIME) {
        LOG_WARN("Factory reset hold detected!");
        displayFactoryReset();

        // Clear WiFi credentials - disconnect and erase
        WiFi.disconnect(true, true);

        // Clear all settings
        factoryResetSettings();

        delay(2000);  // BLOCKING: Brief delay after factory reset before restart (acceptable for rare operation)
        ESP.restart();
      }
    }
  }
#else
  // Buttons disabled - do nothing
#endif
}

// ============================================
// BUTTON ACTION HANDLERS
// ============================================

void handleHeaterButton() {
  LOG_INFO("Button: Heater toggle");
  setHeaterEnabledState(!state.heaterEnabled);

  if (state.emergencyShutdown) {
    clearEmergencyShutdown();
  }
}

void handlePumpButton() {
  LOG_INFO("Button: Pump cycle");

  if (state.emergencyShutdown) {
    clearEmergencyShutdown();
  }

  // Cycle through states: OFF → LOW → HIGH → OFF
  switch (state.pumpState) {
    case PUMP_OFF:
      setPumpState(PUMP_LOW);
      break;
    case PUMP_LOW:
      setPumpState(PUMP_HIGH);
      break;
    case PUMP_HIGH:
      if (canPumpTurnOff()) {
        setPumpState(PUMP_OFF);
      } else {
        LOG_WARN("Cannot turn pump OFF (safety interlock)");
      }
      break;
  }
}

void handleTempUpButton() {
  LOG_INFO("Button: Temperature +1°F");
  setTargetTemperature(state.targetTemp + 1.0);
}

void handleTempDownButton() {
  LOG_INFO("Button: Temperature -1°F");
  setTargetTemperature(state.targetTemp - 1.0);
}

void handleResetButton() {
  // Only handled via hold detection in scanButtons()
  // Short press does nothing
}
