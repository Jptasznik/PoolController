#define LOG_MODULE "LCD"

#include "LCDDisplay.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"
#include "I2CMutex.h"
#include <LiquidCrystal_I2C.h>

#if ENABLE_LCD
// LCD instance (20 columns, 4 rows)
LiquidCrystal_I2C lcd(LCD_ADDRESS, 20, 4);

// Track if LCD needs full clear (prevents flicker)
static bool lcdNeedsClear = true;
#endif

// ============================================
// INITIALIZATION
// ============================================

void initLCD() {
#if ENABLE_LCD
  LOG_INFO("Initializing LCD display...");

  // Lock I2C bus for LCD initialization
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
#ifndef NO_HARDWARE
    LOG_ERROR("Failed to lock I2C for LCD init!");
#endif
    return;
  }

  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Display startup message
  lcd.setCursor(0, 0);
  lcd.print("  Pool Controller");
  lcd.setCursor(0, 1);
  lcd.print("   Initializing...");
  lcd.setCursor(0, 3);
  lcd.print("    Please Wait");

  LOG_INFO("LCD display initialized");
#else
  LOG_WARN("LCD display DISABLED (ENABLE_LCD not defined)");
#endif
}

// ============================================
// MAIN DISPLAY UPDATE
// ============================================

void updateLCDDisplay() {
#if ENABLE_LCD
  // Get state data using StateManager API
  StateManager& stateMgr = StateManager::getInstance();

  float inletTemp = stateMgr.getInletTemp();
  float heaterTemp = stateMgr.getHeaterTemp();
  float targetTemp = stateMgr.getTargetTemp();
  float pumpCurrent = stateMgr.getPumpCurrent();
  bool inletTempValid = stateMgr.isInletTempValid();
  bool heaterTempValid = stateMgr.isHeaterTempValid();
  bool currentSensorValid = stateMgr.isCurrentSensorValid();
  bool blynkConnected = stateMgr.isBlynkConnected();
  bool wifiConnected = stateMgr.isWiFiConnected();
  const char* pumpStateStr = getPumpStateString();
  const char* heaterStatusStr = getHeaterStatusString();

  // Acquire and release the I2C lock per-line (instead of once for the
  // whole redraw) so the higher-priority button scan task gets frequent
  // chances to interleave instead of waiting out one long ~80-char redraw.

  // Clear (only on first call or when needed - prevents flicker)
  if (lcdNeedsClear) {
    I2CScopedLock i2cLock;
    if (i2cLock.isLocked()) {
      lcd.clear();
      lcdNeedsClear = false;
    }
  }

  // Line 1: Inlet: 78.5F   Tgt:80F
  {
    I2CScopedLock i2cLock;
    if (i2cLock.isLocked()) {
      lcd.setCursor(0, 0);
      lcd.print("Inlet:");
      if (inletTempValid) {
        char tempBuf[6];
        snprintf(tempBuf, sizeof(tempBuf), "%5.1f", inletTemp);
        lcd.print(tempBuf);
        lcd.print("F");
      } else {
        lcd.print("--.-F");
      }
      lcd.print(" ");  // Clear trailing char

      lcd.setCursor(12, 0);
      lcd.print("Tgt:");
      char targetBuf[4];
      snprintf(targetBuf, sizeof(targetBuf), "%3d", (int)targetTemp);
      lcd.print(targetBuf);
      lcd.print("F  ");  // Clear trailing chars
    }
  }

  // Line 2: Heater:79.2F
  {
    I2CScopedLock i2cLock;
    if (i2cLock.isLocked()) {
      lcd.setCursor(0, 1);
      lcd.print("Heater:");
      if (heaterTempValid) {
        char tempBuf[6];
        snprintf(tempBuf, sizeof(tempBuf), "%5.1f", heaterTemp);
        lcd.print(tempBuf);
        lcd.print("F");
      } else {
        lcd.print("--.-F");
      }
      lcd.print("        ");  // Clear rest of line
    }
  }

  // Line 3: Pump: LOW  5.2A
  {
    I2CScopedLock i2cLock;
    if (i2cLock.isLocked()) {
      lcd.setCursor(0, 2);
      lcd.print("Pump: ");
      lcd.print(pumpStateStr);

      // Add current reading if sensor is valid
      if (currentSensorValid) {
        lcd.print("  ");
        char currentBuf[6];
        snprintf(currentBuf, sizeof(currentBuf), "%4.1f", pumpCurrent);
        lcd.print(currentBuf);
        lcd.print("A");
      }
      lcd.print("      ");  // Clear rest of line
    }
  }

  // Line 4: Heater: ON      CLOUD
  {
    I2CScopedLock i2cLock;
    if (i2cLock.isLocked()) {
      lcd.setCursor(0, 3);
      lcd.print("Heater: ");
      lcd.print(heaterStatusStr);
      lcd.print("    ");  // Clear gap before status

      // Connection status (right side)
      lcd.setCursor(15, 3);
      if (blynkConnected) {
        lcd.print("CLOUD");
      } else if (wifiConnected) {
        lcd.print("WiFi ");
      } else {
        lcd.print("     ");
      }
    }
  }
#else
  // LCD disabled - do nothing
#endif
}

