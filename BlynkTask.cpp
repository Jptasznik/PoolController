#define LOG_MODULE "BLYNK_TASK"

#include "BlynkTask.h"
#include "Logger.h"
#include "Config.h"
#include "MessageQueue.h"
#include "CredentialsManager.h"
#include "StateManager.h"
#include "StateMutex.h"
#include <BlynkSimpleEsp32.h>
#include <esp_task_wdt.h>

// External Blynk instance (defined in PoolController.ino)
extern BlynkWifi Blynk;

// ============================================
// PERIODIC SYNC HELPERS
// ============================================

static void syncPeriodicDataToBlynk() {
  if (!Blynk.connected()) return;

  // Copy state with mutex protection
  float inletTemp, heaterTemp, ambientTemp, pumpCurrent, pumpRuntimeToday;
  bool inletValid, heaterValid, ambientValid, currentValid, pumpFailureAlert;

  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      inletTemp = state.inletTemp;
      heaterTemp = state.heaterTemp;
      ambientTemp = state.ambientTemp;
      pumpCurrent = state.pumpCurrent;
      pumpRuntimeToday = state.pumpRuntimeToday;
      inletValid = state.inletTempValid;
      heaterValid = state.heaterTempValid;
      ambientValid = state.ambientTempValid;
      currentValid = state.currentSensorValid;
      pumpFailureAlert = state.pumpFailureAlert;
    } else {
      return; // Couldn't get lock, skip this sync
    }
  }

  // Sync temperatures
  if (inletValid) {
    Blynk.virtualWrite(VPIN_INLET_TEMP, inletTemp);
  }
  if (heaterValid) {
    Blynk.virtualWrite(VPIN_HEATER_TEMP, heaterTemp);
  }
  if (ambientValid) {
    Blynk.virtualWrite(VPIN_AMBIENT_TEMP, ambientTemp);
  }

  // Sync current
  if (currentValid) {
    Blynk.virtualWrite(VPIN_PUMP_CURRENT, pumpCurrent);
  }

  // Sync runtime
  Blynk.virtualWrite(VPIN_PUMP_RUNTIME_TODAY, pumpRuntimeToday);

  // Sync pump failure alert
  Blynk.virtualWrite(VPIN_PUMP_FAILURE_ALERT, pumpFailureAlert ? 1 : 0);

  LOG_DEBUG("Periodic sync to Blynk complete");
}

// ============================================
// BLYNK SYNC TASK
// ============================================

