# ESP32-WROOM Physical Pin Mapping Reference

## Quick Pin Lookup - This Project

### Used Pins - Quick Reference

| Physical Pin | GPIO | Function | Component |
|--------------|------|----------|-----------|
| **6** | **35** | Inlet Temperature | DS18B20 Sensor #1 |
| **7** | **32** | Heater Temperature | DS18B20 Sensor #2 |
| **8** | **33** | Ambient Temperature | DS18B20 Sensor #3 |
| **10** | **26** | Pump Enable | Relay Module IN1 |
| **13** | **12** | Heater Control | Relay/SSR |
| **15** | **13** | Pump Speed | Relay Module IN2 |
| **22** | **22** | I2C SCL | LCD + MCP23008 |
| **25** | **21** | I2C SDA | LCD + MCP23008 |

### Power Pins

| Physical Pin | Function | Notes |
|--------------|----------|-------|
| **1** | 3.3V Output | Max 600mA, powers ESP32 and peripherals |
| **19** | 5V Input (Vin) | From USB or external supply |
| **14, 20, 26** | GND | Ground connections |
| **2** | EN (Enable/Reset) | Pull LOW to reset |

## Complete Physical to GPIO Mapping

### Left Side (Pins 1-19)

| Physical | GPIO/Name | Function | This Project |
|----------|-----------|----------|--------------|
| 1 | 3V3 | Power 3.3V Out | Power supply |
| 2 | EN | Reset/Enable | Reset control |
| 3 | GPIO 36 (SVP) | Input Only, ADC1_0 | Available |
| 4 | GPIO 39 (SVN) | Input Only, ADC1_3 | Available |
| 5 | GPIO 34 | Input Only, ADC1_6 | Available |
| **6** | **GPIO 35** | **Input Only, ADC1_7** | **✓ Inlet Temp** |
| **7** | **GPIO 32** | **I/O, ADC1_4, Touch** | **✓ Heater Temp** |
| **8** | **GPIO 33** | **I/O, ADC1_5, Touch** | **✓ Ambient Temp** |
| 9 | GPIO 25 | I/O, ADC2_8, DAC1 | Available |
| **10** | **GPIO 26** | **I/O, ADC2_9, DAC2** | **✓ Pump Enable** |
| 11 | GPIO 27 | I/O, ADC2_7, Touch | Available |
| 12 | GPIO 14 | I/O, ADC2_6, Touch | Available |
| **13** | **GPIO 12** | **I/O, ADC2_5, Touch** | **✓ Heater Relay** |
| 14 | GND | Ground | Ground |
| **15** | **GPIO 13** | **I/O, ADC2_4, Touch** | **✓ Pump Speed** |
| 16 | GPIO 9 (SD2) | Flash - DO NOT USE | Reserved |
| 17 | GPIO 10 (SD3) | Flash - DO NOT USE | Reserved |
| 18 | CMD (GPIO 11) | Flash - DO NOT USE | Reserved |
| 19 | 5V | Power 5V Input | Power supply |

### Right Side (Pins 20-38)

| Physical | GPIO/Name | Function | This Project |
|----------|-----------|----------|--------------|
| 20 | GND | Ground | Ground |
| 21 | GPIO 23 | I/O, VSPI MOSI | Available |
| **22** | **GPIO 22** | **I/O** | **✓ I2C SCL** |
| 23 | GPIO 1 (TX0) | UART0 TX, Serial | Serial Debug |
| 24 | GPIO 3 (RX0) | UART0 RX, Serial | Serial Debug |
| **25** | **GPIO 21** | **I/O** | **✓ I2C SDA** |
| 26 | GND | Ground | Ground |
| 27 | GPIO 19 | I/O, VSPI MISO | Available |
| 28 | GPIO 18 | I/O, VSPI SCK | Available |
| 29 | GPIO 5 | I/O, VSPI SS | Available |
| 30 | GPIO 17 | I/O, UART2 TX | Available |
| 31 | GPIO 16 | I/O, UART2 RX | Available |
| 32 | GPIO 4 | I/O, ADC2_0, Touch | Available |
| 33 | GPIO 0 | I/O, BOOT Button | Boot Control |
| 34 | GPIO 2 | I/O, Onboard LED | LED/Boot |
| 35 | GPIO 15 | I/O, ADC2_3, Touch | Available |
| 36 | GPIO 8 | Flash - DO NOT USE | Reserved |
| 37 | GPIO 7 | Flash - DO NOT USE | Reserved |
| 38 | GPIO 6 | Flash - DO NOT USE | Reserved |

## Pin Categories

### Temperature Sensors (DS18B20 1-Wire)
```
Pin 6  (GPIO 35) → Inlet Temperature Sensor
Pin 7  (GPIO 32) → Heater Temperature Sensor  
Pin 8  (GPIO 33) → Ambient Temperature Sensor
```
**Wiring each sensor:**
- Red (VCC) → Pin 1 (3.3V)
- Black (GND) → Pin 14, 20, or 26 (GND)
- Yellow (Data) → Pin 6, 7, or 8 respectively
- 4.7kΩ resistor between Yellow and Red

