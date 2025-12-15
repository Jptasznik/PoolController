#define LOG_MODULE "WIFI_TASK"

#include "WiFiNetworkTask.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"
#include "StateMutex.h"
#include "MessageQueue.h"
#include "SinricIntegration.h"
#include <BlynkSimpleEsp32.h>
#include <esp_task_wdt.h>

// External Blynk instance (defined in PoolController.ino)
extern BlynkWifi Blynk;

// External sync functions (defined in PoolController.ino)
extern void syncHeaterToBlynk();
extern void syncPumpToBlynk();
extern void syncTargetTempToBlynk();
extern void syncTemperaturesToBlynk();
extern void syncPumpRuntimeToBlynk();
extern void syncPumpCurrentToBlynk();
extern void syncPumpAlertsToBlynk();

// External WiFi failure handler (defined in PoolController.ino)
extern void handleWiFiRuntimeFailure(const char* reason);

// Timing for WiFi runtime checks
static unsigned long lastWiFiRuntimeCheck = 0;

// ============================================
// NETWORK SYNC MESSAGE PROCESSING
// ============================================

void processNetworkSyncMessages() {
  if (!Blynk.connected()) return;

  Message msg;
  const uint32_t receiveTimeout = 10; // 10ms timeout for non-blocking check

  // Process up to 5 messages per iteration to avoid blocking too long
  for (int i = 0; i < 5; i++) {
    if (receiveFromNetworkSyncQueue(&msg, receiveTimeout)) {
      // Process message based on type
      switch (msg.type) {
        case MSG_SYNC_HEATER_STATE:
          LOG_DEBUG("Syncing heater state to Blynk");
          syncHeaterToBlynk();
          break;

        case MSG_SYNC_PUMP_STATE:
          LOG_DEBUG("Syncing pump state to Blynk");
          syncPumpToBlynk();
          break;

        case MSG_SYNC_TARGET_TEMP:
          LOG_DEBUG("Syncing target temperature to Blynk");
          syncTargetTempToBlynk();
          break;

        case MSG_SYNC_TEMPERATURES:
          LOG_DEBUG("Syncing temperatures to Blynk");
          syncTemperaturesToBlynk();
          break;

        case MSG_SYNC_CURRENT:
          LOG_DEBUG("Syncing current to Blynk");
          syncPumpCurrentToBlynk();
          break;

        case MSG_SYNC_RUNTIME:
          LOG_DEBUG("Syncing runtime to Blynk");
          syncPumpRuntimeToBlynk();
          break;

        case MSG_SYNC_ALL_STATE:
          LOG_DEBUG("Syncing all state to Blynk");
          syncHeaterToBlynk();
          syncPumpToBlynk();
          syncTargetTempToBlynk();
          syncTemperaturesToBlynk();
          syncPumpCurrentToBlynk();
          syncPumpRuntimeToBlynk();
          syncPumpAlertsToBlynk();
          break;

        default:
          LOG_WARNF("Unknown network sync message type: %d", msg.type);
          break;
      }
    } else {
      // No more messages in queue
      break;
    }
  }
}

// ============================================
// WIFI RUNTIME FAILURE CHECKS
// ============================================

void checkWiFiRuntimeFailures() {
  unsigned long now = millis();

  // Only check every 10 seconds
  if (now - lastWiFiRuntimeCheck < WIFI_RUNTIME_CHECK_INTERVAL) {
    return;
  }

  lastWiFiRuntimeCheck = now;

  // Check for provisioning hang
  if (isProvisioningHung()) {
    handleWiFiRuntimeFailure("Provisioning timeout - no response for 5 minutes");
    return; // Return after handling failure
  }

  // Check for credential validation timeout
  bool credentialsReceived;
  unsigned long credReceivedTime;
  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      credentialsReceived = state.credentialsReceived;
      credReceivedTime = state.credReceivedTime;
    } else {
      return; // Can't check without state access
    }
  }

  if (credentialsReceived) {
    unsigned long elapsed = now - credReceivedTime;

    // Handle millis() overflow
    if (elapsed > MILLIS_OVERFLOW_THRESHOLD) {
      // Overflow detected, reset tracking
      StateScopedLock stateLock;
      if (stateLock.isLocked()) {
        state.credentialsReceived = false;
        state.credReceivedTime = 0;
      }
    } else if (elapsed >= WIFI_CRED_VALIDATION_TIMEOUT) {
      LOG_WARN("Credential validation timeout - credentials received but not validated");
      handleWiFiRuntimeFailure("Credential validation timeout (1 minute)");
      return; // Return after handling failure
    }
  }

  // Check for excessive disconnections
  int recentDisconnects = getRecentDisconnectCount();
  if (recentDisconnects >= WIFI_MAX_DISCONNECTS) {
    char msg[64];
    snprintf(msg, sizeof(msg), "Excessive disconnects (%d in 10 minutes)", recentDisconnects);
    handleWiFiRuntimeFailure(msg);
  }
}

// ============================================
// WIFI NETWORK TASK
// ============================================

void wifiNetworkTask(void* parameter) {
  LOG_INFO("WiFi network task started");

  // Add this task to watchdog (180 second timeout - same as global)
#ifndef DISABLE_WATCHDOG
  esp_task_wdt_add(NULL);
  LOG_DEBUG("WiFi network task watchdog timer added (180s timeout)");
#else
  LOG_WARN("DISABLE_WATCHDOG: WiFi network task watchdog disabled");
#endif

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(100); // Run every 100ms

  for (;;) {
    // Reset watchdog at start of loop
#ifndef DISABLE_WATCHDOG
    esp_task_wdt_reset();
#endif

    // Check if WiFi is connected
    bool wifiConnected;
    {
      StateScopedLock stateLock;
      if (stateLock.isLocked()) {
        wifiConnected = state.wifiConnected;
      } else {
        wifiConnected = false;
      }
    }

    // Run network services if connected
    if (wifiConnected) {
      // Run Blynk event processing
      Blynk.run();

      // Reset watchdog after Blynk.run() (can block on slow networks)
#ifndef DISABLE_WATCHDOG
      esp_task_wdt_reset();
#endif

      // Process queued network sync messages
      processNetworkSyncMessages();

      // Run SinricPro event processing (Alexa)
      handleSinricEvents();

      // Reset watchdog after SinricPro (can block on slow networks)
#ifndef DISABLE_WATCHDOG
      esp_task_wdt_reset();
#endif

      // Handle OTA updates
      handleOTA();
    }

    // Check for WiFi runtime failures (runs even if not connected)
    checkWiFiRuntimeFailures();

    // Wait for next cycle
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
