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
// NOTE: FIRMWARE_VERSION is defined in Config.h

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
#if ENABLE_WIFI_PROV
  #include "WiFiProvLite.h"
#endif
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

// BlynkTimer removed - periodic sync now handled by WiFiNetworkTask

// ============================================
// BLYNK WRITE HANDLERS
// ============================================

BLYNK_WRITE(VPIN_TARGET_TEMP) {
  float temp = param.asFloat();

  // Validate for NaN/Inf
  if (isnan(temp) || isinf(temp)) {
    LOG_ERROR("Rejected invalid temp from Blynk: NaN/Inf");
    Blynk.virtualWrite(VPIN_TARGET_TEMP, state.targetTemp);  // Revert
    return;
  }

  // Validate range
  if (temp < MIN_TARGET_TEMP || temp > MAX_TARGET_TEMP) {
    LOG_WARNF("Rejected invalid temp from Blynk: %.1f (range: %.1f-%.1f)",
              temp, MIN_TARGET_TEMP, MAX_TARGET_TEMP);
    Blynk.virtualWrite(VPIN_TARGET_TEMP, state.targetTemp);  // Revert
    return;
  }

  LOG_INFOF("Blynk: Target temp %.1f", temp);
  setTargetTemperature(temp);
}

BLYNK_WRITE(VPIN_HEATER_ENABLE) {
  bool enabled = param.asInt();
  LOG_INFOF("Blynk: Heater %s", enabled ? "ENABLED" : "DISABLED");
  setHeaterEnabledState(enabled);
}

BLYNK_WRITE(VPIN_PUMP_ENABLE) {
  bool enabled = param.asInt();
  LOG_INFOF("Blynk: Pump %s", enabled ? "ON" : "OFF");

  if (enabled) {
    if (state.pumpState == PUMP_OFF) {
      setPumpState(PUMP_LOW);
    }
  } else {
    if (canPumpTurnOff()) {
      setPumpState(PUMP_OFF);
    } else {
      LOG_WARN("Cannot turn pump OFF (safety interlock)");
      syncPumpToBlynk();  // Revert in Blynk
    }
  }
}

BLYNK_WRITE(VPIN_PUMP_SPEED) {
  bool high = param.asInt();
  LOG_INFOF("Blynk: Pump speed %s", high ? "HIGH" : "LOW");

  if (state.pumpState != PUMP_OFF) {
    setPumpState(high ? PUMP_HIGH : PUMP_LOW);
  }
}

BLYNK_WRITE(VPIN_MAX_RUNTIME) {
  float hours = param.asFloat();

  // Validate for NaN/Inf
  if (isnan(hours) || isinf(hours)) {
    LOG_ERROR("Rejected invalid max runtime from Blynk: NaN/Inf");
    Blynk.virtualWrite(VPIN_MAX_RUNTIME, state.maxRuntimeHours);  // Revert
    return;
  }

  // Validate range (0-24 hours)
  if (hours < 0 || hours > 24) {
    LOG_WARNF("Rejected invalid max runtime from Blynk: %.1f (range: 0-24)", hours);
    Blynk.virtualWrite(VPIN_MAX_RUNTIME, state.maxRuntimeHours);  // Revert
    return;
  }

  LOG_INFOF("Blynk: Max runtime %.1f", hours);
  setMaxRuntimeHours(hours);
}

BLYNK_WRITE(VPIN_HEATER_START_HOUR) {
  int hour = param.asInt();

  // Validate range (0-23)
  if (hour < 0 || hour > 23) {
    LOG_WARNF("Rejected invalid start hour from Blynk: %d (range: 0-23)", hour);
    Blynk.virtualWrite(VPIN_HEATER_START_HOUR, state.heaterStartHour);  // Revert
    return;
  }

  LOG_INFOF("Blynk: Heater start hour %d", hour);
  setHeaterStartHour(hour);
}

BLYNK_WRITE(VPIN_HEATER_END_HOUR) {
  int hour = param.asInt();

  // Validate range (0-23)
  if (hour < 0 || hour > 23) {
    LOG_WARNF("Rejected invalid end hour from Blynk: %d (range: 0-23)", hour);
    Blynk.virtualWrite(VPIN_HEATER_END_HOUR, state.heaterEndHour);  // Revert
    return;
  }

  LOG_INFOF("Blynk: Heater end hour %d", hour);
  setHeaterEndHour(hour);
}

