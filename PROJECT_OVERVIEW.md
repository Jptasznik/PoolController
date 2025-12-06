# ESP32 Pool Controller - Project Overview

## Project Summary

This is a complete, production-ready pool automation system with professional features including cloud connectivity, voice control, and comprehensive safety interlocks.

**Key Features:**
- ✓ Dual temperature monitoring (inlet + heater outlet)
- ✓ Automatic heater control with thermostat logic
- ✓ Three-speed pump control (OFF/LOW/HIGH)
- ✓ Three SRD-03VDC-SL-C 3.3v logic Relays, powered from the 5v bus (Only 2 on at a time controlled by ICs)
- ✓ MCP23008 I2C button interface (clean, expandable), powered from the 5v bus 
- ✓ 20x4 LCD display with real-time status, powered from the 5v bus 
- ✓ Blynk cloud dashboard
- ✓ Amazon Alexa voice control (via Sinric Pro)
- ✓ WiFi provisioning via BLE
- ✓ Persistent settings in flash
- ✓ Emergency shutdown capability
- ✓ Factory reset capability
- ✓ Comprehensive safety interlocks

## Architecture

**Modular Design:** The code is organized into 11 separate modules for maintainability:

1. **StateManager** - Centralized state management
2. **SettingsManager** - Flash storage persistence  
3. **TemperatureSensors** - DS18B20 sensor handling
4. **LCDDisplay** - 20x4 I2C LCD control
5. **ButtonHandler** - MCP23008 button interface
6. **PumpControl** - Pump logic and safety
7. **HeaterControl** - Heater control with interlocks
8. **SinricIntegration** - Alexa voice control
9. **Config** - All constants and pin definitions
10. **PoolController.ino** - Main coordinator (includes Blynk cloud dashboard)

**Benefits:**
- Easy to understand and modify
- Each module can be tested independently
- Clear separation of concerns
- Scalable for future features

## File Manifest

### Core Arduino Files (Required)
```
PoolController.ino          Main sketch file (7.3 KB)
Config.h                     Pin definitions and constants (3.3 KB)
StateManager.h/.cpp          Centralized state management (10.2 KB)
SettingsManager.h/.cpp       Flash storage (4.5 KB)
TemperatureSensors.h/.cpp    DS18B20 sensors (3.4 KB)
LCDDisplay.h/.cpp            20x4 LCD control (3.0 KB)
ButtonHandler.h/.cpp         MCP23008 buttons (6.0 KB)
PumpControl.h/.cpp           Pump logic (1.8 KB)
HeaterControl.h/.cpp         Heater control (3.7 KB)
SinricIntegration.h/.cpp     Alexa integration (5.4 KB)
```

### Configuration Files (MUST EDIT)
```
blynk_auth.h                 Blynk authentication token
sinric_config.h              Sinric Pro credentials for Alexa
```

### Documentation Files
```
README.md                    Complete documentation (9.2 KB)
INSTALL.md                   Step-by-step installation guide (7.6 KB)
QUICK_REFERENCE.md           Quick reference card (8.1 KB)
WIRING_GUIDE.md              Detailed wiring diagrams (15.2 KB)
ESP32_PIN_REFERENCE.md       Complete ESP32 pin guide with restrictions
PIN_STATUS_SUMMARY.md        Visual pin usage and expansion guide
PHYSICAL_PIN_REFERENCE.md    Physical pin to GPIO mapping reference
PROJECT_OVERVIEW.md          This file
```

**Total:** 26 files, ~70 KB of code and documentation

## Hardware Requirements

**Microcontroller:**
- ESP32-WROOM 38-pin development board

**Sensors:**
- 2× DS18B20 waterproof temperature probes
- 2× 4.7kΩ resistors (pull-ups)

**Display:**
- 20×4 character LCD with I2C interface (PCF8574)

**I/O Expansion:**
- MCP23008 8-bit I2C GPIO expander

**Controls:**
- 5× Momentary push buttons (SPST normally open)

**Output Devices:**
- 1-channel SRD-03VDC-SL-C 3.3v logic relay module (10A+ rated) for High speed pump
- 1-channel SRD-03VDC-SL-C 3.3v logic relay module (10A+ rated) for Low speed pump
- 1-channel SRD-03VDC-SL-C 3.3v logic relay module for heater (switching on 24vac)

**Connectivity:**
- WiFi (built-in to ESP32)
- USB cable for programming

## Software Requirements

**Arduino IDE:**
- ESP32 board support
- 6 required libraries (all available in Library Manager)

**Cloud Services:**
- Blynk account (blynk.cloud) - Free tier sufficient
- Sinric Pro account (sinric.pro) - Free tier sufficient
- Amazon Alexa account (for voice control)