### I2C Bus (Shared)
```
Pin 25 (GPIO 21) → SDA (Data)
Pin 22 (GPIO 22) → SCL (Clock)
```
**Connected devices:**
- LCD Display (0x27 or 0x3F)
- MCP23008 GPIO Expander (0x20)

### Control Relays
```
Pin 10 (GPIO 26) → Pump Enable Relay IN1
Pin 15 (GPIO 13) → Pump Speed Relay IN2
Pin 13 (GPIO 12) → Heater Relay/SSR IN
```

### Power Distribution
```
Pin 1  (3.3V)    → All DS18B20 sensors, I2C devices
Pin 19 (5V)      → Relay modules
Pin 14, 20, 26   → Common GND for all devices
```

## Available Expansion Pins

### Best for Digital I/O
```
Pin 30 (GPIO 17) - UART2 TX, clean
Pin 31 (GPIO 16) - UART2 RX, clean
Pin 28 (GPIO 18) - VSPI SCK
Pin 27 (GPIO 19) - VSPI MISO
Pin 21 (GPIO 23) - VSPI MOSI
Pin 29 (GPIO 5)  - VSPI SS
Pin 12 (GPIO 14) - ADC2_6, Touch
Pin 35 (GPIO 15) - ADC2_3, Touch
Pin 32 (GPIO 4)  - ADC2_0, Touch
Pin 9  (GPIO 25) - ADC2_8, DAC1
Pin 11 (GPIO 27) - ADC2_7, Touch
```

### Input-Only (Sensors/Switches)
```
Pin 3 (GPIO 36) - ADC1_0
Pin 4 (GPIO 39) - ADC1_3  
Pin 5 (GPIO 34) - ADC1_6
```
**Note:** No internal pull-ups, need external pull-ups if required

### Reserved/Avoid
```
Pin 2  (EN)      - Reset control
Pin 33 (GPIO 0)  - Boot button, use with caution
Pin 34 (GPIO 2)  - Onboard LED, boot pin
Pin 23 (GPIO 1)  - Serial TX, lose debug if used
Pin 24 (GPIO 3)  - Serial RX, lose debug if used
Pin 16-18, 36-38 - Flash memory, NEVER USE
```

## Wiring Color Conventions

### Temperature Sensors (DS18B20 Standard)
- **Red** → VCC (Pin 1 = 3.3V)
- **Black** → GND (Pin 14, 20, or 26)
- **Yellow** → Data (Pin 6, 7, or 8)

### Relay Modules (Typical)
- **Red** → VCC (Pin 19 = 5V)
- **Black** → GND (Pin 14)
- **White/Orange** → Signal IN (Pin 10, 13, 15)

### I2C Devices (Common)
- **Red** → VCC (Pin 1 or 19 depending on module)
- **Black** → GND (Pin 14, 20, or 26)
- **Blue/Green** → SDA (Pin 25)
- **Yellow** → SCL (Pin 22)

## Physical Pin to GPIO Quick Lookup

**Frequently Used Pins in This Project:**

- **Pin 6** = GPIO 35 (Inlet Temp)
- **Pin 7** = GPIO 32 (Heater Temp)
- **Pin 8** = GPIO 33 (Ambient Temp)
- **Pin 10** = GPIO 26 (Pump Enable)
- **Pin 13** = GPIO 12 (Heater)
- **Pin 15** = GPIO 13 (Pump Speed)
- **Pin 22** = GPIO 22 (I2C SCL)
- **Pin 25** = GPIO 21 (I2C SDA)

**Power & Ground:**
- **Pin 1** = 3.3V Out
- **Pin 19** = 5V In
- **Pin 14, 20, 26** = GND

## Troubleshooting with Physical Pins

### "Sensor not detected"
1. Check Pin 1 (3.3V) has voltage
2. Check Pin 14, 20, 26 (GND) continuity
3. Verify data wire on correct pin (6, 7, or 8)
4. Check 4.7kΩ resistor between data and Pin 1

### "I2C device not found"
1. Verify Pin 25 (SDA) and Pin 22 (SCL) connected
2. Check device powered from Pin 1 or Pin 19
3. Check common ground (Pin 14, 20, or 26)
4. Scan I2C bus to find actual address

### "Relay not working"
1. Verify Pin 19 (5V) connected to relay VCC
2. Check Pin 14 (GND) to relay GND
3. Confirm signal from Pin 10, 13, or 15
4. Test GPIO output with multimeter (should read 3.3V when HIGH)

## Notes

- **All GPIO voltage is 3.3V logic**
- Most relays need 5V power (Pin 19) but accept 3.3V signal
- I2C pull-ups usually built into modules
- Keep I2C wires short (<30cm) for reliability
- DS18B20 can work up to 10m wire length with good cable
- Never connect 5V to any GPIO pin - only to Vin (Pin 19)

## Board Orientation

When looking at the ESP32 board with the USB connector at the top:
- **LEFT side** = Pins 1-19 (top to bottom)
- **RIGHT side** = Pins 20-38 (top to bottom)

Pin 1 (3.3V) is usually marked on the board.

---

**Quick Tip:** Print this page and keep it near your workbench for easy reference when wiring!
