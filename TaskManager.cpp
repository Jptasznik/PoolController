#include "TaskManager.h"
#include "Config.h"
#include "StateManager.h"
#include "StateMutex.h"
#include "TemperatureSensors.h"
#include "CurrentMonitor.h"
#include "LCDDisplay.h"
#include "ButtonHandler.h"
#include "HeaterControl.h"
#include "PumpControl.h"

// ============================================
// TASK HANDLES
// ============================================

TaskHandle_t temperatureTaskHandle = NULL;
TaskHandle_t currentTaskHandle = NULL;
TaskHandle_t lcdTaskHandle = NULL;
TaskHandle_t buttonTaskHandle = NULL;
TaskHandle_t controlTaskHandle = NULL;

// ============================================
// TEMPERATURE SENSOR TASK
// ============================================

void temperatureSensorTask(void* parameter) {
  Serial.println("[TEMP TASK] Starting temperature sensor task");

  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  Serial.println("[TEMP TASK] Watchdog timer added");

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(TEMP_READ_INTERVAL);

  for (;;) {
    // Reset watchdog
    esp_task_wdt_reset();

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
  Serial.println("[CURRENT TASK] Starting current monitor task");

  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  Serial.println("[CURRENT TASK] Watchdog timer added");

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(CURRENT_READ_INTERVAL);

  for (;;) {
    // Reset watchdog
    esp_task_wdt_reset();

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
  Serial.println("[LCD TASK] Starting LCD update task");

  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  Serial.println("[LCD TASK] Watchdog timer added");

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(LCD_UPDATE_INTERVAL);

  for (;;) {
    // Reset watchdog
    esp_task_wdt_reset();

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
  Serial.println("[BUTTON TASK] Starting button scan task");

  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  Serial.println("[BUTTON TASK] Watchdog timer added");

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(BUTTON_SCAN_INTERVAL);

  for (;;) {
    // Reset watchdog
    esp_task_wdt_reset();

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
  Serial.println("[CONTROL TASK] Starting control system task");

  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  Serial.println("[CONTROL TASK] Watchdog timer added");

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(CONTROL_CYCLE_INTERVAL);

  for (;;) {
    // Reset watchdog
    esp_task_wdt_reset();

    // Update pump runtime
    updatePumpRuntime();

    // Run heater control logic
    runControlSystem();

    // Check for pump failure (pump should be running but no current detected)
    bool pumpFailure = isPumpFailureDetected();
    bool currentPumpFailureAlert;
    {
      StateScopedLock stateLock;
      if (stateLock.isLocked()) {
        currentPumpFailureAlert = state.pumpFailureAlert;
      } else {
        currentPumpFailureAlert = false;
      }
    }

    if (pumpFailure && !currentPumpFailureAlert) {
      StateScopedLock stateLock;
      if (stateLock.isLocked()) {
        state.pumpFailureAlert = true;
      }
      Serial.println("!!! PUMP FAILURE DETECTED !!!");
      Serial.println("Pump should be running but no current detected");
    } else if (!pumpFailure && currentPumpFailureAlert) {
      StateScopedLock stateLock;
      if (stateLock.isLocked()) {
        state.pumpFailureAlert = false;
      }
      Serial.println("Pump failure cleared");
    }

    // Check for overcurrent
    bool overcurrent = isOvercurrentDetected();
    bool currentOvercurrentAlert;
    {
      StateScopedLock stateLock;
      if (stateLock.isLocked()) {
        currentOvercurrentAlert = state.overcurrentAlert;
      } else {
        currentOvercurrentAlert = false;
      }
    }

    if (overcurrent && !currentOvercurrentAlert) {
      StateScopedLock stateLock;
      if (stateLock.isLocked()) {
        state.overcurrentAlert = true;
        Serial.println("!!! OVERCURRENT DETECTED !!!");
        Serial.print("Current: ");
        Serial.print(state.pumpCurrent, 2);
        Serial.println(" A");
      }
    } else if (!overcurrent && currentOvercurrentAlert) {
      StateScopedLock stateLock;
      if (stateLock.isLocked()) {
        state.overcurrentAlert = false;
      }
      Serial.println("Overcurrent condition cleared");
    }

    // Apply states to hardware
    applyPumpState();
    applyHeaterState();

    // Wait for next cycle
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

// ============================================
// TASK INITIALIZATION
// ============================================

void initTasks() {
  Serial.println("========================================");
  Serial.println("Initializing FreeRTOS Tasks");
  Serial.println("========================================");

  BaseType_t result;

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
    Serial.println("FATAL ERROR: Failed to create Temperature sensor task!");
    Serial.println("System cannot operate safely without this task.");
    Serial.println("Restarting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("[INIT] Temperature sensor task created");

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
    Serial.println("FATAL ERROR: Failed to create Current monitor task!");
    Serial.println("System cannot operate safely without this task.");
    Serial.println("Restarting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("[INIT] Current monitor task created");

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
    Serial.println("FATAL ERROR: Failed to create LCD update task!");
    Serial.println("System cannot operate safely without this task.");
    Serial.println("Restarting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("[INIT] LCD update task created");

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
    Serial.println("FATAL ERROR: Failed to create Button scan task!");
    Serial.println("System cannot operate safely without this task.");
    Serial.println("Restarting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("[INIT] Button scan task created");

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
    Serial.println("FATAL ERROR: Failed to create Control system task!");
    Serial.println("System cannot operate safely without this task.");
    Serial.println("Restarting in 5 seconds...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("[INIT] Control system task created");

  Serial.println("========================================");
  Serial.println("All FreeRTOS tasks started successfully");
  Serial.println("========================================\n");
}