**Mobile Apps:**
- ESP BLE Provisioning (Android/iOS) - WiFi setup
- Blynk IoT (Android/iOS) - Cloud dashboard
- Amazon Alexa (Android/iOS) - Voice control

## Quick Start

1. **Assemble hardware** (see WIRING_GUIDE.md)
2. **Install Arduino IDE** and ESP32 support
3. **Install 6 required libraries** via Library Manager
4. **Edit credentials** in blynk_auth.h and sinric_config.h
5. **Upload code** to ESP32
6. **Provision WiFi** using ESP BLE Prov app
7. **Test system** incrementally
8. **Deploy** in weatherproof enclosure

See **INSTALL.md** for detailed step-by-step instructions.

## Safety Features

### Critical Interlocks

**1. Pump Safety:**
- Pump MUST run (minimum LOW) when heater is ON
- Automatically starts pump if heater activates
- User cannot disable pump while heater is running
- Enforced continuously every control cycle

**2. Cooldown Protection:**
- 15-minute minimum cooldown after heater turns OFF
- Pump continues at LOW minimum during cooldown
- Ends when time elapsed OR temperatures equalize (±2°F)
- User cannot override during cooldown

**3. Sensor Validation:**
- Heater disabled if temperature sensor disconnected
- Automatic invalid reading detection
- Operates only within safe temperature range (32-104°F)

**4. Operating Hours:**
- Heater only operates within configured time window
- Default: 6 AM - 10 PM (user configurable)
- Automatic enforcement via NTP time sync

**5. Emergency Controls:**
- 10-second button hold = Emergency shutdown
- Immediately disables both heater and pump
- Saves disabled state to flash
- Requires manual re-enable

## How It Works

### Normal Heating Cycle

1. User enables heater (button/Blynk/Alexa)
2. System monitors inlet temperature
3. When inlet < target, heater turns ON automatically
4. Pump forced to LOW minimum (safety)
5. Water circulates through heater
6. When inlet ≥ target, heater turns OFF automatically
7. 15-minute cooldown begins
8. Pump continues at LOW minimum
9. Cooldown ends when time or temperature condition met
10. Pump returns to user's preference

### Control Synchronization

All three control methods stay perfectly synchronized:
- **Physical buttons** → Updates Blynk + Alexa + LCD
- **Blynk app** → Updates hardware + Alexa + LCD
- **Alexa voice** → Updates hardware + Blynk + LCD

Achieved through centralized state management functions:
- `setHeaterEnabledState()`
- `setPumpState()`
- `setTargetTemperature()`

Every state change propagates to all interfaces automatically.

## Customization

All configuration centralized in **Config.h:**

**Pin Assignments:**
```cpp
#define TEMP_INLET_PIN 25
#define TEMP_HEATER_PIN 26
#define PUMP_ENABLE_PIN 27
#define PUMP_SPEED_PIN 13
#define HEATER_PIN 12
```

**I2C Addresses:**
```cpp
#define LCD_ADDRESS 0x27
#define MCP23008_ADDRESS 0x20
```

**Temperature Limits:**
```cpp
#define MIN_TARGET_TEMP 60.0
#define MAX_TARGET_TEMP 104.0
```

**Timing:**
```cpp
#define TEMP_READ_INTERVAL 5000        // 5 seconds
#define LCD_UPDATE_INTERVAL 1000       // 1 second
#define COOLDOWN_DURATION 900000       // 15 minutes
```

**Defaults:**
```cpp
#define DEFAULT_TARGET_TEMP 78.0
#define DEFAULT_HEATER_START_HOUR 6
#define DEFAULT_HEATER_END_HOUR 22
```

## Testing Checklist

**Before Deployment:**

Hardware:
- [ ] All I2C devices detected (LCD, MCP23008)
- [ ] Temperature sensors reading correctly
- [ ] All 5 buttons responding
- [ ] LCD displaying data
- [ ] Relays clicking on command

Connectivity:
- [ ] WiFi connected
- [ ] Blynk cloud connected
- [ ] Sinric Pro connected
- [ ] Alexa can discover devices
- [ ] NTP time synchronized

Functionality:
- [ ] Heater turns on when temp < target
- [ ] Heater turns off when temp ≥ target
- [ ] Pump auto-starts with heater
- [ ] Cannot disable pump while heater on
- [ ] 15-minute cooldown works
- [ ] Temperatures equalize triggers cooldown end
- [ ] Settings persist across reboot
- [ ] Emergency shutdown works (10s hold)
- [ ] Factory reset works (5s hold)

Control Methods:
- [ ] Buttons control hardware
- [ ] Blynk controls hardware
- [ ] Alexa controls hardware
- [ ] All methods stay synchronized