BLYNK_WRITE(VPIN_MIN_HEATER_TEMP) {
  float temp = param.asFloat();

  // Validate for NaN/Inf
  if (isnan(temp) || isinf(temp)) {
    LOG_ERROR("Rejected invalid min heater temp from Blynk: NaN/Inf");
    Blynk.virtualWrite(VPIN_MIN_HEATER_TEMP, state.minHeaterTemp);  // Revert
    return;
  }

  // Validate range (32-104°F - physical water temperature range)
  if (temp < INVALID_TEMP_LOW || temp > MAX_TARGET_TEMP) {
    LOG_WARNF("Rejected invalid min heater temp from Blynk: %.1f (range: %.1f-%.1f)",
              temp, INVALID_TEMP_LOW, MAX_TARGET_TEMP);
    Blynk.virtualWrite(VPIN_MIN_HEATER_TEMP, state.minHeaterTemp);  // Revert
    return;
  }

  LOG_INFOF("Blynk: Min heater temp %.1f", temp);
  setMinHeaterTemp(temp);
}

BLYNK_WRITE(VPIN_MAX_HEATER_TEMP) {
  float temp = param.asFloat();

  // Validate for NaN/Inf
  if (isnan(temp) || isinf(temp)) {
    LOG_ERROR("Rejected invalid max heater temp from Blynk: NaN/Inf");
    Blynk.virtualWrite(VPIN_MAX_HEATER_TEMP, state.maxHeaterTemp);  // Revert
    return;
  }

  // Validate range (40-110°F - safety limits)
  if (temp < MIN_TARGET_TEMP || temp > MAX_HEATER_TEMP) {
    LOG_WARNF("Rejected invalid max heater temp from Blynk: %.1f (range: %.1f-%.1f)",
              temp, MIN_TARGET_TEMP, MAX_HEATER_TEMP);
    Blynk.virtualWrite(VPIN_MAX_HEATER_TEMP, state.maxHeaterTemp);  // Revert
    return;
  }

  LOG_INFOF("Blynk: Max heater temp %.1f", temp);
  setMaxHeaterTemp(temp);
}

BLYNK_WRITE(VPIN_LOG_LEVEL) {
  int level = param.asInt();
  LOG_INFOF("Blynk: Log level %d", level);

  // Validate log level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR)
  if (level >= LOG_DEBUG && level <= LOG_ERROR) {
    setLogLevel((LogLevel)level);
    saveLogLevel((LogLevel)level);
  } else {
    LOG_WARNF("Invalid log level: %d", level);
  }
}

BLYNK_WRITE(VPIN_LOG_BUFFER_SIZE) {
  int size = param.asInt();
  LOG_INFOF("Blynk: Log buffer size %d", size);

  // Validate buffer size (12-100 entries)
  if (size >= MIN_LOG_BUFFER_SIZE && size <= MAX_LOG_BUFFER_SIZE) {
    setLogBufferMaxEntries(size);
    saveLogBufferSize(size);
  } else {
    LOG_WARNF("Invalid log buffer size: %d (valid range: %d-%d)",
              size, MIN_LOG_BUFFER_SIZE, MAX_LOG_BUFFER_SIZE);
  }
}

// ============================================
// BLYNK CONNECTION HANDLERS
// ============================================

BLYNK_CONNECTED() {
  LOG_INFO("Blynk connected!");
  state.blynkConnected = true;
  syncAllToBlynk();
}

BLYNK_DISCONNECTED() {
  LOG_INFO("Blynk disconnected");
  state.blynkConnected = false;
}

// ============================================
// BLYNK SYNC FUNCTIONS
// ============================================

void initBlynk() {
  if (!hasBlynkCredentials()) {
    LOG_WARN("Blynk credentials not configured, skipping Blynk initialization");
    return;
  }

  LOG_INFO("Connecting to Blynk...");
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
  Blynk.virtualWrite(VPIN_LOG_LEVEL, (int)getLogLevel());
  Blynk.virtualWrite(VPIN_LOG_BUFFER_SIZE, (int)getLogBufferMaxEntries());

  LOG_INFO("Synced all values to Blynk");
}

// ============================================
// BLYNK WRAPPER FUNCTIONS FOR WIFI NETWORK TASK
// ============================================
// These wrapper functions allow WiFiNetworkTask.cpp to call Blynk methods
// without including BlynkSimpleEsp32.h (which would create duplicate global objects)

void runBlynk() {
  Blynk.run();
}

bool isBlynkConnected() {
  return Blynk.connected();
}

