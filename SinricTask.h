#ifndef SINRIC_TASK_H
#define SINRIC_TASK_H

#include <Arduino.h>

// ============================================
// SINRIC PRO SYNC TASK
// ============================================
// FreeRTOS task that handles all SinricPro cloud communication
// via message queue to prevent blocking other tasks

// Task initialization
void initSinricTask();

// Sinric task function (runs in dedicated FreeRTOS task)
void sinricSyncTask(void* parameter);

// SinricPro connection management
void connectSinric();
void disconnectSinric();
bool isSinricConnected();

// SinricPro.handle() must still be called from main loop for network processing
// This task only handles sync operations via message queue

#endif // SINRIC_TASK_H
