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
SettingsManager.h/cpp        # Flash storage persistence
TemperatureSensors.h/cpp     # DS18B20 sensor handling
CurrentMonitor.h/cpp         # ADS1115 + SCT-013 current monitoring
LCDDisplay.h/cpp             # 20x4 I2C LCD control
ButtonHandler.h/cpp          # MCP23008 button interface
PumpControl.h/cpp            # Pump control logic
HeaterControl.h/cpp          # Heater control with safety
SinricIntegration.h/cpp      # Alexa voice control
```

**Configuration Files (User must edit):**
```
blynk_auth.h                 # Blynk authentication token
sinric_config.h              # Sinric Pro credentials
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

### 4. FreeRTOS Multitasking Architecture

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

#### Task Synchronization

Tasks coordinate through the shared `SystemState` structure:

1. **Sensor Tasks** → Write sensor readings to state (with mutex)
2. **Control Task** → Read state, make decisions, write commands (with mutex)
3. **Output Tasks** → Read state, update displays/relays (with mutex)

The centralized state management functions in `StateManager.cpp` are thread-safe and can be called from any task.

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

#### Pin Assignments (Config.h)

**Temperature Sensors (1-Wire DS18B20):**
- `TEMP_INLET_PIN` (GPIO 35, Pin 6) - Water inlet temperature
- `TEMP_HEATER_PIN` (GPIO 32, Pin 7) - Heater outlet temperature
- `TEMP_AMBIENT_PIN` (GPIO 33, Pin 8) - Ambient/outdoor temperature

**I2C Bus (shared):**
- `I2C_SDA` (GPIO 21, Pin 33) - Data line
- `I2C_SCL` (GPIO 22, Pin 36) - Clock line

**Control Outputs:**
- `PUMP_ENABLE_PIN` (GPIO 26, Pin 10) - Pump relay
- `PUMP_SPEED_PIN` (GPIO 13, Pin 15) - Speed relay (LOW/HIGH)
- `HEATER_PIN` (GPIO 12, Pin 13) - Heater relay

**I2C Devices:**
- LCD Display: Address `0x27`
- MCP23008 GPIO Expander: Address `0x20`
- ADS1115 ADC: Address `0x48`

#### Important Hardware Constraints

1. **ESP32 Pin Restrictions:**
   - GPIO 0: Boot mode pin - avoid using
   - GPIO 2: Connected to onboard LED
   - GPIO 6-11: Connected to SPI flash - DO NOT USE
   - GPIO 34-39: Input only, no pull-ups

2. **I2C Limitations:**
   - Only GPIOs 21/22 are standard I2C
   - All I2C devices share the same bus
   - Each device must have unique address

3. **1-Wire Sensors:**
   - Each sensor needs 4.7kΩ pull-up resistor
   - Can be on same or different GPIOs
   - Current configuration uses separate pins

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
```

**Important:** If you add virtual pins, update `BLYNK_WRITE()` handlers in PoolController3.ino

#### Sinric Pro (Alexa)
- Thermostat device: Pool Heater (temperature control)
- Dimmer Switch device: Pool Pump (speed control via percentage)
- Credentials in `sinric_config.h`

### 8. Common Development Tasks

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

### 14. WiFi and Network

#### WiFi Provisioning
- Uses **BLE provisioning** via ESP BLE Provisioning app
- Credentials stored in flash, persist across reboots
- Service name: `POOL_CTRL`
- Proof of possession: `pool1234`

#### Connectivity Handling
```cpp
state.wifiConnected   // WiFi status
state.blynkConnected  // Blynk status
state.sinricConnected // Sinric status
```

Code should handle **offline mode gracefully** - local controls always work.

#### NTP Time Sync
```cpp
configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
```
Used for operating hours enforcement.

### 15. Version Control and Git

Current branch: `claude/claude-md-miu9qz78g82t3q4r-018b8tPRfz77RkdRvHbJHaxB`

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

#### ESP32 Specific
- **GPIO 6-11 are unusable** (connected to flash)
- **Brownout detector** - ensure adequate power supply (5V 2A minimum)
- **WiFi power management** - can cause disconnects, handled by auto-reconnect
- **Watchdog timer** - loop() must not block too long (hence delay(10) at end)

#### Blynk Specific
- **Blynk.run() must be called frequently** (every loop iteration)
- **Virtual pins are strings** - use `.asInt()`, `.asFloat()`, `.asString()`
- **Avoid writing to same pin too fast** - rate limit updates

#### I2C Issues
- **Pull-up resistors** may be needed (usually on modules already)
- **Wire.begin() must be called before I2C access**
- **Address conflicts** - ensure each device has unique address
- **Bus capacitance** - long wires can cause issues

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
3. **No hardcoding** - all constants in Config.h
4. **Modular design** - keep separation of concerns
5. **Test on hardware** - simulator won't catch everything
6. **Document changes** - maintain excellent documentation
7. **Fail gracefully** - handle offline mode and errors

---

**Last Updated:** 2025-12-06
**Project Version:** 1.0
**Claude Assistant:** Ready to help with this codebase!
