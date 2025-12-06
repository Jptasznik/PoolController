# ESP32-WROOM 38-Pin Development Board - Complete Pin Reference

## Pin Layout Diagram

```
                        ESP32-WROOM 38-Pin
                    ┌─────────────────────┐
                    │                     │
            3.3V ──▪│ 3V3            GND │▪── GND
              EN ──▪│ EN             IO23│▪── GPIO 23 (VSPI MOSI)
    (ADC2_3) SVP ──▪│ IO36 (SVP)     IO22│▪── GPIO 22 ✓ I2C SCL
    (ADC2_0) SVN ──▪│ IO39 (SVN)     TX0 │▪── GPIO 1 (TX)
   (ADC2_4) IO34 ──▪│ IO34           RX0 │▪── GPIO 3 (RX)
   (ADC2_5) IO35 ──▪│ IO35           IO21│▪── GPIO 21 ✓ I2C SDA
  (ADC2_6) IO32* ──▪│ IO32           GND │▪── GND
  (ADC2_7) IO33* ──▪│ IO33     ✓    IO19│▪── GPIO 19 (VSPI MISO)
  (ADC2_8) IO25* ──▪│ IO25     ✓    IO18│▪── GPIO 18 (VSPI SCK)
  (ADC2_9) IO26* ──▪│ IO26     ✓    IO5 │▪── GPIO 5 (VSPI SS)
 (ADC2_10) IO27* ──▪│ IO27     ✓    IO17│▪── GPIO 17 (TX2)
  (ADC1_4) IO14  ──▪│ IO14           IO16│▪── GPIO 16 (RX2)
  (ADC1_5) IO12  ──▪│ IO12     ⚠    IO4 │▪── GPIO 4 (ADC2_0)
             GND ──▪│ GND            IO0 │▪── GPIO 0 ⚠ (BOOT)
  (ADC1_6) IO13* ──▪│ IO13     ✓    IO2 │▪── GPIO 2 ⚠ (LED)
(Flash SDO) SD2 ──▪│ IO9            IO15│▪── GPIO 15 (ADC2_3)
(Flash SHD) SD3 ──▪│ IO10           IO8 │▪── GPIO 8 ⚠ (Flash)
(Flash SCK) CMD ──▪│ CMD            IO7 │▪── GPIO 7 ⚠ (Flash)
             5V ──▪│ 5V             IO6 │▪── GPIO 6 ⚠ (Flash)
                    │                     │
                    └─────────────────────┘

Legend:
✓ = Used in this project
* = ADC capable (ADC2 disabled when WiFi active)
⚠ = Special restrictions/boot pins
```

## Complete Pin Reference Table

### GPIO Pins (Usable)

| Physical Pin | GPIO | This Project | Type | Notes & Restrictions |
|--------------|------|-------------|------|---------------------|
| 25 | **GPIO 0** | Available | I/O | ⚠️ **BOOT button** - Must be HIGH during boot. Pulled up. Can use as input/output after boot but avoid LOW at boot. |
| 35 | **GPIO 1** | Available | TX0 | ⚠️ **UART0 TX** - Used for USB serial programming/debug. Avoid using if Serial Monitor needed. |
| 24 | **GPIO 2** | Available | I/O | ⚠️ **Onboard LED** - Must be LOW during boot. Connected to onboard LED. Pull-down at boot required. |
| 34 | **GPIO 3** | Available | RX0 | ⚠️ **UART0 RX** - Used for USB serial programming/debug. Avoid using if Serial Monitor needed. |
| 26 | **GPIO 4** | Available | I/O + ADC2_0 | Safe to use. ADC2 disabled when WiFi active. Touch capable (T0). |
| 29 | **GPIO 5** | Available | I/O | Safe to use. VSPI SS. Can be used as GPIO. |
| 13 | **GPIO 12** | **HEATER RELAY** ✓ | I/O + ADC2_5 | ⚠️ **Boot voltage** - Must be LOW during boot (internal pull-down). Set LOW before boot or use external pull-down. |
| 15 | **GPIO 13** | **PUMP SPEED RELAY** ✓ | I/O + ADC2_4 | Safe to use. Touch capable (T4). ADC2 disabled when WiFi active. |
| 12 | **GPIO 14** | Available | I/O + ADC2_6 | Safe to use. Touch capable (T6). ADC2 disabled when WiFi active. |
| 23 | **GPIO 15** | Available | I/O + ADC2_3 | Safe to use. Touch capable (T3). ADC2 disabled when WiFi active. |
| 27 | **GPIO 16** | Available | I/O | Safe to use. UART2 RX. Can be used as GPIO. |
| 28 | **GPIO 17** | Available | I/O | Safe to use. UART2 TX. Can be used as GPIO. |
| 30 | **GPIO 18** | Available | I/O | Safe to use. VSPI SCK. Can be used as GPIO. |
| 31 | **GPIO 19** | Available | I/O | Safe to use. VSPI MISO. Can be used as GPIO. |
| 33 | **GPIO 21** | **I2C SDA** ✓ | I/O | Used for I2C data (LCD + MCP23008). Safe to use. |
| 36 | **GPIO 22** | **I2C SCL** ✓ | I/O | Used for I2C clock (LCD + MCP23008). Safe to use. |
| 37 | **GPIO 23** | Available | I/O | Safe to use. VSPI MOSI. Can be used as GPIO. |
| 9 | **GPIO 25** | Available | I/O + ADC2_8 + DAC1 | Safe to use. ADC2 disabled when WiFi active. Has DAC output. |
| 10 | **GPIO 26** | **PUMP ENABLE RELAY** ✓ | I/O + ADC2_9 + DAC2 | Safe to use. ADC2 disabled when WiFi active. Has DAC output. |
| 11 | **GPIO 27** | Available | I/O + ADC2_7 | Safe to use. Touch capable (T7). ADC2 disabled when WiFi active. |
| 7 | **GPIO 32** | **HEATER TEMP SENSOR** ✓ | I/O + ADC1_4 | Safe to use. ADC1 works with WiFi. Touch capable (T9). |
| 8 | **GPIO 33** | **AMBIENT TEMP SENSOR** ✓ | I/O + ADC1_5 | Safe to use. ADC1 works with WiFi. Touch capable (T8). |

