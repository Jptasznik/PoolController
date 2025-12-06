#ifndef TEMPERATURE_SENSORS_H
#define TEMPERATURE_SENSORS_H

#include <Arduino.h>

// ============================================
// TEMPERATURE SENSOR MANAGEMENT
// ============================================

// Initialize temperature sensors
void initTemperatureSensors();

// Read all temperature sensors
void readTemperatures();

// Convert Celsius to Fahrenheit
float celsiusToFahrenheit(float celsius);

// Validate temperature reading
bool isValidTemperature(float temp);

#endif // TEMPERATURE_SENSORS_H
