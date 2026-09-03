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
#include "esp_idf_version.h"
#if ENABLE_WIFI_PROV && defined(CONFIG_NETWORK_PROV_NETWORK_TYPE_WIFI)
  #include "WiFiProvLite.h"
  #define WIFI_PROV_ACTIVE 1
#else
  #define WIFI_PROV_ACTIVE 0
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

#if ENABLE_BLYNK
BLYNK_WRITE(VPIN_TARGET_TEMP) {
  StateManager& stateMgr = StateManager::getInstance();
  float temp = param.asFloat();

  // Validate for NaN/Inf
  if (isnan(temp) || isinf(temp)) {
    LOG_ERROR("Rejected invalid temp from Blynk: NaN/Inf");
    Blynk.virtualWrite(VPIN_TARGET_TEMP, stateMgr.getTargetTemp());  // Revert
    return;
  }

  // Validate range
  if (temp < MIN_TARGET_TEMP || temp > MAX_TARGET_TEMP) {
    LOG_WARNF("Rejected invalid temp from Blynk: %.1f (range: %.1f-%.1f)",
              temp, MIN_TARGET_TEMP, MAX_TARGET_TEMP);
    Blynk.virtualWrite(VPIN_TARGET_TEMP, stateMgr.getTargetTemp());  // Revert
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
  StateManager& stateMgr = StateManager::getInstance();
  bool enabled = param.asInt();
  LOG_INFOF("Blynk: Pump %s", enabled ? "ON" : "OFF");

  if (enabled) {
    if (stateMgr.getPumpState() == PUMP_OFF) {
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
  StateManager& stateMgr = StateManager::getInstance();
  bool high = param.asInt();
  LOG_INFOF("Blynk: Pump speed %s", high ? "HIGH" : "LOW");

  if (stateMgr.getPumpState() != PUMP_OFF) {
    setPumpState(high ? PUMP_HIGH : PUMP_LOW);
  }
}

BLYNK_WRITE(VPIN_MAX_RUNTIME) {
  StateManager& stateMgr = StateManager::getInstance();
  float hours = param.asFloat();

  // Validate for NaN/Inf
  if (isnan(hours) || isinf(hours)) {
    LOG_ERROR("Rejected invalid max runtime from Blynk: NaN/Inf");
    Blynk.virtualWrite(VPIN_MAX_RUNTIME, stateMgr.getMaxRuntimeHours());  // Revert
    return;
  }

  // Validate range (0-24 hours)
  if (hours < 0 || hours > 24) {
    LOG_WARNF("Rejected invalid max runtime from Blynk: %.1f (range: 0-24)", hours);
    Blynk.virtualWrite(VPIN_MAX_RUNTIME, stateMgr.getMaxRuntimeHours());  // Revert
    return;
  }

  LOG_INFOF("Blynk: Max runtime %.1f", hours);
  setMaxRuntimeHours(hours);
}

BLYNK_WRITE(VPIN_HEATER_START_HOUR) {
  StateManager& stateMgr = StateManager::getInstance();
  int hour = param.asInt();

  // Validate range (0-23)
  if (hour < 0 || hour > 23) {
    LOG_WARNF("Rejected invalid start hour from Blynk: %d (range: 0-23)", hour);
    Blynk.virtualWrite(VPIN_HEATER_START_HOUR, stateMgr.getHeaterStartHour());  // Revert
    return;
  }

  LOG_INFOF("Blynk: Heater start hour %d", hour);
  setHeaterStartHour(hour);
}

BLYNK_WRITE(VPIN_HEATER_END_HOUR) {
  StateManager& stateMgr = StateManager::getInstance();
  int hour = param.asInt();

  // Validate range (0-23)
  if (hour < 0 || hour > 23) {
    LOG_WARNF("Rejected invalid end hour from Blynk: %d (range: 0-23)", hour);
    Blynk.virtualWrite(VPIN_HEATER_END_HOUR, stateMgr.getHeaterEndHour());  // Revert
    return;
  }

  LOG_INFOF("Blynk: Heater end hour %d", hour);
  setHeaterEndHour(hour);
}

BLYNK_WRITE(VPIN_MIN_HEATER_TEMP) {
  StateManager& stateMgr = StateManager::getInstance();
  float temp = param.asFloat();

  // Validate for NaN/Inf
  if (isnan(temp) || isinf(temp)) {
    LOG_ERROR("Rejected invalid min heater temp from Blynk: NaN/Inf");
    Blynk.virtualWrite(VPIN_MIN_HEATER_TEMP, stateMgr.getMinHeaterTemp());  // Revert
    return;
  }

  // Validate range (32-104°F - physical water temperature range)
  if (temp < INVALID_TEMP_LOW || temp > MAX_TARGET_TEMP) {
    LOG_WARNF("Rejected invalid min heater temp from Blynk: %.1f (range: %.1f-%.1f)",
              temp, INVALID_TEMP_LOW, MAX_TARGET_TEMP);
    Blynk.virtualWrite(VPIN_MIN_HEATER_TEMP, stateMgr.getMinHeaterTemp());  // Revert
    return;
  }

  LOG_INFOF("Blynk: Min heater temp %.1f", temp);
  setMinHeaterTemp(temp);
}

BLYNK_WRITE(VPIN_MAX_HEATER_TEMP) {
  StateManager& stateMgr = StateManager::getInstance();
  float temp = param.asFloat();

  // Validate for NaN/Inf
  if (isnan(temp) || isinf(temp)) {
    LOG_ERROR("Rejected invalid max heater temp from Blynk: NaN/Inf");
    Blynk.virtualWrite(VPIN_MAX_HEATER_TEMP, stateMgr.getMaxHeaterTemp());  // Revert
    return;
  }

  // Validate range (40-110°F - safety limits)
  if (temp < MIN_TARGET_TEMP || temp > MAX_HEATER_TEMP) {
    LOG_WARNF("Rejected invalid max heater temp from Blynk: %.1f (range: %.1f-%.1f)",
              temp, MIN_TARGET_TEMP, MAX_HEATER_TEMP);
    Blynk.virtualWrite(VPIN_MAX_HEATER_TEMP, stateMgr.getMaxHeaterTemp());  // Revert
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
    Blynk.virtualWrite(VPIN_LOG_BUFFER_SIZE, getLogBufferMaxEntries());  // Revert to current value
  }
}

// ============================================
// BLYNK CONNECTION HANDLERS
// ============================================

BLYNK_CONNECTED() {
  StateManager& stateMgr = StateManager::getInstance();
  LOG_INFO("========================================");
  LOG_INFO("Blynk connected!");
  LOG_INFO("========================================");
  stateMgr.setBlynkConnected(true);
  syncAllToBlynk();
}

BLYNK_DISCONNECTED() {
  StateManager& stateMgr = StateManager::getInstance();
  LOG_INFO("========================================");
  LOG_INFO("Blynk disconnected");
  LOG_INFO("========================================");
  stateMgr.setBlynkConnected(false);
}

// ============================================
// BLYNK SYNC FUNCTIONS
// ============================================

void initBlynk() {
  if (!hasBlynkCredentials()) {
    LOG_WARN("Blynk credentials not configured, skipping Blynk initialization");
    return;
  }

  const char* authToken = getBlynkAuthToken();

  // Log first 8 and last 4 chars of token for verification (security safe)
  char tokenPreview[32];
  int tokenLen = strlen(authToken);
  if (tokenLen >= 12) {
    snprintf(tokenPreview, sizeof(tokenPreview), "%.8s...%.4s",
             authToken, authToken + tokenLen - 4);
  } else {
    snprintf(tokenPreview, sizeof(tokenPreview), "***");
  }

  LOG_INFOF("Blynk auth token: %s", tokenPreview);
  LOG_INFOF("Connecting to Blynk server: %s:%d", BLYNK_SERVER, BLYNK_PORT);
  Blynk.config(authToken, BLYNK_SERVER, BLYNK_PORT);

  // Note: Actual connection happens when Blynk.run() is first called in main loop
  LOG_INFO("Blynk configured - waiting for connection...");
}

void syncAllToBlynk() {
  if (!Blynk.connected()) return;

  StateManager& stateMgr = StateManager::getInstance();

  syncTemperaturesToBlynk();
  syncTargetTempToBlynk();
  syncHeaterToBlynk();
  syncPumpToBlynk();
  syncPumpRuntimeToBlynk();
  syncPumpCurrentToBlynk();
  syncPumpAlertsToBlynk();

  Blynk.virtualWrite(VPIN_MAX_RUNTIME, stateMgr.getMaxRuntimeHours());
  Blynk.virtualWrite(VPIN_HEATER_START_HOUR, stateMgr.getHeaterStartHour());
  Blynk.virtualWrite(VPIN_HEATER_END_HOUR, stateMgr.getHeaterEndHour());
  Blynk.virtualWrite(VPIN_MIN_HEATER_TEMP, stateMgr.getMinHeaterTemp());
  Blynk.virtualWrite(VPIN_MAX_HEATER_TEMP, stateMgr.getMaxHeaterTemp());
  Blynk.virtualWrite(VPIN_LOG_LEVEL, (int)getLogLevel());
  Blynk.virtualWrite(VPIN_LOG_BUFFER_SIZE, (int)getLogBufferMaxEntries());

  LOG_INFO("Synced all values to Blynk");
}

// ============================================
// BLYNK WRAPPER FUNCTIONS
// ============================================
// These wrapper functions allow other modules to check Blynk status
// without including BlynkSimpleEsp32.h (which would create duplicate global objects)
// NOTE: Blynk.run() is called directly in main loop, not via wrapper

bool isBlynkConnected() {
  return Blynk.connected();
}

// ============================================
// BLYNK STATE SYNC FUNCTIONS
// ============================================

void syncHeaterToBlynk() {
  if (!Blynk.connected()) return;
  StateManager& stateMgr = StateManager::getInstance();
  Blynk.virtualWrite(VPIN_HEATER_ENABLE, stateMgr.isHeaterEnabled() ? 1 : 0);
}

void syncPumpToBlynk() {
  if (!Blynk.connected()) return;
  StateManager& stateMgr = StateManager::getInstance();

  Blynk.virtualWrite(VPIN_PUMP_ENABLE, stateMgr.getPumpState() != PUMP_OFF ? 1 : 0);
  Blynk.virtualWrite(VPIN_PUMP_SPEED, stateMgr.getPumpState() == PUMP_HIGH ? 1 : 0);
}

void syncTargetTempToBlynk() {
  if (!Blynk.connected()) return;
  StateManager& stateMgr = StateManager::getInstance();
  Blynk.virtualWrite(VPIN_TARGET_TEMP, stateMgr.getTargetTemp());
}

void syncTemperaturesToBlynk() {
  if (!Blynk.connected()) return;
  StateManager& stateMgr = StateManager::getInstance();

  if (stateMgr.isInletTempValid()) {
    Blynk.virtualWrite(VPIN_INLET_TEMP, stateMgr.getInletTemp());
  }

  if (stateMgr.isHeaterTempValid()) {
    Blynk.virtualWrite(VPIN_HEATER_TEMP, stateMgr.getHeaterTemp());
  }

  if (stateMgr.isAmbientTempValid()) {
    Blynk.virtualWrite(VPIN_AMBIENT_TEMP, stateMgr.getAmbientTemp());
  }
}

void syncPumpRuntimeToBlynk() {
  if (!Blynk.connected()) return;
  Blynk.virtualWrite(VPIN_PUMP_RUNTIME_TODAY, getPumpRuntimeHours());
}

void syncPumpCurrentToBlynk() {
  if (!Blynk.connected()) return;
  StateManager& stateMgr = StateManager::getInstance();
  if (stateMgr.isCurrentSensorValid()) {
    Blynk.virtualWrite(VPIN_PUMP_CURRENT, stateMgr.getPumpCurrent());
  }
}

void syncPumpAlertsToBlynk() {
  if (!Blynk.connected()) return;
  StateManager& stateMgr = StateManager::getInstance();
  Blynk.virtualWrite(VPIN_PUMP_FAILURE_ALERT, stateMgr.isPumpFailureAlert() ? 1 : 0);
}
#endif // ENABLE_BLYNK

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
#if WIFI_PROV_ACTIVE
  if (!WiFiProvLite.resetProvision()) {
    LOG_ERROR("Failed to reset the Network Credentials");
    return;
  }
#else
  WiFi.disconnect(true, true);
#endif
  // Reset boot failure counter (this is a runtime failure, not boot failure)
  resetWiFiFailureCount();

  // Clear runtime tracking
  clearWiFiRuntimeTracking();

  // Wait for user to see message
  delay(WIFI_FAILURE_RESET_DELAY);  // BLOCKING: Allow user to read WiFi reset message on LCD (acceptable before restart)

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

#if WIFI_PROV_ACTIVE

void SysProvEvent(arduino_event_t *sys_event) {
  StateManager& stateMgr = StateManager::getInstance();

  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      LOG_INFO("WiFi Provisioning started");
      stateMgr.setProvisioningActive(true);
      stateMgr.setProvisioningStartTime(millis());
      displayWiFiProvisioning();
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV:
      LOG_INFO("WiFi credentials received - waiting for validation");
      stateMgr.setCredentialsReceived(true);
      stateMgr.setCredReceivedTime(millis());
      break;

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      LOG_INFO("WiFi credentials accepted");
      stateMgr.setCredentialsReceived(false);  // Clear flag - validation successful
      stateMgr.setCredReceivedTime(0);
      break;

    case ARDUINO_EVENT_PROV_END:
      LOG_INFO("WiFi Provisioning ended");
      stateMgr.setProvisioningActive(false);
      stateMgr.setProvisioningStartTime(0);
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      LOG_INFOF("WiFi connected - IP: %s", WiFi.localIP().toString().c_str());
      stateMgr.setWiFiConnected(true);
      clearWiFiRuntimeTracking();  // Clear disconnect tracking on successful connection
      updateLCDDisplay();
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      LOG_WARN("WiFi disconnected");
      stateMgr.setWiFiConnected(false);
      stateMgr.setBlynkConnected(false);
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

  LOG_INFO("========================================");
  LOG_INFO("ESP32 Pool Controller");
  LOG_INFO("========================================");

  // Initialize mutexes for thread-safe access
  LOG_INFO("Initializing mutexes...");
  initI2CMutex();

  // Initialize system state (includes state mutex initialization)
  initSystemState();

  // Initialize I2C bus
#ifndef NO_HARDWARE
  bool i2cInitialized = Wire.begin(I2C_SDA, I2C_SCL);
  if (!i2cInitialized) {
    LOG_ERROR("CRITICAL: Failed to initialize I2C bus!");
    LOG_ERROR("Check I2C pin connections (SDA: GPIO21, SCL: GPIO22)");
    LOG_ERROR("System cannot operate without I2C (LCD, buttons, current sensor)");
  } else {
    Wire.setClock(400000);  // Fast Mode - reduces I2C transaction time, cuts LCD lock hold time
    LOG_INFO("I2C bus initialized successfully (400kHz)");
  }
#else
  LOG_INFO("I2C bus initialization skipped (NO_HARDWARE mode)");
#endif

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
  
  #if WIFI_PROV_ACTIVE
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
    StateManager& stateMgr = StateManager::getInstance();
    LOG_INFO("WiFi connected!");
    stateMgr.setWiFiConnected(true);

    // Reset failure counter on successful connection
    resetWiFiFailureCount();

    // Initialize NTP time sync
    LOG_INFO("Syncing time with NTP...");
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
    
    // Initialize cloud services
#if ENABLE_BLYNK
    initBlynk();
#endif
#if ENABLE_SINRIC
    initSinric();
#endif

    // BlynkTimer removed - periodic sync now handled by BlynkSyncTask and SinricSyncTask
    // BlynkSyncTask syncs temperatures/current/runtime/alerts every 2 seconds
    // SinricSyncTask syncs target temp/inlet temp every 30 seconds
  } else {
    LOG_WARN("WiFi not connected - continuing in offline mode");
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
  {
    esp_err_t err;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_task_wdt_config_t wdt_config = {
      .timeout_ms = 180000,   // 180 seconds - matches CONTROL_WDT_TIMEOUT
      .trigger_panic = true
    };
    err = esp_task_wdt_init(&wdt_config);
    if (err != ESP_OK) {
      LOG_ERRORF("Failed to init watchdog: %s", esp_err_to_name(err));
      err = esp_task_wdt_reconfigure(&wdt_config);
    }
#else
    err = esp_task_wdt_init(180, true);  // 180 seconds, panic on timeout
#endif
    if (err != ESP_OK) {
      LOG_ERRORF("Watchdog init/configure failed: %s", esp_err_to_name(err));
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

  LOG_INFO("========================================");
  LOG_INFO("System initialized - entering main loop");
  LOG_INFO("Main loop handles network services");
  LOG_INFO("All other functions run in FreeRTOS tasks");
  LOG_INFO("========================================");
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  // ============================================
  // MAIN LOOP - NETWORK EVENT PROCESSING
  // ============================================
  // All hardware functions run in FreeRTOS tasks:
  // - Temperature reading (TemperatureSensorTask)
  // - Current monitoring (CurrentMonitorTask)
  // - LCD updates (LCDUpdateTask)
  // - Button scanning (ButtonScanTask)
  // - Control system logic (ControlSystemTask)
  //
  // WiFiNetworkTask handles:
  // - Network sync messages (queued updates to Blynk)
  // - SinricPro event processing (Alexa)
  // - OTA firmware updates
  // - WiFi runtime failure detection
  //
  // Main loop handles:
  // - Blynk.run() for incoming commands (must be called frequently)
  // ============================================

  StateManager& stateMgr = StateManager::getInstance();

  // Process Blynk incoming messages if connected
  if (stateMgr.isWiFiConnected()) {
#if ENABLE_BLYNK
    Blynk.run();  // Process incoming Blynk commands (BLYNK_WRITE handlers)
#endif
  }

  // Small delay to prevent tight loop
  delay(10);  // Reduced to 10ms for more responsive Blynk updates
}