### Input-Only Pins

| Physical Pin | GPIO | This Project | Type | Notes & Restrictions |
|--------------|------|-------------|------|---------------------|
| 5 | **GPIO 34** | Available | Input Only + ADC1_6 | ⚠️ **INPUT ONLY** - No internal pull-up/down. ADC1 works with WiFi. |
| 6 | **GPIO 35** | **INLET TEMP SENSOR** ✓ | Input Only + ADC1_7 | ⚠️ **INPUT ONLY** - No internal pull-up/down. ADC1 works with WiFi. |
| 3 | **GPIO 36 (SVP)** | Available | Input Only + ADC1_0 | ⚠️ **INPUT ONLY** - No internal pull-up/down. ADC1 works with WiFi. |
| 4 | **GPIO 39 (SVN)** | Available | Input Only + ADC1_3 | ⚠️ **INPUT ONLY** - No internal pull-up/down. ADC1 works with WiFi. |

### Flash/Strapping Pins (Avoid Using)

| Physical Pin | GPIO | This Project | Type | Notes & Restrictions |
|--------------|------|-------------|------|---------------------|
| 20 | **GPIO 6** | ⚠️ Reserved | Flash | ❌ **DO NOT USE** - Connected to SPI flash memory. |
| 21 | **GPIO 7** | ⚠️ Reserved | Flash | ❌ **DO NOT USE** - Connected to SPI flash memory. |
| 22 | **GPIO 8** | ⚠️ Reserved | Flash | ❌ **DO NOT USE** - Connected to SPI flash memory. |
| 16 | **GPIO 9** | ⚠️ Reserved | Flash | ❌ **DO NOT USE** - Connected to SPI flash memory (SD2). |
| 17 | **GPIO 10** | ⚠️ Reserved | Flash | ❌ **DO NOT USE** - Connected to SPI flash memory (SD3). |
| 18 | **GPIO 11** | ⚠️ Reserved | Flash | ❌ **DO NOT USE** - Connected to SPI flash memory (CMD). |

### Power Pins

| Physical Pin | Pin | Function | Notes |
|--------------|-----|----------|-------|
| 1 | **3.3V** | 3.3V Output | Max 600mA from onboard regulator. Powers ESP32 and peripherals. |
| 19 | **5V (VIN)** | 5V Input | Connected to USB 5V or external 5V supply (6-12V regulated to 5V). |
| 14, 32, 38 | **GND** | Ground | Multiple ground pins available. |
| 2 | **EN** | Reset/Enable | Pull LOW to reset ESP32. Has external pull-up. |

## Project Pin Usage Summary

### Currently Used (11 pins)

| Physical Pin | GPIO | Function | Component |
|--------------|------|----------|-----------|
| 13 | 12 | Heater relay control | Single relay/SSR |
| 15 | 13 | Pump speed relay | 2-channel relay module |
| 33 | 21 | I2C SDA | LCD + MCP23008 (shared bus) |
| 36 | 22 | I2C SCL | LCD + MCP23008 (shared bus) |
| 10 | 26 | Pump enable relay | 2-channel relay module |
| 7 | 32 | Heater temperature | DS18B20 sensor #2 |
| 8 | 33 | Ambient temperature | DS18B20 sensor #3 |
| 6 | 35 | Inlet temperature | DS18B20 sensor #1 |

