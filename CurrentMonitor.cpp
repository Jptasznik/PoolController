#define LOG_MODULE "CURRENT"

#include "CurrentMonitor.h"
#include "Logger.h"
#include "Config.h"
#include "StateManager.h"
#include "I2CMutex.h"
#include "SettingsManager.h"
#include <Adafruit_ADS1X15.h>
#include <Wire.h>

// ============================================
// ADS1115 INSTANCE
// ============================================

#ifdef ENABLE_CURRENT_MONITOR
Adafruit_ADS1115 ads;
#endif

// ============================================
// INITIALIZATION
// ============================================

void initCurrentMonitor() {
#ifdef ENABLE_CURRENT_MONITOR
  LOG_INFO("Initializing current monitor...");

  StateManager& stateMgr = StateManager::getInstance();

  // Lock I2C bus for ADS1115 initialization
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
#ifndef NO_HARDWARE
    LOG_ERROR("Failed to lock I2C for ADS1115 init!");
#endif
    stateMgr.setCurrentSensorValid(false);
    return;
  }

  // Initialize ADS1115
  if (!ads.begin(ADS1115_ADDRESS)) {
#ifndef NO_HARDWARE
    LOG_ERROR("Failed to initialize ADS1115!");
    LOG_ERROR("Check I2C connections and address");
#endif
    stateMgr.setCurrentSensorValid(false);
    return;
  }

  // Configure ADS1115
  // Gain: ±4.096V (allows reading 0-3.3V signals)
  // Data rate: 860 SPS (samples per second) for fast AC measurements
  ads.setGain(GAIN_ONE);  // ±4.096V
  ads.setDataRate(RATE_ADS1115_860SPS);

  LOG_INFO("ADS1115 initialized successfully");
#ifndef NO_HARDWARE
  LOG_INFOF("  I2C Address: 0x%02X", ADS1115_ADDRESS);
  LOG_INFO("  Gain: ±4.096V");
  LOG_INFO("  Data Rate: 860 SPS");
#endif

  stateMgr.setCurrentSensorValid(true);
  stateMgr.setPumpCurrent(0.0);

  // Note: calibration offset is loaded from flash in loadAllSettings()
#ifndef NO_HARDWARE
  LOG_INFOF("Using saved current calibration offset: %.3f A", stateMgr.getCurrentOffset());
#endif
#else
  StateManager& stateMgr = StateManager::getInstance();
  LOG_WARN("Current monitor DISABLED (ENABLE_CURRENT_MONITOR not defined)");
  stateMgr.setCurrentSensorValid(false);
  stateMgr.setPumpCurrent(0.0);
#endif
}

// ============================================
// CURRENT READING
// ============================================

void readPumpCurrent() {
#ifdef ENABLE_CURRENT_MONITOR
  StateManager& stateMgr = StateManager::getInstance();

  bool currentSensorValid = stateMgr.isCurrentSensorValid();
  float currentOffset = stateMgr.getCurrentOffset();

  if (!currentSensorValid) {
    stateMgr.setPumpCurrent(0.0);
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
    stateMgr.setPumpCurrent(0.0);
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

  // Update state using StateManager API
  stateMgr.setPumpCurrent(rmsCurrent);

  // Debug output (optional - can be disabled for production)
#ifndef NO_HARDWARE
  static unsigned long lastDebugPrint = 0;
  if (millis() - lastDebugPrint >= 5000) {  // Print every 5 seconds
    lastDebugPrint = millis();
    LOG_DEBUGF("Pump Current: %.2f A (RMS voltage: %.3f V)", rmsCurrent, rmsVoltage);
  }
#endif
#else
  // Current monitor disabled - do nothing
#endif
}

// ============================================
// CURRENT DETECTION
// ============================================

bool isPumpCurrentDetected() {
  // Consider pump running if current is above threshold
  StateManager& stateMgr = StateManager::getInstance();
  return stateMgr.getPumpCurrent() >= CURRENT_THRESHOLD_PUMP_RUNNING;
}

// ============================================
// PUMP FAILURE DETECTION
// ============================================

bool isPumpFailureDetected() {
  // Pump failure: pump should be running but no current detected
  // Only check if sensor is valid
  StateManager& stateMgr = StateManager::getInstance();

  bool currentSensorValid = stateMgr.isCurrentSensorValid();
  PumpState pumpState = stateMgr.getPumpState();

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
  StateManager& stateMgr = StateManager::getInstance();

  if (!stateMgr.isCurrentSensorValid()) {
    return false;
  }

  return stateMgr.getPumpCurrent() >= CURRENT_THRESHOLD_OVERCURRENT;
}

// ============================================
// CALIBRATION
// ============================================

void calibrateCurrentSensor() {
#ifdef ENABLE_CURRENT_MONITOR
  StateManager& stateMgr = StateManager::getInstance();

  bool currentSensorValid = stateMgr.isCurrentSensorValid();

  if (!currentSensorValid) {
#ifndef NO_HARDWARE
    LOG_ERROR("Cannot calibrate - sensor not initialized");
#endif
    return;
  }

  LOG_INFO("Calibrating current sensor...");
  LOG_INFO("Ensure pump is OFF during calibration");

  // Take multiple readings to measure offset
  const int CAL_SAMPLES = 50;
  float sumCurrent = 0.0;

  {
    I2CScopedLock i2cLock;
    if (!i2cLock.isLocked()) {
#ifndef NO_HARDWARE
      LOG_ERROR("Failed to lock I2C for calibration");
#endif
      return;
    }

    for (int i = 0; i < CAL_SAMPLES; i++) {
      // Read voltage from ADS1115
      int16_t adc = ads.readADC_SingleEnded(CURRENT_SENSOR_CHANNEL);
      float voltage = adc * 0.000125;
      float acVoltage = voltage - CURRENT_SENSOR_BIAS_VOLTAGE;
      float current = (acVoltage / CURRENT_SENSOR_BURDEN_RESISTOR) * CURRENT_SENSOR_CT_RATIO;

      sumCurrent += abs(current);  // Use absolute value for offset
      delay(20);  // BLOCKING: Short delay between ADC samples for accuracy (acceptable for calibration only)
    }
  }  // Release I2C lock

  float calibratedOffset = sumCurrent / CAL_SAMPLES;

  // Update offset using StateManager API
  stateMgr.setCurrentOffset(calibratedOffset);

#ifndef NO_HARDWARE
  LOG_INFOF("Calibration complete - Offset: %.3f A", calibratedOffset);
#endif

  // Save calibration to flash
  saveCurrentCalibration(calibratedOffset);
#else
  LOG_WARN("Cannot calibrate - current monitor disabled");
#endif
}