void blynkSyncTask(void* parameter) {
  LOG_INFO("Blynk sync task started");

  // Add this task to watchdog (60 second timeout for slow network operations)
#ifndef DISABLE_WATCHDOG
  esp_task_wdt_add(NULL);
  LOG_DEBUG("Blynk sync task watchdog timer added (60s timeout)");
#else
  LOG_WARN("DISABLE_WATCHDOG: Blynk sync task watchdog disabled");
#endif

  // Periodic sync timing
  unsigned long lastPeriodicSync = 0;
  const unsigned long periodicSyncInterval = BLYNK_UPDATE_INTERVAL; // 2 seconds

  // Wait for message queue messages
  Message msg;
  const uint32_t receiveTimeout = 100; // 100ms timeout for queue receive

  for (;;) {
    // Reset watchdog
#ifndef DISABLE_WATCHDOG
    esp_task_wdt_reset();
#endif

    // Check if periodic sync is due
    unsigned long now = millis();
    if (now - lastPeriodicSync >= periodicSyncInterval) {
      lastPeriodicSync = now;
      syncPeriodicDataToBlynk();
    }

    // Wait for sync message from queue
    if (receiveFromNetworkSyncQueue(&msg, receiveTimeout)) {
      // Only process if Blynk is connected
      if (!Blynk.connected()) {
        LOG_DEBUG("Blynk not connected, skipping sync");
        continue;
      }

      // Process message based on type
      switch (msg.type) {
        case MSG_SYNC_HEATER_STATE:
          LOG_DEBUG("Syncing heater state to Blynk");
          Blynk.virtualWrite(VPIN_HEATER_ENABLE, msg.payload.networkSync.heaterEnabled ? 1 : 0);
          break;

        case MSG_SYNC_PUMP_STATE:
          LOG_DEBUG("Syncing pump state to Blynk");
          Blynk.virtualWrite(VPIN_PUMP_ENABLE, msg.payload.networkSync.pumpState != PUMP_OFF ? 1 : 0);
          Blynk.virtualWrite(VPIN_PUMP_SPEED, msg.payload.networkSync.pumpState == PUMP_HIGH ? 1 : 0);
          break;

        case MSG_SYNC_TARGET_TEMP:
          LOG_DEBUG("Syncing target temperature to Blynk");
          Blynk.virtualWrite(VPIN_TARGET_TEMP, msg.payload.networkSync.targetTemp);
          break;

        case MSG_SYNC_TEMPERATURES:
          LOG_DEBUG("Syncing temperatures to Blynk");
          if (msg.payload.networkSync.inletTempValid) {
            Blynk.virtualWrite(VPIN_INLET_TEMP, msg.payload.networkSync.inletTemp);
          }
          if (msg.payload.networkSync.heaterTempValid) {
            Blynk.virtualWrite(VPIN_HEATER_TEMP, msg.payload.networkSync.heaterTemp);
          }
          if (msg.payload.networkSync.ambientTempValid) {
            Blynk.virtualWrite(VPIN_AMBIENT_TEMP, msg.payload.networkSync.ambientTemp);
          }
          break;

        case MSG_SYNC_CURRENT:
          LOG_DEBUG("Syncing current to Blynk");
          if (msg.payload.networkSync.currentSensorValid) {
            Blynk.virtualWrite(VPIN_PUMP_CURRENT, msg.payload.networkSync.pumpCurrent);
          }
          break;

        case MSG_SYNC_RUNTIME:
          LOG_DEBUG("Syncing runtime to Blynk");
          Blynk.virtualWrite(VPIN_PUMP_RUNTIME_TODAY, msg.payload.networkSync.pumpRuntimeToday);
          break;

        case MSG_SYNC_ALL_STATE:
          LOG_DEBUG("Syncing all state to Blynk");
          // Heater
          Blynk.virtualWrite(VPIN_HEATER_ENABLE, msg.payload.networkSync.heaterEnabled ? 1 : 0);
          // Pump
          Blynk.virtualWrite(VPIN_PUMP_ENABLE, msg.payload.networkSync.pumpState != PUMP_OFF ? 1 : 0);
          Blynk.virtualWrite(VPIN_PUMP_SPEED, msg.payload.networkSync.pumpState == PUMP_HIGH ? 1 : 0);
          // Target temperature
          Blynk.virtualWrite(VPIN_TARGET_TEMP, msg.payload.networkSync.targetTemp);
          // Temperatures
          if (msg.payload.networkSync.inletTempValid) {
            Blynk.virtualWrite(VPIN_INLET_TEMP, msg.payload.networkSync.inletTemp);
          }
          if (msg.payload.networkSync.heaterTempValid) {
            Blynk.virtualWrite(VPIN_HEATER_TEMP, msg.payload.networkSync.heaterTemp);
          }
          if (msg.payload.networkSync.ambientTempValid) {
            Blynk.virtualWrite(VPIN_AMBIENT_TEMP, msg.payload.networkSync.ambientTemp);
          }
          // Current
          if (msg.payload.networkSync.currentSensorValid) {
            Blynk.virtualWrite(VPIN_PUMP_CURRENT, msg.payload.networkSync.pumpCurrent);
          }
          // Runtime
          Blynk.virtualWrite(VPIN_PUMP_RUNTIME_TODAY, msg.payload.networkSync.pumpRuntimeToday);
          break;

        default:
          LOG_WARNF("Unknown message type for Blynk sync: %d", msg.type);
          break;
      }
    }

    // Small delay to prevent tight loop (queue receive already has timeout)
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ============================================
// BLYNK CONNECTION MANAGEMENT
// ============================================

void connectBlynk() {
  if (!hasBlynkCredentials()) {
    LOG_WARN("No Blynk credentials configured");
    return;
  }

  const char* authToken = getBlynkAuthToken();
  if (authToken == NULL || strlen(authToken) == 0) {
    LOG_ERROR("Blynk auth token is empty");
    return;
  }

  LOG_INFO("Connecting to Blynk...");
  Blynk.config(authToken);
  Blynk.connect();
}

void disconnectBlynk() {
  if (Blynk.connected()) {
    LOG_INFO("Disconnecting from Blynk");
    Blynk.disconnect();
  }
}

bool isBlynkConnected() {
  return Blynk.connected();
}
