/*
 * ESP32 Pool Controller System
 *
 * Main controller file - coordinates all subsystems
 *
 * Features:
 * - Triple DS18B20 temperature sensors (inlet, heater, ambient)
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
#include "StateManager.h"
#include "StateMutex.h"
#include "I2CMutex.h"
#include "Logger.h"
#include "CredentialsManager.h"
#include "SettingsManager.h"
#include "TemperatureSensors.h"
#include "LCDDisplay.h"
#include "ButtonHandler.h"
#include "PumpControl.h"
#include "HeaterControl.h"
#include "SinricIntegration.h"
#include "CurrentMonitor.h"
#include "TaskManager.h"

#include <WiFi.h>
#include <WiFiProv.h>
#include <BlynkSimpleEsp32.h>
#include <time.h>
#include <esp_task_wdt.h>

// ============================================
// MODULE NAME FOR LOGGING
// ============================================

#define LOG_MODULE "MAIN"

// ============================================
// GLOBAL OBJECTS
// ============================================

BlynkTimer blynkTimer;

// ============================================
// TIMING VARIABLES
// ============================================

unsigned long lastWiFiRuntimeCheck = 0;

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

BLYNK_WRITE(VPIN_MIN_HEATER_TEMP) {
  float temp = param.asFloat();
  Serial.print("Blynk: Min heater temp ");
  Serial.println(temp);
  setMinHeaterTemp(temp);
}

BLYNK_WRITE(VPIN_MAX_HEATER_TEMP) {
  float temp = param.asFloat();
  Serial.print("Blynk: Max heater temp ");
  Serial.println(temp);
  setMaxHeaterTemp(temp);
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
  if (!hasBlynkCredentials()) {
    Serial.println("WARNING: Blynk credentials not configured, skipping Blynk initialization");
    return;
  }

  Serial.println("Connecting to Blynk...");
  Blynk.config(getBlynkAuthToken());
}

void syncAllToBlynk() {
  if (!Blynk.connected()) return;

  syncTemperaturesToBlynk();
  syncTargetTempToBlynk();
  syncHeaterToBlynk();
  syncPumpToBlynk();
  syncPumpRuntimeToBlynk();
  syncPumpCurrentToBlynk();
  syncPumpAlertsToBlynk();

  Blynk.virtualWrite(VPIN_MAX_RUNTIME, state.maxRuntimeHours);
  Blynk.virtualWrite(VPIN_HEATER_START_HOUR, state.heaterStartHour);
  Blynk.virtualWrite(VPIN_HEATER_END_HOUR, state.heaterEndHour);
  Blynk.virtualWrite(VPIN_MIN_HEATER_TEMP, state.minHeaterTemp);
  Blynk.virtualWrite(VPIN_MAX_HEATER_TEMP, state.maxHeaterTemp);

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

void syncPumpCurrentToBlynk() {
  if (!Blynk.connected()) return;
  if (state.currentSensorValid) {
    Blynk.virtualWrite(VPIN_PUMP_CURRENT, state.pumpCurrent);
  }
}

void syncPumpAlertsToBlynk() {
  if (!Blynk.connected()) return;
  Blynk.virtualWrite(VPIN_PUMP_FAILURE_ALERT, state.pumpFailureAlert ? 1 : 0);
}

// ============================================
// WIFI RUNTIME FAILURE HANDLER
// ============================================

void handleWiFiRuntimeFailure(const char* reason) {
  Serial.println("========================================");
  Serial.print("WiFi Runtime Failure: ");
  Serial.println(reason);
  Serial.println("Clearing WiFi credentials and resetting...");
  Serial.println("========================================");

  // Display message on LCD
  displayWiFiReset();

  // Clear WiFi credentials
  WiFi.disconnect(true, true);  // disconnect and erase stored credentials

  // Reset boot failure counter (this is a runtime failure, not boot failure)
  resetWiFiFailureCount();

  // Clear runtime tracking
  clearWiFiRuntimeTracking();

  // Wait for user to see message
  delay(WIFI_FAILURE_RESET_DELAY);

  // Restart ESP32
  ESP.restart();
}

// ============================================
// WIFI PROVISIONING CALLBACKS
// ============================================

void SysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      Serial.println("WiFi Provisioning started");
      state.provisioningActive = true;
      state.provisioningStartTime = millis();
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
      state.provisioningActive = false;
      state.provisioningStartTime = 0;
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.print("WiFi connected - IP: ");
      Serial.println(WiFi.localIP());
      state.wifiConnected = true;
      clearWiFiRuntimeTracking();  // Clear disconnect tracking on successful connection
      updateLCDDisplay();
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      Serial.println("WiFi disconnected");
      state.wifiConnected = false;
      state.blynkConnected = false;
      recordWiFiDisconnect();  // Track disconnect for runtime failure detection
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

  // Initialize mutexes for thread-safe access
  Serial.println("Initializing mutexes...");
  initI2CMutex();
  initStateMutex();

  // Initialize system state
  initSystemState();

  // Initialize I2C bus
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("I2C bus initialized");

  // Initialize hardware
  Serial.println("Initializing hardware...");
  initLCD();
  initTemperatureSensors();
  initCurrentMonitor();
  initButtons();
  initPumpControl();
  initHeaterControl();
  
  // Initialize credentials from flash
  initCredentials();

  // Initialize settings and load from flash
  initSettings();
  loadAllSettings();

  // Apply loaded states to hardware
  applyPumpState();
  applyHeaterState();

  // ============================================
  // WIFI FAILURE DETECTION
  // ============================================

  // Load WiFi failure count from flash
  int wifiFailureCount = loadWiFiFailureCount();
  Serial.print("WiFi failure count: ");
  Serial.println(wifiFailureCount);

  // Check if we've exceeded the failure threshold
  if (wifiFailureCount >= WIFI_MAX_BOOT_FAILURES) {
    Serial.println("========================================");
    Serial.println("WARNING: WiFi failure threshold exceeded!");
    Serial.println("Clearing WiFi credentials and resetting...");
    Serial.println("========================================");

    // Clear WiFi credentials
    WiFi.disconnect(true, true);  // disconnect and erase stored credentials

    // Reset failure counter
    resetWiFiFailureCount();

    // Display message on LCD
    displayWiFiReset();

    // Wait for user to see message
    delay(WIFI_FAILURE_RESET_DELAY);

    // Restart ESP32
    ESP.restart();
  }

  // ============================================
  // WIFI PROVISIONING
  // ============================================

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
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECTION_TIMEOUT) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected!");
    state.wifiConnected = true;

    // Reset failure counter on successful connection
    resetWiFiFailureCount();
    
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
      syncPumpCurrentToBlynk();
      syncPumpAlertsToBlynk();
    });
    
    blynkTimer.setInterval(SINRIC_TEMP_UPDATE_INTERVAL, []() {
      syncTargetTempToSinric();
    });
  } else {
    Serial.println("\nWiFi not connected - continuing in offline mode");
    Serial.println("WiFi will auto-connect in background when available");
    Serial.println("Local controls (buttons, LCD) are fully functional");

    // Increment failure counter
    incrementWiFiFailureCount();
    Serial.print("WiFi failures incremented to: ");
    Serial.println(loadWiFiFailureCount());
    Serial.print("Will reset credentials after ");
    Serial.print(WIFI_MAX_BOOT_FAILURES);
    Serial.println(" consecutive boot failures");
  }

  updateLCDDisplay();

  // Initialize watchdog timer (main task: 30 second timeout)
  Serial.println("Initializing main task watchdog timer...");
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 30000,      // 30 seconds in milliseconds
    .trigger_panic = true      // Panic on timeout
  };
  esp_task_wdt_init(&wdt_config);  // Initialize with config struct
  esp_task_wdt_add(NULL);          // Add main task to watchdog
  Serial.println("Main task watchdog timer enabled (30s timeout)");

  // Start FreeRTOS tasks (each task has its own watchdog)
  initTasks();

  Serial.println("\n========================================");
  Serial.println("System initialized - entering main loop");
  Serial.println("Main loop handles network services");
  Serial.println("All other functions run in FreeRTOS tasks");
  Serial.println("========================================\n");
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  // Reset watchdog timer at start of every loop iteration
  esp_task_wdt_reset();

  unsigned long now = millis();

  // ============================================
  // NETWORK SERVICES (Main loop responsibility)
  // ============================================
  // All other functions run in FreeRTOS tasks:
  // - Temperature reading (TemperatureSensorTask)
  // - Current monitoring (CurrentMonitorTask)
  // - LCD updates (LCDUpdateTask)
  // - Button scanning (ButtonScanTask)
  // - Control system logic (ControlSystemTask)
  // ============================================

  bool wifiConnected;
  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      wifiConnected = state.wifiConnected;
    } else {
      wifiConnected = false;
    }
  }

  // Run Blynk
  if (wifiConnected) {
    Blynk.run();
    blynkTimer.run();
  }

  // Run Sinric Pro (Alexa)
  if (wifiConnected) {
    handleSinricEvents();
  }

  // Check for WiFi runtime failures periodically
  if (now - lastWiFiRuntimeCheck >= WIFI_RUNTIME_CHECK_INTERVAL) {
    lastWiFiRuntimeCheck = now;

    // Check for provisioning hang
    if (isProvisioningHung()) {
      handleWiFiRuntimeFailure("Provisioning timeout - no response for 5 minutes");
    }

    // Check for excessive disconnections
    int recentDisconnects = getRecentDisconnectCount();
    if (recentDisconnects >= WIFI_MAX_DISCONNECTS) {
      char msg[64];
      snprintf(msg, sizeof(msg), "Excessive disconnects (%d in 10 minutes)", recentDisconnects);
      handleWiFiRuntimeFailure(msg);
    }
  }

  // Small delay to prevent watchdog issues
  delay(10);
}
