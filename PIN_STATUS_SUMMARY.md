# ESP32 Pool Controller - Pin Status at a Glance

## Quick Visual Reference

```
┌─────────────────────────────────────────────────────────────┐
│                    ESP32-WROOM 38-PIN                       │
│                      PIN STATUS MAP                         │
└─────────────────────────────────────────────────────────────┘

Pin Status Legend:
  ✓ = USED in project
  ○ = AVAILABLE for expansion
  ⚠ = AVAILABLE but has restrictions
  ❌ = DO NOT USE (flash/reserved)
  ▶ = INPUT ONLY


LEFT SIDE (Top to Bottom):          RIGHT SIDE (Top to Bottom):
Pin#  GPIO    Status                 Pin#  GPIO    Status
════════════════════════════         ════════════════════════════
 1    3V3    [Power]                  38   GND    [Ground]
 2    EN     [Reset]                  37   IO23   ○  VSPI MOSI
 3    IO36   ▶  ADC1_0                36   IO22   ✓  I2C SCL (Yellow)
 4    IO39   ▶  ADC1_3                35   TX0    ⚠  Serial TX
 5    IO34   ▶  ADC1_6                34   RX0    ⚠  Serial RX
 6    IO35   ✓  Inlet Temp            33   IO21   ✓  I2C SDA (Blue)
 7    IO32   ✓  Heater Temp           32   GND    [Ground]
 8    IO33   ✓  Ambient Temp          31   IO19   ○  VSPI MISO
 9    IO25   ○  ADC2_8, DAC1          30   IO18   ○  VSPI SCK
10    IO26   ✓  Pump Enable           29   IO5    ○  VSPI SS
11    IO27   ○  ADC2_7                28   IO17   ○  UART2 TX
12    IO14   ○  ADC2_6, Touch         27   IO16   ○  UART2 RX
13    IO12   ✓  Heater Relay          26   IO4    ○  ADC2_0
14    GND    [Ground]                 25   IO0    ⚠  BOOT button
15    IO13   ✓  Pump Speed            24   IO2    ⚠  Onboard LED
16    IO9    ❌ Flash SD2             23   IO15   ○  ADC2_3
17    IO10   ❌ Flash SD3             22   IO8    ❌ Flash
18    CMD    ❌ Flash                 21   IO7    ❌ Flash
19    5V     [Power]                  20   IO6    ❌ Flash
```

## Color-Coded Pin Categories

### 🟢 SAFE TO USE (Best for expansion)
No restrictions, work perfectly:
```
Pin 26 / GPIO 4    Pin 27 / GPIO 16   Pin 30 / GPIO 18
Pin 29 / GPIO 5    Pin 28 / GPIO 17   Pin 31 / GPIO 19
Pin 12 / GPIO 14   Pin 23 / GPIO 15   Pin 37 / GPIO 23
```

### 🔵 INPUT-ONLY PINS
Cannot drive outputs, need external pull-ups:
```
Pin 5 / GPIO 34
Pin 3 / GPIO 36
Pin 4 / GPIO 39
```
**Best for:** Sensors that only need to read (no control)

### 🟡 BOOT/STRAPPING PINS
Usable but need care during boot:
```
Pin 25 / GPIO 0  - Must be HIGH to boot (has pull-up)
Pin 24 / GPIO 2  - Must be LOW to boot (onboard LED)
Pin 13 / GPIO 12 - Must be LOW to boot (internal pull-down) ✓ Used
```

### 🟠 SERIAL PINS
Using these loses Serial Monitor debug:
```
Pin 35 / GPIO 1  - TX0 (Serial transmit)
Pin 34 / GPIO 3  - RX0 (Serial receive)
```

### 🔴 DO NOT USE
Reserved for flash memory:
```
Pin 20 / GPIO 6    Pin 17 / GPIO 10
Pin 21 / GPIO 7    Pin 18 / GPIO 11
Pin 22 / GPIO 8    Pin 16 / GPIO 9
```

## Project Usage Statistics

```
┌──────────────────────────────────────────┐
│  TOTAL PINS: 38 physical pins            │
├──────────────────────────────────────────┤
│  Power/Ground: 6 pins                    │
│  Flash Reserved: 6 pins (GPIO 6-11)      │
│  Usable GPIO: 26 pins                    │
├──────────────────────────────────────────┤
│  ✓ Currently Used: 8 pins                │
│  ○ Available: 18+ pins                   │
├──────────────────────────────────────────┤
│  Usage: 31% of available GPIO            │
│  Remaining: 69% available for expansion  │
└──────────────────────────────────────────┘
```

## Pin Usage Breakdown