// ============================================
// BLYNK STATE SYNC FUNCTIONS
// ============================================

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
  LOG_ERROR("========================================");
  LOG_ERRORF("WiFi Runtime Failure: %s", reason);
  LOG_ERROR("Clearing WiFi credentials and resetting...");
  LOG_ERROR("========================================");

  // Display message on LCD
  displayWiFiReset();

  // Clear WiFi credentials
  if (!WiFiProvLite.resetProvision()) {
    LOG_ERROR("Failed to reset the Network Credentials");
    return;
  }
  // Reset boot failure counter (this is a runtime failure, not boot failure)
  resetWiFiFailureCount();

  // Clear runtime tracking
  clearWiFiRuntimeTracking();

  // Wait for user to see message
  delay(WIFI_FAILURE_RESET_DELAY);

  // Restart ESP32
#ifndef NO_HARDWARE
  ESP.restart();
#else
  LOG_WARN("NO_HARDWARE mode: Skipping restart for WiFi runtime failure");
#endif
}

// ============================================
// WIFI PROVISIONING CALLBACKS
// ============================================

#if ENABLE_WIFI_PROV

void SysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      LOG_INFO("WiFi Provisioning started");
      state.provisioningActive = true;
      state.provisioningStartTime = millis();
      displayWiFiProvisioning();
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV:
      LOG_INFO("WiFi credentials received - waiting for validation");
      state.credentialsReceived = true;
      state.credReceivedTime = millis();
      break;

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      LOG_INFO("WiFi credentials accepted");
      state.credentialsReceived = false;  // Clear flag - validation successful
      state.credReceivedTime = 0;
      break;

    case ARDUINO_EVENT_PROV_END:
      LOG_INFO("WiFi Provisioning ended");
      state.provisioningActive = false;
      state.provisioningStartTime = 0;
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      LOG_INFOF("WiFi connected - IP: %s", WiFi.localIP().toString().c_str());
      state.wifiConnected = true;
      clearWiFiRuntimeTracking();  // Clear disconnect tracking on successful connection
      updateLCDDisplay();
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      LOG_WARN("WiFi disconnected");
      state.wifiConnected = false;
      state.blynkConnected = false;
      recordWiFiDisconnect();  // Track disconnect for runtime failure detection
      updateLCDDisplay();
      break;
  }
}

#endif

// ============================================
// SETUP
// ============================================

