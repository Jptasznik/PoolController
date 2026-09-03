# CLAUDE.md - AI Assistant Guide for ESP32 Pool Controller

## Project Overview

**Project Type:** ESP32 embedded IoT system (Arduino framework)
**Language:** C++ (Arduino/ESP32)
**Purpose:** Production-ready pool automation with temperature control, pump management, and cloud connectivity
**Hardware Platform:** ESP32-WROOM 38-pin development board
**Development Environment:** Arduino IDE with ESP32 board support

This is a **modular, multi-file Arduino project** controlling pool equipment through physical buttons, cloud dashboard (Blynk), and voice commands (Alexa via Sinric Pro).

## Critical Information for AI Assistants

### 1. Arduino Multi-File Project Structure

This is NOT a typical C/C++ project with a build system. It uses the Arduino IDE convention:

- **Main sketch file:** `PoolController3.ino` (contains `setup()` and `loop()` and all Blynk Code )
- **All files must be in the same directory** - no subdirectories
- **Header files (.h) and implementation files (.cpp)** are peers in the same folder
- **Arduino IDE automatically includes all .cpp files** in the folder
- **No Makefile, CMakeLists.txt, or package.json** - Arduino IDE handles compilation

### 2. File Organization Rules

**Core Files (Do NOT rename or remove):**
```
PoolController.ino           # Main coordinator (setup/loop)
Config.h                     # All pin definitions and constants
StateManager.h/cpp           # Centralized state management
StateMutex.h/cpp             # Thread-safe state access
I2CMutex.h/cpp               # Thread-safe I2C bus access
TaskManager.h/cpp            # FreeRTOS task management
MessageQueue.h/cpp           # FreeRTOS message queues for inter-task communication
Logger.h/cpp                 # Structured logging framework
CredentialsManager.h/cpp     # Secure credential storage in flash
SettingsManager.h/cpp        # Flash storage persistence
TemperatureSensors.h/cpp     # DS18B20 sensor handling
CurrentMonitor.h/cpp         # ADS1115 + SCT-013 current monitoring
LCDDisplay.h/cpp             # 20x4 I2C LCD control
ButtonHandler.h/cpp          # MCP23008 button interface
PumpControl.h/cpp            # Pump control logic
HeaterControl.h/cpp          # Heater control with safety
SinricIntegration.h/cpp      # Alexa voice control
```

**Configuration Files:**
```
blynk_auth.h.template        # Blynk credential configuration template
sinric_config.h.template     # Sinric Pro credential configuration template
.gitignore                   # Prevents credential files from being committed

IMPORTANT: Actual credentials (blynk_auth.h and sinric_config.h) are no longer used.
All credentials are now stored securely in ESP32 flash memory via CredentialsManager.
See "Credential Management" section below for configuration instructions.
```

**Documentation Files:**
```
README.md                    # Complete user documentation
INSTALL.md                   # Installation guide
PROJECT_OVERVIEW.md          # Architecture overview
WIRING_GUIDE.md             # Hardware connections
ESP32_PIN_REFERENCE.md      # Pin mapping reference
QUICK_REFERENCE.md          # Quick reference card
```

### 3. Architecture Patterns

#### Centralized State Management
All system state is managed through `StateManager`:

```cpp
// CORRECT - Always use centralized setters
setHeaterEnabledState(true);
setPumpState(PUMP_HIGH);
setTargetTemperature(80.0);

// INCORRECT - Never directly modify state
state.heaterEnabled = true;  // DON'T DO THIS
state.pumpState = PUMP_HIGH; // DON'T DO THIS
```

**Why:** Centralized setters ensure all interfaces (buttons/Blynk/Alexa) stay synchronized and settings are saved to flash.

#### Global State Structure
```cpp
extern SystemState state;  // Defined in StateManager.cpp
```

All modules access the same global `state` object. This is intentional for this embedded system.

#### Module Initialization Pattern
Each module follows this pattern:
```cpp
// In module.h
void initModuleName();

// In PoolController3.ino setup()
void setup() {
  initModuleName();
}

// In module.cpp
void initModuleName() {
  // Initialize hardware/state
}
```

### 4. Credential Management

All cloud service credentials are **securely stored in ESP32 flash memory** using the `CredentialsManager` module. This approach provides several benefits:

- **Security**: Credentials never stored in source code
- **No Version Control Leaks**: Credentials can't be accidentally committed to Git
- **Persistence**: Credentials survive code updates and reboots
- **Encryption**: ESP32 Preferences library provides encrypted storage

#### Supported Credentials

1. **Blynk Authentication Token** - For cloud dashboard access
2. **Sinric Pro Credentials** - For Alexa voice control (App Key, App Secret, Device IDs)

#### Configuration Methods

**Method 1: Serial Monitor (Recommended)**
```cpp
// Send via Serial Monitor at 115200 baud:
BLYNK:your_auth_token_here
SINRIC:app_key:app_secret:thermostat_id:pump_id
```

**Method 2: One-Time Hardcode**
```cpp
// In setup(), add ONE TIME:
setBlynkAuthToken("your_token");
setSinricCredentials("app_key", "app_secret", "thermo_id", "pump_id");
// Then remove the hardcoded values and re-upload
```

#### Key Functions

```cpp
// Initialization
initCredentials();  // Load from flash

// Blynk
const char* getBlynkAuthToken();
bool setBlynkAuthToken(const char* token);
bool hasBlynkCredentials();

// Sinric Pro
const char* getSinricAppKey();
const char* getSinricAppSecret();
const char* getSinricThermostatId();
const char* getSinricPumpId();
bool setSinricCredentials(const char* appKey, const char* appSecret,
                          const char* thermostatId, const char* pumpId);
bool hasSinricCredentials();

// Management
clearAllCredentials();  // Factory reset credentials
printCredentialStatus();  // Debug output
```

#### Important Notes

- Template files (`blynk_auth.h.template`, `sinric_config.h.template`) show configuration instructions
- Actual credential files (`blynk_auth.h`, `sinric_config.h`) are in `.gitignore` and should never be committed
- Cloud services are automatically disabled if credentials are not configured
- Credentials are thread-safe (protected by mutex)

### 5. Logging Framework

The system includes a comprehensive **logging framework** that is **fully integrated across the entire codebase**. All modules use structured logging with multiple severity levels instead of Serial.println(). The framework currently outputs to Serial but is designed for extensibility to support flash storage or cloud logging.

#### Current Integration Status

**✅ Fully Integrated** - All 11 core modules have been migrated to use the logging framework:

| Module | LOG_MODULE Name | Primary Functions |
|--------|----------------|-------------------|
| PoolController.ino | "MAIN" | Setup, loop, Blynk handlers, WiFi events |
| TaskManager.cpp | "TASKS" | FreeRTOS task management, watchdog |
| StateManager.cpp | "STATE" | State setters, cooldown, runtime tracking |
| SettingsManager.cpp | "SETTINGS" | Flash storage operations |
| TemperatureSensors.cpp | "TEMP" | DS18B20 sensor readings |
| CurrentMonitor.cpp | "CURRENT" | ADS1115 current measurement |
| LCDDisplay.cpp | "LCD" | Display updates |
| ButtonHandler.cpp | "BUTTONS" | Physical button interface |
| PumpControl.cpp | "PUMP" | Pump relay control |
| HeaterControl.cpp | "HEATER" | Heater logic and safety |
| SinricIntegration.cpp | "SINRIC" | Alexa voice control |

All Serial.println() calls have been replaced with appropriate LOG_* macros throughout the codebase.

#### Log Levels

```cpp
LOG_DEBUG  // Detailed debugging information
LOG_INFO   // General informational messages
LOG_WARN   // Warning messages (potential issues)
LOG_ERROR  // Error messages (serious problems)
```

#### Using the Logger

**1. Define module name at the top of your .cpp file:**
```cpp
#define LOG_MODULE "MYMODULE"

#include "MyModule.h"
#include "Logger.h"
// ... other includes
```

**2. Use logging macros in your code:**
```cpp
// Simple messages
LOG_DEBUG("Entering function");
LOG_INFO("System initialized");
LOG_WARN("Temperature approaching limit");
LOG_ERROR("Sensor failure detected");

// Formatted messages (printf-style)
LOG_DEBUGF("Reading sensor %d", sensorId);
LOG_INFOF("Temperature: %.1f°F", temp);
LOG_WARNF("Retry attempt %d of %d", attempt, maxAttempts);
LOG_ERRORF("Failed to connect after %d attempts", attempts);
```

**3. Initialize in setup():**
```cpp
void setup() {
  Serial.begin(115200);
  initLogger();  // Initialize logging system
  // ... rest of setup
}
```

#### Configuration

```cpp
// Set minimum log level (filters out lower priority messages)
setLogLevel(LOG_INFO);  // Only show INFO, WARN, ERROR

// Enable/disable timestamps
setLogTimestamps(true);  // Default: enabled

// Enable colored output (if terminal supports ANSI colors)
setLogColors(true);  // Default: disabled
```

#### Log Output Format

```
[timestamp] [LEVEL] [MODULE] Message
```

Example:
```
[12345] [INF] [CREDENTIALS] Blynk credentials loaded
[12567] [ERR] [SENSOR] Temperature sensor timeout
[12890] [WRN] [PUMP] Current below threshold
```

#### NVS Persistence and Circular Log Buffer

The logging framework includes **persistent storage** in ESP32 NVS flash memory:

**Features:**
- **Persistent Log Level** - Log level setting survives reboots
- **Dynamic Buffer Size** - Configurable buffer size (12-100 entries) via Blynk
- **Circular Log Buffer** - Stores recent log entries in flash memory
- **Remote Control** - Blynk virtual pins for log level and buffer size
- **Flash Wear Protection** - Only INFO/WARN/ERROR messages stored (DEBUG excluded)

