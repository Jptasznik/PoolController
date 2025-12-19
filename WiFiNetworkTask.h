#ifndef WIFI_NETWORK_TASK_H
#define WIFI_NETWORK_TASK_H

#include <Arduino.h>

// ============================================
// WIFI NETWORK TASK
// ============================================
// This module handles all WiFi and network operations
// in a dedicated FreeRTOS task to keep main loop clean
//
// Responsibilities:
// - Blynk cloud service (Blynk.run())
// - SinricPro cloud service (handleSinricEvents())
// - OTA firmware updates (handleOTA())
// - WiFi runtime failure detection
// - Network sync message queue processing
// ============================================

// Initialize WiFi network task (called from TaskManager)
void wifiNetworkTask(void* parameter);

// Helper functions (called from task)
void processNetworkSyncMessages();
void checkWiFiRuntimeFailures();

#endif // WIFI_NETWORK_TASK_H
