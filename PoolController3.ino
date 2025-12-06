/*
 * ESP32 Pool Controller System
 * 
 * Main controller file - coordinates all subsystems
 * 
 * Features:
 * - Dual DS18B20 temperature sensors
 * - MCP23008 I2C button interface
 * - 20x4 I2C LCD display
 * - Blynk cloud integration
 * - Amazon Alexa voice control (via Sinric Pro)
 * - WiFi provisioning via BLE
 * - Comprehensive safety interlocks
 * - Persistent settings in flash
 * 
 * Hardware: ESP32-WROOM 38-pin development board
 */

// ============================================
// INCLUDES
// ============================================

#include "Config.h"
#include "blynk_auth.h"
#include "StateManager.h"
#include "SettingsManager.h"
#include "TemperatureSensors.h"
#include "LCDDisplay.h"
#include "ButtonHandler.h"
#include "PumpControl.h"
#include "HeaterControl.h"
#include "SinricIntegration.h"

#include <WiFi.h>
#include <WiFiProv.h>
#include <BlynkSimpleEsp32.h>
#include <time.h>

// ============================================
// GLOBAL OBJECTS
// ============================================

BlynkTimer blynkTimer;

// ============================================
// TIMING VARIABLES
// ============================================

unsigned long lastTempRead = 0;
unsigned long lastLCDUpdate = 0;
unsigned long lastBlynkUpdate = 0;
unsigned long lastSinricTempUpdate = 0;
unsigned long lastControlCycle = 0;

// ============================================
// BLYNK WRITE HANDLERS
// ============================================

BLYNK_WRITE(VPIN_TARGET_TEMP) {
  float temp = param.asFloat();
  Serial.print("Blynk: Target temp ");
  Serial.println(temp);
  setTargetTemperature(temp);
}

BLYNK_WRITE(VPIN_HEATER_ENABLE) {
  bool enabled = param.asInt();
  Serial.print("Blynk: Heater ");
  Serial.println(enabled ? "ENABLED" : "DISABLED");
  setHeaterEnabledState(enabled);
}

BLYNK_WRITE(VPIN_PUMP_ENABLE) {
  bool enabled = param.asInt();
  Serial.print("Blynk: Pump ");
  Serial.println(enabled ? "ON" : "OFF");
  
  if (enabled) {
    if (state.pumpState == PUMP_OFF) {
      setPumpState(PUMP_LOW);
    }
  } else {
    if (canPumpTurnOff()) {
      setPumpState(PUMP_OFF);
    } else {
      Serial.println("Cannot turn pump OFF (safety interlock)");
      syncPumpToBlynk();  // Revert in Blynk
    }
  }
}

BLYNK_WRITE(VPIN_PUMP_SPEED) {
  bool high = param.asInt();
  Serial.print("Blynk: Pump speed ");
  Serial.println(high ? "HIGH" : "LOW");
  
  if (state.pumpState != PUMP_OFF) {
    setPumpState(high ? PUMP_HIGH : PUMP_LOW);
  }
}

BLYNK_WRITE(VPIN_MAX_RUNTIME) {
  float hours = param.asFloat();
  Serial.print("Blynk: Max runtime ");
  Serial.println(hours);
  setMaxRuntimeHours(hours);
}

BLYNK_WRITE(VPIN_HEATER_START_HOUR) {
  int hour = param.asInt();
  Serial.print("Blynk: Heater start hour ");
  Serial.println(hour);
  setHeaterStartHour(hour);
}

BLYNK_WRITE(VPIN_HEATER_END_HOUR) {
  int hour = param.asInt();
  Serial.print("Blynk: Heater end hour ");
  Serial.println(hour);
  setHeaterEndHour(hour);
}

// ============================================
// BLYNK CONNECTION HANDLERS
// ============================================

BLYNK_CONNECTED() {
  Serial.println("Blynk connected!");
  state.blynkConnected = true;
  syncAllToBlynk();
}

BLYNK_DISCONNECTED() {
  Serial.println("Blynk disconnected");
  state.blynkConnected = false;
}

// ============================================
// BLYNK SYNC FUNCTIONS
// ============================================

void initBlynk() {
  Serial.println("Connecting to Blynk...");
  Blynk.config(auth);
}

void syncAllToBlynk() {
  if (!Blynk.connected()) return;
  
  syncTemperaturesToBlynk();
  syncTargetTempToBlynk();
  syncHeaterToBlynk();
  syncPumpToBlynk();
  syncPumpRuntimeToBlynk();
  
  Blynk.virtualWrite(VPIN_MAX_RUNTIME, state.maxRuntimeHours);
  Blynk.virtualWrite(VPIN_HEATER_START_HOUR, state.heaterStartHour);
  Blynk.virtualWrite(VPIN_HEATER_END_HOUR, state.heaterEndHour);
  
  Serial.println("Synced all values to Blynk");
}

void syncHeaterToBlynk() {
  if (!Blynk.connected()) return;
  Blynk.virtualWrite(VPIN_HEATER_ENABLE, state.heaterEnabled ? 1 : 0);
}

