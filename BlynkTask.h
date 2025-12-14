#ifndef BLYNK_TASK_H
#define BLYNK_TASK_H

#include <Arduino.h>

// ============================================
// BLYNK SYNC TASK
// ============================================
// FreeRTOS task that handles all Blynk cloud communication
// via message queue to prevent blocking other tasks

// Task initialization
void initBlynkTask();

// Blynk task function (runs in dedicated FreeRTOS task)
void blynkSyncTask(void* parameter);

// Blynk connection management
void connectBlynk();
void disconnectBlynk();
bool isBlynkConnected();

// Blynk.run() must still be called from main loop for network processing
// This task only handles sync operations via message queue

#endif // BLYNK_TASK_H
