#define LOG_MODULE "SINRIC_TASK"

#include "SinricTask.h"
#include "Logger.h"
#include "Config.h"
#include "MessageQueue.h"
#include "CredentialsManager.h"
#include "StateManager.h"
#include "StateMutex.h"
#include <SinricPro.h>
#include <SinricProThermostat.h>
#include <SinricProDimSwitch.h>

// ============================================
// PERIODIC SYNC HELPERS
// ============================================

static void syncPeriodicDataToSinric() {
  if (!SinricPro.isConnected() || !hasSinricCredentials()) return;

  // Copy state with mutex protection
  float targetTemp, inletTemp;
  bool inletValid;

  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      targetTemp = state.targetTemp;
      inletTemp = state.inletTemp;
      inletValid = state.inletTempValid;
    } else {
      return; // Couldn't get lock, skip this sync
    }
  }

  // Sync target temperature and current temperature to thermostat
  SinricProThermostat &thermostat = SinricPro[getSinricThermostatId()];
  thermostat.sendTargetTemperatureEvent(targetTemp);

  if (inletValid) {
    thermostat.sendTemperatureEvent(inletTemp);
  }

  LOG_DEBUG("Periodic sync to Sinric complete");
}

// ============================================
// SINRIC PRO SYNC TASK
// ============================================

void sinricSyncTask(void* parameter) {
  LOG_INFO("Sinric sync task started");

  // Add this task to watchdog (60 second timeout for slow network operations)
#ifndef DISABLE_WATCHDOG
  esp_task_wdt_add(NULL);
  LOG_DEBUG("Sinric sync task watchdog timer added (60s timeout)");
#else
  LOG_WARN("DISABLE_WATCHDOG: Sinric sync task watchdog disabled");
#endif

  // Periodic sync timing
  unsigned long lastPeriodicSync = 0;
  const unsigned long periodicSyncInterval = SINRIC_TEMP_UPDATE_INTERVAL; // 30 seconds

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
      syncPeriodicDataToSinric();
    }

    // Wait for sync message from queue
    if (receiveFromNetworkSyncQueue(&msg, receiveTimeout)) {
      // Only process if Sinric is connected and credentials are configured
      if (!SinricPro.isConnected() || !hasSinricCredentials()) {
        LOG_DEBUG("Sinric not connected or no credentials, skipping sync");
        continue;
      }

      // Get device references
      SinricProThermostat &thermostat = SinricPro[getSinricThermostatId()];
      SinricProDimSwitch &pumpSwitch = SinricPro[getSinricPumpId()];

      // Process message based on type
      switch (msg.type) {
        case MSG_SYNC_HEATER_STATE:
          LOG_DEBUG("Syncing heater state to Sinric");
          thermostat.sendPowerStateEvent(msg.payload.networkSync.heaterEnabled);
          thermostat.sendThermostatModeEvent(msg.payload.networkSync.heaterEnabled ? "HEAT" : "OFF");
          break;

        case MSG_SYNC_PUMP_STATE:
          {
            LOG_DEBUG("Syncing pump state to Sinric");
            bool pumpOn = (msg.payload.networkSync.pumpState != PUMP_OFF);
            int pumpPercentage = 0;

            if (msg.payload.networkSync.pumpState == PUMP_LOW) {
              pumpPercentage = 50;  // LOW = 50%
            } else if (msg.payload.networkSync.pumpState == PUMP_HIGH) {
              pumpPercentage = 100; // HIGH = 100%
            }

            pumpSwitch.sendPowerStateEvent(pumpOn);
            if (pumpOn) {
              pumpSwitch.sendPowerLevelEvent(pumpPercentage);
            }
          }
          break;

        case MSG_SYNC_TARGET_TEMP:
          LOG_DEBUG("Syncing target temperature to Sinric");
          thermostat.sendTargetTemperatureEvent(msg.payload.networkSync.targetTemp);
          break;

        case MSG_SYNC_TEMPERATURES:
          LOG_DEBUG("Syncing temperatures to Sinric");
          if (msg.payload.networkSync.inletTempValid) {
            thermostat.sendTemperatureEvent(msg.payload.networkSync.inletTemp);
          }
          break;

        case MSG_SYNC_ALL_STATE:
          LOG_DEBUG("Syncing all state to Sinric");
          // Heater
          thermostat.sendPowerStateEvent(msg.payload.networkSync.heaterEnabled);
          thermostat.sendThermostatModeEvent(msg.payload.networkSync.heaterEnabled ? "HEAT" : "OFF");
          thermostat.sendTargetTemperatureEvent(msg.payload.networkSync.targetTemp);
          if (msg.payload.networkSync.inletTempValid) {
            thermostat.sendTemperatureEvent(msg.payload.networkSync.inletTemp);
          }

          // Pump
          {
            bool pumpOn = (msg.payload.networkSync.pumpState != PUMP_OFF);
            int pumpPercentage = 0;

            if (msg.payload.networkSync.pumpState == PUMP_LOW) {
              pumpPercentage = 50;
            } else if (msg.payload.networkSync.pumpState == PUMP_HIGH) {
              pumpPercentage = 100;
            }

            pumpSwitch.sendPowerStateEvent(pumpOn);
            if (pumpOn) {
              pumpSwitch.sendPowerLevelEvent(pumpPercentage);
            }
          }
          break;

        default:
          // Ignore other message types (Sinric doesn't sync current/runtime)
          break;
      }
    }

    // Small delay to prevent tight loop (queue receive already has timeout)
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ============================================
// SINRIC PRO CONNECTION MANAGEMENT
// ============================================

void connectSinric() {
  if (!hasSinricCredentials()) {
    LOG_WARN("No Sinric credentials configured");
    return;
  }

  const char* appKey = getSinricAppKey();
  const char* appSecret = getSinricAppSecret();
  const char* thermostatId = getSinricThermostatId();
  const char* pumpId = getSinricPumpId();

  if (appKey == NULL || appSecret == NULL || thermostatId == NULL || pumpId == NULL) {
    LOG_ERROR("Sinric credentials are incomplete");
    return;
  }

  LOG_INFO("Connecting to SinricPro...");
  SinricPro.begin(appKey, appSecret);
}

void disconnectSinric() {
  if (SinricPro.isConnected()) {
    LOG_INFO("Disconnecting from SinricPro");
    // Note: SinricPro library doesn't have explicit disconnect method
    // Connection will close when WiFi disconnects
  }
}

bool isSinricConnected() {
  return SinricPro.isConnected();
}
