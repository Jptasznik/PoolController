# ESP32 Pool Controller - Multi-File Project

Complete pool automation system with temperature control, safety interlocks, and cloud connectivity.

## Project Structure

```
PoolController/
├── PoolController.ino          # Main sketch file (setup & loop)
├── Config.h                     # All pin definitions and constants
├── blynk_auth.h                # Blynk authentication (YOU MUST EDIT)
├── sinric_config.h             # Sinric Pro credentials (YOU MUST EDIT)
│
├── StateManager.h/.cpp         # Centralized state management
├── StateMutex.h/.cpp           # Thread-safe state access
├── I2CMutex.h/.cpp             # Thread-safe I2C bus access
├── TaskManager.h/.cpp          # FreeRTOS task management
├── MessageQueue.h/.cpp         # FreeRTOS message queues for inter-task communication
├── Logger.h/.cpp               # Structured logging with NVS persistence
├── SettingsManager.h/.cpp      # Flash storage (Preferences)
├── CredentialsManager.h/.cpp   # Secure credential storage
├── TemperatureSensors.h/.cpp   # DS18B20 sensor handling
├── CurrentMonitor.h/.cpp       # ADS1115 + SCT-013 current monitoring
├── LCDDisplay.h/.cpp           # 20x4 I2C LCD control
├── ButtonHandler.h/.cpp        # MCP23008 button interface
├── PumpControl.h/.cpp          # Pump control logic
├── HeaterControl.h/.cpp        # Heater control & safety
└── SinricIntegration.h/.cpp    # Alexa/Sinric Pro integration

**Note:** Blynk cloud connectivity is implemented directly in PoolController.ino

**Architecture:** This system uses FreeRTOS multitasking with 6 concurrent tasks communicating via message queues, each with its own watchdog timer. All I2C and state access is protected by mutexes for thread safety. The system features comprehensive structured logging with NVS persistence, event-driven inter-task communication, and remote control capabilities.
```

## Quick Start Guide

### 1. Hardware Setup

