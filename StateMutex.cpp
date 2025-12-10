#include "StateMutex.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ============================================
// STATE MUTEX IMPLEMENTATION
// ============================================

static SemaphoreHandle_t stateMutex = NULL;

void initStateMutex() {
  if (stateMutex == NULL) {
    stateMutex = xSemaphoreCreateMutex();
    if (stateMutex == NULL) {
      Serial.println("ERROR: Failed to create state mutex!");
    } else {
      Serial.println("State mutex created successfully");
    }
  }
}

bool lockState(uint32_t timeout_ms) {
  if (stateMutex == NULL) {
    Serial.println("ERROR: State mutex not initialized!");
    return false;
  }

  TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
  if (xSemaphoreTake(stateMutex, timeout_ticks) == pdTRUE) {
    return true;
  } else {
    Serial.println("WARNING: State mutex lock timeout!");
    return false;
  }
}

void unlockState() {
  if (stateMutex == NULL) {
    Serial.println("ERROR: State mutex not initialized!");
    return;
  }

  xSemaphoreGive(stateMutex);
}

// ============================================
// SCOPED LOCK IMPLEMENTATION
// ============================================

StateScopedLock::StateScopedLock(uint32_t timeout_ms) {
  locked = lockState(timeout_ms);
  if (!locked) {
    Serial.println("ERROR: Failed to acquire state lock!");
  }
}

StateScopedLock::~StateScopedLock() {
  if (locked) {
    unlockState();
  }
}
