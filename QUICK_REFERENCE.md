# ESP32 Pool Controller - Quick Reference Card

## Button Functions (MCP23008)

| Button | Function | Special Action |
|--------|----------|----------------|
| **1** (GP0) | Toggle heater ON/OFF | - |
| **2** (GP1) | Cycle pump: OFF → LOW → HIGH → OFF | Hold 10s = Emergency Shutdown |
| **3** (GP2) | Increase target temp (+1°F) | - |
| **4** (GP3) | Decrease target temp (-1°F) | - |
| **5** (GP4) | (unused for short press) | Hold 5s = Factory Reset |

## LCD Display (20x4)

```
Line 1: Inlet: 78.5F   Tgt:80F
Line 2: Heater:79.2F
Line 3: Pump: LOW   5.2A
Line 4: Heater: ON      CLOUD
```

**Pump States:** OFF, LOW, HIGH, COOLDOWN
**Heater States:** OFF, ENABLED, ON
**Connection:** WiFi, CLOUD (Blynk connected)
**Current Display:** Shows pump current in amps (if sensor connected)

## Alexa Voice Commands

### Heater Commands
```
"Alexa, turn on the pool heater"
"Alexa, turn off the pool heater"
"Alexa, set pool heater to 80 degrees"
"Alexa, increase pool heater by 3 degrees"
"Alexa, decrease pool heater by 2 degrees"
"Alexa, what's the pool heater temperature?"
"Alexa, what's the pool heater set to?"
```

### Pump Commands
```
"Alexa, turn on the pool pump"
"Alexa, turn off the pool pump"
"Alexa, set pool pump to 25 percent"     (LOW speed)
"Alexa, set pool pump to 75 percent"     (HIGH speed)
"Alexa, set pool pump to 100 percent"    (HIGH speed)
```

## Safety Interlocks

✓ **Pump MUST run when heater is ON**
- Automatically starts pump if needed
- Cannot manually turn off pump while heating

✓ **15-minute cooldown after heater stops**
- Pump runs at minimum LOW speed
- Ends after 15 min OR when temps equalize (±2°F)

✓ **Temperature sensor validation**
- Heater disabled if sensor disconnected
- Auto-detection and alerts

✓ **Pump current monitoring** (if ADS1115 + SCT-013 installed)
- Real-time current measurement (0.5-30A range)
- Automatic pump failure detection (relay on but no current)
- Overcurrent alerting (>25A threshold)
- Alerts sent to Blynk dashboard

✓ **Operating hours enforcement**
- Heater only runs within set hours (default 6 AM - 10 PM)

✓ **Temperature limits**
- Target range: 60-104°F
- Invalid readings prevent operation

## Blynk Virtual Pins

| Pin | Function | Type | Range |
|-----|----------|------|-------|
| V0 | Inlet temperature | Display | Read-only |
| V1 | Heater temperature | Display | Read-only |
| V2 | Target temperature | Slider | 60-104°F |
| V3 | Heater enable | Switch | ON/OFF |
| V4 | Pump enable | Switch | ON/OFF |
| V5 | Pump speed | Switch | LOW/HIGH |
| V6 | Max runtime/day | Slider | 0-24 hrs |
| V7 | Heater start hour | Slider | 0-23 |
| V8 | Heater end hour | Slider | 0-23 |
| V9 | Today's runtime | Display | Hours |
| V10 | Ambient temperature | Display | Read-only |
| V11 | Pump current | Display | 0-30A |
| V12 | Pump failure alert | LED | 0=OK, 1=FAIL |

## System Behavior

### Normal Heating Cycle

1. User enables heater (button/Blynk/Alexa)
2. System checks all conditions
3. If inlet temp < target, heater turns ON
4. Pump forced to LOW minimum
5. Water heats up
6. Inlet reaches target, heater turns OFF
7. 15-minute cooldown begins
8. Pump continues at LOW minimum
9. Cooldown ends (time or temp equalized)
10. Pump returns to user's setting

### Emergency Shutdown

**Trigger:** Hold pump button (Button 2) for 10 seconds

**Actions:**
- Immediately turns OFF heater and pump (hardware)
- Disables both in software
- Saves disabled state to flash
- Cancels any active cooldown
- Updates all interfaces (LCD, Blynk, Alexa)
- Shows "EMERGENCY SHUTDOWN!" on LCD

**Recovery:** Press any button or use Blynk/Alexa to re-enable

