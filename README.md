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
├── SettingsManager.h/.cpp      # Flash storage (Preferences)
├── TemperatureSensors.h/.cpp   # DS18B20 sensor handling
├── LCDDisplay.h/.cpp           # 20x4 I2C LCD control
├── ButtonHandler.h/.cpp        # MCP23008 button interface
├── PumpControl.h/.cpp          # Pump control logic
├── HeaterControl.h/.cpp        # Heater control & safety
└── SinricIntegration.h/.cpp    # Alexa/Sinric Pro integration

**Note:** Blynk cloud connectivity is implemented directly in PoolController3.ino
```

## Quick Start Guide

### 1. Hardware Setup

**Required Components:**
- ESP32-WROOM 38-pin development board
- 3x DS18B20 temperature sensors with 4.7kΩ pull-ups
- MCP23008 I2C GPIO expander
- 20x4 I2C LCD display (address 0x27 or 0x3F)
- 2-channel relay module for pump
- 1-channel relay or SSR for heater
- 5 momentary push buttons

**Pin Connections:**
```
Temperature Sensors:
- GPIO 35 (Pin 6):  Inlet temperature (DS18B20)
- GPIO 32 (Pin 7):  Heater outlet temperature (DS18B20)
- GPIO 33 (Pin 8):  Ambient/outdoor temperature (DS18B20)

I2C Bus (shared):
- GPIO 21 (Pin 25): SDA (LCD + MCP23008)
- GPIO 22 (Pin 22): SCL (LCD + MCP23008)

Control Outputs:
- GPIO 26 (Pin 10): Pump enable relay
- GPIO 13 (Pin 15): Pump speed relay (LOW=low, HIGH=high)
- GPIO 12 (Pin 13): Heater relay

MCP23008 Address: 0x20
- Connect A0, A1, A2 to GND
- Connect RESET to 3.3V
- Buttons connect GP pins to GND (internal pull-ups)
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
2. Select board: Tools → Board → ESP32 Dev Module
3. Select port: Tools → Port → (your COM port)
4. Click Upload
5. Open Serial Monitor (115200 baud)
6. Download "ESP BLE Provisioning" app (Android/iOS)
7. In app:
   - Scan for "POOL_CTRL"
   - Enter proof-of-possession: `pool1234`
   - Enter your WiFi credentials
8. ESP32 will connect and save credentials

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

## Troubleshooting

### I2C Device Not Found

Run this I2C scanner sketch to verify addresses:
```cpp
#include <Wire.h>
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
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

Set baud rate to **115200** to view:
- System initialization
- Temperature readings
- Button presses
- Alexa commands
- Blynk updates
- Control decisions
- Safety interlock triggers
- Connection status
- Error messages

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
2. Verify all I2C devices detected
3. Confirm WiFi and cloud connections
4. Review error messages in serial output
5. Test each subsystem independently

## Version History

**v1.0** - Initial multi-file implementation
- Modular architecture
- Full safety interlocks
- Triple control interface (buttons/Blynk/Alexa)
- Persistent settings
- Emergency shutdown capability