**I2C Devices (on bus):**
- LCD Display (address 0x27)
- MCP23008 (address 0x20) - handles 5 buttons

### Available for Expansion (18+ pins)

**Recommended for general use:**
- GPIO 4, 5, 14, 15, 16, 17, 18, 19, 23, 25, 27
- **Best choices:** GPIO 16, 17, 18, 19, 23 (no special restrictions)

**Input-only (sensors/switches):**
- GPIO 34, 36, 39
- **Note:** Require external pull-ups if needed

**Usable but with caution:**
- GPIO 0 (boot pin, has pull-up)
- GPIO 2 (boot pin, onboard LED, needs pull-down)
- GPIO 1, 3 (Serial - lose debug if used)

## ADC Channel Reference

### ADC1 (Works WITH WiFi) ✓
- GPIO 36 (SVP) - ADC1_0
- GPIO 39 (SVN) - ADC1_3
- GPIO 34 - ADC1_6
- GPIO 35 - ADC1_7 ✓ Used (inlet temp)
- GPIO 32 - ADC1_4 ✓ Used (heater temp)
- GPIO 33 - ADC1_5 ✓ Used (ambient temp)

### ADC2 (Disabled when WiFi active) ⚠️
- GPIO 4 - ADC2_0
- GPIO 0 - ADC2_1
- GPIO 2 - ADC2_2
- GPIO 15 - ADC2_3
- GPIO 13 - ADC2_4 ✓ Used (pump speed)
- GPIO 12 - ADC2_5 ✓ Used (heater)
- GPIO 14 - ADC2_6
- GPIO 27 - ADC2_7
- GPIO 25 - ADC2_8
- GPIO 26 - ADC2_9 ✓ Used (pump enable)

**Important:** ADC2 cannot be used while WiFi is active! Use ADC1 channels if you need analog input with WiFi.

## Touch Sensor Pins

These pins have capacitive touch sensing capability:
- T0 = GPIO 4
- T2 = GPIO 2
- T3 = GPIO 15
- T4 = GPIO 13 ✓ Used (pump speed relay)
- T5 = GPIO 12 ✓ Used (heater relay)
- T6 = GPIO 14
- T7 = GPIO 27 ✓ Used (pump enable relay)
- T8 = GPIO 33 ✓ Used (ambient temp)
- T9 = GPIO 32

## DAC (Digital-to-Analog Converter)

- DAC1 = GPIO 25
- DAC2 = GPIO 26 ✓ Used (pump enable relay)

Can output analog voltage (0-3.3V). Still works as digital GPIO.

## SPI Buses

### VSPI (Can be used as regular GPIO)
- **MISO:** GPIO 19
- **MOSI:** GPIO 23
- **SCK:** GPIO 18
- **SS:** GPIO 5

### HSPI (Can be used as regular GPIO)
- **MISO:** GPIO 12 ✓ Used (heater relay)
- **MOSI:** GPIO 13 ✓ Used (pump speed relay)
- **SCK:** GPIO 14
- **SS:** GPIO 15

## UART Interfaces

### UART0 (USB Serial)
- **TX:** GPIO 1
- **RX:** GPIO 3
- Used for programming and Serial Monitor

### UART1 (Not broken out on most boards)
- Internal flash communication

### UART2 (Available)
- **TX:** GPIO 17
- **RX:** GPIO 16

## I2C Interface

### Default I2C (Used in project)
- **SDA:** GPIO 21 ✓ Used
- **SCL:** GPIO 22 ✓ Used

**Note:** ESP32 I2C is software-based, can use any GPIO pins. We use default pins for compatibility.

## Boot Mode Strapping Pins

These pins determine boot mode. Be careful with them:

| GPIO | Boot Mode Function | Safe Usage |
|------|-------------------|------------|
| GPIO 0 | Must be HIGH to boot normally | Can use after boot, just ensure HIGH at startup |
| GPIO 2 | Must be LOW to boot | Can use after boot, has pull-down |
| GPIO 5 | VSPI SS, strapping | Safe to use |
| GPIO 12 | Boot voltage select | ⚠️ Must be LOW at boot (has internal pull-down) |
| GPIO 15 | Strapping | Safe to use, has pull-up |

## Pin Selection Guide

