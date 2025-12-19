#define LOG_MODULE "I2C"

#include "I2CMutex.h"
#include "Logger.h"
#include "Config.h"
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
      LOG_ERROR("Failed to create I2C mutex!");
    } else {
      LOG_INFO("I2C mutex created successfully");
    }
  }
}

bool lockI2C(uint32_t timeout_ms) {
  if (i2cMutex == NULL) {
#ifndef NO_HARDWARE
    LOG_ERROR("I2C mutex not initialized!");
#endif
    return false;
  }

  TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);
  if (xSemaphoreTake(i2cMutex, timeout_ticks) == pdTRUE) {
    return true;
  } else {
#ifndef NO_HARDWARE
    LOG_WARN("I2C mutex lock timeout!");
#endif
    return false;
  }
}

void unlockI2C() {
  if (i2cMutex == NULL) {
#ifndef NO_HARDWARE
    LOG_ERROR("I2C mutex not initialized!");
#endif
    return;
  }

  xSemaphoreGive(i2cMutex);
}

// ============================================
// SCOPED LOCK IMPLEMENTATION
// ============================================

I2CScopedLock::I2CScopedLock(uint32_t timeout_ms) {
  locked = lockI2C(timeout_ms);
#ifndef NO_HARDWARE
  if (!locked) {
    LOG_ERROR("Failed to acquire I2C lock!");
  }
#endif
}

I2CScopedLock::~I2CScopedLock() {
  if (locked) {
    unlockI2C();
  }
}
