#define LOG_MODULE "MSGQUEUE"

#include "MessageQueue.h"
#include "Logger.h"
#include "StateManager.h"

// ============================================
// QUEUE HANDLES
// ============================================

QueueHandle_t controlQueue = NULL;
QueueHandle_t displayQueue = NULL;
QueueHandle_t eventQueue = NULL;
QueueHandle_t networkSyncQueue = NULL;

// ============================================
// INITIALIZATION
// ============================================

void initMessageQueues() {
  LOG_INFO("Initializing message queues...");

  // Create control queue
  controlQueue = xQueueCreate(CONTROL_QUEUE_SIZE, sizeof(Message));
  if (controlQueue == NULL) {
    LOG_ERROR("Failed to create control queue!");
  } else {
    LOG_INFOF("Control queue created (size: %d)", CONTROL_QUEUE_SIZE);
  }

  // Create display queue
  displayQueue = xQueueCreate(DISPLAY_QUEUE_SIZE, sizeof(Message));
  if (displayQueue == NULL) {
    LOG_ERROR("Failed to create display queue!");
  } else {
    LOG_INFOF("Display queue created (size: %d)", DISPLAY_QUEUE_SIZE);
  }

  // Create event queue
  eventQueue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(Message));
  if (eventQueue == NULL) {
    LOG_ERROR("Failed to create event queue!");
  } else {
    LOG_INFOF("Event queue created (size: %d)", EVENT_QUEUE_SIZE);
  }

  // Create network sync queue
  networkSyncQueue = xQueueCreate(NETWORK_SYNC_QUEUE_SIZE, sizeof(Message));
  if (networkSyncQueue == NULL) {
    LOG_ERROR("Failed to create network sync queue!");
  } else {
    LOG_INFOF("Network sync queue created (size: %d)", NETWORK_SYNC_QUEUE_SIZE);
  }

  LOG_INFO("Message queue system initialized");
}

// ============================================
// SEND FUNCTIONS
// ============================================

bool sendToControlQueue(const Message* msg) {
  if (controlQueue == NULL || msg == NULL) {
    return false;
  }

  BaseType_t result = xQueueSend(controlQueue, msg, pdMS_TO_TICKS(QUEUE_SEND_TIMEOUT_MS));

  if (result != pdTRUE) {
    LOG_WARNF("Control queue full, dropped message type %d", msg->type);
    return false;
  }

  return true;
}

bool sendToDisplayQueue(const Message* msg) {
  if (displayQueue == NULL || msg == NULL) {
    return false;
  }

  BaseType_t result = xQueueSend(displayQueue, msg, pdMS_TO_TICKS(QUEUE_SEND_TIMEOUT_MS));

  if (result != pdTRUE) {
    // Display updates can be dropped - not critical
    LOG_DEBUGF("Display queue full, dropped message type %d", msg->type);
    return false;
  }

  return true;
}

bool sendToEventQueue(const Message* msg) {
  if (eventQueue == NULL || msg == NULL) {
    return false;
  }

  BaseType_t result = xQueueSend(eventQueue, msg, pdMS_TO_TICKS(QUEUE_SEND_TIMEOUT_MS));

  if (result != pdTRUE) {
    LOG_WARNF("Event queue full, dropped message type %d", msg->type);
    return false;
  }

  return true;
}

bool sendToNetworkSyncQueue(const Message* msg) {
  if (networkSyncQueue == NULL || msg == NULL) {
    return false;
  }

  BaseType_t result = xQueueSend(networkSyncQueue, msg, pdMS_TO_TICKS(QUEUE_SEND_TIMEOUT_MS));

  if (result != pdTRUE) {
    // Network sync messages can be dropped - not critical (will sync on next timer)
    LOG_DEBUGF("Network sync queue full, dropped message type %d", msg->type);
    return false;
  }

  return true;
}

// ============================================
// RECEIVE FUNCTIONS
// ============================================

bool receiveFromControlQueue(Message* msg, uint32_t timeoutMs) {
  if (controlQueue == NULL || msg == NULL) {
    return false;
  }

  BaseType_t result = xQueueReceive(controlQueue, msg, pdMS_TO_TICKS(timeoutMs));
  return (result == pdTRUE);
}

bool receiveFromDisplayQueue(Message* msg, uint32_t timeoutMs) {
  if (displayQueue == NULL || msg == NULL) {
    return false;
  }

  BaseType_t result = xQueueReceive(displayQueue, msg, pdMS_TO_TICKS(timeoutMs));
  return (result == pdTRUE);
}

bool receiveFromEventQueue(Message* msg, uint32_t timeoutMs) {
  if (eventQueue == NULL || msg == NULL) {
    return false;
  }

  BaseType_t result = xQueueReceive(eventQueue, msg, pdMS_TO_TICKS(timeoutMs));
  return (result == pdTRUE);
}

bool receiveFromNetworkSyncQueue(Message* msg, uint32_t timeoutMs) {
  if (networkSyncQueue == NULL || msg == NULL) {
    return false;
  }

  BaseType_t result = xQueueReceive(networkSyncQueue, msg, pdMS_TO_TICKS(timeoutMs));
  return (result == pdTRUE);
}

// ============================================
// QUEUE STATUS FUNCTIONS
// ============================================

uint32_t getControlQueueWaiting() {
  if (controlQueue == NULL) {
    return 0;
  }
  return uxQueueMessagesWaiting(controlQueue);
}

uint32_t getDisplayQueueWaiting() {
  if (displayQueue == NULL) {
    return 0;
  }
  return uxQueueMessagesWaiting(displayQueue);
}

