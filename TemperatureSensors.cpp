#define LOG_MODULE "TEMP"

#include "TemperatureSensors.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#ifdef ENABLE_TEMPERATURE_SENSORS
// OneWire instances for each sensor
OneWire inletWire(TEMP_INLET_PIN);
OneWire heaterWire(TEMP_HEATER_PIN);
OneWire ambientWire(TEMP_AMBIENT_PIN);

// DallasTemperature instances
DallasTemperature inletSensor(&inletWire);
DallasTemperature heaterSensor(&heaterWire);
DallasTemperature ambientSensor(&ambientWire);
#endif

// ============================================
// INITIALIZATION
// ============================================

void initTemperatureSensors() {
#ifdef ENABLE_TEMPERATURE_SENSORS
  LOG_INFO("Initializing temperature sensors...");

  inletSensor.begin();
  heaterSensor.begin();
  ambientSensor.begin();

#ifndef NO_HARDWARE
  LOG_INFOF("Inlet sensor devices: %d", inletSensor.getDeviceCount());
  LOG_INFOF("Heater sensor devices: %d", heaterSensor.getDeviceCount());
  LOG_INFOF("Ambient sensor devices: %d", ambientSensor.getDeviceCount());
#endif

  // Set resolution to 12-bit (0.0625°C precision)
  inletSensor.setResolution(12);
  heaterSensor.setResolution(12);
  ambientSensor.setResolution(12);

  LOG_INFO("Temperature sensors initialized");
#else
  LOG_WARN("Temperature sensors DISABLED (ENABLE_TEMPERATURE_SENSORS not defined)");
#endif
}

// ============================================
// TEMPERATURE READING
// ============================================

void readTemperatures() {
#ifdef ENABLE_TEMPERATURE_SENSORS
  // Request temperatures from all sensors (no mutex needed - hardware access)
  inletSensor.requestTemperatures();
  heaterSensor.requestTemperatures();
  ambientSensor.requestTemperatures();

  // Read and convert temperatures to local variables
  float inletTemp = 0.0, heaterTemp = 0.0, ambientTemp = 0.0;
  bool inletValid = false, heaterValid = false, ambientValid = false;

  // Read inlet temperature
  float inletC = inletSensor.getTempCByIndex(0);
  if (inletC != DEVICE_DISCONNECTED_C) {
    inletTemp = celsiusToFahrenheit(inletC);
    inletValid = isValidTemperature(inletTemp);

#ifndef NO_HARDWARE
    if (!inletValid) {
      LOG_WARNF("Inlet temp out of range: %.1f", inletTemp);
    }
  } else {
    LOG_WARN("Inlet sensor disconnected");
#endif
  }

  // Read heater temperature
  float heaterC = heaterSensor.getTempCByIndex(0);
  if (heaterC != DEVICE_DISCONNECTED_C) {
    heaterTemp = celsiusToFahrenheit(heaterC);
    heaterValid = isValidTemperature(heaterTemp);

#ifndef NO_HARDWARE
    if (!heaterValid) {
      LOG_WARNF("Heater temp out of range: %.1f", heaterTemp);
    }
  } else {
    LOG_WARN("Heater sensor disconnected");
#endif
  }

  // Read ambient temperature (3rd sensor)
  float ambientC = ambientSensor.getTempCByIndex(0);
  if (ambientC != DEVICE_DISCONNECTED_C) {
    ambientTemp = celsiusToFahrenheit(ambientC);
    ambientValid = isValidTemperature(ambientTemp);

#ifndef NO_HARDWARE
    if (!ambientValid) {
      LOG_WARNF("Ambient temp out of range: %.1f", ambientTemp);
    }
  } else {
    LOG_WARN("Ambient sensor disconnected");
#endif
  }

  // Check for critical sensor failure (both primary sensors failed)
  StateManager& stateMgr = StateManager::getInstance();

  if (!inletValid && !heaterValid) {
#ifndef NO_HARDWARE
    LOG_ERROR("CRITICAL: Both inlet and heater sensors failed!");
#endif

    // Update state first
    stateMgr.setInletTemp(inletTemp);
    stateMgr.setInletTempValid(inletValid);
    stateMgr.setHeaterTemp(heaterTemp);
    stateMgr.setHeaterTempValid(heaterValid);
    stateMgr.setAmbientTemp(ambientTemp);
    stateMgr.setAmbientTempValid(ambientValid);

#ifndef NO_HARDWARE
    // Enter failsafe mode if not already in it
    if (!stateMgr.isInFailsafeMode()) {
      enterFailsafeMode("CRITICAL_SENSOR_FAILURE");
    }
    return;
#else
    return;  // Skip failsafe in NO_HARDWARE mode
#endif
  }

  // Check for single critical sensor failure
  if (!inletValid) {
#ifndef NO_HARDWARE
    LOG_ERROR("CRITICAL: Inlet temperature sensor failure!");
    // Enter failsafe if not already in it
    if (!stateMgr.isInFailsafeMode()) {
      enterFailsafeMode("INLET_SENSOR_FAILURE");
    }
#endif
  }

  // Update state
  stateMgr.setInletTemp(inletTemp);
  stateMgr.setInletTempValid(inletValid);
  stateMgr.setHeaterTemp(heaterTemp);
  stateMgr.setHeaterTempValid(heaterValid);
  stateMgr.setAmbientTemp(ambientTemp);
  stateMgr.setAmbientTempValid(ambientValid);

  // Log valid temperatures (using local copies)
  // Build a single formatted string for all valid temperatures
#ifndef NO_HARDWARE
  char tempMsg[80] = "";
  bool hasData = false;

  if (inletValid) {
    sprintf(tempMsg + strlen(tempMsg), "Inlet: %.1f°F  ", inletTemp);
    hasData = true;
  }

  if (heaterValid) {
    sprintf(tempMsg + strlen(tempMsg), "Heater: %.1f°F  ", heaterTemp);
    hasData = true;
  }

  if (ambientValid) {
    sprintf(tempMsg + strlen(tempMsg), "Ambient: %.1f°F", ambientTemp);
    hasData = true;
  }

  if (hasData) {
    LOG_DEBUG(tempMsg);
  }
#endif
#else
  // Temperature sensors disabled - do nothing
#endif
}

// ============================================
// HELPER FUNCTIONS
// ============================================

float celsiusToFahrenheit(float celsius) {
  return (celsius * 9.0 / 5.0) + 32.0;
}

bool isValidTemperature(float temp) {
  return (temp >= INVALID_TEMP_LOW && temp <= INVALID_TEMP_HIGH);
}
