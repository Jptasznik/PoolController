#ifndef ESP32_OTA_HELPER_H
#define ESP32_OTA_HELPER_H

#if defined(ESP32)

#include <Arduino.h>

// Perform OTA update from URL
// Returns empty string on success, error message on failure
String startOtaUpdate(const String &url);

#endif // ESP32

#endif // ESP32_OTA_HELPER_H
