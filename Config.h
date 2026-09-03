#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// TARGET BOARD SELECTION (MUST BE FIRST - PIN DEFINITIONS DEPEND ON THIS)
// ============================================
// Select exactly ONE target board by commenting/uncommenting below.
// Pin assignments differ between boards because:
// - ESP32-S3 does not have GPIO22-25 (present on the original ESP32)
// - ESP32-S3 N16R8 modules use Octal SPI PSRAM, which reserves GPIO26-37
//   internally for the flash/PSRAM interface (unusable as GPIO)
//#define BOARD_ESP32_WROOM      1   // Original ESP32-WROOM-32 38-pin dev board
#define BOARD_ESP32S3_N16R8    2   // ESP32-S3 N16R8 (16MB flash, 8MB Octal PSRAM)

// #define TARGET_BOARD BOARD_ESP32_WROOM
#define TARGET_BOARD BOARD_ESP32S3_N16R8

// ============================================
// FIRMWARE VERSION (MUST BE FIRST - REQUIRED BY SINRICPRO)
// ============================================
// Format: "major.minor.patch" (semantic versioning)
// Increment this when creating new firmware for OTA updates
#define FIRMWARE_VERSION "1.0.0"


// ============================================
// NO TASK TESTING MODE
// ============================================
// Uncomment to run without creating any tasks - NOT for production
// #define DO_NOT_CREATE_TASKS

// ============================================
// HARDWARE TESTING MODE
// ============================================
// Uncomment to run without hardware connected (for testing/development)
// Disables failsafe triggers and hardware-dependent safety reboots
// WARNING: Only use for development - NOT for production
// #define NO_HARDWARE

// ============================================
// WATCHDOG DEBUGGING MODE
// ============================================
// Uncomment to disable all watchdog timers (for debugging/development)
// Allows running tasks without watchdog resets (useful for debugging hangs)
// WARNING: Only use for debugging - NOT for production
// #define DISABLE_WATCHDOG

// ============================================
// HARDWARE COMPONENT ENABLES
// ============================================
// Comment out any component to disable it for independent testing
// Allows testing each hardware component separately
// All components enabled by default - comment out to disable specific hardware
#define ENABLE_TEMPERATURE_SENSORS 0    // DS18B20 temperature sensors
#define ENABLE_CURRENT_MONITOR 0        // ADS1115 current sensor
#define ENABLE_LCD 1                    // I2C LCD display
#define ENABLE_BUTTONS 1                // MCP23008 button interface
#define ENABLE_PUMP 1                   // Pump relay control
#define ENABLE_HEATER 1                 // Heater relay control
#define ENABLE_BLYNK 0                  // Blynk cloud integration
#define ENABLE_SINRIC 0                 // SinricPro/Alexa integration

// ============================================
// BLYNK SERVER CONFIGURATION
// ============================================
// Choose your Blynk server:
// - Blynk IoT (new): "blynk.cloud" port 80
// - Blynk Legacy (old): "blynk-cloud.com" port 80
// - Custom server: specify your own server and port
#define BLYNK_SERVER "blynk.cloud"      // Default: Blynk IoT cloud
#define BLYNK_PORT 80                    // Default: 80 (or 443 for SSL)

// ============================================
// PIN DEFINITIONS
// ============================================

#if TARGET_BOARD == BOARD_ESP32_WROOM

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

#elif TARGET_BOARD == BOARD_ESP32S3_N16R8

// Temperature Sensors (1-Wire)
// Original pins (35/32/33) are reserved for Octal PSRAM/flash on this module
#define TEMP_INLET_PIN 7        // was 35
#define TEMP_HEATER_PIN 5       // was 32
#define TEMP_AMBIENT_PIN 6      // was 33 - 3rd sensor - ambient/outdoor temperature

// I2C Bus (shared by LCD and MCP23008)
#define I2C_SDA 21               // unchanged - valid & free on ESP32-S3
#define I2C_SCL 14               // was 22 - GPIO22 does not exist on ESP32-S3

// Control Outputs
// Original PUMP_ENABLE_PIN (26) is reserved for Octal PSRAM/flash on this module
#define PUMP_ENABLE_PIN 4        // was 26
#define PUMP_SPEED_PIN 13        // unchanged - LOW=low speed, HIGH=high speed
#define HEATER_PIN 12            // unchanged

#else
#error "TARGET_BOARD not set to a supported value - see TARGET BOARD SELECTION above"
#endif

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
#define ADS1115_ADDRESS 0x48  // ADC for current sensor

// ============================================
// MCP23008 BUTTON PIN ASSIGNMENTS
// ============================================
// TEMP DIAGNOSTIC SWAP: Heater/Pump <-> TempUp/TempDown remapped to isolate
// whether GP0/GP1 not responding is a wiring/pin fault or a switch fault.
// Revert to the original GP0/1/2/3 mapping once diagnosis is complete.
#define BUTTON_HEATER 2        // GP2 (was GP0)
#define BUTTON_PUMP 3          // GP3 (was GP1)
#define BUTTON_TEMP_UP 0       // GP0 (was GP2)
#define BUTTON_TEMP_DOWN 1     // GP1 (was GP3)
#define BUTTON_RESET 4         // GP4

// ============================================
// TIMING CONSTANTS (milliseconds)
// ============================================
#define TEMP_READ_INTERVAL 5000       // 5 seconds
#define CURRENT_READ_INTERVAL 2000    // 2 seconds
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
#define COOLDOWN_SANITY_CHECK 3600000       // 1 hour - max cooldown before overflow check
#define BUTTON_SHORT_PRESS_TIME 1000        // 1 second - max duration for short press
#define WIFI_CONNECTION_TIMEOUT 10          // 10 seconds - reduced from 60

