#include "CurrentMonitor.h"
#include "Config.h"
#include "StateManager.h"
#include "StateMutex.h"
#include "I2CMutex.h"
#include "SettingsManager.h"
#include <Adafruit_ADS1X15.h>
#include <Wire.h>

// ============================================
// ADS1115 INSTANCE
// ============================================

Adafruit_ADS1115 ads;

// ============================================
// INITIALIZATION
// ============================================

void initCurrentMonitor() {
  Serial.println("Initializing current monitor...");

  // Lock I2C bus for ADS1115 initialization
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
    Serial.println("ERROR: Failed to lock I2C for ADS1115 init!");
    state.currentSensorValid = false;
    return;
  }

  // Initialize ADS1115
  if (!ads.begin(ADS1115_ADDRESS)) {
    Serial.println("ERROR: Failed to initialize ADS1115!");
    Serial.println("Check I2C connections and address");
    state.currentSensorValid = false;
    return;
  }

  // Configure ADS1115
  // Gain: ±4.096V (allows reading 0-3.3V signals)
  // Data rate: 860 SPS (samples per second) for fast AC measurements
  ads.setGain(GAIN_ONE);  // ±4.096V
  ads.setDataRate(RATE_ADS1115_860SPS);

  Serial.println("ADS1115 initialized successfully");
  Serial.print("  I2C Address: 0x");
  Serial.println(ADS1115_ADDRESS, HEX);
  Serial.println("  Gain: ±4.096V");
  Serial.println("  Data Rate: 860 SPS");

  state.currentSensorValid = true;
  state.pumpCurrent = 0.0;

  // Note: calibration offset is loaded from flash in loadAllSettings()
  Serial.print("Using saved current calibration offset: ");
  Serial.print(state.currentOffset, 3);
  Serial.println(" A");
}

// ============================================
// CURRENT READING
// ============================================

void readPumpCurrent() {
  bool currentSensorValid;
  float currentOffset;

  // Read state values with lock
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      return;
    }
    currentSensorValid = state.currentSensorValid;
    currentOffset = state.currentOffset;
  }

  if (!currentSensorValid) {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      state.pumpCurrent = 0.0;
    }
    return;
  }

  // Sample AC waveform to calculate RMS current
  // For 60Hz AC, one cycle is ~16.67ms
  // Sample for multiple cycles to get accurate RMS

  const int NUM_SAMPLES = 100;  // Number of samples
  const int SAMPLE_DELAY_US = 200;  // 200us between samples = 20ms total (>1 AC cycle)

  float sumSquares = 0.0;
  int validSamples = 0;

  // Lock I2C for entire sampling period
  {
    I2CScopedLock i2cLock;
    if (!i2cLock.isLocked()) {
      return;  // Can't access I2C, skip this reading
    }

    for (int i = 0; i < NUM_SAMPLES; i++) {
      // Read voltage from ADS1115 (Channel 0)
      int16_t adc = ads.readADC_SingleEnded(CURRENT_SENSOR_CHANNEL);

      // Convert ADC reading to voltage
      // ADS1115: 16-bit, ±4.096V range
      // Resolution: 4.096V / 32768 = 0.000125 V/bit
      float voltage = adc * 0.000125;

      // Subtract bias voltage (should be centered around 1.65V)
      // This gives us the AC component
      float acVoltage = voltage - CURRENT_SENSOR_BIAS_VOLTAGE;

      // Accumulate squared values for RMS calculation
      sumSquares += acVoltage * acVoltage;
      validSamples++;

      // Small delay between samples
      delayMicroseconds(SAMPLE_DELAY_US);
    }
  }  // Release I2C lock

  if (validSamples == 0) {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      state.pumpCurrent = 0.0;
    }
    return;
  }

  // Calculate RMS voltage
  float rmsVoltage = sqrt(sumSquares / validSamples);

  // Convert RMS voltage to RMS current
  // Current (A) = RMS Voltage (V) / Burden Resistor (Ω) × CT Ratio
  // For SCT-013-030: 1000:1 ratio, 30A max
  // With burden resistor (configured in Config.h)
  float rmsCurrent = (rmsVoltage / CURRENT_SENSOR_BURDEN_RESISTOR) * CURRENT_SENSOR_CT_RATIO;

  // Apply calibration offset
  rmsCurrent -= currentOffset;

  // Clamp to valid range
  if (rmsCurrent < 0.0) {
    rmsCurrent = 0.0;
  }
  if (rmsCurrent > CURRENT_SENSOR_MAX_CURRENT) {
    rmsCurrent = CURRENT_SENSOR_MAX_CURRENT;
  }

  // Update state with lock
  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      state.pumpCurrent = rmsCurrent;
    }
  }

  // Debug output (optional - can be disabled for production)
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint >= 5000) {  // Print every 5 seconds
    lastDebugPrint = millis();
    Serial.print("Pump Current: ");
    Serial.print(rmsCurrent, 2);
    Serial.print(" A (RMS voltage: ");
    Serial.print(rmsVoltage, 3);
    Serial.println(" V)");
  }
}

