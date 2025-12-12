#include "TemperatureSensors.h"
#include "Config.h"
#include "StateManager.h"
#include "StateMutex.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// OneWire instances for each sensor
OneWire inletWire(TEMP_INLET_PIN);
OneWire heaterWire(TEMP_HEATER_PIN);
OneWire ambientWire(TEMP_AMBIENT_PIN);

// DallasTemperature instances
DallasTemperature inletSensor(&inletWire);
DallasTemperature heaterSensor(&heaterWire);
DallasTemperature ambientSensor(&ambientWire);

// ============================================
// INITIALIZATION
// ============================================

void initTemperatureSensors() {
  Serial.println("Initializing temperature sensors...");
  
  inletSensor.begin();
  heaterSensor.begin();
  ambientSensor.begin();
  
  Serial.print("Inlet sensor devices: ");
  Serial.println(inletSensor.getDeviceCount());
  
  Serial.print("Heater sensor devices: ");
  Serial.println(heaterSensor.getDeviceCount());
  
  Serial.print("Ambient sensor devices: ");
  Serial.println(ambientSensor.getDeviceCount());
  
  // Set resolution to 12-bit (0.0625°C precision)
  inletSensor.setResolution(12);
  heaterSensor.setResolution(12);
  ambientSensor.setResolution(12);
  
  Serial.println("Temperature sensors initialized");
}

// ============================================
// TEMPERATURE READING
// ============================================

void readTemperatures() {
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

    if (!inletValid) {
      Serial.print("WARNING: Inlet temp out of range: ");
      Serial.println(inletTemp, 1);
    }
  } else {
    Serial.println("WARNING: Inlet sensor disconnected");
  }

  // Read heater temperature
  float heaterC = heaterSensor.getTempCByIndex(0);
  if (heaterC != DEVICE_DISCONNECTED_C) {
    heaterTemp = celsiusToFahrenheit(heaterC);
    heaterValid = isValidTemperature(heaterTemp);

    if (!heaterValid) {
      Serial.print("WARNING: Heater temp out of range: ");
      Serial.println(heaterTemp, 1);
    }
  } else {
    Serial.println("WARNING: Heater sensor disconnected");
  }

  // Read ambient temperature (3rd sensor)
  float ambientC = ambientSensor.getTempCByIndex(0);
  if (ambientC != DEVICE_DISCONNECTED_C) {
    ambientTemp = celsiusToFahrenheit(ambientC);
    ambientValid = isValidTemperature(ambientTemp);

    if (!ambientValid) {
      Serial.print("WARNING: Ambient temp out of range: ");
      Serial.println(ambientTemp, 1);
    }
  } else {
    Serial.println("WARNING: Ambient sensor disconnected");
  }

  // Update state with mutex protection
  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      state.inletTemp = inletTemp;
      state.inletTempValid = inletValid;
      state.heaterTemp = heaterTemp;
      state.heaterTempValid = heaterValid;
      state.ambientTemp = ambientTemp;
      state.ambientTempValid = ambientValid;
    } else {
      Serial.println("ERROR: Failed to update temperature state (mutex timeout)");
      return;
    }
  }

  // Log valid temperatures (using local copies)
  if (inletValid) {
    Serial.print("Inlet: ");
    Serial.print(inletTemp, 1);
    Serial.print("°F  ");
  }

  if (heaterValid) {
    Serial.print("Heater: ");
    Serial.print(heaterTemp, 1);
    Serial.print("°F  ");
  }

  if (ambientValid) {
    Serial.print("Ambient: ");
    Serial.print(ambientTemp, 1);
    Serial.println("°F");
  } else if (inletValid || heaterValid) {
    Serial.println();
  }
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