uint32_t getEventQueueWaiting() {
  if (eventQueue == NULL) {
    return 0;
  }
  return uxQueueMessagesWaiting(eventQueue);
}

uint32_t getNetworkSyncQueueWaiting() {
  if (networkSyncQueue == NULL) {
    return 0;
  }
  return uxQueueMessagesWaiting(networkSyncQueue);
}

// ============================================
// MESSAGE HELPER FUNCTIONS
// ============================================

Message createTemperatureMessage(float inlet, float heater, float ambient, bool valid) {
  Message msg;
  msg.type = MSG_TEMPERATURE_UPDATE;
  msg.timestamp = millis();
  msg.payload.temperature.inletTemp = inlet;
  msg.payload.temperature.heaterTemp = heater;
  msg.payload.temperature.ambientTemp = ambient;
  msg.payload.temperature.valid = valid;
  return msg;
}

Message createCurrentMessage(float current, bool valid) {
  Message msg;
  msg.type = MSG_CURRENT_UPDATE;
  msg.timestamp = millis();
  msg.payload.current.current = current;
  msg.payload.current.valid = valid;
  return msg;
}

Message createButtonMessage(uint8_t buttonNumber, bool isLongPress) {
  Message msg;
  msg.type = MSG_BUTTON_PRESS;
  msg.timestamp = millis();
  msg.payload.button.buttonNumber = buttonNumber;
  msg.payload.button.isLongPress = isLongPress;
  return msg;
}

Message createPumpCommandMessage(PumpState newState, bool forceOverride) {
  Message msg;
  msg.type = MSG_PUMP_COMMAND;
  msg.timestamp = millis();
  msg.payload.pumpCommand.newState = newState;
  msg.payload.pumpCommand.forceOverride = forceOverride;
  return msg;
}

Message createHeaterCommandMessage(bool enabled, bool forceOverride) {
  Message msg;
  msg.type = MSG_HEATER_COMMAND;
  msg.timestamp = millis();
  msg.payload.heaterCommand.enabled = enabled;
  msg.payload.heaterCommand.forceOverride = forceOverride;
  return msg;
}

Message createTargetTempMessage(float newTemp) {
  Message msg;
  msg.type = MSG_TARGET_TEMP_CHANGE;
  msg.timestamp = millis();
  msg.payload.targetTemp.newTemp = newTemp;
  return msg;
}

Message createDisplayUpdateMessage(bool forceRefresh) {
  Message msg;
  msg.type = MSG_DISPLAY_UPDATE_REQUEST;
  msg.timestamp = millis();
  msg.payload.displayUpdate.forceRefresh = forceRefresh;
  return msg;
}

// ============================================
// NETWORK SYNC MESSAGE HELPERS
// ============================================

// Helper to copy current state into network sync payload
static void copyStateToPayload(NetworkSyncData* payload) {
  // Copy state values using StateManager API
  StateManager& stateMgr = StateManager::getInstance();

  payload->heaterEnabled = stateMgr.isHeaterEnabled();
  payload->heaterOn = (stateMgr.getHeaterStatus() == HEATER_ON);
  payload->pumpState = stateMgr.getPumpState();
  payload->targetTemp = stateMgr.getTargetTemp();
  payload->inletTemp = stateMgr.getInletTemp();
  payload->heaterTemp = stateMgr.getHeaterTemp();
  payload->ambientTemp = stateMgr.getAmbientTemp();
  payload->pumpCurrent = stateMgr.getPumpCurrent();
  payload->pumpRuntimeToday = stateMgr.getPumpRuntimeToday();
  payload->inletTempValid = stateMgr.isInletTempValid();
  payload->heaterTempValid = stateMgr.isHeaterTempValid();
  payload->ambientTempValid = stateMgr.isAmbientTempValid();
  payload->currentSensorValid = stateMgr.isCurrentSensorValid();
  payload->wifiConnected = stateMgr.isWiFiConnected();
  payload->blynkConnected = stateMgr.isBlynkConnected();
  payload->sinricConnected = stateMgr.isSinricConnected();
}

Message createSyncHeaterMessage() {
  Message msg;
  msg.type = MSG_SYNC_HEATER_STATE;
  msg.timestamp = millis();
  copyStateToPayload(&msg.payload.networkSync);
  return msg;
}

Message createSyncPumpMessage() {
  Message msg;
  msg.type = MSG_SYNC_PUMP_STATE;
  msg.timestamp = millis();
  copyStateToPayload(&msg.payload.networkSync);
  return msg;
}

Message createSyncTargetTempMessage() {
  Message msg;
  msg.type = MSG_SYNC_TARGET_TEMP;
  msg.timestamp = millis();
  copyStateToPayload(&msg.payload.networkSync);
  return msg;
}

Message createSyncAllMessage() {
  Message msg;
  msg.type = MSG_SYNC_ALL_STATE;
  msg.timestamp = millis();
  copyStateToPayload(&msg.payload.networkSync);
  return msg;
}

Message createSyncTemperaturesMessage() {
  Message msg;
  msg.type = MSG_SYNC_TEMPERATURES;
  msg.timestamp = millis();
  copyStateToPayload(&msg.payload.networkSync);
  return msg;
}

Message createSyncCurrentMessage() {
  Message msg;
  msg.type = MSG_SYNC_CURRENT;
  msg.timestamp = millis();
  copyStateToPayload(&msg.payload.networkSync);
  return msg;
}

Message createSyncRuntimeMessage() {
  Message msg;
  msg.type = MSG_SYNC_RUNTIME;
  msg.timestamp = millis();
  copyStateToPayload(&msg.payload.networkSync);
  return msg;
}
