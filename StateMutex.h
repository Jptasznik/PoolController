#ifndef STATE_MUTEX_H
#define STATE_MUTEX_H

#include <Arduino.h>

// ============================================
// STATE MUTEX FOR THREAD-SAFE ACCESS
// ============================================
// Provides mutex protection for SystemState
// to ensure thread-safe access from multiple tasks.
//
// Usage:
//   StateScopedLock lock;
//   // Access state safely within this scope
//   float temp = state.inletTemp;
// ============================================

// Initialize state mutex (call once in setup)
void initStateMutex();

// Lock state for exclusive access
// Returns true if lock acquired, false if timeout
bool lockState(uint32_t timeout_ms = 100);

// Unlock state
void unlockState();

// ============================================
// RAII-STYLE SCOPED LOCK
// ============================================

class StateScopedLock {
public:
  StateScopedLock(uint32_t timeout_ms = 100);
  ~StateScopedLock();

  bool isLocked() const { return locked; }

private:
  bool locked;
};

#endif // STATE_MUTEX_H
