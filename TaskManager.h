#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include <esp_task_wdt.h>

// ============================================
// FREERTOS TASK MANAGEMENT
// ============================================
// This module manages all FreeRTOS tasks with
// individual watchdog timers for each task.
//
// Task Architecture:
// - TemperatureSensorTask: Read DS18B20 sensors (5s interval)
// - CurrentMonitorTask: Read ADS1115 current sensor (2s interval)
// - LCDUpdateTask: Update LCD display (1s interval)
// - ButtonScanTask: Scan MCP23008 buttons (50ms interval)
// - ControlSystemTask: Run heater/pump logic (60s interval)
// - BlynkSyncTask: Handle Blynk cloud sync via message queue
// - SinricSyncTask: Handle SinricPro cloud sync via message queue
// - Main loop: Network event processing (Blynk.run(), SinricPro.handle())
// ============================================

// ============================================
// TASK CONFIGURATION
// ============================================

// Stack sizes (in words, not bytes)
#define TEMPERATURE_TASK_STACK_SIZE 4096
#define CURRENT_TASK_STACK_SIZE 4096
#define LCD_TASK_STACK_SIZE 4096
#define BUTTON_TASK_STACK_SIZE 3072
#define CONTROL_TASK_STACK_SIZE 4096
#define BLYNK_SYNC_TASK_STACK_SIZE 4096
#define SINRIC_SYNC_TASK_STACK_SIZE 4096

// Task priorities (0 = lowest, configMAX_PRIORITIES-1 = highest)
// Higher priority = more important/time-critical
#define BUTTON_TASK_PRIORITY 3        // Highest - responsive to user input
#define CONTROL_TASK_PRIORITY 2       // High - safety critical
#define CURRENT_TASK_PRIORITY 2       // High - safety monitoring
#define TEMPERATURE_TASK_PRIORITY 1   // Medium
#define LCD_TASK_PRIORITY 1           // Medium - can tolerate delays
#define BLYNK_SYNC_TASK_PRIORITY 1    // Low - network sync (can tolerate delays)
#define SINRIC_SYNC_TASK_PRIORITY 1   // Low - network sync (can tolerate delays)

// Task core assignment (ESP32 has 2 cores: 0 and 1)
// Core 0: Used by WiFi/network stack
// Core 1: Application tasks (default)
#define TASK_CORE_0 0
#define TASK_CORE_1 1

// Watchdog timeouts (milliseconds)
// Should be 2-3x the task interval to allow for delays
#define TEMPERATURE_WDT_TIMEOUT 15000  // 15s (3x 5s interval)
#define CURRENT_WDT_TIMEOUT 6000       // 6s (3x 2s interval)
#define LCD_WDT_TIMEOUT 3000           // 3s (3x 1s interval)
#define BUTTON_WDT_TIMEOUT 500         // 500ms (10x 50ms interval)
#define CONTROL_WDT_TIMEOUT 180000     // 180s (3x 60s interval)
#define BLYNK_SYNC_WDT_TIMEOUT 60000   // 60s - network operations can be very slow
#define SINRIC_SYNC_WDT_TIMEOUT 60000  // 60s - network operations can be very slow

// ============================================
// TASK INITIALIZATION
// ============================================

// Initialize and start all FreeRTOS tasks
void initTasks();

// ============================================
// TASK FUNCTIONS (called by FreeRTOS)
// ============================================

void temperatureSensorTask(void* parameter);
void currentMonitorTask(void* parameter);
void lcdUpdateTask(void* parameter);
void buttonScanTask(void* parameter);
void controlSystemTask(void* parameter);
void blynkSyncTask(void* parameter);
void sinricSyncTask(void* parameter);

// ============================================
// WATCHDOG MANAGEMENT
// ============================================

// Task handles for watchdog management
extern TaskHandle_t temperatureTaskHandle;
extern TaskHandle_t currentTaskHandle;
extern TaskHandle_t lcdTaskHandle;
extern TaskHandle_t buttonTaskHandle;
extern TaskHandle_t controlTaskHandle;
extern TaskHandle_t blynkSyncTaskHandle;
extern TaskHandle_t sinricSyncTaskHandle;

#endif // TASK_MANAGER_H
