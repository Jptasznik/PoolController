#define LOG_MODULE "LCD"

#include "LCDDisplay.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"
#include "I2CMutex.h"
#include "StateMutex.h"
#include <LiquidCrystal_I2C.h>

#ifdef ENABLE_LCD
// LCD instance (20 columns, 4 rows)
LiquidCrystal_I2C lcd(LCD_ADDRESS, 20, 4);

// Track if LCD needs full clear (prevents flicker)
static bool lcdNeedsClear = true;
#endif

// ============================================
// INITIALIZATION
// ============================================

void initLCD() {
#ifdef ENABLE_LCD
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
#ifdef ENABLE_LCD
  // Copy state data while holding state lock (minimize lock time)
  float inletTemp, heaterTemp, targetTemp, pumpCurrent;
  bool inletTempValid, heaterTempValid, currentSensorValid;
  bool blynkConnected, wifiConnected;
  const char* pumpStateStr;
  const char* heaterStatusStr;

  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      return;  // Can't access state, skip update
    }

    // Copy values we need
    inletTemp = state.inletTemp;
    heaterTemp = state.heaterTemp;
    targetTemp = state.targetTemp;
    pumpCurrent = state.pumpCurrent;
    inletTempValid = state.inletTempValid;
    heaterTempValid = state.heaterTempValid;
    currentSensorValid = state.currentSensorValid;
    blynkConnected = state.blynkConnected;
    wifiConnected = state.wifiConnected;
    pumpStateStr = getPumpStateString();
    heaterStatusStr = getHeaterStatusString();
  }  // Release state lock

  // Now update LCD with I2C lock (state lock is released)
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
    return;  // Can't access I2C, skip update
  }

  // Only clear on first call or when needed (prevents flicker)
  if (lcdNeedsClear) {
    lcd.clear();
    lcdNeedsClear = false;
  }

  // Line 1: Inlet: 78.5F   Tgt:80F
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

  // Line 2: Heater:79.2F
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

  // Line 3: Pump: LOW  5.2A
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

  // Line 4: Heater: ON      CLOUD
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
#else
  // LCD disabled - do nothing
#endif
}

// ============================================
// SPECIAL MESSAGES
// ============================================

void displayEmergencyShutdown() {
#ifdef ENABLE_LCD
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
    return;
  }

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("    EMERGENCY");
  lcd.setCursor(0, 2);
  lcd.print("    SHUTDOWN!");
  delay(3000);

  // Force clear on next updateLCDDisplay call
  lcdNeedsClear = true;
  updateLCDDisplay();
#endif
}

void displayFactoryReset() {
#ifdef ENABLE_LCD
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
#ifdef ENABLE_LCD
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
#ifdef ENABLE_LCD
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
#ifdef ENABLE_LCD
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
#ifdef ENABLE_LCD
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
