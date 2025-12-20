# Quick Installation Guide

## File Organization

Create a folder named `PoolController` and place ALL these files in it:

```
PoolController/
├── PoolController.ino          ← Main sketch (REQUIRED)
├── Config.h                     ← Pin definitions
├── blynk_auth.h                ← EDIT with your token
├── sinric_config.h             ← EDIT with your credentials
├── StateManager.h
├── StateManager.cpp
├── SettingsManager.h
├── SettingsManager.cpp
├── TemperatureSensors.h
├── TemperatureSensors.cpp
├── CurrentMonitor.h
├── CurrentMonitor.cpp
├── LCDDisplay.h
├── LCDDisplay.cpp
├── ButtonHandler.h
├── ButtonHandler.cpp
├── PumpControl.h
├── PumpControl.cpp
├── HeaterControl.h
├── HeaterControl.cpp
├── SinricIntegration.h
└── SinricIntegration.cpp
```

⚠️ **IMPORTANT:** All files MUST be in the same folder!

## Installation Steps

### 1. Install Arduino IDE
Download from: https://www.arduino.cc/en/software

### 2. Add ESP32 Board Support
- File → Preferences
- Add this URL to "Additional Board Manager URLs":
  ```
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
  ```
- Tools → Board → Boards Manager
- Search "ESP32" → Install "ESP32 by Espressif Systems"

### 3. Install Libraries
Go to: Sketch → Include Library → Manage Libraries

Install these 7 libraries:
1. **Blynk** by Volodymyr Shymanskyy
2. **DallasTemperature** by Miles Burton
3. **OneWire** by Paul Stoffregen
4. **LiquidCrystal I2C** by Marco Schwartz (or search "ESP32 LiquidCrystal")
   - If you see AVR warning, the library will still work, just ignore it
5. **SinricPro** by Boris Jaeger
6. **Adafruit MCP23008 library** by Adafruit
7. **Adafruit ADS1X15** by Adafruit (for ADS1115 current sensor)

### 4. Get Blynk Credentials

1. Go to https://blynk.cloud and create account
2. Create new template:
   - Template ID: `TMPL2oQXCsxt1`
   - Name: "Pool Controller"
3. Add virtual pins (see README.md for full list)
4. Create a device from this template
5. Copy the Auth Token
6. **EDIT `blynk_auth.h`** and paste your token:
   ```cpp
   char auth[] = "YourActualTokenHere";
   ```

### 5. Get Sinric Pro Credentials (for Alexa)

1. Go to https://sinric.pro and create account
2. Connect your Amazon Alexa account
3. Create two devices:
   - Device 1: "Pool Heater" - Type: **Thermostat**
   - Device 2: "Pool Pump" - Type: **Dimmable Switch**
4. Copy:
   - App Key
   - App Secret
   - Pool Heater Device ID
   - Pool Pump Device ID
5. **EDIT `sinric_config.h`** and paste your credentials

### 6. Hardware Connections

**Minimum connections to test:**

Temperature Sensors (DS18B20):
- Red wire → 3.3V (Pin 1)
- Black wire → GND (Pin 14, 32, or 38)
- Yellow wire → GPIO 35 (Pin 6 - inlet) or GPIO 32 (Pin 7 - heater) or GPIO 33 (Pin 8 - ambient)
- 4.7kΩ resistor between yellow and red wire

I2C Devices - **Use Blue wire for SDA, Yellow for SCL:**
- SDA / **Blue** → GPIO 21 (Pin 33)
- SCL / **Yellow** → GPIO 22 (Pin 36)
- VCC → 3.3V (Pin 1) or 5V (Pin 19) depending on device
- GND → GND (Pin 14, 32, or 38)

LCD Display (I2C):
- Shares GPIO 21 (SDA/Blue) and GPIO 22 (SCL/Yellow)
- Address usually 0x27 or 0x3F

MCP23008:
- Shares GPIO 21 (SDA/Blue) and GPIO 22 (SCL/Yellow)
- A0, A1, A2 → GND (sets address to 0x20)
- RESET → 3.3V
- GP0-GP4 → Connect buttons to GND when pressed

Control Relays:
- GPIO 26 (Pin 10) → Pump enable relay
- GPIO 13 (Pin 15) → Pump speed relay
- GPIO 12 (Pin 13) → Heater relay

### 7. Upload Code

1. Connect ESP32 to computer via USB
2. Open `PoolController.ino` in Arduino IDE
3. Select board: **Tools → Board → ESP32 Dev Module**
4. Select port: **Tools → Port → (your COM port)**
5. Click **Upload** button (→)
6. Wait for "Done uploading" message

### 8. Configure WiFi

1. Open Serial Monitor (Tools → Serial Monitor)
2. Set baud rate to **115200**
3. Download "ESP BLE Provisioning" app:
   - **Android:** https://play.google.com/store/apps/details?id=com.espressif.provble
   - **iOS:** https://apps.apple.com/app/esp-ble-provisioning/id1473590141
4. In the app:
   - Tap "Provision New Device"
   - Select "POOL_CTRL"
   - Enter Proof of Possession: `pool1234`
   - Enter your WiFi SSID and password
   - Wait for connection
5. ESP32 saves credentials and auto-connects on future boots

### 9. Test Alexa Integration

1. Open Alexa app on phone
2. Say: "Alexa, discover devices"
3. Wait for discovery to complete
4. You should see:
   - "Pool Heater"
   - "Pool Pump"
5. Test: "Alexa, set pool heater to 80 degrees"

## Verification Checklist

Open Serial Monitor (115200 baud) and verify:

- [ ] "Temperature sensors initialized" - shows device count
- [ ] "MCP23008 found at address 0x20"
- [ ] "LCD display initialized"
- [ ] "WiFi connected - IP: xxx.xxx.xxx.xxx"
- [ ] "Blynk connected!"
- [ ] Temperature readings appear every 5 seconds
- [ ] LCD displays data
- [ ] Button presses detected

## Common Issues

**"MCP23008 not found!"**
- Check I2C wiring: Blue wire to GPIO 21 (SDA), Yellow wire to GPIO 22 (SCL)
- Verify A0, A1, A2 connected to GND
- Run I2C scanner (see README.md)

**"Inlet sensor devices: 0"**
- Check 4.7kΩ pull-up resistor
- Verify sensor wiring
- Try different GPIO pin

**"WiFi connection failed"**
- Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
- Check WiFi credentials
- Move closer to router
- Factory reset and re-provision

**"Blynk disconnected"**
- Verify auth token in `blynk_auth.h`
- Check template ID: `TMPL2oQXCsxt1`
- Ensure device created in Blynk dashboard

## Next Steps

1. Read full **README.md** for detailed information
2. Configure Blynk dashboard with all virtual pins
3. Set up operating hours and temperature limits
4. Test all safety interlocks
5. Install in weatherproof enclosure
6. Have qualified electrician wire to actual pool equipment

## Safety Reminder

⚠️ This system controls high-voltage equipment. Installation must be performed by a qualified electrician following all local codes.

## Need Help?

1. Check Serial Monitor output at 115200 baud
2. Review README.md for troubleshooting section
3. Verify all hardware connections
4. Test each subsystem independently
