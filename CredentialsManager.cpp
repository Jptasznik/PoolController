#define LOG_MODULE "CREDENTIALS"

#include "CredentialsManager.h"
#include "Logger.h"
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// ============================================
// INTERNAL STATE
// ============================================

static Preferences preferences;
static SemaphoreHandle_t credentialsMutex = NULL;

// NVS configuration
#define CREDENTIALS_NVS_NAMESPACE "credentials"     // NVS namespace for credentials
#define CREDENTIALS_MUTEX_TIMEOUT_MS 1000           // Timeout for acquiring credentials mutex (ms)

// Credential buffers (loaded from flash on init)
static char blynkAuthToken[BLYNK_AUTH_MAX_LENGTH] = "";
static char sinricAppKey[SINRIC_APP_KEY_MAX_LENGTH] = "";
static char sinricAppSecret[SINRIC_APP_SECRET_MAX_LENGTH] = "";
static char sinricThermostatId[SINRIC_DEVICE_ID_MAX_LENGTH] = "";
static char sinricPumpId[SINRIC_DEVICE_ID_MAX_LENGTH] = "";

// Flags to track if credentials are configured
static bool blynkConfigured = false;
static bool sinricConfigured = false;

// ============================================
// INITIALIZATION
// ============================================

void initCredentials() {
  LOG_INFO("========================================");
  LOG_INFO("Initializing Credentials Manager");
  LOG_INFO("========================================");

  // Create mutex for thread-safe credential access
  if (credentialsMutex == NULL) {
    credentialsMutex = xSemaphoreCreateMutex();
    if (credentialsMutex == NULL) {
      LOG_ERROR("Failed to create credentials mutex!");
      return;
    }
  }

  // Load credentials from flash
  if (xSemaphoreTake(credentialsMutex, pdMS_TO_TICKS(CREDENTIALS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    preferences.begin(CREDENTIALS_NVS_NAMESPACE, true);  // Read-only

    // Load Blynk credentials
    String blynkAuth = preferences.getString("blynkAuth", "");
    if (blynkAuth.length() > 0) {
      strncpy(blynkAuthToken, blynkAuth.c_str(), BLYNK_AUTH_MAX_LENGTH - 1);
      blynkAuthToken[BLYNK_AUTH_MAX_LENGTH - 1] = '\0';
      blynkConfigured = true;
      LOG_INFO("Blynk credentials loaded");
    } else {
      LOG_INFO("No Blynk credentials found");
    }

    // Load Sinric Pro credentials
    String appKey = preferences.getString("sinricKey", "");
    String appSecret = preferences.getString("sinricSecret", "");
    String thermostatId = preferences.getString("sinricThermo", "");
    String pumpId = preferences.getString("sinricPump", "");

    if (appKey.length() > 0 && appSecret.length() > 0) {
      strncpy(sinricAppKey, appKey.c_str(), SINRIC_APP_KEY_MAX_LENGTH - 1);
      sinricAppKey[SINRIC_APP_KEY_MAX_LENGTH - 1] = '\0';

      strncpy(sinricAppSecret, appSecret.c_str(), SINRIC_APP_SECRET_MAX_LENGTH - 1);
      sinricAppSecret[SINRIC_APP_SECRET_MAX_LENGTH - 1] = '\0';

      strncpy(sinricThermostatId, thermostatId.c_str(), SINRIC_DEVICE_ID_MAX_LENGTH - 1);
      sinricThermostatId[SINRIC_DEVICE_ID_MAX_LENGTH - 1] = '\0';

      strncpy(sinricPumpId, pumpId.c_str(), SINRIC_DEVICE_ID_MAX_LENGTH - 1);
      sinricPumpId[SINRIC_DEVICE_ID_MAX_LENGTH - 1] = '\0';

      sinricConfigured = true;
      LOG_INFO("Sinric Pro credentials loaded");
    } else {
      LOG_INFO("No Sinric Pro credentials found");
    }

    preferences.end();
    xSemaphoreGive(credentialsMutex);
  } else {
    LOG_ERROR("Credentials mutex timeout during initialization");
  }

  printCredentialStatus();
  LOG_INFO("========================================\n");
}

// ============================================
// BLYNK CREDENTIALS
// ============================================

const char* getBlynkAuthToken() {
  return blynkAuthToken;
}

bool setBlynkAuthToken(const char* token) {
  if (token == NULL || strlen(token) == 0) {
    LOG_ERROR("Invalid Blynk token");
    return false;
  }

  if (xSemaphoreTake(credentialsMutex, pdMS_TO_TICKS(CREDENTIALS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    // Update internal buffer
    strncpy(blynkAuthToken, token, BLYNK_AUTH_MAX_LENGTH - 1);
    blynkAuthToken[BLYNK_AUTH_MAX_LENGTH - 1] = '\0';
    blynkConfigured = true;

    // Save to flash
    preferences.begin(CREDENTIALS_NVS_NAMESPACE, false);
    preferences.putString("blynkAuth", blynkAuthToken);
    preferences.end();

    xSemaphoreGive(credentialsMutex);
    LOG_INFO("Blynk credentials saved to flash");
    return true;
  } else {
    LOG_ERROR("Credentials mutex timeout in setBlynkAuthToken");
    return false;
  }
}

bool hasBlynkCredentials() {
  return blynkConfigured;
}

// ============================================
// SINRIC PRO CREDENTIALS
// ============================================

const char* getSinricAppKey() {
  return sinricAppKey;
}

const char* getSinricAppSecret() {
  return sinricAppSecret;
}

const char* getSinricThermostatId() {
  return sinricThermostatId;
}

const char* getSinricPumpId() {
  return sinricPumpId;
}

bool setSinricCredentials(
  const char* appKey,
  const char* appSecret,
  const char* thermostatId,
  const char* pumpId
) {
  if (appKey == NULL || appSecret == NULL ||
      strlen(appKey) == 0 || strlen(appSecret) == 0) {
    LOG_ERROR("Invalid Sinric credentials");
    return false;
  }

  if (xSemaphoreTake(credentialsMutex, pdMS_TO_TICKS(CREDENTIALS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    // Update internal buffers
    strncpy(sinricAppKey, appKey, SINRIC_APP_KEY_MAX_LENGTH - 1);
    sinricAppKey[SINRIC_APP_KEY_MAX_LENGTH - 1] = '\0';

    strncpy(sinricAppSecret, appSecret, SINRIC_APP_SECRET_MAX_LENGTH - 1);
    sinricAppSecret[SINRIC_APP_SECRET_MAX_LENGTH - 1] = '\0';

    strncpy(sinricThermostatId, thermostatId, SINRIC_DEVICE_ID_MAX_LENGTH - 1);
    sinricThermostatId[SINRIC_DEVICE_ID_MAX_LENGTH - 1] = '\0';

    strncpy(sinricPumpId, pumpId, SINRIC_DEVICE_ID_MAX_LENGTH - 1);
    sinricPumpId[SINRIC_DEVICE_ID_MAX_LENGTH - 1] = '\0';

    sinricConfigured = true;

    // Save to flash
    preferences.begin(CREDENTIALS_NVS_NAMESPACE, false);
    preferences.putString("sinricKey", sinricAppKey);
    preferences.putString("sinricSecret", sinricAppSecret);
    preferences.putString("sinricThermo", sinricThermostatId);
    preferences.putString("sinricPump", sinricPumpId);
    preferences.end();

    xSemaphoreGive(credentialsMutex);
    LOG_INFO("Sinric Pro credentials saved to flash");
    return true;
  } else {
    LOG_ERROR("Credentials mutex timeout in setSinricCredentials");
    return false;
  }
}

bool hasSinricCredentials() {
  return sinricConfigured;
}

// ============================================
// CREDENTIAL MANAGEMENT
// ============================================

void clearAllCredentials() {
  LOG_INFO("Clearing all stored credentials...");

  if (xSemaphoreTake(credentialsMutex, pdMS_TO_TICKS(CREDENTIALS_MUTEX_TIMEOUT_MS)) == pdTRUE) {
    // Clear internal buffers
    memset(blynkAuthToken, 0, sizeof(blynkAuthToken));
    memset(sinricAppKey, 0, sizeof(sinricAppKey));
    memset(sinricAppSecret, 0, sizeof(sinricAppSecret));
    memset(sinricThermostatId, 0, sizeof(sinricThermostatId));
    memset(sinricPumpId, 0, sizeof(sinricPumpId));

    blynkConfigured = false;
    sinricConfigured = false;

    // Clear flash storage
    preferences.begin(CREDENTIALS_NVS_NAMESPACE, false);
    preferences.clear();
    preferences.end();

    xSemaphoreGive(credentialsMutex);
    LOG_INFO("All credentials cleared");
  } else {
    LOG_ERROR("Credentials mutex timeout in clearAllCredentials");
  }
}

bool hasAnyCredentials() {
  return blynkConfigured || sinricConfigured;
}

void printCredentialStatus() {
  LOG_INFO("Credential Status:");
  LOG_INFOF("  Blynk: %s", blynkConfigured ? "Configured" : "Not configured");
  LOG_INFOF("  Sinric Pro: %s", sinricConfigured ? "Configured" : "Not configured");
}