### Digital Outputs (5 pins used)
```
Pin 13  GPIO 12  ✓  Heater relay control
Pin 15  GPIO 13  ✓  Pump speed relay
Pin 10  GPIO 26  ✓  Pump enable relay
Pin 33  GPIO 21  ✓  I2C SDA / Blue (LCD + MCP23008 + ADS1115)
Pin 36  GPIO 22  ✓  I2C SCL / Yellow (LCD + MCP23008 + ADS1115)
```

### Temperature Sensors (3 pins used)
```
Pin 6   GPIO 35  ✓  DS18B20 - Inlet temperature
Pin 7   GPIO 32  ✓  DS18B20 - Heater outlet temperature  
Pin 8   GPIO 33  ✓  DS18B20 - Ambient/outdoor temperature
```

### Available for Expansion

#### Best Choices (No restrictions)
```
Pin 27 / GPIO 16  ○  Free, UART2 RX
Pin 28 / GPIO 17  ○  Free, UART2 TX
Pin 30 / GPIO 18  ○  Free, VSPI SCK
Pin 31 / GPIO 19  ○  Free, VSPI MISO
Pin 37 / GPIO 23  ○  Free, VSPI MOSI
```

#### Good Choices (Minor notes)
```
Pin 26 / GPIO 4   ○  Free, ADC2_0 (WiFi disables ADC2)
Pin 29 / GPIO 5   ○  Free, VSPI SS
Pin 12 / GPIO 14  ○  Free, ADC2_6, Touch
Pin 23 / GPIO 15  ○  Free, ADC2_3, Touch
Pin  9 / GPIO 25  ○  Free, ADC2_8, DAC1
Pin 11 / GPIO 27  ○  Free, ADC2_7, Touch
```

#### Input-Only (Sensors/Switches)
```
Pin 5 / GPIO 34  ▶  Free, ADC1_6 (no pull-ups)
Pin 3 / GPIO 36  ▶  Free, ADC1_0 (no pull-ups)
Pin 4 / GPIO 39  ▶  Free, ADC1_3 (no pull-ups)
```

## ADC Usage (Analog Inputs)

### ADC1 - Works WITH WiFi ✓
```
Available:
  Pin 3 / GPIO 36  ▶  ADC1_0 (input-only)
  Pin 4 / GPIO 39  ▶  ADC1_3 (input-only)
  Pin 5 / GPIO 34  ▶  ADC1_6 (input-only)

Used:
  Pin 6 / GPIO 35  ✓  ADC1_7 (Inlet temp sensor)
  Pin 7 / GPIO 32  ✓  ADC1_4 (Heater temp sensor)
  Pin 8 / GPIO 33  ✓  ADC1_5 (Ambient temp sensor)
```

### ADC2 - Disabled when WiFi active ⚠
```
Used (Digital only, ADC not usable):
  Pin 13 / GPIO 12  ✓  ADC2_5 (Heater relay)
  Pin 15 / GPIO 13  ✓  ADC2_4 (Pump speed)
  Pin 10 / GPIO 26  ✓  ADC2_9 (Pump enable)

Available (Digital only):
  Pin 26 / GPIO 4   ○  ADC2_0
  Pin 12 / GPIO 14  ○  ADC2_6
  Pin 23 / GPIO 15  ○  ADC2_3
  Pin  9 / GPIO 25  ○  ADC2_8
  Pin 11 / GPIO 27  ○  ADC2_7
```

**⚠️ Important:** If you need analog input AND WiFi, MUST use ADC1 pins!

## Recommended Expansion Pins

### For Next Digital Output (Relay/LED):
```
1st choice: Pin 27/GPIO 16 or Pin 28/GPIO 17 (UART2, clean)
2nd choice: Pin 30/GPIO 18, Pin 31/GPIO 19, or Pin 37/GPIO 23 (VSPI, clean)
3rd choice: Pin 26/GPIO 4 or Pin 29/GPIO 5 (safe, widely used)
```

### For Next Analog Input (with WiFi):
```
1st choice: Pin 3/GPIO 36 (ADC1_0, input-only)
2nd choice: Pin 4/GPIO 39 (ADC1_3, input-only)
3rd choice: Pin 5/GPIO 34 (ADC1_6, input-only)
Note: GPIO 32, 33, 35 already used for temp sensors
```

### For Next Temperature Sensor:
```
1st choice: Pin 26/GPIO 4 (clean, no restrictions)
2nd choice: Pin 27/GPIO 16 or Pin 28/GPIO 17 (clean UART2 pins)
3rd choice: Pin  9/GPIO 25 (DAC1, clean)
```

## I2C Bus Capacity

**Wire Color Code: Blue = SDA (GPIO 21), Yellow = SCL (GPIO 22)**

