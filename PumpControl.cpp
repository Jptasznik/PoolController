#define LOG_MODULE "PUMP"

#include "PumpControl.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"

// ============================================
// INITIALIZATION
// ============================================

void initPumpControl() {
#if ENABLE_PUMP
  LOG_INFO("Initializing pump control...");

  pinMode(PUMP_ENABLE_PIN, OUTPUT);
  pinMode(PUMP_SPEED_PIN, OUTPUT);

  digitalWrite(PUMP_ENABLE_PIN, LOW);
  digitalWrite(PUMP_SPEED_PIN, LOW);

  LOG_INFO("Pump control initialized");
#else
  LOG_WARN("Pump control DISABLED (ENABLE_PUMP not defined)");
#endif
}

// ============================================
// PUMP STATE APPLICATION
// ============================================

void applyPumpState() {
#if ENABLE_PUMP
  StateManager& stateMgr = StateManager::getInstance();
  PumpState pumpState = stateMgr.getPumpState();

  switch (pumpState) {
    case PUMP_OFF:
      digitalWrite(PUMP_ENABLE_PIN, LOW);
      digitalWrite(PUMP_SPEED_PIN, LOW);
      break;

    case PUMP_LOW:
      digitalWrite(PUMP_ENABLE_PIN, HIGH);
      digitalWrite(PUMP_SPEED_PIN, LOW);
      break;

    case PUMP_HIGH:
      digitalWrite(PUMP_ENABLE_PIN, HIGH);
      digitalWrite(PUMP_SPEED_PIN, HIGH);
      break;
  }
#else
  // Pump disabled - do nothing
#endif
}

// ============================================
// RUNTIME CHECKING
// ============================================

bool isRuntimeLimitExceeded() {
  StateManager& stateMgr = StateManager::getInstance();
  float maxRuntimeHours = stateMgr.getMaxRuntimeHours();

  if (maxRuntimeHours <= 0) {
    return false;  // No limit
  }

  float currentRuntime = getPumpRuntimeHours();
  return (currentRuntime >= maxRuntimeHours);
}