**NVS Functions:**
```cpp
// Save/load log level to/from flash
saveLogLevel(LOG_INFO);
LogLevel level = loadLogLevel();

// Save/load buffer size to/from flash
saveLogBufferSize(50);
unsigned int size = loadLogBufferSize();

// Get/set buffer size at runtime
unsigned int maxEntries = getLogBufferMaxEntries();
setLogBufferMaxEntries(50);

// Initialize circular buffer
initLogBuffer();

// Retrieve stored logs
char buffer[8192];  // Large enough for max 100 entries
int count = getLogEntries(buffer, sizeof(buffer));

// Clear all stored logs
clearLogBuffer();

// Get current buffer usage
size_t bytes = getLogBufferSize();
```

**Blynk Integration:**
- Virtual Pin **V15** - Log level control (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR)
- Virtual Pin **V16** - Log buffer size (12-100 entries)
- Settings automatically sync to Blynk dashboard on connection
- Changes are immediately saved to NVS flash

**Storage Details:**
- Configurable buffer size: 12-100 entries (default 12)
- Each entry up to 80 characters
- Buffer size range: ~1KB (12 entries) to ~8KB (100 entries)
- Circular buffer overwrites oldest entries when full
- Persists across reboots and power cycles
- Thread-safe with dedicated NVS mutex
- Non-blocking writes (10ms timeout) to avoid task delays

#### Future Extensibility

The logging framework is designed to be extensible. You can add custom log handlers for:
- **Cloud logging** - Send logs to remote server
- **Multiple outputs** - Write to Serial + file + network simultaneously
- **Custom filtering** - Filter logs by module or pattern

```cpp
// Add custom log handler
void myCustomHandler(LogLevel level, const char* module,
                     const char* message, unsigned long timestamp) {
  // Send to cloud, write to file, etc.
}

addLogHandler(myCustomHandler);
```

#### Thread Safety

All logging functions are **thread-safe** and can be called from any FreeRTOS task. The logger uses a mutex to prevent concurrent access.

#### Best Practices

- **Use appropriate log levels** - Don't log everything as ERROR
- **Be concise** - Log messages should be brief and informative
- **Include context** - Module names help identify the source
- **Use formatted logging** - LOG_INFOF is cleaner than building strings
- **Avoid logging in tight loops** - Can impact performance
- **Use DEBUG for verbose info** - Set log level to INFO in production

#### Example Log Output

Here are real examples of log output from the system:

```
[1234] [INF] [MAIN] ESP32 Pool Controller
[1456] [INF] [SETTINGS] Loading settings from flash...
[1567] [INF] [TASKS] Initializing FreeRTOS Tasks
[2345] [INF] [TEMP] Initializing temperature sensors...
[2456] [INF] [CURRENT] ADS1115 initialized successfully
[5678] [DBG] [TEMP] Inlet: 72.5°F  Heater: 75.2°F  Ambient: 68.1°F
[8901] [INF] [STATE] Target temperature: 78.0°F
[9012] [INF] [HEATER] Heater status changed: ON (Inlet: 72.5°F, Target: 78.0°F)
[9023] [WRN] [HEATER] SAFETY: Starting pump (LOW) because heater is ON
[12345] [INF] [BUTTONS] Button: Temperature +1°F
```

#### Migration from Serial.println (Completed)

**Status: ✅ Complete** - The entire codebase has been migrated from Serial.println() to the logging framework.

Before migration:
```cpp
Serial.println("ERROR: Sensor failed");
Serial.print("Temperature: ");
Serial.println(temp);
```

After migration:
```cpp
LOG_ERROR("Sensor failed");
LOG_INFOF("Temperature: %.1f", temp);
```

**Benefits achieved:**
- Consistent formatting across all modules
- Module-based context for easy debugging
- Severity-based filtering capability
- Thread-safe logging for FreeRTOS tasks
- Extensible for future enhancements (flash storage, cloud logging)

### 6. FreeRTOS Multitasking Architecture

This system uses **FreeRTOS multitasking** to efficiently manage multiple concurrent operations. Each subsystem runs in its own task with proper synchronization.

#### Task Structure

The system runs **6 concurrent tasks** across the ESP32's dual cores:

**Core 1 (Application Tasks):**
1. **TemperatureSensorTask** (Priority: 1)
   - Interval: 5 seconds
   - Function: Read DS18B20 temperature sensors
   - Watchdog: 15 seconds
   - No I2C access (uses 1-Wire)

2. **CurrentMonitorTask** (Priority: 2)
   - Interval: 2 seconds
   - Function: Read ADS1115 current sensor
   - Watchdog: 6 seconds
   - **Uses I2C mutex** (ADS1115 on address 0x48)

3. **LCDUpdateTask** (Priority: 1)
   - Interval: 1 second
   - Function: Update 20x4 LCD display
   - Watchdog: 3 seconds
   - **Uses I2C mutex** (LCD on address 0x27)

4. **ButtonScanTask** (Priority: 3 - highest)
   - Interval: 50 milliseconds
   - Function: Scan MCP23008 button inputs
   - Watchdog: 500 milliseconds
   - **Uses I2C mutex** (MCP23008 on address 0x20)

5. **ControlSystemTask** (Priority: 2)
   - Interval: 60 seconds
   - Function: Run heater/pump control logic, safety checks
   - Watchdog: 180 seconds
   - No I2C access (only state modification)

**Core 0/1 (Main Loop - default Arduino task):**
6. **Main Loop** (Priority: 1)
   - Function: Network services (Blynk, Sinric, WiFi monitoring)
   - Watchdog: 30 seconds
   - No I2C access

#### Thread-Safe Programming

All shared resources are protected by **FreeRTOS mutexes**:

**I2C Bus Mutex (`I2CMutex.h/cpp`):**
```cpp
// CORRECT - Always use scoped lock for I2C access
{
  I2CScopedLock i2cLock;
  if (!i2cLock.isLocked()) {
    return;  // Couldn't acquire lock, skip operation
  }
  // Safe to access I2C devices here (LCD, MCP23008, ADS1115)
  lcd.print("Hello");
}  // Lock automatically released
```

**State Mutex (`StateMutex.h/cpp`):**
```cpp
// CORRECT - Use scoped lock when accessing/modifying state
{
  StateScopedLock stateLock;
  if (!stateLock.isLocked()) {
    return;  // Couldn't acquire lock, skip operation
  }
  // Safe to access state here
  float temp = state.inletTemp;
  state.pumpCurrent = 5.2;
}  // Lock automatically released
```

**Best Practices:**
- **Minimize lock time** - Copy data out of locked sections quickly
- **Never nest locks** - Avoid deadlocks by not holding multiple locks
- **Use scoped locks** - RAII ensures locks are always released
- **Check lock success** - Handle timeouts gracefully

#### Watchdog Timers

Each task has its own watchdog timer configured with appropriate timeouts:

| Task | Timeout | Reason |
|------|---------|--------|
| TemperatureSensorTask | 15s | 3x task interval (5s) + margin |
| CurrentMonitorTask | 6s | 3x task interval (2s) + margin |
| LCDUpdateTask | 3s | 3x task interval (1s) + margin |
| ButtonScanTask | 500ms | 10x task interval (50ms) + margin |
| ControlSystemTask | 180s | 3x task interval (60s) + margin |
| Main Loop | 30s | Network operations can be slow |

**Important:** Each task calls `esp_task_wdt_reset()` at the start of every loop iteration. If a task hangs, the watchdog will trigger a system reset.

**Disabling Watchdogs for Debugging:**

For debugging task hangs or performance issues, watchdog timers can be disabled using the `DISABLE_WATCHDOG` define in Config.h:

```cpp
// WATCHDOG DEBUGGING MODE
// Uncomment to disable all watchdog timers (for debugging/development)
// Allows running tasks without watchdog resets (useful for debugging hangs)
// WARNING: Only use for debugging - NOT for production
#define DISABLE_WATCHDOG
```

When enabled:
- All watchdog initialization code is skipped (`esp_task_wdt_init`, `esp_task_wdt_add`)
- All watchdog reset calls are skipped (`esp_task_wdt_reset`)
- Warning messages logged at startup indicating watchdogs are disabled
- Tasks run normally but won't trigger system resets on hangs

**Use cases for DISABLE_WATCHDOG:**
- Debugging task hangs with breakpoints (watchdog would normally trigger during debugging)
- Performance profiling (watchdog resets interfere with measurements)
- Testing infinite loop recovery logic
- Development without worrying about watchdog timeouts

**Important warnings:**
- ⚠️ **NEVER use in production** - Watchdogs are critical for system reliability
- ⚠️ **Tasks can hang indefinitely** without watchdog protection
- ⚠️ **Re-enable before deployment** - Comment out `#define DISABLE_WATCHDOG`
- ⚠️ **Monitor for hangs** - Without watchdogs, you must manually detect task failures

**Example log output with DISABLE_WATCHDOG:**
```
[1234] [WRN] [MAIN] DISABLE_WATCHDOG: Main task watchdog timer disabled
[2345] [WRN] [TASKS] DISABLE_WATCHDOG: Temperature task watchdog disabled
[2456] [WRN] [TASKS] DISABLE_WATCHDOG: Current monitor task watchdog disabled
[2567] [WRN] [TASKS] DISABLE_WATCHDOG: LCD update task watchdog disabled
[2678] [WRN] [TASKS] DISABLE_WATCHDOG: Button scan task watchdog disabled
[2789] [WRN] [TASKS] DISABLE_WATCHDOG: Control system task watchdog disabled
```

#### Task Synchronization

Tasks coordinate through two mechanisms:

**1. Shared State (Legacy)**
- Tasks access shared `SystemState` structure with mutexes
- Centralized state management functions in `StateManager.cpp`
- Thread-safe via `StateMutex` protection

