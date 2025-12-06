#include "LCDDisplay.h"
#include "Config.h"
#include "StateManager.h"
#include <LiquidCrystal_I2C.h>

// LCD instance (20 columns, 4 rows)
LiquidCrystal_I2C lcd(LCD_ADDRESS, 20, 4);

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
  lcd.clear();
  
  // Line 1: Inlet: 78.5F   Tgt:80F
  lcd.setCursor(0, 0);
  lcd.print("Inlet:");
  if (state.inletTempValid) {
    lcd.print(state.inletTemp, 1);
    lcd.print("F");
  } else {
    lcd.print("--.-F");
  }
  
  lcd.setCursor(12, 0);
  lcd.print("Tgt:");
  lcd.print((int)state.targetTemp);
  lcd.print("F");
  
  // Line 2: Heater:79.2F
  lcd.setCursor(0, 1);
  lcd.print("Heater:");
  if (state.heaterTempValid) {
    lcd.print(state.heaterTemp, 1);
    lcd.print("F");
  } else {
    lcd.print("--.-F");
  }
  
  // Line 3: Pump: LOW / COOLDOWN
  lcd.setCursor(0, 2);
  lcd.print("Pump: ");
  lcd.print(getPumpStateString());
  
  // Line 4: Heater: ON      CLOUD
  lcd.setCursor(0, 3);
  lcd.print("Heater: ");
  lcd.print(getHeaterStatusString());
  
  // Connection status (right side)
  lcd.setCursor(15, 3);
  if (state.blynkConnected) {
    lcd.print("CLOUD");
  } else if (state.wifiConnected) {
    lcd.print("WiFi");
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
}
