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
// - WiFiNetworkTask: Network services (Blynk, SinricPro, OTA, WiFi monitoring) (100ms interval)
// - Main loop: Minimal coordination only
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
#define WIFI_NETWORK_TASK_STACK_SIZE 8192  // Larger for Blynk/Sinric network libs

// Task priorities (0 = lowest, configMAX_PRIORITIES-1 = highest)
// Higher priority = more important/time-critical
#define BUTTON_TASK_PRIORITY 3        // Highest - responsive to user input
#define CONTROL_TASK_PRIORITY 2       // High - safety critical
#define CURRENT_TASK_PRIORITY 2       // High - safety monitoring
#define TEMPERATURE_TASK_PRIORITY 1   // Medium
#define LCD_TASK_PRIORITY 1           // Medium - can tolerate delays
#define WIFI_NETWORK_TASK_PRIORITY 1  // Low - network sync (can tolerate delays)

// Task core assignment (ESP32 has 2 cores: 0 and 1)
// Core 0: Used by WiFi/network stack
// Core 1: Application tasks (default)
#define TASK_CORE_0 0
#define TASK_CORE_1 1

// Watchdog timeouts (milliseconds) - INFORMATIONAL ONLY
// NOTE: ESP32 uses a single global watchdog timeout for all tasks
// The actual timeout is set in PoolController.ino setup() to 180s (CONTROL_WDT_TIMEOUT)
// These values show the ideal timeout for each task based on interval * 3
#define TEMPERATURE_WDT_TIMEOUT 15000  // 15s (3x 5s interval)
#define CURRENT_WDT_TIMEOUT 6000       // 6s (3x 2s interval)
#define LCD_WDT_TIMEOUT 3000           // 3s (3x 1s interval)
#define BUTTON_WDT_TIMEOUT 500         // 500ms (10x 50ms interval)
#define CONTROL_WDT_TIMEOUT 180000     // 180s (3x 60s interval) - USED AS GLOBAL TIMEOUT

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
void wifiNetworkTask(void* parameter);

// ============================================
// WATCHDOG MANAGEMENT
// ============================================

// Helper function to add current task to watchdog timer
// Returns true if successful, false if failed
void addTaskToWatchdog(const char* taskName);

// Health monitoring functions
void monitorTaskStackUsage();  // Check all task stack high-water marks
void monitorHeapMemory();       // Check free heap and minimum free heap

// Task handles for watchdog management
extern TaskHandle_t temperatureTaskHandle;
extern TaskHandle_t currentTaskHandle;
extern TaskHandle_t lcdTaskHandle;
extern TaskHandle_t buttonTaskHandle;
extern TaskHandle_t controlTaskHandle;
extern TaskHandle_t wifiNetworkTaskHandle;

#endif // TASK_MANAGER_H
