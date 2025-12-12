#ifndef CREDENTIALS_MANAGER_H
#define CREDENTIALS_MANAGER_H

#include <Arduino.h>

// ============================================
// CREDENTIALS MANAGER
// ============================================
// Manages secure storage of credentials in ESP32 flash memory
// using the Preferences library. Credentials are stored encrypted
// and persist across reboots.
//
// This module provides secure storage for:
// - Blynk authentication token
// - Sinric Pro API credentials (app key, secret, device IDs)
//
// Credentials are stored separately from source code and not
// committed to version control.

// Credential string length limits
#define BLYNK_AUTH_MAX_LENGTH 64
#define SINRIC_APP_KEY_MAX_LENGTH 64
#define SINRIC_APP_SECRET_MAX_LENGTH 128
#define SINRIC_DEVICE_ID_MAX_LENGTH 32

// ============================================
// INITIALIZATION
// ============================================

// Initialize credentials manager and load credentials from flash
void initCredentials();

// ============================================
// BLYNK CREDENTIALS
// ============================================

// Get Blynk authentication token (returns pointer to internal buffer)
const char* getBlynkAuthToken();

// Set Blynk authentication token and save to flash
// Returns true if saved successfully
bool setBlynkAuthToken(const char* token);

// Check if Blynk credentials are configured
bool hasBlynkCredentials();

// ============================================
// SINRIC PRO CREDENTIALS
// ============================================

// Get Sinric Pro app key (returns pointer to internal buffer)
const char* getSinricAppKey();

// Get Sinric Pro app secret (returns pointer to internal buffer)
const char* getSinricAppSecret();

// Get Sinric Pro thermostat device ID (returns pointer to internal buffer)
const char* getSinricThermostatId();

// Get Sinric Pro pump device ID (returns pointer to internal buffer)
const char* getSinricPumpId();

// Set Sinric Pro credentials and save to flash
// Returns true if saved successfully
bool setSinricCredentials(
  const char* appKey,
  const char* appSecret,
  const char* thermostatId,
  const char* pumpId
);

// Check if Sinric Pro credentials are configured
bool hasSinricCredentials();

// ============================================
// CREDENTIAL MANAGEMENT
// ============================================

// Clear all stored credentials (returns to unconfigured state)
void clearAllCredentials();

// Check if any credentials are configured
bool hasAnyCredentials();

// Print credential status (for debugging - does not show actual values)
void printCredentialStatus();

#endif // CREDENTIALS_MANAGER_H
