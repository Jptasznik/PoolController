#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// PIN DEFINITIONS
// ============================================

// Temperature Sensors (1-Wire)
#define TEMP_INLET_PIN 35
#define TEMP_HEATER_PIN 32
#define TEMP_AMBIENT_PIN 33    // 3rd sensor - ambient/outdoor temperature

// I2C Bus (shared by LCD and MCP23008)
#define I2C_SDA 21
#define I2C_SCL 22

// Control Outputs
#define PUMP_ENABLE_PIN 26
#define PUMP_SPEED_PIN 13    // LOW=low speed, HIGH=high speed
#define HEATER_PIN 12

// NOTE: For hardware-level relay interlock protection, you can optionally
// add SN74LS08N (AND gate) and SN74LS04N (inverter) logic gates between
// the ESP32 outputs and relay inputs to ensure only one relay is active
// at a time. See RELAY_INTERLOCK_LOGIC.md for complete implementation.
// The code above remains unchanged - the hardware enforces mutual exclusion.

// ============================================
// I2C ADDRESSES
// ============================================
#define LCD_ADDRESS 0x27      // or 0x3F depending on LCD module
#define MCP23008_ADDRESS 0x20

// ============================================
// MCP23008 BUTTON PIN ASSIGNMENTS
// ============================================
#define BUTTON_HEATER 0       // GP0
#define BUTTON_PUMP 1         // GP1
#define BUTTON_TEMP_UP 2      // GP2
#define BUTTON_TEMP_DOWN 3    // GP3
#define BUTTON_RESET 4        // GP4

// ============================================
// TIMING CONSTANTS (milliseconds)
// ============================================
#define TEMP_READ_INTERVAL 5000       // 5 seconds
#define LCD_UPDATE_INTERVAL 1000      // 1 second
#define BLYNK_UPDATE_INTERVAL 2000    // 2 seconds
#define SINRIC_TEMP_UPDATE_INTERVAL 30000  // 30 seconds
#define CONTROL_CYCLE_INTERVAL 60000  // 60 seconds
#define BUTTON_SCAN_INTERVAL 50       // 50ms
#define BUTTON_DEBOUNCE_TIME 50       // 50ms
#define BUTTON_HOLD_CHECK_INTERVAL 100  // 100ms

#define EMERGENCY_SHUTDOWN_HOLD_TIME 10000  // 10 seconds
#define FACTORY_RESET_HOLD_TIME 5000        // 5 seconds
#define COOLDOWN_DURATION 900000            // 15 minutes

// ============================================
// TEMPERATURE LIMITS
// ============================================
#define MIN_TARGET_TEMP 60.0
#define MAX_TARGET_TEMP 104.0
#define TEMP_DIFFERENTIAL_THRESHOLD 2.0  // °F for cooldown end
#define INVALID_TEMP_LOW 32.0
#define INVALID_TEMP_HIGH 104.0

// ============================================
// BLYNK CONFIGURATION
// ============================================
#define BLYNK_TEMPLATE_ID "TMPL2oQXCsxt1"
#define BLYNK_TEMPLATE_NAME "Pool Controller"

// Virtual Pin Assignments
#define VPIN_INLET_TEMP V0
#define VPIN_HEATER_TEMP V1
#define VPIN_TARGET_TEMP V2
#define VPIN_HEATER_ENABLE V3
#define VPIN_PUMP_ENABLE V4
#define VPIN_PUMP_SPEED V5
#define VPIN_MAX_RUNTIME V6
#define VPIN_HEATER_START_HOUR V7
#define VPIN_HEATER_END_HOUR V8
#define VPIN_PUMP_RUNTIME_TODAY V9
#define VPIN_AMBIENT_TEMP V10    // 3rd temperature sensor

// ============================================
// WIFI PROVISIONING
// ============================================
#define WIFI_PROV_SERVICE_NAME "POOL_CTRL"
#define WIFI_PROV_POP "pool1234"

// ============================================
// NTP CONFIGURATION
// ============================================
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define GMT_OFFSET_SEC -18000      // EST: -5 hours
#define DAYLIGHT_OFFSET_SEC 3600   // DST: +1 hour

// ============================================
// DEFAULT SETTINGS
// ============================================
#define DEFAULT_TARGET_TEMP 78.0
#define DEFAULT_MAX_RUNTIME_HOURS 8.0
#define DEFAULT_HEATER_START_HOUR 6
#define DEFAULT_HEATER_END_HOUR 22

#endif // CONFIG_H
