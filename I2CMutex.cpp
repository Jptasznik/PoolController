#include "I2CMutex.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ============================================
// I2C MUTEX IMPLEMENTATION
// ============================================

static SemaphoreHandle_t i2cMutex = NULL;

void initI2CMutex() {
  if (i2cMutex == NULL) {
    i2cMutex = xSemaphoreCreateMutex();
    if (i2cMutex == NULL) {
      Serial.println("ERROR: Failed to create I2C mutex!");
    } else {
      Serial.println("I2C mutex created successfully");
    }
  }
}

bool lockI2C(uint32_t timeout_ms) {
  if (i2cMutex == NULL) {
    Serial.println("ERROR: I2C mutex not initialized!");
    return false;
  }

  TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
  if (xSemaphoreTake(i2cMutex, timeout_ticks) == pdTRUE) {
    return true;
  } else {
    Serial.println("WARNING: I2C mutex lock timeout!");
    return false;
  }
}

void unlockI2C() {
  if (i2cMutex == NULL) {
    Serial.println("ERROR: I2C mutex not initialized!");
    return;
  }

  xSemaphoreGive(i2cMutex);
}

// ============================================
// SCOPED LOCK IMPLEMENTATION
// ============================================

I2CScopedLock::I2CScopedLock(uint32_t timeout_ms) {
  locked = lockI2C(timeout_ms);
  if (!locked) {
    Serial.println("ERROR: Failed to acquire I2C lock!");
  }
}

I2CScopedLock::~I2CScopedLock() {
  if (locked) {
    unlockI2C();
  }
}