**2. Message Queues (Recommended)**
- Tasks communicate via FreeRTOS message queues
- Decouples tasks from direct state modification
- Clear data flow and event handling
- Defined in `MessageQueue.h/cpp`

#### Message Queue System

The system provides **three message queues** for inter-task communication:

**Control Queue** (`controlQueue`)
- **Purpose**: Sensor data and control commands
- **Size**: 10 messages
- **Producers**: Temperature task, current monitor task, button task
- **Consumers**: Control system task
- **Messages**: Temperature updates, current readings, button presses, pump/heater commands

**Display Queue** (`displayQueue`)
- **Purpose**: Display update requests
- **Size**: 5 messages (can drop if full)
- **Producers**: Control task, button task, network handlers
- **Consumers**: LCD update task
- **Messages**: Display update requests, refresh commands

**Event Queue** (`eventQueue`)
- **Purpose**: System events for logging and monitoring
- **Size**: 20 messages
- **Producers**: All tasks
- **Consumers**: Main loop (for logging/telemetry)
- **Messages**: Cooldown events, connection status changes, runtime updates

**Message Queue Usage Example:**

```cpp
// Sending a temperature update (in TemperatureSensorTask)
Message msg = createTemperatureMessage(inlet, heater, ambient, valid);
if (!sendToControlQueue(&msg)) {
  LOG_WARN("Control queue full!");
}

// Receiving messages (in ControlSystemTask)
Message msg;
if (receiveFromControlQueue(&msg, QUEUE_RECEIVE_TIMEOUT_MS)) {
  switch (msg.type) {
    case MSG_TEMPERATURE_UPDATE:
      float inletTemp = msg.payload.temperature.inletTemp;
      // Process temperature data
      break;

    case MSG_BUTTON_PRESS:
      uint8_t button = msg.payload.button.buttonNumber;
      // Handle button press
      break;
  }
}
```

**Available Message Types:**
- `MSG_TEMPERATURE_UPDATE` - Sensor readings
- `MSG_CURRENT_UPDATE` - Current measurement
- `MSG_BUTTON_PRESS` - Button event
- `MSG_PUMP_COMMAND` - Pump control
- `MSG_HEATER_COMMAND` - Heater control
- `MSG_TARGET_TEMP_CHANGE` - Temperature setpoint
- `MSG_DISPLAY_UPDATE_REQUEST` - Display refresh
- `MSG_COOLDOWN_START/END` - Cooldown events
- `MSG_WIFI_STATUS_CHANGE` - Network events

**Helper Functions:**
```cpp
// Queue status
uint32_t waiting = getControlQueueWaiting();

// Message creation helpers
Message msg1 = createTemperatureMessage(72.5, 75.0, 68.0, true);
Message msg2 = createButtonMessage(BUTTON_TEMP_UP, false);
Message msg3 = createPumpCommandMessage(PUMP_HIGH, false);
```

**Benefits:**
- **Decoupling**: Tasks don't need direct access to each other
- **Thread Safety**: Built-in synchronization
- **Event-Driven**: React to events rather than polling
- **Debugging**: Trace message flow easily
- **Reliability**: Messages queued even if consumer is busy

#### Adding New Tasks

To add a new task:

1. **Define task function in TaskManager.cpp:**
```cpp
void myNewTask(void* parameter) {
  esp_task_wdt_add(NULL);  // Add to watchdog

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(1000);  // 1 second

  for (;;) {
    esp_task_wdt_reset();  // Reset watchdog

    // Do work here (use mutexes for shared resources)

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}
```

2. **Create task in initTasks():**
```cpp
xTaskCreatePinnedToCore(
  myNewTask,           // Function
  "MyTask",            // Name
  4096,                // Stack size
  NULL,                // Parameters
  1,                   // Priority
  &myTaskHandle,       // Handle
  TASK_CORE_1          // Core
);
```

3. **Update TaskManager.h** with constants and handle declaration

### 5. Hardware Configuration

#### Target Board Selection (Config.h)

This project supports two hardware targets, selected by a single define at the
very top of `Config.h` — **must be set before anything else**, since the pin
section below it is conditionally compiled based on this value:

```cpp
#define BOARD_ESP32_WROOM      1   // Original ESP32-WROOM-32 38-pin dev board
#define BOARD_ESP32S3_N16R8    2   // ESP32-S3 N16R8 (16MB flash, 8MB Octal PSRAM)

#define TARGET_BOARD BOARD_ESP32_WROOM
// #define TARGET_BOARD BOARD_ESP32S3_N16R8
```

Comment/uncomment the two `TARGET_BOARD` lines to switch targets, then
re-flash. **All GPIO pin `#define`s in Config.h live inside
`#if TARGET_BOARD == ...` / `#elif` blocks keyed off this value** — see
"Pin Assignments" below for exactly what changes between boards. Non-pin
settings (I2C device addresses, timing constants, feature enables, cloud
credentials) are unaffected by this switch.

See "Supporting ESP32-S3 Boards" further below for why this exists and the
full list of pin differences.

#### Wiring Color Code Standard

For consistency and ease of troubleshooting, use the following color code for I2C connections on the original ESP32-WROOM board:

- **I2C_SCL (GPIO 22, Pin 36)** - **Yellow wire** - Clock line
- **I2C_SDA (GPIO 21, Pin 33)** - **Blue wire** - Data line

This color coding should be used in all wiring diagrams and physical connections to maintain consistency across documentation and hardware builds. (On ESP32-S3, SDA stays Blue/GPIO21, but SCL moves to GPIO14 — keep using Yellow for SCL regardless of which GPIO it lands on, so the color code still means "clock" at a glance.)

#### Pin Assignments (Config.h)

**`BOARD_ESP32_WROOM` (default):**

Temperature Sensors (1-Wire DS18B20):
- `TEMP_INLET_PIN` (GPIO 35, Pin 6) - Water inlet temperature
- `TEMP_HEATER_PIN` (GPIO 32, Pin 7) - Heater outlet temperature
- `TEMP_AMBIENT_PIN` (GPIO 33, Pin 8) - Ambient/outdoor temperature

I2C Bus (shared):
- `I2C_SDA` (GPIO 21, Pin 33) - **Blue** - Data line
- `I2C_SCL` (GPIO 22, Pin 36) - **Yellow** - Clock line

Control Outputs:
- `PUMP_ENABLE_PIN` (GPIO 26, Pin 10) - Pump relay
- `PUMP_SPEED_PIN` (GPIO 13, Pin 15) - Speed relay (LOW/HIGH)
- `HEATER_PIN` (GPIO 12, Pin 13) - Heater relay

**`BOARD_ESP32S3_N16R8`:**

Temperature Sensors (1-Wire DS18B20):
- `TEMP_INLET_PIN` (GPIO 7, was GPIO 35) - Water inlet temperature
- `TEMP_HEATER_PIN` (GPIO 5, was GPIO 32) - Heater outlet temperature
- `TEMP_AMBIENT_PIN` (GPIO 6, was GPIO 33) - Ambient/outdoor temperature

I2C Bus (shared):
- `I2C_SDA` (GPIO 21, unchanged) - **Blue** - Data line
- `I2C_SCL` (GPIO 14, was GPIO 22) - **Yellow** - Clock line

Control Outputs:
- `PUMP_ENABLE_PIN` (GPIO 4, was GPIO 26) - Pump relay
- `PUMP_SPEED_PIN` (GPIO 13, unchanged) - Speed relay (LOW/HIGH)
- `HEATER_PIN` (GPIO 12, unchanged) - Heater relay

**I2C Devices (both boards, all share SDA/Blue and SCL/Yellow):**
- LCD Display: Address `0x27`
- MCP23008 GPIO Expander: Address `0x20`
- ADS1115 ADC: Address `0x48`

#### Important Hardware Constraints

1. **ESP32-WROOM-32 Pin Restrictions:**
   - GPIO 0: Boot mode pin - avoid using
   - GPIO 2: Connected to onboard LED
   - GPIO 6-11: Connected to SPI flash - DO NOT USE
   - GPIO 34-39: Input only, no pull-ups