// ============================================
// CURRENT DETECTION
// ============================================

bool isPumpCurrentDetected() {
  // Consider pump running if current is above threshold
  StateScopedLock stateLock;
  if (!stateLock.isLocked()) {
    return false;
  }
  return state.pumpCurrent >= CURRENT_THRESHOLD_PUMP_RUNNING;
}

// ============================================
// PUMP FAILURE DETECTION
// ============================================

bool isPumpFailureDetected() {
  // Pump failure: pump should be running but no current detected
  // Only check if sensor is valid
  bool currentSensorValid;
  PumpState pumpState;

  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      return false;
    }
    currentSensorValid = state.currentSensorValid;
    pumpState = state.pumpState;
  }

  if (!currentSensorValid) {
    return false;
  }

  // If pump state is not OFF, but current is below threshold
  if (pumpState != PUMP_OFF && !isPumpCurrentDetected()) {
    // Additional check: wait for stabilization
    // Don't report failure immediately after pump state change
    static unsigned long lastPumpStateChange = 0;
    static PumpState lastPumpState = PUMP_OFF;

    if (pumpState != lastPumpState) {
      lastPumpStateChange = millis();
      lastPumpState = pumpState;
      return false;  // Don't report failure during transition
    }

    // Wait for pump to stabilize (5 seconds)
    if (millis() - lastPumpStateChange >= CURRENT_FAILURE_DETECTION_DELAY) {
      return true;  // Pump should be running but no current detected
    }
  }

  return false;
}

// ============================================
// OVERCURRENT DETECTION
// ============================================

bool isOvercurrentDetected() {
  // Check if current exceeds maximum threshold
  StateScopedLock stateLock;
  if (!stateLock.isLocked()) {
    return false;
  }

  if (!state.currentSensorValid) {
    return false;
  }

  return state.pumpCurrent >= CURRENT_THRESHOLD_OVERCURRENT;
}

// ============================================
// CALIBRATION
// ============================================

void calibrateCurrentSensor() {
  bool currentSensorValid;
  {
    StateScopedLock stateLock;
    if (!stateLock.isLocked()) {
      return;
    }
    currentSensorValid = state.currentSensorValid;
  }

  if (!currentSensorValid) {
    Serial.println("Cannot calibrate - sensor not initialized");
    return;
  }

  Serial.println("Calibrating current sensor...");
  Serial.println("Ensure pump is OFF during calibration");

  // Take multiple readings to measure offset
  const int CAL_SAMPLES = 50;
  float sumCurrent = 0.0;

  {
    I2CScopedLock i2cLock;
    if (!i2cLock.isLocked()) {
      Serial.println("Failed to lock I2C for calibration");
      return;
    }

    for (int i = 0; i < CAL_SAMPLES; i++) {
      // Read voltage from ADS1115
      int16_t adc = ads.readADC_SingleEnded(CURRENT_SENSOR_CHANNEL);
      float voltage = adc * 0.000125;
      float acVoltage = voltage - CURRENT_SENSOR_BIAS_VOLTAGE;
      float current = (acVoltage / CURRENT_SENSOR_BURDEN_RESISTOR) * CURRENT_SENSOR_CT_RATIO;

      sumCurrent += abs(current);  // Use absolute value for offset
      delay(20);  // Short delay between samples
    }
  }  // Release I2C lock

  float calibratedOffset = sumCurrent / CAL_SAMPLES;

  {
    StateScopedLock stateLock;
    if (stateLock.isLocked()) {
      state.currentOffset = calibratedOffset;
    }
  }

  Serial.print("Calibration complete - Offset: ");
  Serial.print(calibratedOffset, 3);
  Serial.println(" A");

  // Save calibration to flash
  saveCurrentCalibration(calibratedOffset);
}