Current I2C devices:
```
Address 0x20: MCP23008 GPIO expander (5 buttons)
Address 0x27: 20x4 LCD display
```

Available I2C addresses:
```
Room for 100+ more I2C devices on same bus!
Common addresses still available:
  0x68, 0x69 - MPU6050 gyro/accel
  0x76, 0x77 - BMP280 pressure
  0x48-0x4F  - ADS1115 ADC
  0x3C, 0x3D - OLED displays
  ... and many more
```

## Expansion Ideas with Pin Assignments

### Water Quality Monitoring
```
pH Sensor (analog):        Pin 3 / GPIO 36 (ADC1_0, input-only)
ORP Sensor (analog):       Pin 5 / GPIO 34 (ADC1_6, input-only)
TDS Sensor (analog):       Pin 4 / GPIO 39 (ADC1_3, input-only)
```

### Additional Controls
```
Filter Pump Relay:         Pin 27 / GPIO 16 (clean)
Pool Light Relay:          Pin 28 / GPIO 17 (clean)
Chlorinator Control:       Pin 30 / GPIO 18 (clean)
Valve Control:             Pin 31 / GPIO 19 (clean)
```

### Safety/Monitoring
```
Water Level Switch:        Pin 26 / GPIO 4 (digital input)
Flow Meter (pulse):        Pin 29 / GPIO 5 (interrupt capable)
Pressure Sensor (analog):  Pin 3  / GPIO 36 (ADC1_0, input-only)
Leak Detector:             Pin 12 / GPIO 14 (digital input)
```

### User Interface
```
Status LED (WiFi):         Pin 26 / GPIO 4
Status LED (Heater):       Pin 29 / GPIO 5
Status LED (Error):        Pin 12 / GPIO 14
Buzzer/Alarm:              Pin 23 / GPIO 15
External Display (SPI):    Pin 30,31,37,29 / GPIO 18,19,23,5 (VSPI)
```

## Quick Decision Chart

```
Need to add a...          Use this pin...               Notes
──────────────────────────────────────────────────────────────────
Digital Output            Pin 27/GPIO 16, Pin 28/GPIO 17    Cleanest
(relay, LED, etc.)        Pin 30/GPIO 18, Pin 31/GPIO 19    Also clean
                          Pin 37/GPIO 23                     Also clean
                          Pin 26/GPIO 4, Pin 29/GPIO 5       Safe

Digital Input             Pin 27/GPIO 16, Pin 28/GPIO 17    With pull-up
(button, switch)          Pin 3/GPIO 36, Pin 4/GPIO 39      Need ext. pull-up
                          Pin 5/GPIO 34                      Need ext. pull-up

Analog Input              Pin 3/GPIO 36, Pin 4/GPIO 39      Works with WiFi
(with WiFi active)        Pin 5/GPIO 34                      Input-only

Temperature Sensor        Pin 26/GPIO 4, Pin 27/GPIO 16     Any GPIO works
(DS18B20, etc.)          Pin 28/GPIO 17, Pin 9/GPIO 25     Future options

I2C Device               Pin 33/GPIO 21 (Blue), Pin 36/GPIO 22 (Yellow)     Already configured
(sensor, display)        (shared bus)                       127 addresses available

SPI Device               Pin 30,31,37,29 / GPIO 18,19,23,5  VSPI bus
                         (SCK,MISO,MOSI,SS)                 Share or dedicate
```

## Final Recommendations

### 🟢 Top 5 Pins for General Expansion
1. **Pin 27 / GPIO 16** - UART2 RX, no restrictions
2. **Pin 28 / GPIO 17** - UART2 TX, no restrictions
3. **Pin 30 / GPIO 18** - VSPI SCK, no restrictions
4. **Pin 26 / GPIO 4** - Clean, widely compatible
5. **Pin 29 / GPIO 5** - VSPI SS, clean

### 🔵 For Analog Sensors (WiFi active)
1. **Pin 3 / GPIO 36** - ADC1_0, input-only
2. **Pin 4 / GPIO 39** - ADC1_3, input-only
3. **Pin 5 / GPIO 34** - ADC1_6, input-only
Note: GPIO 32, 33, 35 already used for temperature sensors

### ⚠️ Avoid Unless Necessary
- Pin 25 / GPIO 0, Pin 24 / GPIO 2 (boot issues)
- Pin 13 / GPIO 12 (boot voltage, ✓ used for heater)
- Pin 35 / GPIO 1, Pin 34 / GPIO 3 (lose serial debug)
- Pin 16-18, 20-22 / GPIO 6-11 (flash, never use)

---

**Remember:** You have plenty of pins available! The ESP32 is very flexible.
Current project uses only **31%** of available GPIO - lots of room to grow! 🚀
