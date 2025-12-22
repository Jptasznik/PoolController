#define LOG_MODULE "PUMP"

#include "PumpControl.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"

// ============================================
// INITIALIZATION
// ============================================

void initPumpControl() {
#ifdef ENABLE_PUMP
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
#ifdef ENABLE_PUMP
  switch (state.pumpState) {
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
  if (state.maxRuntimeHours <= 0) {
    return false;  // No limit
  }
  
  float currentRuntime = getPumpRuntimeHours();
  return (currentRuntime >= state.maxRuntimeHours);
}
