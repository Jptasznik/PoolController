#include "LCDDisplay.h"
#include "Config.h"
#include "StateManager.h"
#include <LiquidCrystal_I2C.h>

// LCD instance (20 columns, 4 rows)
LiquidCrystal_I2C lcd(LCD_ADDRESS, 20, 4);

// Track if LCD needs full clear (prevents flicker)
static bool lcdNeedsClear = true;

// ============================================
// INITIALIZATION
// ============================================

void initLCD() {
  Serial.println("Initializing LCD display...");
  
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
  
  Serial.println("LCD display initialized");
}

// ============================================
// MAIN DISPLAY UPDATE
// ============================================

void updateLCDDisplay() {
  // Only clear on first call or when needed (prevents flicker)
  if (lcdNeedsClear) {
    lcd.clear();
    lcdNeedsClear = false;
  }

  // Line 1: Inlet: 78.5F   Tgt:80F
  lcd.setCursor(0, 0);
  lcd.print("Inlet:");
  if (state.inletTempValid) {
    char tempBuf[6];
    snprintf(tempBuf, sizeof(tempBuf), "%5.1f", state.inletTemp);
    lcd.print(tempBuf);
    lcd.print("F");
  } else {
    lcd.print("--.-F");
  }
  lcd.print(" ");  // Clear trailing char

  lcd.setCursor(12, 0);
  lcd.print("Tgt:");
  char targetBuf[4];
  snprintf(targetBuf, sizeof(targetBuf), "%3d", (int)state.targetTemp);
  lcd.print(targetBuf);
  lcd.print("F  ");  // Clear trailing chars

  // Line 2: Heater:79.2F
  lcd.setCursor(0, 1);
  lcd.print("Heater:");
  if (state.heaterTempValid) {
    char tempBuf[6];
    snprintf(tempBuf, sizeof(tempBuf), "%5.1f", state.heaterTemp);
    lcd.print(tempBuf);
    lcd.print("F");
  } else {
    lcd.print("--.-F");
  }
  lcd.print("        ");  // Clear rest of line

  // Line 3: Pump: LOW  5.2A
  lcd.setCursor(0, 2);
  lcd.print("Pump: ");
  lcd.print(getPumpStateString());

  // Add current reading if sensor is valid
  if (state.currentSensorValid) {
    lcd.print("  ");
    char currentBuf[6];
    snprintf(currentBuf, sizeof(currentBuf), "%4.1f", state.pumpCurrent);
    lcd.print(currentBuf);
    lcd.print("A");
  }
  lcd.print("      ");  // Clear rest of line

  // Line 4: Heater: ON      CLOUD
  lcd.setCursor(0, 3);
  lcd.print("Heater: ");
  lcd.print(getHeaterStatusString());
  lcd.print("    ");  // Clear gap before status

  // Connection status (right side)
  lcd.setCursor(15, 3);
  if (state.blynkConnected) {
    lcd.print("CLOUD");
  } else if (state.wifiConnected) {
    lcd.print("WiFi ");
  } else {
    lcd.print("     ");
  }
}

// ============================================
// SPECIAL MESSAGES
// ============================================

void displayEmergencyShutdown() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("    EMERGENCY");
  lcd.setCursor(0, 2);
  lcd.print("    SHUTDOWN!");
  delay(3000);

  // Force clear on next updateLCDDisplay call
  lcdNeedsClear = true;
  updateLCDDisplay();
}

void displayFactoryReset() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("  FACTORY RESET!");
  lcd.setCursor(0, 2);
  lcd.print("  Restarting...");
}

void displayWiFiProvisioning() {
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
}

void displayWiFiReset() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("   WiFi Reset");
  lcd.setCursor(0, 1);
  lcd.print("Too many failures");
  lcd.setCursor(0, 2);
  lcd.print("Credentials cleared");
  lcd.setCursor(0, 3);
  lcd.print("  Rebooting...");
}