2. **ESP32-S3 Pin Restrictions (N16R8 / Octal PSRAM modules):**
   - GPIO 22-25: **Do not exist on the ESP32-S3 chip at all** (present on the original ESP32 — the single most common porting mistake, since `I2C_SCL = 22` silently hangs the boot with no error if left unchanged)
   - GPIO 26-37: Reserved internally for the flash/Octal-PSRAM SPI interface - DO NOT USE (a wider reserved range than the original ESP32's GPIO 6-11; narrower — GPIO 26-32 only — on Quad-PSRAM S3 modules, but this project targets N16R8/Octal so treat 26-37 as fully reserved)
   - GPIO 0, 3, 45, 46: Strapping pins (boot mode / voltage select) - avoid for outputs that must not glitch during boot (e.g. relay control)
   - GPIO 19, 20: Native USB D-/D+ - avoid if the native USB peripheral is used
   - GPIO 43, 44: Default UART0 TX/RX (serial monitor / flashing on most S3 DevKitC boards) - avoid reusing these
   - GPIO 46: Input-only

3. **I2C Limitations:**
   - Only one I2C bus is used; both boards' SDA/SCL pins are set via `I2C_SDA`/`I2C_SCL` in Config.h (see Pin Assignments above)
   - All I2C devices share the same bus, protected by a single FreeRTOS mutex (`I2CMutex.h/cpp`) - see "I2C Bus Mutex" under Thread-Safe Programming
   - Each device must have unique address
   - Bus runs at 400kHz (Fast Mode) - see `Wire.setClock(400000)` in `PoolController.ino` `setup()`. Do not revert to the default 100kHz without re-checking button-scan responsiveness (see "NVS Persistence" era note in Version History for why this matters)

4. **1-Wire Sensors:**
   - Each sensor needs 4.7kΩ pull-up resistor
   - Can be on same or different GPIOs
   - Current configuration uses separate pins

#### Supporting ESP32-S3 Boards

The project originally targeted only the ESP32-WROOM-32. ESP32-S3 support
(specifically N16R8 modules: 16MB flash, 8MB **Octal** SPI PSRAM) was added
via the `TARGET_BOARD` toggle described above, because the S3's pin
constraints are meaningfully different from the original ESP32's:

- **GPIO 22-25 don't exist on the ESP32-S3 silicon.** The original board's
  `I2C_SCL = GPIO 22` is invalid on S3 and causes the boot to hang silently
  right after mutex initialization, with no error printed — the hang happens
  inside `Wire.begin(I2C_SDA, I2C_SCL)` in `PoolController.ino`, before any
  further log line runs.
- **GPIO 26-37 are reserved for the internal flash/PSRAM SPI interface** on
  Octal-PSRAM modules like the N16R8 (a much wider reserved range than the
  original ESP32's GPIO 6-11). Any Config.h pin in that range must be
  remapped for the S3 build.

When porting further code to also run identically on both boards:
- Never hardcode a raw GPIO number for a signal already defined in
  Config.h's board-conditional pin block — always reference the `#define`.
- If adding a **new** physical signal (new sensor, new relay, etc.), add its
  pin definition inside both the `BOARD_ESP32_WROOM` and
  `BOARD_ESP32S3_N16R8` branches of the `#if TARGET_BOARD == ...` block in
  Config.h, choosing an S3-safe GPIO per the constraints above.
- Board selection only changes pins - it does not gate any `ENABLE_*`
  feature flags, cloud credentials, or timing constants.
- If you need to support a **Quad-PSRAM** S3 module (not N16R8), note that
  its reserved range is narrower (GPIO 26-32 only, not 26-37) - add a third
  `BOARD_*` variant rather than reusing `BOARD_ESP32S3_N16R8`, since GPIO
  33-37 would be valid GPIO on that variant.

### 6. Safety Critical Code

This system has **mandatory safety interlocks**. When modifying code:

#### Critical Safety Rules

1. **Pump MUST run when heater is ON**
   - Location: `HeaterControl.cpp` and `StateManager.cpp`
   - Never allow heater ON without pump running
   - Enforced in `canPumpTurnOff()` function

2. **15-minute cooldown after heater OFF**
   - Location: `StateManager.cpp` - cooldown tracking
   - Pump stays at minimum LOW speed
   - Ends when time elapsed OR temperatures equalize

3. **Sensor validation**
   - Location: `TemperatureSensors.cpp`
   - Heater disabled if sensor reading invalid
   - Valid range: 32-104°F

4. **Operating hours enforcement**
   - Location: `HeaterControl.cpp`
   - `isWithinOperatingHours()` check
   - Uses NTP-synchronized time

#### DO NOT Modify These Without Understanding Safety Implications:
- `canPumpTurnOff()` in StateManager.cpp
- `startCooldown()` / `endCooldown()` in StateManager.cpp
- `runControlSystem()` in HeaterControl.cpp
- Temperature validation logic in TemperatureSensors.cpp

### 7. Cloud Integration

#### Blynk Virtual Pins
Defined in Config.h - these are the API contract with the cloud:

```cpp
V0  - Inlet temperature (read-only)
V1  - Heater outlet temperature (read-only)
V2  - Target temperature (read/write)
V3  - Heater enable (read/write)
V4  - Pump enable (read/write)
V5  - Pump speed (read/write)
V6  - Max runtime hours (read/write)
V7  - Heater start hour (read/write)
V8  - Heater end hour (read/write)
V9  - Pump runtime today (read-only)
V10 - Ambient temperature (read-only)
V11 - Pump current (read-only)
V12 - Pump failure alert (read-only)
V13 - Minimum heater temperature (read/write)
V14 - Maximum heater temperature (read/write)
V15 - Log level (read/write) - 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR
V16 - Log buffer size (read/write) - 12-100 entries
```

**Important:** If you add virtual pins, update `BLYNK_WRITE()` handlers in PoolController3.ino

#### Sinric Pro (Alexa)
- Thermostat device: Pool Heater (temperature control)
- Dimmer Switch device: Pool Pump (speed control via percentage)
- Credentials in `sinric_config.h`
- **OTA Update Support** - Remote firmware updates via SinricPro

### 8. Over-The-Air (OTA) Updates

The system supports **remote firmware updates** via SinricPro OTA. This allows you to update the ESP32 firmware remotely via the SinricPro dashboard without physical access to the device.

#### OTA Configuration

**Enable/Disable OTA** in Config.h:
```cpp
#define ENABLE_SINRIC_OTA 1                    // 1=enabled, 0=disabled
#define OTA_UPDATE_TIMEOUT 300000              // 5 minutes max (not currently enforced)
#define OTA_SAFETY_SHUTDOWN_DELAY 2000         // 2 second delay before OTA
```

**Firmware Version** in Config.h (at the top of the file):
```cpp
#define FIRMWARE_VERSION "1.0.0"  // Format: "major.minor.patch"
```

Increment the version number in Config.h when creating new firmware for OTA distribution.

#### Required Files

The OTA implementation requires these helper files (from SinricPro SDK):

- **ESP32OTAHelper.h** - Handles HTTPS download and firmware installation
- **SemVer.h** - Semantic version comparison class

These files are included in the project and based on the official SinricPro example at:
https://github.com/sinricpro/esp8266-esp32-sdk/tree/master/examples/OTAUpdate

#### Safety Features

The OTA implementation includes **critical safety measures**:

1. **Version Comparison** - Only installs newer firmware (unless forced)
2. **Heater Shutdown** - Heater is immediately disabled before OTA starts
3. **Pump Control** - Pump is turned OFF if safe (respects cooldown interlock)
4. **State Persistence** - Runtime state saved before OTA
5. **LCD Warning** - Display shows "DO NOT POWER OFF" during update
6. **Error Reporting** - Errors sent back to SinricPro dashboard
7. **Auto-Recovery** - System continues with old firmware if OTA fails

#### OTA Process Flow

```cpp
1. User uploads .bin firmware to SinricPro dashboard with version number
2. SinricPro sends OTA update command to device
3. handleOTAUpdate() callback triggered with URL and version info:
   - Compare current version vs new version (semantic versioning)
   - Return false if already up-to-date (unless force update flag set)
   - Set OTA in progress flag
   - Disable heater and save to NVS
   - Turn pump OFF (if safe, respects cooldown interlock)
   - Display "OTA UPDATE - DO NOT POWER OFF" on LCD
   - 2-second hardware stabilization delay
   - Save runtime state to NVS
4. startOtaUpdate(url) performs the actual update:
   - Download firmware via HTTPS (SSL/TLS)
   - Write to flash using ESP32 Update library
   - Update.onProgress() callback updates LCD display in real-time:
     * Shows percentage progress (0-100%)
     * Shows bytes downloaded (KB format)
     * Updates every 5% or every 50KB
   - All progress logged via LOG_INFO
   - Automatically restart device on success
5. Device reboots with new firmware
6. If OTA fails at any step:
   - Error message sent to SinricPro dashboard
   - OTA in progress flag cleared
   - System returns to normal operation with old firmware
```

#### LCD Display During OTA

The 20x4 LCD shows real-time OTA progress:

```
Line 0:   OTA UPDATE
Line 1:   Progress:  XX%
Line 2:   DO NOT POWER OFF
Line 3:   XXX/YYY KB
```

**Example progression:**
- Initial: "Progress:   0%    0/512 KB"
- Midway:  "Progress:  47%  241/512 KB"
- Final:   "Progress: 100%  512/512 KB"

The display updates automatically as firmware downloads, providing visual feedback without requiring serial monitor access.

#### OTA Callbacks (SinricIntegration.cpp)

```cpp
// SinricPro OTA Callback (registered in initSinric):
bool handleOTAUpdate(const String& url, int major, int minor, int patch, bool forceUpdate);
```

**Parameters:**
- `url` - HTTPS URL to firmware binary
- `major, minor, patch` - New firmware version numbers
- `forceUpdate` - If true, install regardless of version comparison

**Returns:**
- `true` - Update initiated successfully (device will restart)
- `false` - Update rejected or failed (error message set via SinricPro.setResponseMessage())

#### Using OTA Updates

**From SinricPro Dashboard:**
1. Log into SinricPro portal at https://sinric.pro
2. Navigate to your Pool Controller device
3. Click "OTA Update" or "Firmware Update" option
4. Upload compiled .bin firmware file and specify version number
5. SinricPro will send the update to the device
6. Monitor progress on device LCD and serial logs

**Preparing Firmware Binary:**

*Arduino IDE:*
1. Update FIRMWARE_VERSION in Config.h (e.g., "1.0.1")
2. Sketch → Export compiled Binary
3. Find .bin file in sketch folder/build directory
4. Upload to SinricPro dashboard with matching version number

*PlatformIO:*
1. Update FIRMWARE_VERSION in Config.h (e.g., "1.0.1")
2. Run "Build" task
3. Find .bin file in .pio/build/<environment>/ directory
4. Upload to SinricPro dashboard with matching version number

**Important Notes:**
- **Increment version** - Always increment FIRMWARE_VERSION in Config.h when creating new builds
- **Never power off** during OTA update
- **Wait for completion** - takes 1-3 minutes typically depending on firmware size and network speed
- **Stable WiFi required** - OTA will fail on poor connection
- **Heater disabled** - Will not automatically re-enable after OTA
- **Manual verification** - Check system status after OTA completes
- **SinricPro connection required** - Device must be connected to SinricPro to receive OTA updates
- **Version check** - Device will reject updates to same or older version (unless forced)

#### Troubleshooting OTA

**OTA Fails to Start:**
- Check SinricPro connection: `state.sinricConnected` should be true
- Verify ENABLE_SINRIC_OTA is set to 1 in Config.h
- Verify WiFi is connected and stable
- Check SinricPro credentials are configured correctly
- Try rebooting ESP32 and waiting for SinricPro connection

**OTA Update Not Available in Dashboard:**
- Verify your SinricPro account has OTA update feature enabled
- Check that the device is online in SinricPro dashboard
- Ensure firmware binary is valid ESP32 .bin file

**OTA Timeout:**
- Improve WiFi signal strength (move ESP32 closer to router or use WiFi extender)
- Upload smaller firmware (disable unused features in Config.h)
- Try during off-peak network hours
- Check internet connection speed (upload and download)

**System Doesn't Restart:**
- Check serial logs for error messages
- Manually restart via reset button
- System will boot with old firmware if OTA failed
- Check SinricPro dashboard for OTA status/error messages

**Heater Won't Re-Enable After OTA:**
- This is intentional - verify system is working correctly first
- Manually enable heater via Blynk, Alexa, or physical button
- Check for any safety lockouts (failsafe, overheat)
- Verify all sensors are reading valid temperatures

### 9. Common Development Tasks

#### Adding a New Virtual Pin to Blynk

1. **Define in Config.h:**
```cpp
#define VPIN_NEW_FEATURE V11
```

2. **Add BLYNK_WRITE handler in PoolController3.ino:**
```cpp
BLYNK_WRITE(VPIN_NEW_FEATURE) {
  int value = param.asInt();
  // Handle the change
}
```

3. **Add to sync function:**
```cpp
void syncAllToBlynk() {
  // ... existing code
  Blynk.virtualWrite(VPIN_NEW_FEATURE, someValue);
}
```

4. **Configure in Blynk dashboard** (user must do this)

#### Adding a New Temperature Sensor

See `3RD_SENSOR_ADDITION.md` for detailed guide. Key steps:

1. Define pin in Config.h
2. Add OneWire and DallasTemperature objects in TemperatureSensors.cpp
3. Add state variables to SystemState struct
4. Initialize in `initTemperatureSensors()`
5. Read in `readTemperatures()`
6. Display on LCD if needed

#### Adding a New Button

1. **Define in Config.h:**
```cpp
#define BUTTON_NEW_FUNCTION 5  // GP5 on MCP23008
```

2. **Add handler in ButtonHandler.cpp:**
```cpp
void handleButton5Press() {
  // Handle button press
}
```

3. **Wire to scanButtons() or button handlers**

#### Changing Pin Assignments

**ALWAYS modify Config.h only** - never hardcode pins:

```cpp
// CORRECT
#define NEW_PIN 14
digitalWrite(NEW_PIN, HIGH);

// INCORRECT
digitalWrite(14, HIGH);  // Don't hardcode
```

Refer to `ESP32_PIN_REFERENCE.md` for safe pin choices.

#### Adding Persistent Settings

1. **Add to SystemState struct in StateManager.h:**
```cpp
struct SystemState {
  // ... existing fields
  float newSetting;
};
```

2. **Add save/load in SettingsManager.cpp:**
```cpp
void saveNewSetting(float value) {
  preferences.begin("pool", false);
  preferences.putFloat("new_key", value);
  preferences.end();
}

float loadNewSetting() {
  preferences.begin("pool", true);
  float val = preferences.getFloat("new_key", DEFAULT_VALUE);
  preferences.end();
  return val;
}
```

3. **Add to loadAllSettings()** in SettingsManager.cpp

### 9. Testing and Debugging

#### Serial Monitor
- **Baud rate:** 115200
- **Use Serial.print() liberally** for debugging
- System prints status messages, temperatures, button presses

#### Common Debug Patterns
```cpp
Serial.println("=== Function Name ===");
Serial.print("Variable: ");
Serial.println(value);
```

#### I2C Device Detection
If I2C devices not found, scan for addresses:
```cpp
Wire.begin(I2C_SDA, I2C_SCL);
for(byte addr = 1; addr < 127; addr++) {
  Wire.beginTransmission(addr);
  if(Wire.endTransmission() == 0) {
    Serial.print("Found device at 0x");
    Serial.println(addr, HEX);
  }
}
```

#### Testing Safety Interlocks
Always verify:
1. Cannot turn pump OFF while heater ON
2. Pump auto-starts when heater turns ON
3. Cooldown prevents pump OFF for 15 minutes
4. Invalid temp reading disables heater

#### NO_HARDWARE Testing Mode

For development and testing **without physical hardware** connected, the system includes a `NO_HARDWARE` mode that disables safety code that would otherwise trigger reboots or failsafe modes.

**Enable NO_HARDWARE mode** in Config.h:
```cpp
// HARDWARE TESTING MODE
// Uncomment to run without hardware connected (for testing/development)
// Disables failsafe triggers and hardware-dependent safety reboots
// WARNING: Only use for development - NOT for production
#define NO_HARDWARE
```

**What NO_HARDWARE mode does:**

1. **Starts all FreeRTOS tasks normally** - All 5 concurrent tasks run (temperature, current, LCD, buttons, control)
2. **Disables failsafe triggers** - Sensor failures log warnings instead of entering failsafe mode
3. **Disables hardware safety reboots** - WiFi failures and validation timeouts don't restart ESP32
4. **Logs warnings instead** - All skipped safety actions are logged as warnings for debugging

**Safety code disabled in NO_HARDWARE mode:**

| Location | Condition | Normal Behavior | NO_HARDWARE Behavior |
|----------|-----------|-----------------|---------------------|
| TemperatureSensors.cpp | Both sensors failed | `enterFailsafeMode("CRITICAL_SENSOR_FAILURE")` | Log warning only |
| TemperatureSensors.cpp | Inlet sensor failed | `enterFailsafeMode("INLET_SENSOR_FAILURE")` | Log warning only |
| HeaterControl.cpp | Heater overheat (>110°F) | Hardware shutdown + failsafe + lockout | Log warning only |
| PoolController.ino | WiFi runtime failure | `ESP.restart()` (credential reset) | Log warning only |
| PoolController.ino | WiFi boot failures (5+) | `ESP.restart()` (credential reset) | Log warning only |

**What still works in NO_HARDWARE mode:**
- ✅ All tasks execute normally
- ✅ State management and logic
- ✅ Control system decisions
- ✅ Logging framework
- ✅ Serial monitor output
- ✅ User-initiated factory reset (Button 5 held)
- ✅ OTA updates (if WiFi connected)

**What won't work without hardware:**
- ❌ Temperature sensor readings (will show invalid/NaN)
- ❌ Current sensor readings (will show 0.0A)
- ❌ LCD display updates (I2C device not found)
- ❌ Physical button presses (I2C device not found)
- ❌ Pump/heater relay control (GPIOs may not respond)
- ❌ WiFi provisioning (may fail without proper hardware)

**Use cases:**
- **Code logic testing** - Verify control algorithms without hardware
- **Development** - Work on features without physical setup
- **Debugging** - Test state management and flow control
- **Simulation** - Run system with mock data

**Important warnings:**
- ⚠️ **NEVER use NO_HARDWARE in production** - Safety interlocks are critical
- ⚠️ **Always test on hardware** before deploying - simulator won't catch everything
- ⚠️ **Re-enable safety** - Comment out `#define NO_HARDWARE` before production upload
- ⚠️ **Monitor logs** - Watch for "NO_HARDWARE mode: Skipping..." warnings

**Example log output in NO_HARDWARE mode:**
```
[12345] [ERR] [TEMP] CRITICAL: Both inlet and heater sensors failed!
[12346] [WRN] [TEMP] NO_HARDWARE mode: Skipping failsafe trigger for sensor failure
[23456] [ERR] [HEATER] !!! EMERGENCY: HEATER OVERHEAT DETECTED !!!
[23457] [WRN] [HEATER] NO_HARDWARE mode: Skipping failsafe trigger for overheat
[34567] [WRN] [MAIN] WiFi Runtime Failure: Credential validation timeout (1 minute)
[34568] [WRN] [MAIN] NO_HARDWARE mode: Skipping restart for WiFi runtime failure
```

### 10. Coding Conventions

#### Naming Conventions
- **Functions:** camelCase (`initPumpControl()`, `readTemperatures()`)
- **Constants:** UPPER_SNAKE_CASE (`TEMP_INLET_PIN`, `MIN_TARGET_TEMP`)
- **Variables:** camelCase (`inletTemp`, `heaterEnabled`)
- **Enums:** PascalCase (`PumpState`, `HeaterStatus`)

#### File Organization
- **Headers (.h):** Declarations, extern, constants, function prototypes
- **Implementation (.cpp):** Function definitions, static helpers
- **Main sketch (.ino):** setup(), loop(), Blynk handlers, WiFi provisioning

#### Comments
```cpp
// Single line comments for brief explanations

/* Multi-line comments for complex logic
   that needs more explanation */

// ============================================
// SECTION HEADERS - use bars for major sections
// ============================================
```

#### ESP32 Error Handling

**ESP-IDF Functions Return esp_err_t:**

Many ESP32 functions (from ESP-IDF framework) return `esp_err_t` error codes that **must be checked**. Never ignore return values from ESP functions.

**Common ESP Functions That Return esp_err_t:**
- `esp_task_wdt_init()` - Initialize watchdog timer
- `esp_task_wdt_add()` - Add task to watchdog
- `esp_task_wdt_reset()` - Reset watchdog (can fail if task not added)
- `esp_task_wdt_delete()` - Remove task from watchdog
- `esp_wifi_*()` - WiFi configuration functions
- `nvs_*()` - Non-volatile storage (flash) operations
- `esp_ota_*()` - Over-the-air update functions

**Error Code Values:**
```cpp
ESP_OK          // Success (0)
ESP_FAIL        // Generic failure
ESP_ERR_NO_MEM  // Out of memory
ESP_ERR_INVALID_ARG  // Invalid argument
ESP_ERR_INVALID_STATE  // Invalid state
ESP_ERR_TIMEOUT  // Operation timed out
// ... many more defined in esp_err.h
```

**Best Practices:**

1. **Always check return codes:**
```cpp
// CORRECT - Check return value
esp_err_t err = esp_task_wdt_init(&wdt_config);
if (err != ESP_OK) {
  LOG_ERRORF("Failed to init watchdog: %s", esp_err_to_name(err));
  // Handle error appropriately
}

// INCORRECT - Ignoring return value
esp_task_wdt_init(&wdt_config);  // DON'T DO THIS
```

2. **Use esp_err_to_name() for logging:**
```cpp
esp_err_t err = esp_task_wdt_add(NULL);
if (err != ESP_OK) {
  LOG_ERRORF("Failed to add task to watchdog: %s", esp_err_to_name(err));
}
```

3. **Handle errors gracefully:**
```cpp
// Option 1: Retry on failure
esp_err_t err = nvs_set_u32(handle, "key", value);
if (err != ESP_OK) {
  LOG_WARNF("NVS write failed: %s - retrying", esp_err_to_name(err));
  vTaskDelay(pdMS_TO_TICKS(100));
  err = nvs_set_u32(handle, "key", value);
}

// Option 2: Return error to caller
esp_err_t initSomething() {
  esp_err_t err = esp_task_wdt_init(&config);
  if (err != ESP_OK) {
    return err;  // Propagate error
  }
  return ESP_OK;
}

// Option 3: Enter safe mode on critical failure
esp_err_t err = esp_task_wdt_add(NULL);
if (err != ESP_OK) {
  LOG_ERROR("Critical: Cannot add watchdog - entering failsafe");
  enterFailsafeMode("WATCHDOG_INIT_FAILURE");
}
```

4. **Document expected errors:**
```cpp
// This function may return:
// - ESP_OK: Success
// - ESP_ERR_NO_MEM: Out of memory
// - ESP_ERR_TIMEOUT: I2C timeout
esp_err_t readSensor() {
  // Implementation
}
```

**ESP_ERROR_CHECK() Macro:**

ESP-IDF provides `ESP_ERROR_CHECK()` macro that aborts on error (useful for critical init):

```cpp
// Aborts program if initialization fails (use sparingly)
ESP_ERROR_CHECK(esp_task_wdt_init(&wdt_config));

// Better for production: Check and handle gracefully
esp_err_t err = esp_task_wdt_init(&wdt_config);
if (err != ESP_OK) {
  LOG_ERRORF("Watchdog init failed: %s", esp_err_to_name(err));
  // Graceful degradation or failsafe mode
  return;
}
```

**Reference:**
- ESP-IDF Error Handling: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/error-handling.html
- Error Codes Reference: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/error-codes.html

### 11. Dependencies and Libraries

**Required Arduino Libraries:**
1. Blynk (by Volodymyr Shymanskyy)
2. DallasTemperature (by Miles Burton)
3. OneWire (by Paul Stoffregen)
4. LiquidCrystal_I2C (by Marco Schwartz or Frank de Brabander)
5. SinricPro (by Boris Jaeger)
6. Adafruit_MCP23X08 (by Adafruit)
7. Adafruit_ADS1X15 (by Adafruit) - for ADS1115 ADC

**ESP32 Built-in Libraries:**
- WiFi.h
- WiFiProv.h
- Preferences.h (for flash storage)
- Wire.h (I2C)
- time.h (NTP)

**Installation:** All via Arduino IDE Library Manager (Sketch → Include Library → Manage Libraries)

### 12. Memory Considerations

ESP32 has limited RAM (~320KB). Best practices:

- **Use `const` for strings:** `const char* msg = "Error";`
- **Avoid String class** - use char arrays for fixed strings
- **F() macro** for Serial.print: `Serial.println(F("Message"));`
- **Watch global variables** - state struct is already large
- **Avoid dynamic allocation** (malloc/new) when possible

### 13. Timing and Task Scheduling

The system uses **FreeRTOS task scheduling** with cooperative multitasking:

```cpp
void myTask(void* parameter) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(1000);  // 1 second

  for (;;) {
    esp_task_wdt_reset();  // Reset watchdog

    // Do work here

    vTaskDelayUntil(&xLastWakeTime, xFrequency);  // Precise timing
  }
}
```

**Task Intervals (defined in Config.h):**
- Temperature reading: 5s (TEMP_READ_INTERVAL)
- Current monitoring: 2s (CURRENT_READ_INTERVAL)
- LCD update: 1s (LCD_UPDATE_INTERVAL)
- Button scan: 50ms (BUTTON_SCAN_INTERVAL)
- Control cycle: 60s (CONTROL_CYCLE_INTERVAL)
- Blynk update: 2s (BLYNK_UPDATE_INTERVAL)

**Important Notes:**
- **Use vTaskDelayUntil()** - Provides precise periodic timing
- **Use vTaskDelay()** for relative delays
- **delay() can still be used** in tasks but blocks that task only (not the entire system)
- **Main loop** handles network services and should avoid blocking operations
- **Each task is independent** and runs according to its own schedule

#### Millis() Overflow Handling

The ESP32 `millis()` function returns milliseconds since boot and **overflows after ~49.7 days** (2^32 milliseconds). The codebase uses several strategies to handle this:

**1. Unsigned Arithmetic (Safe by Design)**

Most timing code uses the standard pattern which is **overflow-safe**:
```cpp
unsigned long now = millis();
unsigned long elapsed = now - previousTime;
if (elapsed >= interval) {
  // Do something
  previousTime = now;
}
```

This works correctly even when overflow occurs because unsigned arithmetic wraps around:
- If `previousTime = 0xFFFFFF00` (just before overflow)
- And `now = 0x00000200` (just after overflow)
- Then `elapsed = 0x00000200 - 0xFFFFFF00 = 0x00000300` (correctly calculates 768ms)

**2. Sanity Checks (Critical Paths)**

For critical operations that accumulate time or make decisions based on elapsed time, sanity checks are used:

```cpp
unsigned long elapsed = now - startTime;

// Detect impossible elapsed times (indicates overflow or system suspension)
if (elapsed > SANITY_THRESHOLD) {
  // Handle anomaly - log warning, reset tracking, or return safe value
  Serial.println("WARNING: Time tracking anomaly detected");
  return safe_value;
}
```

**Examples in codebase:**
- `isCooldownComplete()` - Uses 1 hour sanity check (`COOLDOWN_SANITY_CHECK`)
- `updatePumpRuntime()` - Uses 1 hour sanity check (prevents runtime corruption)
- `getRecentDisconnectCount()` - Uses `MILLIS_OVERFLOW_THRESHOLD` to detect invalid timestamps
- `isProvisioningHung()` - Uses `MILLIS_OVERFLOW_THRESHOLD` to detect overflow

**3. Overflow Detection Constant**

The constant `MILLIS_OVERFLOW_THRESHOLD` (0xF0000000 = ~46.6 days) is defined in Config.h:
```cpp
#define MILLIS_OVERFLOW_THRESHOLD 0xF0000000
```

This is used to detect when elapsed time calculations produce impossible values due to overflow.

**Best Practices:**
- ✅ Always use `unsigned long` for millis() values
- ✅ Use subtraction pattern for intervals: `(now - previous) >= interval`
- ✅ Add sanity checks for long-running timers (>1 hour intervals)
- ✅ Don't compare absolute millis() values (e.g., `now > target` is unsafe)
- ✅ For critical accumulation (runtime tracking), verify elapsed < reasonable maximum
- ❌ Never use signed arithmetic with millis()
- ❌ Don't assume millis() never decreases

**Intervals Safe from Overflow:**
- All intervals < 24.8 days (2^31 ms) are inherently safe with unsigned arithmetic
- All task intervals in this project are < 60 seconds, well within safe range

### 14. WiFi Provisioning and Failure Detection

The system includes a **comprehensive WiFi management system** with BLE provisioning and automatic failure detection/recovery. This ensures the system remains operational even with WiFi issues.

#### WiFi Provisioning (BLE)

**Configuration** (Config.h):
```cpp
#define WIFI_PROV_SERVICE_NAME "POOL_CTRL"      // BLE service name
#define WIFI_PROV_POP "pool1234"                // Proof of possession
#define WIFI_CONNECTION_TIMEOUT 10               // 10 seconds
```

**Implementation** (PoolController.ino):
- Uses ESP32's **built-in BLE provisioning** via WiFiProv library
- Credentials stored in ESP32 flash memory (persist across reboots)
- User provisions via **ESP BLE Provisioning** mobile app (iOS/Android)
- Provisioning service name: `POOL_CTRL`
- Proof of possession (password): `pool1234`

**Provisioning Process:**
1. On first boot (no credentials), ESP32 starts BLE provisioning
2. User opens ESP BLE Provisioning app on phone
3. App discovers device "POOL_CTRL"
4. User enters proof of possession: "pool1234"
5. App sends WiFi SSID and password to ESP32
6. ESP32 stores credentials in flash
7. ESP32 connects to WiFi
8. Future boots auto-connect using stored credentials

**Event Callbacks** (PoolController.ino):
```cpp
void SysProvEvent(arduino_event_t *sys_event) {
  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      // Provisioning started
      state.provisioningActive = true;
      state.provisioningStartTime = millis();
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV:
      // Credentials received - start validation timeout tracking
      state.credentialsReceived = true;
      state.credReceivedTime = millis();
      break;

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      // Credentials accepted - clear validation timeout
      state.credentialsReceived = false;
      state.credReceivedTime = 0;
      break;

    case ARDUINO_EVENT_PROV_END:
      // Provisioning complete
      state.provisioningActive = false;
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      // WiFi connected - clear all tracking
      state.wifiConnected = true;
      clearWiFiRuntimeTracking();
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      // WiFi disconnected - record for failure detection
      state.wifiConnected = false;
      recordWiFiDisconnect();
      break;
  }
}
```

#### Boot Failure Detection and Auto-Recovery

**Problem:** If WiFi credentials become invalid (router changed, network issues), device cannot connect at boot.

**Solution:** Track consecutive boot failures and auto-reset credentials after threshold exceeded.

**Configuration** (Config.h):
```cpp
#define WIFI_MAX_BOOT_FAILURES 5        // Clear credentials after 5 consecutive failures
#define WIFI_FAILURE_RESET_DELAY 3000   // Display message for 3 seconds before reset
```

**Implementation** (PoolController.ino + SettingsManager.cpp):
```cpp
// On boot:
int wifiFailureCount = loadWiFiFailureCount();  // Load from NVS flash

if (wifiFailureCount >= WIFI_MAX_BOOT_FAILURES) {
  LOG_WARN("WiFi failure threshold exceeded!");
  WiFi.disconnect(true, true);  // Clear stored credentials
  resetWiFiFailureCount();       // Reset counter
  displayWiFiReset();            // Show message on LCD
  ESP.restart();                 // Reboot to start provisioning
}

// After WiFi connection attempt:
if (WiFi.status() == WL_CONNECTED) {
  resetWiFiFailureCount();       // Success - reset counter
} else {
  incrementWiFiFailureCount();   // Failure - increment counter
}
```

**Failure Count Functions** (SettingsManager.cpp):
```cpp
int loadWiFiFailureCount();        // Load from NVS
void saveWiFiFailureCount(int);    // Save to NVS
void incrementWiFiFailureCount();  // Increment and save
void resetWiFiFailureCount();      // Set to 0 and save
```

**Result:** After 5 consecutive boot failures, device auto-clears credentials and enters provisioning mode, allowing user to reconfigure WiFi.

#### Runtime Failure Detection

**Problem:** WiFi may fail after successful boot (provisioning hangs, excessive disconnects).

**Solution:** Monitor WiFi health at runtime and auto-recover if issues detected.

**Configuration** (Config.h):
```cpp
#define WIFI_CRED_VALIDATION_TIMEOUT 60000    // 1 minute - credential validation timeout
#define WIFI_PROVISIONING_TIMEOUT 300000      // 5 minutes max for provisioning
#define WIFI_DISCONNECT_WINDOW 600000         // 10 minutes time window
#define WIFI_MAX_DISCONNECTS 10               // Max disconnects in time window
#define WIFI_RUNTIME_CHECK_INTERVAL 10000     // Check every 10 seconds
```

**Failure Scenarios Detected:**

1. **Provisioning Hang** - Provisioning started but no connection after 5 minutes
2. **Credential Validation Timeout** - Credentials received but not validated within 1 minute
3. **Excessive Disconnects** - More than 10 disconnects within 10 minutes

**Implementation** (PoolController.ino main loop):
```cpp
// Check every 10 seconds
if (now - lastWiFiRuntimeCheck >= WIFI_RUNTIME_CHECK_INTERVAL) {
  lastWiFiRuntimeCheck = now;

  // Check for provisioning hang
  if (isProvisioningHung()) {
    handleWiFiRuntimeFailure("Provisioning timeout - no response for 5 minutes");
  }

  // Check for credential validation timeout
  if (state.credentialsReceived) {
    unsigned long elapsed = now - state.credReceivedTime;
    if (elapsed >= WIFI_CRED_VALIDATION_TIMEOUT) {
      handleWiFiRuntimeFailure("Credential validation timeout (1 minute)");
    }
  }

  // Check for excessive disconnections
  int recentDisconnects = getRecentDisconnectCount();
  if (recentDisconnects >= WIFI_MAX_DISCONNECTS) {
    handleWiFiRuntimeFailure("Excessive disconnects (10 in 10 minutes)");
  }
}
```

**Runtime Tracking Functions** (StateManager.cpp):
```cpp
void recordWiFiDisconnect();           // Called on WiFi disconnect event
int getRecentDisconnectCount();        // Count disconnects in time window
bool isProvisioningHung();             // Check if provisioning timed out
void clearWiFiRuntimeTracking();       // Clear tracking on successful connection
```

**Disconnect Tracking Algorithm:**
- Circular buffer stores last 10 disconnect timestamps
- `getRecentDisconnectCount()` counts how many are within WIFI_DISCONNECT_WINDOW
- Handles millis() overflow (uses MILLIS_OVERFLOW_THRESHOLD)

**Recovery Handler** (PoolController.ino):
```cpp
void handleWiFiRuntimeFailure(const char* reason) {
  LOG_ERRORF("WiFi Runtime Failure: %s", reason);
  WiFi.disconnect(true, true);       // Clear credentials
  resetWiFiFailureCount();           // Reset boot counter
  clearWiFiRuntimeTracking();        // Clear runtime tracking
  displayWiFiReset();                // Show message on LCD
  delay(WIFI_FAILURE_RESET_DELAY);   // Let user read message
  ESP.restart();                     // Reboot to provisioning mode
}
```

#### State Variables

**WiFi Connection State:**
```cpp
state.wifiConnected      // Current WiFi connection status
state.blynkConnected     // Blynk cloud connection status
state.sinricConnected    // SinricPro connection status
```

**Provisioning State:**
```cpp
state.provisioningActive      // Currently in provisioning mode
state.provisioningStartTime   // When provisioning started (for timeout detection)
state.credentialsReceived     // Credentials received, waiting for validation
state.credReceivedTime        // When credentials were received
```

**Runtime Tracking State:**
```cpp
state.disconnectCount                      // Total disconnect count
state.disconnectTimestamps[10]             // Last 10 disconnect times (circular buffer)
```

#### Offline Mode

**Important:** The system is designed to work **fully offline** if WiFi fails:
- Local controls (buttons) always work
- LCD display always works
- Pump and heater can be controlled manually
- Safety interlocks remain active
- Only cloud features (Blynk, Alexa) are unavailable

**Graceful Degradation:**
```cpp
// In main loop - only run cloud services if connected
if (wifiConnected) {
  Blynk.run();
  blynkTimer.run();
  handleSinricEvents();
  handleOTA();
}
```

#### Connectivity Handling

**WiFi Event Handling:**
```cpp
case ARDUINO_EVENT_WIFI_STA_GOT_IP:
  state.wifiConnected = true;
  clearWiFiRuntimeTracking();   // Clear disconnect tracking
  updateLCDDisplay();
  break;

case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
  state.wifiConnected = false;
  state.blynkConnected = false;
  recordWiFiDisconnect();       // Track for runtime failure detection
  updateLCDDisplay();
  break;
```

#### NTP Time Sync

**Configuration** (Config.h):
```cpp
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define GMT_OFFSET_SEC -18000      // EST: -5 hours
#define DAYLIGHT_OFFSET_SEC 3600   // DST: +1 hour
```

**Initialization** (PoolController.ino setup):
```cpp
configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
```

Used for operating hours enforcement in HeaterControl.cpp.

#### Summary

The WiFi management system provides:
- ✅ **Easy provisioning** via BLE and mobile app
- ✅ **Automatic recovery** from boot failures (5 consecutive failures → reset credentials)
- ✅ **Runtime monitoring** (provisioning hangs, excessive disconnects)
- ✅ **Persistent credentials** stored in ESP32 flash
- ✅ **Graceful offline mode** - local controls always work
- ✅ **Millis() overflow handling** in disconnect tracking
- ✅ **User feedback** via LCD display during failures

This ensures the pool controller remains operational and automatically recovers from WiFi issues without manual intervention.

### 15. Version Control and Git

**When committing changes:**
1. Test thoroughly on hardware first
2. Verify safety interlocks still work
3. Update relevant documentation
4. Use clear commit messages describing what changed and why
5. Push to the designated branch

**Do NOT commit:**
- User's actual Blynk auth token
- User's actual Sinric Pro credentials
- Temporary test files
- IDE-specific files

### 16. Common Pitfalls and Gotchas

#### Arduino IDE Quirks
- **Case-sensitive filenames** on Linux, case-insensitive on Windows
- **Include order matters** - Config.h should be included first
- **Forward declarations** may be needed for complex cross-references

#### `#ifdef FLAG` vs `#if FLAG` (critical gotcha)
- All `ENABLE_*` flags in Config.h are defined with an explicit value:
  `#define ENABLE_TEMPERATURE_SENSORS 0`. **`#ifdef ENABLE_TEMPERATURE_SENSORS`
  only checks whether the symbol is defined at all - it evaluates true
  regardless of whether the value is 0 or 1**, because the symbol is
  defined either way. Guarding these flags anywhere in the codebase
  requires `#if ENABLE_TEMPERATURE_SENSORS`, not `#ifdef`.
- This bug previously existed in `TaskManager.cpp`, `TemperatureSensors.cpp`,
  `CurrentMonitor.cpp`, `LCDDisplay.cpp`, `ButtonHandler.cpp`,
  `PumpControl.cpp`, `HeaterControl.cpp`, `SinricIntegration.cpp`, and
  `PoolController.ino` — every `#ifdef ENABLE_*` guard was silently a no-op,
  so "disabling" a feature by setting its flag to `0` never actually
  stopped its task from being created. This caused spurious sensor-failure
  log spam and unnecessary I2C bus contention from tasks (e.g. the current
  monitor) that were believed to be off. Fixed by changing every
  `#ifdef ENABLE_*` to `#if ENABLE_*` — **always use `#if`, never `#ifdef`,
  for these flags** when adding new guarded code.
- When a value is guarded by `#if ENABLE_BLYNK` in one file (e.g. functions
  defined in `PoolController.ino`) but called via `extern` from another file
  (e.g. `WiFiNetworkTask.cpp`), the `extern` declarations and every call site
  in the *calling* file must also be wrapped in the same `#if ENABLE_BLYNK`
  guard, or the linker fails with "undefined reference" when the flag is 0
  (the definition is compiled out, but the call site isn't).

#### ESP32 Specific
- **GPIO 6-11 are unusable** (connected to flash) - ESP32-WROOM-32 only; see
  "Supporting ESP32-S3 Boards" under Hardware Configuration for the very
  different (and wider) reserved-pin range on ESP32-S3
- **Brownout detector** - ensure adequate power supply (5V 2A minimum)
- **WiFi power management** - can cause disconnects, handled by auto-reconnect
- **Watchdog timer** - loop() must not block too long (hence delay(10) at end)
- **ESP32 Arduino core version matters for API compatibility.** Code that
  compiles on core 3.x (ESP-IDF 5.x) can fail to compile on core 2.x
  (ESP-IDF 4.x) and vice versa. Two APIs in this project are version-gated:
  - `esp_task_wdt_config_t` / `esp_task_wdt_reconfigure()` only exist on
    ESP-IDF ≥ 5.0. `PoolController.ino`'s watchdog init checks
    `#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)` and falls back to
    the older two-argument `esp_task_wdt_init(180, true)` on core 2.x.
  - `WiFiProvLite` (custom wrapper over `network_provisioning/manager.h`)
    only compiles when `CONFIG_NETWORK_PROV_NETWORK_TYPE_WIFI` is defined by
    the installed core's sdkconfig (true on core 3.x, not on 2.x). All
    `WiFiProvLite`/`NETWORK_PROV_*` usage in `PoolController.ino` is gated
    behind a derived `WIFI_PROV_ACTIVE` macro
    (`ENABLE_WIFI_PROV && defined(CONFIG_NETWORK_PROV_NETWORK_TYPE_WIFI)`),
    not `ENABLE_WIFI_PROV` alone — use `WIFI_PROV_ACTIVE` for any new
    `WiFiProvLite` call site, and fall back to plain `WiFi.disconnect(true, true)`
    where credential-clearing is needed but provisioning isn't active.
  - When adding new ESP-IDF API calls, check whether the API is version-gated
    upstream and mirror this pattern rather than assuming the currently
    installed core's API surface is universal.

#### Blynk Specific
- **Blynk.run() must be called frequently** (every loop iteration)
- **Virtual pins are strings** - use `.asInt()`, `.asFloat()`, `.asString()`
- **Avoid writing to same pin too fast** - rate limit updates

#### I2C Issues
- **Pull-up resistors** may be needed (usually on modules already)
- **Wire.begin() must be called before I2C access**
- **Address conflicts** - ensure each device has unique address
- **Bus capacitance** - long wires can cause issues
- **A single slow/long I2C transaction blocks every other I2C-using task**
  while it holds `I2CMutex` — the mutex protects the whole bus, not a
  per-device lock, so a long hold by one task (e.g. a full LCD screen
  redraw) can starve latency-sensitive ones (e.g. button scanning) even
  though the button task has higher FreeRTOS priority (FreeRTOS mutex
  priority inheritance prevents unbounded priority inversion, but it does
  not make the slow task's I2C transaction itself go faster). Prefer
  several short lock/unlock scopes over one long one — see
  `updateLCDDisplay()` in `LCDDisplay.cpp`, which locks/unlocks once per
  LCD line rather than once for the whole 4-line redraw, specifically to
  give `ButtonScanTask` (50ms interval, 100ms lock timeout) frequent
  chances to interleave. Also consider `Wire.setClock(400000)` (set in
  `PoolController.ino` `setup()`) over the default 100kHz where the
  connected I2C devices support Fast Mode — it directly cuts every
  transaction's hold time.

### 17. Feature Enhancement Guidelines

When adding new features:

1. **Maintain modular structure** - create new .h/.cpp pair if substantial
2. **Use centralized state management** - add to StateManager if shared
3. **Add to Config.h** - never hardcode constants
4. **Update documentation** - at minimum README.md and this file
5. **Preserve safety interlocks** - never compromise safety
6. **Test offline mode** - features should degrade gracefully without WiFi
7. **Consider flash wear** - don't write to Preferences too frequently

### 18. Quick Reference Commands

#### Compile and Upload
```
Arduino IDE: Sketch → Upload (or Ctrl+U)
Board: ESP32 Dev Module
Baud: 115200
```

#### Serial Monitor
```
Tools → Serial Monitor
Baud: 115200
```

#### Factory Reset
Hold Button 5 (BUTTON_RESET on GP4) for 5 seconds

#### Emergency Shutdown
Hold Button 2 (BUTTON_PUMP on GP1) for 10 seconds

### 19. Documentation Files Reference

- **README.md** - Complete user-facing documentation, troubleshooting
- **INSTALL.md** - Step-by-step installation and setup guide
- **PROJECT_OVERVIEW.md** - High-level architecture and feature list
- **WIRING_GUIDE.md** - Detailed hardware connections and wiring diagrams
- **ESP32_PIN_REFERENCE.md** - Complete ESP32 pin capabilities and restrictions
- **PIN_STATUS_SUMMARY.md** - Visual summary of current pin usage
- **PHYSICAL_PIN_REFERENCE.md** - Physical pin to GPIO mapping
- **3RD_SENSOR_ADDITION.md** - Guide for adding temperature sensor
- **QUICK_REFERENCE.md** - Quick reference card for common operations
- **CLAUDE.md** (this file) - AI assistant development guide

### 20. When to Ask User for Clarification

Ask the user before:
- **Changing pin assignments** (may affect their hardware)
- **Modifying safety interlock logic** (critical for safety)
- **Changing Blynk virtual pin mappings** (breaks their dashboard)
- **Altering temperature thresholds** (affects pool operation)
- **Removing or renaming existing features** (may be in use)
- **Changing timing intervals significantly** (affects user experience)

### 21. Testing Checklist for Changes

Before considering a change complete:

**Code Quality:**
- [ ] Compiles without errors or warnings
- [ ] Follows existing naming conventions
- [ ] Constants defined in Config.h (not hardcoded)
- [ ] Changes use centralized state management
- [ ] Serial debug output added where helpful

**Functionality:**
- [ ] Feature works as intended
- [ ] Doesn't break existing features
- [ ] Works offline (without WiFi) if applicable
- [ ] LCD displays updated info (if relevant)
- [ ] Blynk syncs correctly (if relevant)
- [ ] Alexa commands work (if relevant)

**Safety:**
- [ ] Safety interlocks still functional
- [ ] Cannot turn off pump with heater on
- [ ] Cooldown logic preserved
- [ ] Invalid sensor readings handled
- [ ] No unsafe state transitions possible

**Documentation:**
- [ ] README.md updated (if user-facing change)
- [ ] CLAUDE.md updated (if architecture change)
- [ ] Code comments added for complex logic
- [ ] Pin changes documented (if applicable)

## Summary

This is a well-structured, safety-critical embedded system. The modular architecture makes it easy to enhance, but safety must always be the top priority. When in doubt about a change, test thoroughly on hardware and verify all safety interlocks remain functional.

**Key Principles:**
1. **Safety first** - never compromise safety interlocks
2. **Use centralized state** - always use setter functions
3. **Use structured logging** - LOG_* macros only, never Serial.println()
4. **No hardcoding** - all constants in Config.h
5. **Modular design** - keep separation of concerns
6. **Test on hardware** - simulator won't catch everything
7. **Document changes** - maintain excellent documentation
8. **Fail gracefully** - handle offline mode and errors

---

**Last Updated:** 2026-09-02
**Project Version:** 1.3
**Major Updates:**
- ✅ Logging framework fully integrated across all 11 modules (December 2025)
- ✅ All Serial.println() calls replaced with structured logging
- ✅ Module-based LOG_MODULE identifiers for all components
- ✅ NVS persistent logging with configurable circular buffer (December 2025)
- ✅ Remote log control via Blynk (log level V15, buffer size V16)
- ✅ **Production safety hardening (December 22, 2025):**
  - NO_HARDWARE mode disabled for production
  - Thread-safe state access with mutex protection on all 18 state functions
  - Standardized mutex error handling with logging
  - WiFi credential validation timeout restored to 60 seconds
  - All blocking delays documented
- ✅ **Feature-flag, compatibility, and I2C reliability fixes (September 2026):**
  - Fixed `#ifdef` vs `#if` bug that made every `ENABLE_*` flag a no-op when
    set to `0` — see "`#ifdef FLAG` vs `#if FLAG`" under Common Pitfalls
  - Guarded `WiFiNetworkTask.cpp`'s Blynk sync calls behind `#if ENABLE_BLYNK`
    to fix linker errors when Blynk is disabled
  - ESP32 Arduino core 2.x (ESP-IDF 4.x) backward compatibility for
    `WiFiProvLite` and watchdog init, alongside core 3.x — see "ESP32
    Specific" under Common Pitfalls
  - Fixed I2C mutex timeouts and button-scan lag: `Wire.setClock(400000)`
    (Fast Mode) plus splitting the LCD's full-screen redraw into
    per-line lock/unlock scopes instead of one long lock — see "I2C Issues"
    under Common Pitfalls
- ✅ **ESP32-S3 (N16R8) support added (September 2026):**
  - New `TARGET_BOARD` toggle in Config.h selects `BOARD_ESP32_WROOM` or
    `BOARD_ESP32S3_N16R8`, switching the pin definitions accordingly — see
    "Supporting ESP32-S3 Boards" under Hardware Configuration

**Claude Assistant:** Ready to help with this codebase!
