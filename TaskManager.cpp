#define LOG_MODULE "TASKS"

#include "TaskManager.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"
#include "MessageQueue.h"
#include "TemperatureSensors.h"
#include "CurrentMonitor.h"
#include "LCDDisplay.h"
#include "ButtonHandler.h"
#include "HeaterControl.h"
#include "PumpControl.h"
#include "WiFiNetworkTask.h"

// ============================================
// TASK HANDLES
// ============================================

TaskHandle_t temperatureTaskHandle = NULL;
TaskHandle_t currentTaskHandle = NULL;
TaskHandle_t lcdTaskHandle = NULL;
TaskHandle_t buttonTaskHandle = NULL;
TaskHandle_t controlTaskHandle = NULL;
TaskHandle_t wifiNetworkTaskHandle = NULL;

// ============================================
// TEMPERATURE SENSOR TASK
// ============================================

void temperatureSensorTask(void* parameter) {
  LOG_INFO("Temperature sensor task started");

  // Add this task to watchdog
#ifndef DISABLE_WATCHDOG
  esp_err_t err = esp_task_wdt_add(NULL);
  if (err != ESP_OK) {
    LOG_ERRORF("Failed to add temperature task to watchdog: %s", esp_err_to_name(err));
  } else {
    LOG_DEBUG("Temperature task watchdog timer added");
  }
#else
  LOG_WARN("DISABLE_WATCHDOG: Temperature task watchdog disabled");
#endif

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(TEMP_READ_INTERVAL);

  for (;;) {
    // Reset watchdog
#ifndef DISABLE_WATCHDOG
    esp_task_wdt_reset();
#endif

    // Read all temperature sensors
    readTemperatures();

    // Wait for next cycle
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ============================================
// CURRENT MONITOR TASK
// ============================================

void currentMonitorTask(void* parameter) {
  LOG_INFO("Current monitor task started");

  // Add this task to watchdog
#ifndef DISABLE_WATCHDOG
  esp_err_t err = esp_task_wdt_add(NULL);
  if (err != ESP_OK) {
    LOG_ERRORF("Failed to add current monitor task to watchdog: %s", esp_err_to_name(err));
  } else {
    LOG_DEBUG("Current task watchdog timer added");
  }
#else
  LOG_WARN("DISABLE_WATCHDOG: Current monitor task watchdog disabled");
#endif

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(CURRENT_READ_INTERVAL);

  for (;;) {
    // Reset watchdog
#ifndef DISABLE_WATCHDOG
    esp_task_wdt_reset();
#endif

    // Read pump current
    readPumpCurrent();

    // Wait for next cycle
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ============================================
// LCD UPDATE TASK
// ============================================

void lcdUpdateTask(void* parameter) {
  LOG_INFO("LCD update task started");

  // Add this task to watchdog
#ifndef DISABLE_WATCHDOG
  esp_err_t err = esp_task_wdt_add(NULL);
  if (err != ESP_OK) {
    LOG_ERRORF("Failed to add LCD update task to watchdog: %s", esp_err_to_name(err));
  } else {
    LOG_DEBUG("LCD task watchdog timer added");
  }
#else
  LOG_WARN("DISABLE_WATCHDOG: LCD update task watchdog disabled");
#endif

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(LCD_UPDATE_INTERVAL);

  for (;;) {
    // Reset watchdog
#ifndef DISABLE_WATCHDOG
    esp_task_wdt_reset();
#endif

    // Update LCD display
    updateLCDDisplay();

    // Wait for next cycle
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ============================================
// BUTTON SCAN TASK
// ============================================

void buttonScanTask(void* parameter) {
  LOG_INFO("Button scan task started");

  // Add this task to watchdog
#ifndef DISABLE_WATCHDOG
  esp_err_t err = esp_task_wdt_add(NULL);
  if (err != ESP_OK) {
    LOG_ERRORF("Failed to add button scan task to watchdog: %s", esp_err_to_name(err));
  } else {
    LOG_DEBUG("Button task watchdog timer added");
  }
#else
  LOG_WARN("DISABLE_WATCHDOG: Button scan task watchdog disabled");
#endif

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(BUTTON_SCAN_INTERVAL);

  for (;;) {
    // Reset watchdog
#ifndef DISABLE_WATCHDOG
    esp_task_wdt_reset();
#endif

    // Scan buttons
    scanButtons();

    // Wait for next cycle
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ============================================
// CONTROL SYSTEM TASK
// ============================================

void controlSystemTask(void* parameter) {
  LOG_INFO("Control system task started");

  // Add this task to watchdog
#ifndef DISABLE_WATCHDOG
  esp_err_t err = esp_task_wdt_add(NULL);
  if (err != ESP_OK) {
    LOG_ERRORF("Failed to add control system task to watchdog: %s", esp_err_to_name(err));
  } else {
    LOG_DEBUG("Control task watchdog timer added");
  }
#else
  LOG_WARN("DISABLE_WATCHDOG: Control system task watchdog disabled");
#endif

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(CONTROL_CYCLE_INTERVAL);

  for (;;) {
    // Reset watchdog
#ifndef DISABLE_WATCHDOG
    esp_task_wdt_reset();
#endif

    // Update pump runtime
    updatePumpRuntime();

    // Run heater control logic
    runControlSystem();

    // Reset watchdog after control logic (multiple state locks and operations)
#ifndef DISABLE_WATCHDOG
    esp_task_wdt_reset();
#endif

    // Check for pump failure (pump should be running but no current detected)
    StateManager& stateMgr = StateManager::getInstance();
    bool pumpFailure = isPumpFailureDetected();
    bool currentPumpFailureAlert = stateMgr.isPumpFailureAlert();

    if (pumpFailure && !currentPumpFailureAlert) {
      stateMgr.setPumpFailureAlert(true);
      LOG_ERROR("!!! PUMP FAILURE DETECTED !!!");
      LOG_ERROR("Pump should be running but no current detected");
    } else if (!pumpFailure && currentPumpFailureAlert) {
      stateMgr.setPumpFailureAlert(false);
      LOG_INFO("Pump failure cleared");
    }

    // Check for overcurrent
    bool overcurrent = isOvercurrentDetected();
    bool currentOvercurrentAlert = stateMgr.isOvercurrentAlert();
    float pumpCurrent = stateMgr.getPumpCurrent();

    if (overcurrent && !currentOvercurrentAlert) {
      stateMgr.setOvercurrentAlert(true);
      LOG_ERROR("!!! OVERCURRENT DETECTED !!!");
      LOG_ERRORF("Current: %.2f A", pumpCurrent);
    } else if (!overcurrent && currentOvercurrentAlert) {
      stateMgr.setOvercurrentAlert(false);
      LOG_INFO("Overcurrent condition cleared");
    }

    // Apply states to hardware
    applyPumpState();
    applyHeaterState();

    // Health monitoring (periodic checks)
    static unsigned long lastStackMonitor = 0;
    static unsigned long lastHeapMonitor = 0;
    unsigned long now = millis();

    // Check task stack usage every STACK_MONITOR_INTERVAL (5 minutes)
    if (now - lastStackMonitor >= STACK_MONITOR_INTERVAL) {
      monitorTaskStackUsage();
      lastStackMonitor = now;
    }

    // Check heap memory every HEAP_MONITOR_INTERVAL (1 minute)
    if (now - lastHeapMonitor >= HEAP_MONITOR_INTERVAL) {
      monitorHeapMemory();
      lastHeapMonitor = now;
    }

    // Wait for next cycle
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ============================================
// TASK INITIALIZATION
// ============================================

void initTasks() {
  LOG_INFO("========================================");
  LOG_INFO("Initializing FreeRTOS Tasks");
  LOG_INFO("========================================");

  // Initialize message queues first (tasks need them)
  initMessageQueues();

  BaseType_t result;

#ifdef ENABLE_TEMPERATURE_SENSORS
  // Create temperature sensor task
  result = xTaskCreatePinnedToCore(
    temperatureSensorTask,           // Task function
    "TemperatureTask",               // Name
    TEMPERATURE_TASK_STACK_SIZE,     // Stack size
    NULL,                            // Parameters
    TEMPERATURE_TASK_PRIORITY,       // Priority
    &temperatureTaskHandle,          // Task handle
    TASK_CORE_1                      // Core
  );
  if (result != pdPASS) {
    LOG_ERROR("FATAL ERROR: Failed to create Temperature sensor task!");
    LOG_ERROR("System cannot operate safely without this task.");
    LOG_ERROR("Restarting in 5 seconds...");
    delay(5000);  // BLOCKING: Fatal error - allow user to see error before restart (system cannot continue)
    ESP.restart();
  }
  LOG_INFO("[INIT] Temperature sensor task created");
#else
  LOG_WARN("[INIT] Temperature sensor task DISABLED");
#endif

#ifdef ENABLE_CURRENT_MONITOR
  // Create current monitor task
  result = xTaskCreatePinnedToCore(
    currentMonitorTask,
    "CurrentTask",
    CURRENT_TASK_STACK_SIZE,
    NULL,
    CURRENT_TASK_PRIORITY,
    &currentTaskHandle,
    TASK_CORE_1
  );
  if (result != pdPASS) {
    LOG_ERROR("FATAL ERROR: Failed to create Current monitor task!");
    LOG_ERROR("System cannot operate safely without this task.");
    LOG_ERROR("Restarting in 5 seconds...");
    delay(5000);  // BLOCKING: Fatal error - allow user to see error before restart (system cannot continue)
    ESP.restart();
  }
  LOG_INFO("[INIT] Current monitor task created");
#else
  LOG_WARN("[INIT] Current monitor task DISABLED");
#endif

#ifdef ENABLE_LCD
  // Create LCD update task
  result = xTaskCreatePinnedToCore(
    lcdUpdateTask,
    "LCDTask",
    LCD_TASK_STACK_SIZE,
    NULL,
    LCD_TASK_PRIORITY,
    &lcdTaskHandle,
    TASK_CORE_1
  );
  if (result != pdPASS) {
    LOG_ERROR("FATAL ERROR: Failed to create LCD update task!");
    LOG_ERROR("System cannot operate safely without this task.");
    LOG_ERROR("Restarting in 5 seconds...");
    delay(5000);  // BLOCKING: Fatal error - allow user to see error before restart (system cannot continue)
    ESP.restart();
  }
  LOG_INFO("[INIT] LCD update task created");
#else
  LOG_WARN("[INIT] LCD update task DISABLED");
#endif

#ifdef ENABLE_BUTTONS
  // Create button scan task
  result = xTaskCreatePinnedToCore(
    buttonScanTask,
    "ButtonTask",
    BUTTON_TASK_STACK_SIZE,
    NULL,
    BUTTON_TASK_PRIORITY,
    &buttonTaskHandle,
    TASK_CORE_1
  );
  if (result != pdPASS) {
    LOG_ERROR("FATAL ERROR: Failed to create Button scan task!");
    LOG_ERROR("System cannot operate safely without this task.");
    LOG_ERROR("Restarting in 5 seconds...");
    delay(5000);  // BLOCKING: Fatal error - allow user to see error before restart (system cannot continue)
    ESP.restart();
  }
  LOG_INFO("[INIT] Button scan task created");
#else
  LOG_WARN("[INIT] Button scan task DISABLED");
#endif

  // Create control system task
  result = xTaskCreatePinnedToCore(
    controlSystemTask,
    "ControlTask",
    CONTROL_TASK_STACK_SIZE,
    NULL,
    CONTROL_TASK_PRIORITY,
    &controlTaskHandle,
    TASK_CORE_1
  );
  if (result != pdPASS) {
    LOG_ERROR("FATAL ERROR: Failed to create Control system task!");
    LOG_ERROR("System cannot operate safely without this task.");
    LOG_ERROR("Restarting in 5 seconds...");
    delay(5000);  // BLOCKING: Fatal error - allow user to see error before restart (system cannot continue)
    ESP.restart();
  }
  LOG_INFO("[INIT] Control system task created");

  // Create WiFi network task
  result = xTaskCreatePinnedToCore(
    wifiNetworkTask,
    "WiFiNetworkTask",
    WIFI_NETWORK_TASK_STACK_SIZE,
    NULL,
    WIFI_NETWORK_TASK_PRIORITY,
    &wifiNetworkTaskHandle,
    TASK_CORE_0  // Run on Core 0 with WiFi stack
  );
  if (result != pdPASS) {
    LOG_WARN("Failed to create WiFi network task - network services will be degraded");
  } else {
    LOG_INFO("[INIT] WiFi network task created");
  }

  LOG_INFO("========================================");
  LOG_INFO("All FreeRTOS tasks started successfully");
  LOG_INFO("Network sync handled in WiFiNetworkTask");
  LOG_INFO("========================================\n");
}

// ============================================
// HEALTH MONITORING
// ============================================

void monitorTaskStackUsage() {
  struct TaskInfo {
    const char* name;
    TaskHandle_t handle;
  };

  TaskInfo tasks[] = {
    {"Temperature", temperatureTaskHandle},
    {"Current", currentTaskHandle},
    {"LCD", lcdTaskHandle},
    {"Button", buttonTaskHandle},
    {"Control", controlTaskHandle},
    {"WiFiNetwork", wifiNetworkTaskHandle}
  };

  for (int i = 0; i < 6; i++) {
    if (tasks[i].handle != NULL) {
      UBaseType_t stackHighWater = uxTaskGetStackHighWaterMark(tasks[i].handle);
      UBaseType_t stackFreeBytes = stackHighWater * 4;  // Convert words to bytes

      if (stackFreeBytes < STACK_WARNING_THRESHOLD) {
        LOG_WARNF("%s task stack low: %u bytes free", tasks[i].name, stackFreeBytes);
      } else {
        LOG_DEBUGF("%s task stack: %u bytes free", tasks[i].name, stackFreeBytes);
      }
    }
  }
}

void monitorHeapMemory() {
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t minFreeHeap = ESP.getMinFreeHeap();

  if (freeHeap < HEAP_WARNING_THRESHOLD) {
    LOG_WARNF("Low heap memory: %u bytes free (min ever: %u bytes)", freeHeap, minFreeHeap);
  } else {
    LOG_DEBUGF("Heap: %u bytes free (min ever: %u bytes)", freeHeap, minFreeHeap);
  }
}