// ============================================
// SPECIAL MESSAGES
// ============================================

void displayEmergencyShutdown() {
#if ENABLE_LCD
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("    EMERGENCY");
  lcd.setCursor(0, 2);
  lcd.print("    SHUTDOWN!");
  delay(3000);  // BLOCKING: Allow user to read critical emergency message (acceptable for safety-critical event)

  // Force clear on next updateLCDDisplay call
  lcdNeedsClear = true;
  updateLCDDisplay();
#endif
}

void displayFactoryReset() {
#if ENABLE_LCD
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("  FACTORY RESET!");
  lcd.setCursor(0, 2);
  lcd.print("  Restarting...");
#endif
}

void displayWiFiProvisioning() {
#if ENABLE_LCD
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi Provisioning");
  lcd.setCursor(0, 1);
  lcd.print("Use ESP BLE Prov app");
  lcd.setCursor(0, 2);
  lcd.print("Device: POOL_CTRL");
  lcd.setCursor(0, 3);
  lcd.print("Pop: pool1234");

  // Force clear on next updateLCDDisplay call
  lcdNeedsClear = true;
#endif
}

void displayWiFiReset() {
#if ENABLE_LCD
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   WiFi Reset");
  lcd.setCursor(0, 1);
  lcd.print("Too many failures");
  lcd.setCursor(0, 2);
  lcd.print("Credentials cleared");
  lcd.setCursor(0, 3);
  lcd.print("  Rebooting...");
#endif
}

void displayOTAUpdate() {
#if ENABLE_LCD
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  OTA UPDATE");
  lcd.setCursor(0, 1);
  lcd.print("  In Progress...");
  lcd.setCursor(0, 2);
  lcd.print("  DO NOT POWER OFF");
  lcd.setCursor(0, 3);
  lcd.print("  Please wait...");

  // Force clear on next updateLCDDisplay call
  lcdNeedsClear = true;
#endif
}

void displayOTAProgress(unsigned int progress, unsigned int total) {
#if ENABLE_LCD
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
    return;
  }

  // Calculate percentage
  unsigned int percent = (progress * 100) / total;

  // Update line 1 with progress bar
  lcd.setCursor(0, 1);
  lcd.print("  Progress: ");
  if (percent < 10) lcd.print(" ");
  if (percent < 100) lcd.print(" ");
  lcd.print(percent);
  lcd.print("%  ");

  // Update line 3 with bytes transferred
  lcd.setCursor(0, 3);
  lcd.print("  ");
  lcd.print(progress / 1024);
  lcd.print("/");
  lcd.print(total / 1024);
  lcd.print(" KB     ");

  // Don't set lcdNeedsClear - we're updating an existing display
#endif
}
