#ifndef WIFI_NETWORK_TASK_H
#define WIFI_NETWORK_TASK_H

#include <Arduino.h>

// ============================================
// WIFI NETWORK TASK
// ============================================
// This module handles network operations in a dedicated FreeRTOS task
//
// Responsibilities:
// - Network sync message queue processing (Blynk sync operations)
// - SinricPro cloud service (handleSinricEvents())
// - OTA firmware updates (handleOTA())
// - WiFi runtime failure detection
//
// NOTE: Blynk.run() is called in main loop for responsive incoming commands
// ============================================

// Initialize WiFi network task (called from TaskManager)
void wifiNetworkTask(void* parameter);

// Helper functions (called from task)
void processNetworkSyncMessages();
void checkWiFiRuntimeFailures();

#endif // WIFI_NETWORK_TASK_H