### For Digital Outputs (Relays, LEDs)
**Best choices:**
- GPIO 16, 17, 18, 19, 23 (no restrictions)
- GPIO 5, 14, 15 (safe, minor restrictions)
- GPIO 4, 25, 26, 27, 32, 33 (safe with WiFi)

**Avoid:**
- GPIO 0, 2 (boot pins)
- GPIO 1, 3 (serial)
- GPIO 6-11 (flash)
- GPIO 12 unless you manage boot state

### For Digital Inputs (Buttons, Sensors)
**Best choices:**
- GPIO 16, 17, 18, 19, 23 (no restrictions)
- GPIO 34, 35, 36, 39 (input-only, need external pull-ups)
- GPIO 4, 5, 14, 15, 25, 26, 27, 32, 33

**Avoid:**
- GPIO 6-11 (flash)

### For Analog Inputs (with WiFi)
**Use ADC1 only:**
- GPIO 32, 33, 34, 35, 36, 39
- ADC2 (GPIO 0, 2, 4, 12-15, 25-27) will NOT work with WiFi

### For I2C Devices
**Standard:**
- SDA: GPIO 21 (default)
- SCL: GPIO 22 (default)

**Alternative:** Can use any GPIO pair, but stick to standard for compatibility

### For Additional Temperature Sensors
**Good choices:**
- GPIO 4, 5, 14, 15, 16, 17, 18, 19, 23, 32
- **Best:** GPIO 4, 32 (ADC1 capable for future analog sensors)

## Expansion Ideas with Available Pins

### Easy Additions:

1. **Water Level Sensor** (analog or digital)
   - Use GPIO 32 or 35 (ADC1 for analog, works with WiFi)

2. **Flow Meter** (digital pulse)
   - Use GPIO 16 or 17 (no restrictions)

3. **pH Sensor** (analog)
   - Use GPIO 32, 34, 35, or 36 (ADC1)

4. **Additional Relay** (filter, lights, etc.)
   - Use GPIO 4, 5, 14, 15, 16, 17, 18, 19, or 23

5. **Status LEDs**
   - Use GPIO 4, 5, 16, 17, 18, 19, 23

6. **External Buttons** (alternative to MCP23008)
   - Use GPIO 4, 5, 16, 17, 18, 19, 23 with external pull-ups

7. **ORP Sensor** (analog)
   - Use GPIO 34, 35, 36, or 39 (ADC1)

8. **Pressure Sensor** (analog)
   - Use GPIO 32 or 35 (ADC1)

## Critical Reminders

### ❌ Never Use:
- GPIO 6, 7, 8, 9, 10, 11 (flash memory)

### ⚠️ Use with Caution:
- GPIO 0 (boot - keep HIGH)
- GPIO 2 (boot - keep LOW, onboard LED)
- GPIO 12 (boot voltage - keep LOW)
- GPIO 1, 3 (serial - lose debug output)

### ✓ Safest for General Use:
- GPIO 4, 5, 14, 15, 16, 17, 18, 19, 23, 32, 33

### ✓ Best for Analog (with WiFi):
- GPIO 32, 33, 34, 35, 36, 39 (ADC1)

### 📝 Input-Only:
- GPIO 34, 35, 36, 39 (no pull-ups, ADC1)

## Current vs Maximum Pin Usage

**Currently Used:** 8 GPIO pins + I2C bus  
**Available:** 18+ GPIO pins remaining  
**Reserved/Avoid:** 9 pins (flash + serial + boot issues)

**Total GPIO Capacity:** ~28 usable pins (out of 38 physical pins)

## Recommendations for This Project

### Good expansion pins (in order of preference):
1. **GPIO 16, 17** - UART2, safe, no restrictions
2. **GPIO 18, 19, 23** - VSPI, safe, no restrictions  
3. **GPIO 4, 5** - Safe, good options
4. **GPIO 32** - ADC1, safe with WiFi
5. **GPIO 14, 15** - Safe with minor notes

### For analog sensors (must work with WiFi):
1. **GPIO 32** - ADC1_4, available
2. **GPIO 34** - ADC1_6, input-only
3. **GPIO 35** - ADC1_7, input-only
4. **GPIO 36** - ADC1_0, input-only
5. **GPIO 39** - ADC1_3, input-only

## Notes

- All voltage levels are **3.3V logic**
- Maximum current per pin: 12mA (40mA absolute max)
- Use transistors/MOSFETs for loads >12mA
- Always use current-limiting resistors with LEDs
- Use external pull-ups for input-only pins if needed
- I2C pull-ups typically built into modules
- Total current from all pins: <200mA recommended

## References

- [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [Random Nerd Tutorials - ESP32 Pinout](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
