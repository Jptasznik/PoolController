#ifndef I2C_MUTEX_H
#define I2C_MUTEX_H

#include <Arduino.h>
#include <Wire.h>

// ============================================
// I2C MUTEX FOR THREAD-SAFE ACCESS
// ============================================
// The I2C bus is shared by multiple devices:
// - LCD Display (0x27)
// - MCP23008 GPIO Expander (0x20)
// - ADS1115 ADC (0x48)
//
// This module provides mutex protection to ensure
// only one task accesses the I2C bus at a time.
// ============================================

// Initialize I2C mutex (call once in setup)
void initI2CMutex();

// Lock I2C bus for exclusive access
// Returns true if lock acquired, false if timeout
// timeout_ms: Maximum time to wait for lock (default 100ms)
bool lockI2C(uint32_t timeout_ms = 100);

// Unlock I2C bus
void unlockI2C();

// ============================================
// RAII-STYLE SCOPED LOCK
// ============================================
// Usage:
//   {
//     I2CScopedLock lock;  // Acquires lock
//     // ... I2C operations ...
//   }  // Releases lock automatically

class I2CScopedLock {
public:
  I2CScopedLock(uint32_t timeout_ms = 100);
  ~I2CScopedLock();

  bool isLocked() const { return locked; }

private:
  bool locked;
};

#endif // I2C_MUTEX_H