> **Board choice:** This project supports both the original **ESP32-WROOM-32**
> and **ESP32-S3 (N16R8)**. Pin numbers below are for ESP32-WROOM-32 — if
> you're on an ESP32-S3, several pins differ (GPIO 22 doesn't exist on S3,
> and GPIO 26-37 are reserved for flash/PSRAM). See
> [Supporting ESP32-S3 Boards](#supporting-esp32-s3-boards) for the S3 pin
> map and the `TARGET_BOARD` switch in `Config.h` before wiring.

**Required Components:**
- ESP32-WROOM 38-pin development board (or ESP32-S3 N16R8 — see board choice note above)
- 3x DS18B20 temperature sensors with 4.7kΩ pull-ups
- SCT-013-030 current transformer (30A max) for pump monitoring
- ADS1115 16-bit ADC module (I2C)
- MCP23008 I2C GPIO expander
- 20x4 I2C LCD display (address 0x27 or 0x3F)
- 2-channel relay module for pump
- 1-channel relay or SSR for heater
- 5 momentary push buttons
- 62Ω burden resistor and voltage divider circuit for SCT-013

**Optional Components (for hardware relay interlock):**
- SN74LS08N quad AND gate (14-pin DIP)
- SN74LS04N hex inverter (14-pin DIP)
- 2x 0.1µF ceramic capacitors
- See `RELAY_INTERLOCK_LOGIC.md` for complete details

**Pin Connections:**
```
Temperature Sensors:
- GPIO 35 (Pin 6):  Inlet temperature (DS18B20)
- GPIO 32 (Pin 7):  Heater outlet temperature (DS18B20)
- GPIO 33 (Pin 8):  Ambient/outdoor temperature (DS18B20)

I2C Bus (shared) - **Use Blue wire for SDA, Yellow wire for SCL:**
- GPIO 21 (Pin 33): SDA / **Blue** (LCD + MCP23008 + ADS1115)
- GPIO 22 (Pin 36): SCL / **Yellow** (LCD + MCP23008 + ADS1115)

Control Outputs:
- GPIO 26 (Pin 10): Pump enable relay
- GPIO 13 (Pin 15): Pump speed relay (LOW=low, HIGH=high)
- GPIO 12 (Pin 13): Heater relay

I2C Device Addresses (all share Blue/SDA and Yellow/SCL wires):
- LCD Display: 0x27 (or 0x3F)
- MCP23008: 0x20 (A0, A1, A2 to GND; RESET to 3.3V)
- ADS1115: 0x48 (default address)

MCP23008 Buttons:
- Buttons connect GP pins to GND (internal pull-ups)

Current Sensor (SCT-013-030):
- Connect to ADS1115 channel A0
- Requires 62Ω burden resistor
- Voltage divider to bias to 1.65V (VDD/2)
- Clamp one wire of pump power through CT
```

### 2. Arduino IDE Setup

**Install ESP32 Board Support:**
1. File → Preferences
2. Add to "Additional Board Manager URLs":
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Tools → Board → Board Manager
4. Search "ESP32" and install

**Install Required Libraries:**
Go to Sketch → Include Library → Manage Libraries, search and install:
1. `Blynk` by Volodymyr Shymanskyy
2. `DallasTemperature` by Miles Burton
3. `OneWire` by Paul Stoffregen
4. `LiquidCrystal I2C` by Marco Schwartz (or search "ESP32 LiquidCrystal")
   - **Note:** If you see AVR architecture warning, try "LiquidCrystal I2C" by Frank de Brabander v1.1.2+
5. `SinricPro` by Boris Jaeger
6. `Adafruit MCP23008 library` by Adafruit
7. `Adafruit ADS1X15` by Adafruit (for ADS1115 ADC)

### 3. Configure Credentials

**Edit `blynk_auth.h`:**
```cpp
char auth[] = "YOUR_ACTUAL_BLYNK_TOKEN_HERE";
```

**Get Blynk Token:**
1. Go to https://blynk.cloud
2. Create new template with ID: `TMPL2oQXCsxt1`
3. Create device from template
4. Copy auth token

**Edit `sinric_config.h`:**
```cpp
#define SINRIC_APP_KEY "your_app_key"
#define SINRIC_APP_SECRET "your_app_secret"
#define SINRIC_THERMOSTAT_ID "your_thermostat_id"
#define SINRIC_PUMP_ID "your_dimswitch_id"
```

**Get Sinric Pro Credentials:**
1. Go to https://sinric.pro
2. Link Amazon Alexa account
3. Create "Pool Heater" - Type: Thermostat
4. Create "Pool Pump" - Type: Dimmer Switch
5. Copy all IDs and keys

### 4. Upload and Configure WiFi

1. Connect ESP32 to computer via USB
2. Set `TARGET_BOARD` in `Config.h` to match your hardware (see [Supporting ESP32-S3 Boards](#supporting-esp32-s3-boards) if using ESP32-S3)
3. Select board: Tools → Board → **ESP32 Dev Module** (ESP32-WROOM-32) or **ESP32S3 Dev Module** (ESP32-S3) — must match `TARGET_BOARD` above
4. Select port: Tools → Port → (your COM port)
5. Click Upload
6. Open Serial Monitor (115200 baud)
7. Download "ESP BLE Provisioning" app (Android/iOS)
8. In app:
   - Scan for "POOL_CTRL"
   - Enter proof-of-possession: `pool1234`
   - Enter your WiFi credentials
9. ESP32 will connect and save credentials

## Button Functions

**Button 1 (GP0):** Toggle heater enable/disable  
**Button 2 (GP1):** Cycle pump (OFF → LOW → HIGH → OFF)  
- **Hold 10 seconds:** Emergency shutdown (both off)  
**Button 3 (GP2):** Increase target temperature (+1°F)  
**Button 4 (GP3):** Decrease target temperature (-1°F)  
**Button 5 (GP4):** Hold 5 seconds for factory reset

## Safety Features

### Critical Safety Interlocks

1. **Pump MUST run when heater is ON**
   - Automatically starts pump if heater turns on
   - User cannot turn off pump while heater is running
   - Enforced every control cycle

2. **15-minute cooldown after heater turns off**
   - Pump stays on at minimum LOW speed
   - Ends when 15 minutes elapsed OR temperatures equalize (within 2°F)

3. **Temperature sensor validation**
   - Heater disabled if sensor disconnected
   - Automatic detection and error reporting

4. **Operating hours enforcement**
   - Heater only runs within configured time window
   - Default: 6 AM - 10 PM

5. **Temperature limits**
   - Target range: 60-104°F
   - Automatic bounds checking

6. **Pump current monitoring** (SCT-013-030 + ADS1115)
   - Real-time monitoring of pump current draw
   - Automatic detection of pump failure (should be running but no current)
   - Overcurrent detection and alerting
   - Current readings displayed on LCD and Blynk dashboard

7. **Optional hardware relay interlock** (see `RELAY_INTERLOCK_LOGIC.md`)
   - Add SN74LS08N and SN74LS04N logic gates for physical mutual exclusion
   - Ensures only one pump relay can be active at a time
   - Hardware-enforced safety independent of software

## Alexa Voice Commands

**Heater Control:**
```
"Alexa, turn on the pool heater"
"Alexa, turn off the pool heater"
"Alexa, set pool heater to 80 degrees"
"Alexa, increase pool heater by 2 degrees"
"Alexa, what's the pool heater temperature?"
```

**Pump Control:**
```
"Alexa, turn on the pool pump"
"Alexa, turn off the pool pump"
"Alexa, set pool pump to 25 percent"    (LOW speed)
"Alexa, set pool pump to 75 percent"    (HIGH speed)
```

## Blynk Virtual Pins

Configure these in your Blynk template:

- **V0:** Inlet temperature (display, read-only)
- **V1:** Heater outlet temperature (display, read-only)
- **V2:** Target temperature (slider, 60-104°F)
- **V3:** Heater enable/disable (switch)
- **V4:** Pump enable/disable (switch)
- **V5:** Pump speed (switch, LOW/HIGH)
- **V6:** Max pump runtime per day (slider, 0-24 hours)
- **V7:** Heater start hour (slider, 0-23)
- **V8:** Heater end hour (slider, 0-23)
- **V9:** Today's pump runtime (display, hours, read-only)
- **V10:** Ambient temperature (display, read-only)
- **V11:** Pump current (display, amps, read-only)
- **V12:** Pump failure alert (LED indicator, read-only)
- **V13:** Minimum heater temperature (slider, 32-104°F)
- **V14:** Maximum heater temperature (slider, 32-104°F)
- **V15:** Log level (slider, 0-3: DEBUG/INFO/WARN/ERROR)
- **V16:** Log buffer size (slider, 12-100 entries)

## LCD Display Layout

```
Line 1: Inlet: 78.5F   Tgt:80F
Line 2: Heater:79.2F
Line 3: Pump: LOW
Line 4: Heater: ON      CLOUD
```

Status indicators:
- Pump: OFF / LOW / HIGH / COOLDOWN
- Heater: OFF / ENABLED / ON
- Connection: WiFi / CLOUD (Blynk)

## Supporting ESP32-S3 Boards

Starting in **v1.3**, this project supports both the original **ESP32-WROOM-32**
and the **ESP32-S3 (N16R8: 16MB flash, 8MB Octal PSRAM)**, selected with a
single toggle in `Config.h` — no code changes needed elsewhere, only rewiring
the pins that differ.

### Why a toggle is needed

The ESP32-S3 is not pin-compatible with the original ESP32:
- **GPIO 22-25 don't exist on the ESP32-S3 chip at all.** The original
  board's I2C clock line (`I2C_SCL = GPIO 22`) is invalid on S3 — leaving it
  unchanged causes the firmware to hang silently right after mutex
  initialization, with no error printed, because it hangs inside
  `Wire.begin()` before the next log line runs.
- **GPIO 26-37 are reserved internally for the flash/PSRAM SPI interface**
  on Octal-PSRAM S3 modules like the N16R8 — a much wider reserved range
  than the original ESP32's GPIO 6-11. Any signal wired to a pin in that
  range must move.

### Selecting your board

Edit the very top of `Config.h`:

```cpp
#define BOARD_ESP32_WROOM      1   // Original ESP32-WROOM-32 38-pin dev board
#define BOARD_ESP32S3_N16R8    2   // ESP32-S3 N16R8 (16MB flash, 8MB Octal PSRAM)

#define TARGET_BOARD BOARD_ESP32_WROOM
// #define TARGET_BOARD BOARD_ESP32S3_N16R8
```

Comment out the line for the board you're *not* using, and uncomment the one
you are. All pin `#define`s further down in `Config.h` are wrapped in
`#if TARGET_BOARD == ...` blocks and switch automatically — nothing else in
the codebase needs to change. Then in Arduino IDE, select **Tools → Board →
ESP32 Dev Module** (WROOM) or **ESP32S3 Dev Module** (S3) to match.

### Pin differences (ESP32-WROOM-32 → ESP32-S3 N16R8)

| Signal | WROOM-32 pin | S3 N16R8 pin | Why it moved |
|---|---|---|---|
| `I2C_SCL` | GPIO 22 | **GPIO 14** | GPIO 22 doesn't exist on ESP32-S3 |
| `PUMP_ENABLE_PIN` | GPIO 26 | **GPIO 4** | Reserved for Octal PSRAM/flash on S3 |
| `TEMP_HEATER_PIN` | GPIO 32 | **GPIO 5** | Reserved for Octal PSRAM/flash on S3 |
| `TEMP_AMBIENT_PIN` | GPIO 33 | **GPIO 6** | Reserved for Octal PSRAM/flash on S3 |
| `TEMP_INLET_PIN` | GPIO 35 | **GPIO 7** | Reserved for Octal PSRAM/flash on S3 |
| `I2C_SDA` | GPIO 21 | GPIO 21 (unchanged) | Valid and free on both boards |
| `PUMP_SPEED_PIN` | GPIO 13 | GPIO 13 (unchanged) | Valid and free on both boards |
| `HEATER_PIN` | GPIO 12 | GPIO 12 (unchanged) | Valid and free on both boards |

Everything else — I2C device addresses (`0x20`/`0x27`/`0x48`), timing
constants, feature flags, and all cloud credentials — is unaffected by
`TARGET_BOARD` and works identically on both boards.

### Other ESP32-S3 GPIOs to avoid if extending the pin map

If you add a new sensor or output and need another free GPIO on the S3
N16R8, avoid these in addition to GPIO 22-25 and GPIO 26-37 above:
- **GPIO 0, 3, 45, 46** — strapping pins (boot mode / voltage select); risky
  for outputs that must not glitch during boot, like a relay
- **GPIO 19, 20** — native USB D-/D+
- **GPIO 43, 44** — default UART0 TX/RX (serial monitor / flashing on most
  S3 DevKitC boards)
- **GPIO 46** — input-only

### Using a Quad-PSRAM S3 module instead of N16R8

The N16R8 uses **Octal** PSRAM, which reserves GPIO 26-37. A **Quad**-PSRAM
S3 module (not covered by `BOARD_ESP32S3_N16R8`) only reserves GPIO 26-32 —
GPIO 33-37 would be free GPIO on that variant. Add a third `BOARD_*` option
in `Config.h` rather than reusing `BOARD_ESP32S3_N16R8` if you're on a
Quad-PSRAM module, so you don't give up 5 usable pins unnecessarily.

## Troubleshooting

### I2C Device Not Found

First verify your wiring matches `I2C_SDA`/`I2C_SCL` in `Config.h` for your selected `TARGET_BOARD`:
- **ESP32-WROOM:** Blue wire on GPIO 21 (SDA), Yellow wire on GPIO 22 (SCL)
- **ESP32-S3 (N16R8):** Blue wire on GPIO 21 (SDA), Yellow wire on **GPIO 14** (SCL) — GPIO 22 does not exist on ESP32-S3, see [Supporting ESP32-S3 Boards](#supporting-esp32-s3-boards)

Run this I2C scanner sketch to verify addresses (update the `Wire.begin()` pins to match your board above):
```cpp
#include <Wire.h>
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);  // ESP32-WROOM: (21, 22)   ESP32-S3 N16R8: (21, 14)
  Serial.println("Scanning I2C bus...");
  for(byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if(Wire.endTransmission() == 0) {
      Serial.print("Device found at 0x");
      Serial.println(addr, HEX);
    }
  }
}
void loop() {}
```

Expected devices:
- 0x20 - MCP23008
- 0x27 or 0x3F - LCD

### I2C Mutex Timeouts / Laggy Button Response

If Serial Monitor shows repeated `[WRN] [I2C] I2C mutex lock timeout!` /
`[ERR] [I2C] Failed to acquire I2C lock!`, and physical buttons feel slow
to respond, this was a known issue fixed in **v1.3** — make sure you're on
the latest firmware (see [Version History](#version-history)). It was caused
by the LCD task holding the shared I2C bus for its entire ~80-character
screen redraw once per second, starving the button-scan task of bus access.
The fix runs I2C at 400kHz and locks/unlocks the bus once per LCD line
instead of once for the whole screen.

If you still see this after updating, also double-check whether you've set
any `ENABLE_*` flag in `Config.h` to `0` expecting it to disable that
task/feature — confirm you're running v1.3+, since versions before it had a
bug where `0` didn't actually disable anything (see Version History).

### Temperature Sensor Not Detected

Check:
- 4.7kΩ pull-up resistor between data and VCC
- Correct GPIO pin (25 for inlet, 26 for heater)
- Sensor power (red to 3.3V, black to GND)
- Serial monitor for "devices found" count

### WiFi Won't Connect

1. Factory reset: Hold button 5 for 5 seconds
2. Re-provision using ESP BLE Prov app
3. Check WiFi credentials
4. Ensure 2.4GHz network (ESP32 doesn't support 5GHz)

### Blynk Not Connecting

1. Verify auth token in `blynk_auth.h`
2. Check template ID matches: `TMPL2oQXCsxt1`
3. Ensure WiFi connected first
4. Check Serial Monitor for connection messages

### Alexa Not Responding

1. Verify Sinric Pro credentials in `sinric_config.h`
2. Check device IDs are correct
3. Ensure Alexa account linked in Sinric Pro dashboard
4. Try: "Alexa, discover devices"

## Settings Persistence

All settings automatically save to ESP32 flash memory:
- Target temperature
- Heater enabled state
- Pump state
- Operating hours
- Max runtime
- WiFi credentials

Settings survive:
- Power cycles
- Reboots
- Code updates (unless flash is erased)

Factory reset clears all settings.

## Serial Monitor Output

Set baud rate to **115200** to view structured logging output:

**Log Format:** `[timestamp] [LEVEL] [MODULE] Message`

**Example Output:**
```
[1234] [INF] [MAIN] ESP32 Pool Controller
[1456] [INF] [SETTINGS] Loading settings from flash...
[2345] [INF] [TEMP] Initializing temperature sensors...
[5678] [DBG] [TEMP] Inlet: 72.5°F  Heater: 75.2°F
[8901] [INF] [STATE] Target temperature: 78.0°F
[9012] [WRN] [HEATER] SAFETY: Starting pump (LOW) because heater is ON
```

**Log Levels:**
- `[DBG]` - Debug: Detailed diagnostic information
- `[INF]` - Info: General system information
- `[WRN]` - Warning: Potential issues or safety triggers
- `[ERR]` - Error: Serious problems requiring attention

**Module Names:**
- MAIN, TASKS, STATE, SETTINGS, TEMP, CURRENT, LCD, BUTTONS, PUMP, HEATER, SINRIC

### Remote Log Level Control

You can adjust the log level remotely via the **Blynk dashboard** using virtual pin **V15**:

- **0** = DEBUG (show all messages including detailed diagnostics)
- **1** = INFO (show general system information and above)
- **2** = WARN (show only warnings and errors)
- **3** = ERROR (show only errors)

The log level setting is saved to flash memory and persists across reboots.

### Persistent Log Buffer

The system maintains a **configurable circular log buffer** in ESP32 flash memory:
- **Adjustable buffer size** via Blynk V16 (12-100 entries, default 12)
- Buffer size range: ~1KB (12 entries) to ~8KB (100 entries)
- Stores recent log entries (INFO level and above)
- Persists across reboots and power cycles
- Automatically overwrites oldest entries when full
- Only INFO, WARN, and ERROR messages stored (DEBUG excluded to reduce flash wear)
- Buffer size setting is saved to flash and persists across reboots

This buffer can be useful for debugging issues that occur when Serial Monitor is not connected. Increase the buffer size via Blynk to capture more historical log data.

## Customization

**Change timezone:**
Edit in `Config.h`:
```cpp
#define GMT_OFFSET_SEC -18000      // Seconds from GMT
#define DAYLIGHT_OFFSET_SEC 3600   // DST offset
```

**Change LCD I2C address:**
Edit in `Config.h`:
```cpp
#define LCD_ADDRESS 0x27  // or 0x3F
```

**Change default settings:**
Edit in `Config.h`:
```cpp
#define DEFAULT_TARGET_TEMP 78.0
#define DEFAULT_MAX_RUNTIME_HOURS 8.0
#define DEFAULT_HEATER_START_HOUR 6
#define DEFAULT_HEATER_END_HOUR 22
```

## License

This project is provided as-is for educational and personal use.

## Safety Warning

⚠️ **ELECTRICAL SAFETY WARNING** ⚠️

This system controls high-voltage equipment (pumps and heaters). Installation must be performed by a qualified electrician following all local electrical codes. Improper installation can result in:
- Electric shock
- Fire hazard
- Equipment damage
- Personal injury

Always:
- Use properly rated relays/SSRs
- Include appropriate circuit breakers
- Follow GFCI requirements for wet locations
- Test all safety interlocks before deployment
- Never bypass safety features

## Support

For issues or questions:
1. Check Serial Monitor output at 115200 baud
2. Verify all I2C devices detected (check Blue/Yellow wiring if not)
3. Confirm WiFi and cloud connections
4. Review error messages in serial output
5. Test each subsystem independently

## Version History

**v1.3** - Feature-Flag Fix, ESP32-S3 Support, I2C Reliability (September 2026)
- ✅ **Fixed feature-flag bug:** `ENABLE_TEMPERATURE_SENSORS`, `ENABLE_CURRENT_MONITOR`,
  `ENABLE_LCD`, `ENABLE_BUTTONS`, `ENABLE_PUMP`, `ENABLE_HEATER`, `ENABLE_BLYNK`, and
  `ENABLE_SINRIC` were previously checked with `#ifdef`, which only tests whether a
  symbol is *defined* — not its value — so setting a flag to `0` never actually
  disabled that feature's task. All guards now correctly use `#if`.
- ✅ **Fixed I2C mutex timeouts and laggy button response:** I2C bus raised to
  400kHz (Fast Mode), and the LCD's full-screen redraw now locks/unlocks the
  I2C bus once per line instead of holding it for the entire ~80-character
  redraw — this was starving the button-scan task of bus access once per
  second. See [I2C Mutex Timeouts / Laggy Button Response](#i2c-mutex-timeouts--laggy-button-response).
- ✅ **Added ESP32-S3 (N16R8) support:** New `TARGET_BOARD` toggle in `Config.h`
  switches pin assignments between ESP32-WROOM-32 and ESP32-S3 N16R8 with
  no other code changes. See [Supporting ESP32-S3 Boards](#supporting-esp32-s3-boards).
- ✅ **ESP32 Arduino core 2.x/3.x compatibility:** Watchdog initialization and
  `WiFiProvLite` (WiFi provisioning) now build correctly on both ESP32 Arduino
  core 2.x (ESP-IDF 4.x) and 3.x (ESP-IDF 5.x), instead of requiring 3.x only.
- ✅ **Fixed Blynk-disabled link errors:** `WiFiNetworkTask.cpp`'s calls into
  Blynk sync functions are now correctly guarded by `#if ENABLE_BLYNK`,
  fixing "undefined reference" linker errors when Blynk is disabled.

**v1.2.1** - Production Safety Hardening (December 22, 2025)
- ✅ **Thread-safe state management:** Mutex protection added to all 18 state functions
- ✅ **Production safety:** NO_HARDWARE mode disabled for production deployment
- ✅ **Standardized error handling:** Consistent mutex failure logging and safe defaults
- ✅ **WiFi reliability:** Credential validation timeout restored to 60 seconds
- ✅ **Code quality:** Blocking delays documented, watchdog helper function added
- ✅ **System status:** Production-ready with all critical safety features active

**v1.2** - NVS Persistent Logging (December 2025)
- Configurable circular log buffer in NVS flash (12-100 entries)
- Remote log level control via Blynk (V15)
- Remote buffer size control via Blynk (V16)
- Log settings persist across reboots
- Flash wear protection (INFO+ only stored)

**v1.1** - Structured Logging Framework (December 2025)
- Complete migration to structured logging framework
- Module-based logging with severity levels (DEBUG/INFO/WARN/ERROR)
- Thread-safe logging across all FreeRTOS tasks
- All 11 modules integrated with consistent logging
- Replaced all Serial.println() calls with LOG_* macros

**v1.0** - Initial multi-file implementation
- Modular architecture
- Full safety interlocks
- Triple control interface (buttons/Blynk/Alexa)
- Persistent settings
- Emergency shutdown capability