void syncPumpToBlynk() {
  if (!Blynk.connected()) return;
  
  Blynk.virtualWrite(VPIN_PUMP_ENABLE, state.pumpState != PUMP_OFF ? 1 : 0);
  Blynk.virtualWrite(VPIN_PUMP_SPEED, state.pumpState == PUMP_HIGH ? 1 : 0);
}

void syncTargetTempToBlynk() {
  if (!Blynk.connected()) return;
  Blynk.virtualWrite(VPIN_TARGET_TEMP, state.targetTemp);
}

void syncTemperaturesToBlynk() {
  if (!Blynk.connected()) return;
  
  if (state.inletTempValid) {
    Blynk.virtualWrite(VPIN_INLET_TEMP, state.inletTemp);
  }
  
  if (state.heaterTempValid) {
    Blynk.virtualWrite(VPIN_HEATER_TEMP, state.heaterTemp);
  }
  
  if (state.ambientTempValid) {
    Blynk.virtualWrite(VPIN_AMBIENT_TEMP, state.ambientTemp);
  }
}

void syncPumpRuntimeToBlynk() {
  if (!Blynk.connected()) return;
  Blynk.virtualWrite(VPIN_PUMP_RUNTIME_TODAY, getPumpRuntimeHours());
}

// ============================================
// WIFI PROVISIONING CALLBACKS
// ============================================

void SysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      Serial.println("WiFi Provisioning started");
      displayWiFiProvisioning();
      break;
      
    case ARDUINO_EVENT_PROV_CRED_RECV:
      Serial.println("WiFi credentials received");
      break;
      
    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("WiFi credentials accepted");
      break;
      
    case ARDUINO_EVENT_PROV_END:
      Serial.println("WiFi Provisioning ended");
      break;
      
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("WiFi connected - IP: ");
      Serial.println(WiFi.localIP());
      state.wifiConnected = true;
      updateLCDDisplay();
      break;
      
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi disconnected");
      state.wifiConnected = false;
      state.blynkConnected = false;
      updateLCDDisplay();
      break;
  }
}

// ============================================
// SETUP
// ============================================

void setup() {
  // Initialize serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n========================================");
  Serial.println("ESP32 Pool Controller");
  Serial.println("========================================\n");
  
  // Initialize system state
  initSystemState();
  
  // Initialize hardware
  Serial.println("Initializing hardware...");
  initLCD();
  initTemperatureSensors();
  initButtons();
  initPumpControl();
  initHeaterControl();
  
  // Initialize settings and load from flash
  initSettings();
  loadAllSettings();
  
  // Apply loaded states to hardware
  applyPumpState();
  applyHeaterState();
  
  // WiFi provisioning
  Serial.println("Starting WiFi provisioning...");
  WiFi.onEvent(SysProvEvent);
  
  // Check if already provisioned
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Already provisioned, connecting to WiFi...");
  } else {
    WiFiProv.beginProvision(
      NETWORK_PROV_SCHEME_BLE,
      NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM,
      NETWORK_PROV_SECURITY_1,
      WIFI_PROV_POP,
      WIFI_PROV_SERVICE_NAME
    );
  }
  
  // Wait for WiFi
  Serial.println("Waiting for WiFi connection...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 60) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    state.wifiConnected = true;
    
    // Initialize NTP time sync
    Serial.println("Syncing time with NTP...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
    
    // Initialize cloud services
    initBlynk();
    initSinric();
    
    // Setup Blynk timers
    blynkTimer.setInterval(BLYNK_UPDATE_INTERVAL, []() {
      syncTemperaturesToBlynk();
      syncPumpRuntimeToBlynk();
    });
    
    blynkTimer.setInterval(SINRIC_TEMP_UPDATE_INTERVAL, []() {
      syncTargetTempToSinric();
    });
  } else {
    Serial.println("\nWiFi connection failed - running in offline mode");
  }
  
  updateLCDDisplay();
  
  Serial.println("\n========================================");
  Serial.println("System initialized - entering main loop");
  Serial.println("========================================\n");
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  unsigned long now = millis();
  
  // Run Blynk
  if (state.wifiConnected) {
    Blynk.run();
    blynkTimer.run();
  }
  
  // Run Sinric Pro (Alexa)
  if (state.wifiConnected) {
    handleSinricEvents();
  }
  
  // Scan buttons
  scanButtons();
  
  // Read temperatures periodically
  if (now - lastTempRead >= TEMP_READ_INTERVAL) {
    lastTempRead = now;
    readTemperatures();
  }
  
  // Update LCD periodically
  if (now - lastLCDUpdate >= LCD_UPDATE_INTERVAL) {
    lastLCDUpdate = now;
    updateLCDDisplay();
  }
  
  // Run control system logic
  if (now - lastControlCycle >= CONTROL_CYCLE_INTERVAL) {
    lastControlCycle = now;
    
    // Update pump runtime
    updatePumpRuntime();
    
    // Run heater control logic
    runControlSystem();
    
    // Apply states to hardware
    applyPumpState();
    applyHeaterState();
  }
  
  // Small delay to prevent watchdog issues
  delay(10);
}
