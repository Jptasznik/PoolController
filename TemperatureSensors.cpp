#include "TemperatureSensors.h"
#include "Config.h"
#include "StateManager.h"
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
  // Request temperatures from all sensors
  inletSensor.requestTemperatures();
  heaterSensor.requestTemperatures();
  ambientSensor.requestTemperatures();
  
  // Read inlet temperature
  float inletC = inletSensor.getTempCByIndex(0);
  if (inletC != DEVICE_DISCONNECTED_C) {
    state.inletTemp = celsiusToFahrenheit(inletC);
    state.inletTempValid = isValidTemperature(state.inletTemp);
    
    if (!state.inletTempValid) {
      Serial.print("WARNING: Inlet temp out of range: ");
      Serial.println(state.inletTemp, 1);
    }
  } else {
    state.inletTemp = 0.0;
    state.inletTempValid = false;
    Serial.println("WARNING: Inlet sensor disconnected");
  }
  
  // Read heater temperature
  float heaterC = heaterSensor.getTempCByIndex(0);
  if (heaterC != DEVICE_DISCONNECTED_C) {
    state.heaterTemp = celsiusToFahrenheit(heaterC);
    state.heaterTempValid = isValidTemperature(state.heaterTemp);
    
    if (!state.heaterTempValid) {
      Serial.print("WARNING: Heater temp out of range: ");
      Serial.println(state.heaterTemp, 1);
    }
  } else {
    state.heaterTemp = 0.0;
    state.heaterTempValid = false;
    Serial.println("WARNING: Heater sensor disconnected");
  }
  
  // Read ambient temperature (3rd sensor)
  float ambientC = ambientSensor.getTempCByIndex(0);
  if (ambientC != DEVICE_DISCONNECTED_C) {
    state.ambientTemp = celsiusToFahrenheit(ambientC);
    state.ambientTempValid = isValidTemperature(state.ambientTemp);
    
    if (!state.ambientTempValid) {
      Serial.print("WARNING: Ambient temp out of range: ");
      Serial.println(state.ambientTemp, 1);
    }
  } else {
    state.ambientTemp = 0.0;
    state.ambientTempValid = false;
    Serial.println("WARNING: Ambient sensor disconnected");
  }
  
  // Log valid temperatures
  if (state.inletTempValid) {
    Serial.print("Inlet: ");
    Serial.print(state.inletTemp, 1);
    Serial.print("°F  ");
  }
  
  if (state.heaterTempValid) {
    Serial.print("Heater: ");
    Serial.print(state.heaterTemp, 1);
    Serial.print("°F  ");
  }
  
  if (state.ambientTempValid) {
    Serial.print("Ambient: ");
    Serial.print(state.ambientTemp, 1);
    Serial.println("°F");
  } else if (state.inletTempValid || state.heaterTempValid) {
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
  return (temp > INVALID_TEMP_LOW && temp < INVALID_TEMP_HIGH);
}