Safety:
- [ ] Heater disabled if sensor disconnected
- [ ] Operating hours enforced
- [ ] Temperature limits enforced
- [ ] Runtime limit enforced
- [ ] All interlocks functioning

## Performance Specifications

**Response Times:**
- Button press to action: <100ms
- Blynk command to hardware: <500ms
- Alexa command to hardware: <1s
- Temperature reading cycle: 5s
- Display update: 1s
- Control logic cycle: 60s

**Accuracy:**
- Temperature reading: ±0.5°C (±0.9°F)
- Temperature control: ±1°F hysteresis
- Time keeping: NTP synchronized (±1s)

**Reliability:**
- WiFi auto-reconnect
- Blynk auto-reconnect
- Watchdog protection
- Flash wear leveling
- Button debouncing (50ms)

**Capacity:**
- Daily pump runtime tracking
- Unlimited temperature history (via cloud)
- Settings stored in flash (10,000+ write cycles)

## Troubleshooting

**Common Issues:**

| Symptom | Likely Cause | Solution |
|---------|-------------|----------|
| LCD blank | Wrong I2C address | Try 0x3F instead of 0x27 |
| Buttons not working | MCP23008 not found | Check I2C wiring, verify 0x20 |
| Temp shows "--.-F" | Sensor disconnected | Check pull-up resistor, wiring |
| WiFi won't connect | Wrong credentials | Factory reset, re-provision |
| Blynk offline | Wrong auth token | Verify token in blynk_auth.h |
| Alexa not responding | Sinric not configured | Check credentials, discover devices |

See **README.md** troubleshooting section for comprehensive solutions.

## Expansion Possibilities

The modular architecture makes it easy to add features:

**Already Supported:**
- GP5-GP7 on MCP23008 reserved for 3 more buttons
- Additional I2C devices can share the bus
- More Blynk virtual pins available
- More Sinric Pro devices can be added

**Potential Enhancements:**
- Pool chemical monitoring (pH, ORP, chlorine)
- Water level sensor
- Filter pressure gauge
- Multiple temperature zones
- Solar heating integration
- Energy usage monitoring
- Advanced scheduling
- Weather integration
- Mobile notifications
- Data logging and analytics

## Support and Documentation

**Primary Resources:**
1. **README.md** - Complete system documentation
2. **INSTALL.md** - Step-by-step setup guide
3. **QUICK_REFERENCE.md** - Command reference
4. **WIRING_GUIDE.md** - Detailed wiring diagrams
5. **Serial Monitor** - Real-time system diagnostics (115200 baud)

**External Documentation:**
- ESP32 Arduino Core: https://docs.espressif.com/projects/arduino-esp32/
- Blynk Documentation: https://docs.blynk.io/
- Sinric Pro Docs: https://sinric.pro/de-de/docs/index.html
- Arduino Libraries: Each library has examples and documentation

## License and Warranty

**License:** This project is provided as-is for educational and personal use.

**Warranty:** No warranty expressed or implied. Use at your own risk.

**Safety:** Electrical work must be performed by qualified personnel following all local codes and regulations.

## Credits

**Technologies Used:**
- ESP32 (Espressif Systems)
- Arduino Framework
- Blynk IoT Platform
- Sinric Pro / Amazon Alexa
- Dallas Temperature (DS18B20)
- MCP23008 GPIO Expander
- I2C Protocol
- OneWire Protocol

**Libraries:**
- Blynk by Volodymyr Shymanskyy
- DallasTemperature by Miles Burton
- OneWire by Paul Stoffregen
- LiquidCrystal_I2C by Frank de Brabander
- SinricPro by Boris Jaeger
- Adafruit_MCP23X08 by Adafruit

## Version

**Current Version:** 1.0  
**Release Date:** December 2024  
**Platform:** ESP32-WROOM  
**Framework:** Arduino IDE  

## Getting Started

**Next Steps:**

1. Read **INSTALL.md** for complete setup instructions
2. Review **WIRING_GUIDE.md** for hardware connections
3. Install required libraries (list in INSTALL.md)
4. Edit **blynk_auth.h** with your Blynk token
5. Edit **sinric_config.h** with your Sinric credentials
6. Upload code and test incrementally
7. Deploy in weatherproof enclosure
8. Monitor via Serial Monitor for debugging

**Need Help?**
- Check Serial Monitor output (115200 baud)
- Review troubleshooting in README.md
- Verify all connections per WIRING_GUIDE.md
- Test each subsystem independently

---

**Ready to build your pool controller?**

All files are included in this package. Start with **INSTALL.md** for step-by-step instructions!
