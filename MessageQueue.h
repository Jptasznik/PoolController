#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "StateManager.h"

// ============================================
// MESSAGE QUEUE SYSTEM
// ============================================
// Provides FreeRTOS message queues for inter-task communication.
// Benefits:
// - Decouples tasks from direct state modification
// - Clear data flow and event handling
// - Built-in synchronization
// - Easier debugging and testing
// - Improved system reliability

// ============================================
// MESSAGE TYPES
// ============================================

enum MessageType {
  // Sensor readings
  MSG_TEMPERATURE_UPDATE,      // Temperature sensor reading update
  MSG_CURRENT_UPDATE,          // Current sensor reading update

  // Button events
  MSG_BUTTON_PRESS,            // Button press event
  MSG_BUTTON_EMERGENCY_STOP,   // Emergency shutdown triggered
  MSG_BUTTON_FACTORY_RESET,    // Factory reset triggered

  // Control commands
  MSG_PUMP_COMMAND,            // Pump state change command
  MSG_HEATER_COMMAND,          // Heater state change command
  MSG_TARGET_TEMP_CHANGE,      // Target temperature change

  // System events
  MSG_COOLDOWN_START,          // Cooldown period started
  MSG_COOLDOWN_END,            // Cooldown period ended
  MSG_WIFI_STATUS_CHANGE,      // WiFi connection status changed
  MSG_BLYNK_STATUS_CHANGE,     // Blynk connection status changed

  // Display updates
  MSG_DISPLAY_UPDATE_REQUEST,  // Request LCD display update
  MSG_DISPLAY_REFRESH,         // Force display refresh

  // Logging
  MSG_LOG_EVENT,               // Log event for processing

  // Runtime tracking
  MSG_RUNTIME_UPDATE           // Pump runtime update
};

// ============================================
// MESSAGE PAYLOAD STRUCTURES
// ============================================

// Temperature reading data
struct TemperatureData {
  float inletTemp;
  float heaterTemp;
  float ambientTemp;
  bool valid;
};

// Current reading data
struct CurrentData {
  float current;
  bool valid;
};

// Button event data
struct ButtonData {
  uint8_t buttonNumber;
  bool isLongPress;
};

// Pump command data
struct PumpCommandData {
  PumpState newState;
  bool forceOverride;  // Override safety checks (use with caution)
};

// Heater command data
struct HeaterCommandData {
  bool enabled;
  bool forceOverride;  // Override safety checks (use with caution)
};

// Target temperature change data
struct TargetTempData {
  float newTemp;
};

// WiFi/Blynk status data
struct ConnectionStatusData {
  bool connected;
};

// Display update data
struct DisplayUpdateData {
  bool forceRefresh;
};

// Runtime update data
struct RuntimeUpdateData {
  unsigned long durationMs;
};

// ============================================
// UNIFIED MESSAGE STRUCTURE
// ============================================

struct Message {
  MessageType type;
  unsigned long timestamp;  // millis() when message was created

  // Union to hold different payload types
  union {
    TemperatureData temperature;
    CurrentData current;
    ButtonData button;
    PumpCommandData pumpCommand;
    HeaterCommandData heaterCommand;
    TargetTempData targetTemp;
    ConnectionStatusData connectionStatus;
    DisplayUpdateData displayUpdate;
    RuntimeUpdateData runtime;
  } payload;
};

// ============================================
// QUEUE HANDLES
// ============================================

// Main control queue - receives sensor data and commands
extern QueueHandle_t controlQueue;

// Display queue - receives display update requests
extern QueueHandle_t displayQueue;

// Event queue - receives system events for logging
extern QueueHandle_t eventQueue;

// ============================================
// QUEUE CONFIGURATION
// ============================================

// Queue sizes (number of messages each queue can hold)
#define CONTROL_QUEUE_SIZE 10      // Control decisions, sensor data
#define DISPLAY_QUEUE_SIZE 5       // Display updates (can be dropped if full)
#define EVENT_QUEUE_SIZE 20        // System events for logging

// Queue timeouts (milliseconds)
#define QUEUE_SEND_TIMEOUT_MS 10   // Non-blocking send timeout
#define QUEUE_RECEIVE_TIMEOUT_MS 100  // Receive timeout for polling

// ============================================
// INITIALIZATION
// ============================================

// Initialize all message queues
void initMessageQueues();

// ============================================
// UTILITY FUNCTIONS
// ============================================

// Send message to queue (non-blocking)
// Returns true if message was sent, false if queue was full
bool sendToControlQueue(const Message* msg);
bool sendToDisplayQueue(const Message* msg);
bool sendToEventQueue(const Message* msg);

// Receive message from queue
// Returns true if message was received, false if timeout
bool receiveFromControlQueue(Message* msg, uint32_t timeoutMs);
bool receiveFromDisplayQueue(Message* msg, uint32_t timeoutMs);
bool receiveFromEventQueue(Message* msg, uint32_t timeoutMs);

// Get queue status
uint32_t getControlQueueWaiting();
uint32_t getDisplayQueueWaiting();
uint32_t getEventQueueWaiting();

// ============================================
// MESSAGE HELPERS
// ============================================

// Create common message types (convenience functions)
Message createTemperatureMessage(float inlet, float heater, float ambient, bool valid);
Message createCurrentMessage(float current, bool valid);
Message createButtonMessage(uint8_t buttonNumber, bool isLongPress);
Message createPumpCommandMessage(PumpState newState, bool forceOverride);
Message createHeaterCommandMessage(bool enabled, bool forceOverride);
Message createTargetTempMessage(float newTemp);
Message createDisplayUpdateMessage(bool forceRefresh);

#endif // MESSAGE_QUEUE_H
