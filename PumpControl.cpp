#define LOG_MODULE "PUMP"

#include "PumpControl.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"

// ============================================
// INITIALIZATION
// ============================================

void initPumpControl() {
  LOG_INFO("Initializing pump control...");

  pinMode(PUMP_ENABLE_PIN, OUTPUT);
  pinMode(PUMP_SPEED_PIN, OUTPUT);

  digitalWrite(PUMP_ENABLE_PIN, LOW);
  digitalWrite(PUMP_SPEED_PIN, LOW);

  LOG_INFO("Pump control initialized");
}

// ============================================
// PUMP STATE APPLICATION
// ============================================

void applyPumpState() {
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
