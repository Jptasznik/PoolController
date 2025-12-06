#include "ButtonHandler.h"
#include "Config.h"
#include "StateManager.h"
#include "SettingsManager.h"
#include "LCDDisplay.h"
#include <Adafruit_MCP23X08.h>
#include <WiFi.h>
#include <WiFiProv.h>

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

// ============================================
// INITIALIZATION
// ============================================

void initButtons() {
  Serial.println("Initializing MCP23008 button handler...");
  
  if (!mcp.begin_I2C(MCP23008_ADDRESS)) {
    Serial.println("ERROR: MCP23008 not found!");
    return;
  }
  
  Serial.println("MCP23008 found at address 0x20");
  
  // Configure all button pins as inputs with pull-ups
  for (int i = 0; i <= 4; i++) {
    mcp.pinMode(i, INPUT_PULLUP);
    buttons[i].pressed = false;
    buttons[i].lastState = true;  // Pull-up means HIGH when not pressed
    buttons[i].pressStartTime = 0;
    buttons[i].lastDebounceTime = 0;
  }
  
  Serial.println("Button handler initialized");
}

// ============================================
// BUTTON SCANNING
// ============================================

void scanButtons() {
  unsigned long now = millis();
  
  // Scan buttons at regular interval
  if (now - lastButtonScan < BUTTON_SCAN_INTERVAL) {
    return;
  }
  lastButtonScan = now;
  
  // Read all button states
  uint8_t gpio = mcp.readGPIO();
  
  // Track if any button is currently pressed
  int pressedCount = 0;
  int pressedButton = -1;
  
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
        // Button just pressed
        buttons[i].pressed = true;
        buttons[i].pressStartTime = now;
        pressedButton = i;
      } else if (!currentState && buttons[i].pressed) {
        // Button just released
        buttons[i].pressed = false;
      }
    }
    
    buttons[i].lastState = currentState;
    
    if (buttons[i].pressed) {
      pressedCount++;
    }
  }
  
  // Only process single button presses
  if (pressedCount == 1 && pressedButton >= 0) {
    // Check for short press (handle on release)
    if (!buttons[pressedButton].pressed) {
      unsigned long pressDuration = now - buttons[pressedButton].pressStartTime;
      
      // Short press actions (not hold)
      if (pressDuration < 1000) {
        switch (pressedButton) {
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
            // Don't handle short press for reset
            break;
        }
      }
    }
  }
  
  // Check for hold actions
  if (now - lastHoldCheck >= BUTTON_HOLD_CHECK_INTERVAL) {
    lastHoldCheck = now;
    
    // Emergency shutdown (Button 2 - Pump button)
    if (buttons[BUTTON_PUMP].pressed) {
      unsigned long holdTime = now - buttons[BUTTON_PUMP].pressStartTime;
      if (holdTime >= EMERGENCY_SHUTDOWN_HOLD_TIME) {
        Serial.println("Emergency shutdown hold detected!");
        triggerEmergencyShutdown();
        buttons[BUTTON_PUMP].pressed = false;  // Prevent repeat
      }
    }
    
    // Factory reset (Button 5)
    if (buttons[BUTTON_RESET].pressed) {
      unsigned long holdTime = now - buttons[BUTTON_RESET].pressStartTime;
      if (holdTime >= FACTORY_RESET_HOLD_TIME) {
        Serial.println("Factory reset hold detected!");
        displayFactoryReset();
        
        // Clear WiFi credentials - disconnect and erase
        WiFi.disconnect(true, true);
        
        // Clear all settings
        factoryResetSettings();
        
        delay(2000);
        ESP.restart();
      }
    }
  }
}

// ============================================
// BUTTON ACTION HANDLERS
// ============================================

void handleHeaterButton() {
  Serial.println("Button: Heater toggle");
  setHeaterEnabledState(!state.heaterEnabled);
  
  if (state.emergencyShutdown) {
    clearEmergencyShutdown();
  }
}

void handlePumpButton() {
  Serial.println("Button: Pump cycle");
  
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
        Serial.println("Cannot turn pump OFF (safety interlock)");
      }
      break;
  }
}

void handleTempUpButton() {
  Serial.println("Button: Temperature +1°F");
  setTargetTemperature(state.targetTemp + 1.0);
}

void handleTempDownButton() {
  Serial.println("Button: Temperature -1°F");
  setTargetTemperature(state.targetTemp - 1.0);
}

void handleResetButton() {
  // Only handled via hold detection in scanButtons()
  // Short press does nothing
}