// Millis overflow detection threshold (millis() wraps at ~49.7 days)
// Used to detect invalid timestamp arithmetic when millis() overflows
#define MILLIS_OVERFLOW_THRESHOLD 0xF0000000  // ~46.6 days in milliseconds

// ============================================
// TEMPERATURE LIMITS
// ============================================
#define MIN_TARGET_TEMP 60.0
#define MAX_TARGET_TEMP 104.0
#define TEMP_DIFFERENTIAL_THRESHOLD 2.0  // °F for cooldown end
#define INVALID_TEMP_LOW 32.0
#define INVALID_TEMP_HIGH 104.0
#define MAX_HEATER_TEMP 110.0  // °F - emergency overheat cutoff

// ============================================
// CURRENT SENSOR CONFIGURATION (SCT-013-030 + ADS1115)
// ============================================
#define CURRENT_SENSOR_CHANNEL 0           // ADS1115 channel (A0)
#define CURRENT_SENSOR_BIAS_VOLTAGE 1.65   // Bias voltage (half of 3.3V)
#define CURRENT_SENSOR_BURDEN_RESISTOR 62.0  // Burden resistor in ohms
#define CURRENT_SENSOR_CT_RATIO 1000.0     // SCT-013-030 CT ratio (1000:1)
#define CURRENT_SENSOR_MAX_CURRENT 30.0    // Maximum measurable current (A)

// Current thresholds
#define CURRENT_THRESHOLD_PUMP_RUNNING 0.5  // Minimum current to consider pump running (A)
#define CURRENT_THRESHOLD_OVERCURRENT 25.0  // Overcurrent alarm threshold (A)
#define CURRENT_FAILURE_DETECTION_DELAY 5000  // Wait time before reporting pump failure (ms)

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
#define VPIN_PUMP_CURRENT V11    // Pump current reading
#define VPIN_PUMP_FAILURE_ALERT V12  // Pump failure alert
#define VPIN_MIN_HEATER_TEMP V13 // Minimum water temp to allow heater ON
#define VPIN_MAX_HEATER_TEMP V14 // Maximum water temp to allow heater ON
#define VPIN_LOG_LEVEL V15       // Log level control (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR)
#define VPIN_LOG_BUFFER_SIZE V16 // Log buffer size (12-100 entries)

// ============================================
// WIFI PROVISIONING
// ============================================

#define ENABLE_WIFI_PROV            1


// These enable BlueTooth Provisioning, but will likely create a binary too big for a 4 MB chip with OTA enabled
//
// #define CONFIG_BLUEDROID_ENABLED_1  1
// #define CONFIG_NIMBLE_ENABLED_1    1

#define WIFI_PROV_SERVICE_NAME "POOL_CTRL"
#define WIFI_PROV_POP "pool1234"

// WiFi Failure Detection
#define WIFI_MAX_BOOT_FAILURES 5        // Clear credentials after this many consecutive boot failures
#define WIFI_FAILURE_RESET_DELAY 3000   // Delay before reset (ms)
#define WIFI_CRED_VALIDATION_TIMEOUT 60000  // 60 seconds (1 minute) - timeout for credential validation

// WiFi Runtime Failure Detection
#define WIFI_PROVISIONING_TIMEOUT 300000      // 5 minutes - max time for provisioning
#define WIFI_DISCONNECT_WINDOW 600000         // 10 minutes - time window for disconnect tracking
#define WIFI_MAX_DISCONNECTS 10               // Max disconnects in time window before reset
#define WIFI_RUNTIME_CHECK_INTERVAL 10000     // 10 seconds - how often to check runtime failures


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
#define DEFAULT_MIN_HEATER_TEMP 50.0  // Minimum water temp to allow heater (°F)
#define DEFAULT_MAX_HEATER_TEMP 95.0  // Maximum water temp cutoff (°F)

// ============================================
// LOGGING BUFFER SIZE
// ============================================
#define MIN_LOG_BUFFER_SIZE 12    // Minimum log entries in NVS
#define MAX_LOG_BUFFER_SIZE 100   // Maximum log entries in NVS
#define DEFAULT_LOG_BUFFER_SIZE 12  // Default buffer size
#define LOG_RETRIEVAL_BUFFER_SIZE 8192  // Buffer size for retrieving logs from NVS

// ============================================
// CURRENT SENSOR CALIBRATION
// ============================================
#define CURRENT_CALIBRATION_SAMPLES 100  // Number of samples for calibration
#define CURRENT_SAMPLE_DELAY_MS 20       // Delay between ADC samples (ms)

// ============================================
// TASK STACK MONITORING
// ============================================
#define STACK_WARNING_THRESHOLD 512      // Warn when stack has less than this many bytes free
#define STACK_MONITOR_INTERVAL 300000    // Check stack usage every 5 minutes (ms)

// ============================================
// HEAP MONITORING
// ============================================
#define HEAP_MONITOR_INTERVAL 60000      // Check heap every 1 minute (ms)
#define HEAP_WARNING_THRESHOLD 10000     // Warn when free heap drops below this (bytes)

// ============================================
// OTA (OVER-THE-AIR) UPDATE CONFIGURATION
// ============================================
#define ENABLE_SINRIC_OTA 1                   // Enable SinricPro OTA updates
#define OTA_UPDATE_TIMEOUT 300000              // 5 minutes - max time for OTA update
#define OTA_SAFETY_SHUTDOWN_DELAY 2000         // 2 seconds - delay before starting OTA

#endif // CONFIG_H