void setup() {
  // Initialize serial
  Serial.begin(115200);
  delay(1000);

  // Initialize logging system (must be after Serial.begin)
  initLogger();
  initLogBuffer();

  LOG_INFO("\n\n========================================");
  LOG_INFO("ESP32 Pool Controller");
  LOG_INFO("========================================\n");

  // Initialize mutexes for thread-safe access
  LOG_INFO("Initializing mutexes...");
  initI2CMutex();
  initStateMutex();

  // Initialize system state
  initSystemState();

  // Initialize I2C bus
  Wire.begin(I2C_SDA, I2C_SCL);
  LOG_INFO("I2C bus initialized");

  // Initialize hardware
  LOG_INFO("Initializing hardware...");
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

  // Load runtime state (boot count, crash detection, cooldown recovery)
  loadRuntimeState();

  // Apply loaded states to hardware
  applyPumpState();
  applyHeaterState();

  // ============================================
  // WIFI FAILURE DETECTION
  // ============================================

  // Load WiFi failure count from flash
  int wifiFailureCount = loadWiFiFailureCount();
  LOG_INFOF("WiFi failure count: %d", wifiFailureCount);

  // Check if we've exceeded the failure threshold
  if (wifiFailureCount >= WIFI_MAX_BOOT_FAILURES) {
    LOG_WARN("========================================");
    LOG_WARN("WiFi failure threshold exceeded!");
    LOG_WARN("Clearing WiFi credentials and resetting...");
    LOG_WARN("========================================");

    // Clear WiFi credentials
    WiFi.disconnect(true, true);  // disconnect and erase stored credentials

    // Reset failure counter
    resetWiFiFailureCount();

    // Display message on LCD
    displayWiFiReset();

    // Wait for user to see message
    delay(WIFI_FAILURE_RESET_DELAY);

    // Restart ESP32
#ifndef NO_HARDWARE
    ESP.restart();
#else
    LOG_WARN("NO_HARDWARE mode: Skipping restart for WiFi boot failure");
#endif
  }

  // ============================================
  // WIFI PROVISIONING
  // ============================================
  
  #if ENABLE_WIFI_PROV
  LOG_INFO("Starting WiFi provisioning...");
  WiFi.onEvent(SysProvEvent);

  // Check if already provisioned
  if (WiFi.status() == WL_CONNECTED) {
    LOG_INFO("Already provisioned, connecting to WiFi...");
  } else {
    WiFiProvLite.beginProvision(
      NETWORK_PROV_SCHEME_SOFTAP, 
      NETWORK_PROV_SCHEME_HANDLER_NONE,
      // NETWORK_PROV_SCHEME_BLE,
      // NETWORK_PROV_SCHEME_HANDLER_FREE_BTDM,
      NETWORK_PROV_SECURITY_1,
      WIFI_PROV_POP,
      WIFI_PROV_SERVICE_NAME
    );
  } 
  #endif

  // Wait for WiFi
  LOG_INFO("Waiting for WiFi connection...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECTION_TIMEOUT) {
    delay(1000);
    Serial.print(".");  // Keep dots on one line
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    LOG_INFO("\nWiFi connected!");
    state.wifiConnected = true;

    // Reset failure counter on successful connection
    resetWiFiFailureCount();

    // Initialize NTP time sync
    LOG_INFO("Syncing time with NTP...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
    
    // Initialize cloud services
    initBlynk();
    initSinric();

    // BlynkTimer removed - periodic sync now handled by BlynkSyncTask and SinricSyncTask
    // BlynkSyncTask syncs temperatures/current/runtime/alerts every 2 seconds
    // SinricSyncTask syncs target temp/inlet temp every 30 seconds
  } else {
    LOG_WARN("\nWiFi not connected - continuing in offline mode");
    LOG_INFO("WiFi will auto-connect in background when available");
    LOG_INFO("Local controls (buttons, LCD) are fully functional");

    // Increment failure counter
    incrementWiFiFailureCount();
    LOG_INFOF("WiFi failures incremented to: %d", loadWiFiFailureCount());
    LOG_INFOF("Will reset credentials after %d consecutive boot failures", WIFI_MAX_BOOT_FAILURES);
  }

  updateLCDDisplay();

  // Initialize watchdog timer (global timeout for all tasks)
  // Note: ESP32 uses a single global watchdog timeout for all tasks
  // Set to 180s to accommodate the control task (60s interval * 3 = 180s)
#ifndef DISABLE_WATCHDOG
  LOG_INFO("Initializing watchdog timer...");
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 180000,     // 180 seconds (3 minutes) - matches CONTROL_WDT_TIMEOUT
    .trigger_panic = true      // Panic on timeout
  };

  // Initialize watchdog with error checking
  esp_err_t err = esp_task_wdt_init(&wdt_config);
  if (err != ESP_OK) {
    LOG_ERRORF("Failed to init watchdog: %s", esp_err_to_name(err));
    err = esp_task_wdt_reconfigure(&wdt_config);
    if ( err != ESP_OK) {
      LOG_ERROR("Watchdog timer failed to configure the timeout");
    } else {
      LOG_INFO("Watchdog timer enabled (180s timeout - global for all tasks)");
    }
  } else {
    // Add main task to watchdog
    err = esp_task_wdt_add(NULL);
    if (err != ESP_OK) {
      LOG_ERRORF("Failed to add main task to watchdog: %s", esp_err_to_name(err));
      LOG_WARN("Main task will run without watchdog monitoring");
    } else {
      LOG_INFO("Watchdog timer enabled (180s timeout - global for all tasks)");
    }
  }
#else
  LOG_WARN("DISABLE_WATCHDOG: Main task watchdog timer disabled");
#endif

  // Start FreeRTOS tasks (each task has its own watchdog)
  #ifndef DO_NOT_CREATE_TASKS
    initTasks();
    LOG_INFO("Creating tasks, monitoring hardware.");
  #else
    LOG_INFO("NOT Creating tasks, not monitoring hardware.");
  #endif

  LOG_INFO("\n========================================");
  LOG_INFO("System initialized - entering main loop");
  LOG_INFO("Main loop handles network services");
  LOG_INFO("All other functions run in FreeRTOS tasks");
  LOG_INFO("========================================\n");
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  // Reset watchdog timer at start of every loop iteration
#ifndef DISABLE_WATCHDOG
  esp_task_wdt_reset();
#endif

  // ============================================
  // MAIN LOOP - MINIMAL COORDINATION
  // ============================================
  // All functions now run in FreeRTOS tasks:
  // - Temperature reading (TemperatureSensorTask)
  // - Current monitoring (CurrentMonitorTask)
  // - LCD updates (LCDUpdateTask)
  // - Button scanning (ButtonScanTask)
  // - Control system logic (ControlSystemTask)
  // - Network services (WiFiNetworkTask - Blynk, Sinric, OTA, WiFi monitoring)
  //
  // Main loop is now minimal - just keep watchdog happy
  // ============================================

  // Small delay to prevent tight loop
  delay(100);
}