### Factory Reset

**Trigger:** Hold reset button (Button 5) for 5 seconds

**Actions:**
- Clears WiFi credentials
- Clears all saved settings
- Resets to defaults
- Shows "FACTORY RESET!" on LCD
- Automatically restarts ESP32

**After Reset:**
- Use ESP BLE Prov app to reconfigure WiFi
- Reconfigure Blynk and Sinric Pro credentials
- Re-enter your preferences

## Pin Connections

**Temperature Sensors (DS18B20):**
- Pin 6  / GPIO 35 = Inlet temperature
- Pin 7  / GPIO 32 = Heater outlet temperature
- Pin 8  / GPIO 33 = Ambient/outdoor temperature
- Each needs 4.7kΩ pull-up resistor (data to VCC)

**I2C Bus (shared):**
- Pin 33 / GPIO 21 = SDA
- Pin 36 / GPIO 22 = SCL
- LCD address: 0x27 or 0x3F
- MCP23008 address: 0x20

**Control Outputs:**
- Pin 10 / GPIO 26 = Pump enable relay
- Pin 15 / GPIO 13 = Pump speed relay (LOW=low, HIGH=high)
- Pin 13 / GPIO 12 = Heater relay

**Power:**
- Pin 19 = 5V via USB or Vin pin
- Pin 1  = 3.3V output (max 600mA)
- Pin 14, 20, 26 = GND
- Logic: 3.3V

## Serial Monitor Commands

**Baud Rate:** 115200

**Messages to watch for:**
```
"Temperature sensors initialized"
"MCP23008 found at address 0x20"
"WiFi connected - IP: xxx.xxx.xxx.xxx"
"Blynk connected!"
"Inlet: 78.5°F  Heater: 79.2°F"
"Button: Heater toggle"
"Heater status changed: ON"
"Starting 15-minute cooldown period"
```

**Error messages:**
```
"ERROR: MCP23008 not found!"
"WARNING: Inlet sensor disconnected"
"WARNING: Temperature out of safe range"
"Cannot turn pump OFF (safety interlock)"
```

## Default Settings

- Target temperature: 78°F
- Heater operating hours: 6 AM - 10 PM
- Max pump runtime: 8 hours/day
- Timezone: EST (GMT-5)
- WiFi device name: POOL_CTRL
- WiFi proof-of-possession: pool1234

## Customization

Edit `Config.h` to change:
- Pin assignments
- I2C addresses
- Default settings
- Temperature limits
- Timing intervals
- Timezone offset
- NTP servers

## Troubleshooting Quick Fixes

**LCD blank:**
- Check I2C address (try 0x3F if 0x27 doesn't work)
- Verify I2C wiring (SDA/SCL)
- Adjust contrast pot on back of LCD

**Buttons not working:**
- Verify MCP23008 detected at 0x20
- Check I2C wiring
- Ensure A0, A1, A2 connected to GND
- Verify buttons connect GP pins to GND

**Temperature always "--.-F":**
- Check 4.7kΩ pull-up resistor
- Verify sensor power and ground
- Check GPIO pin connections
- Ensure sensor is DS18B20 (not DHT or other type)

**Blynk not updating:**
- Verify WiFi connected first
- Check auth token in blynk_auth.h
- Ensure template ID correct
- Create device in Blynk dashboard

**Alexa can't find devices:**
- Verify Sinric Pro credentials
- Check Alexa account linked in Sinric Pro
- Ensure WiFi connected
- Try "Alexa, discover devices"

## Maintenance

**Daily:**
- Check LCD display for normal operation
- Verify pump runtime tracking

**Weekly:**
- Review Serial Monitor logs
- Check temperature sensor readings
- Test button functions

**Monthly:**
- Test emergency shutdown
- Verify all safety interlocks
- Test Blynk and Alexa integration
- Check relay operation

**Before Pool Season:**
- Test full heating cycle
- Verify cooldown logic
- Check all wiring connections
- Test with actual pool equipment

## Support Files

- **README.md** - Complete documentation
- **INSTALL.md** - Detailed installation guide
- **Config.h** - All configuration options
- **blynk_auth.h** - Blynk credentials (EDIT THIS)
- **sinric_config.h** - Alexa credentials (EDIT THIS)

## Version

**v1.0** - Multi-file modular implementation
- Released: December 2024
- Platform: ESP32-WROOM
- Arduino IDE compatible
