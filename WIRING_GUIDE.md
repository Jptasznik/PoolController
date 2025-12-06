# ESP32 Pool Controller - Wiring Diagram Guide

## Power Requirements

**ESP32:**
- Input: 5V DC via USB or Vin pin
- Current: 500mA typical, 1A recommended
- Logic level: 3.3V

**Power Distribution:**
```
5V Power Supply (2A minimum)
  ├─ ESP32 Vin pin (or USB)
  ├─ LCD Display VCC (if 5V type)
  └─ Relay modules VCC

3.3V (from ESP32 regulator)
  ├─ MCP23008 VDD
  ├─ DS18B20 sensors VCC (all 3)
  └─ LCD Display VCC (if 3.3V type)

Common Ground
  └─ All devices share GND connection
```

## Temperature Sensors (DS18B20)

**Each sensor requires:**
- 4.7kΩ pull-up resistor between DATA and VCC

### Inlet Temperature Sensor (GPIO 35 - Pin 6)
```
DS18B20 Sensor #1:
  Red (VCC)    ──────────── ESP32 3.3V (Pin 1)
  Black (GND)  ──────────── ESP32 GND (Pin 14)
  Yellow (DATA)─────┬────── ESP32 GPIO 35 (Pin 6)
                    │
                  [4.7kΩ resistor]
                    │
                    └────── ESP32 3.3V (Pin 1)
```

### Heater Outlet Temperature Sensor (GPIO 32 - Pin 7)
```
DS18B20 Sensor #2:
  Red (VCC)    ──────────── ESP32 3.3V (Pin 1)
  Black (GND)  ──────────── ESP32 GND (Pin 14)
  Yellow (DATA)─────┬────── ESP32 GPIO 32 (Pin 7)
                    │
                  [4.7kΩ resistor]
                    │
                    └────── ESP32 3.3V (Pin 1)
```

### Ambient/Outdoor Temperature Sensor (GPIO 33 - Pin 8)
```
DS18B20 Sensor #3:
  Red (VCC)    ──────────── ESP32 3.3V (Pin 1)
  Black (GND)  ──────────── ESP32 GND (Pin 14)
  Yellow (DATA)─────┬────── ESP32 GPIO 33 (Pin 8)
                    │
                  [4.7kΩ resistor]
                    │
                    └────── ESP32 3.3V (Pin 1)
```

**Notes:**
- Waterproof probes recommended for pool use
- Keep wire runs under 10 meters for reliability
- Use shielded cable for long runs
- Test resistance: ~10kΩ at room temperature

## I2C Bus (Shared by LCD and MCP23008)

**Pull-up resistors:** 4.7kΩ on SDA and SCL (usually built-in to modules)

```
ESP32 GPIO 21 (Pin 33/SDA) ─────┬─────────── LCD SDA
                                │
                                └─────────── MCP23008 SDA (Pin 1)

ESP32 GPIO 22 (Pin 36/SCL) ─────┬─────────── LCD SCL
                                │
                                └─────────── MCP23008 SCL (Pin 2)

ESP32 3.3V or 5V (Pin 1/19) ────┬─────────── LCD VCC (check module voltage)
                                │
                                └─────────── MCP23008 VDD (Pin 18)

ESP32 GND (Pin 14/20/26) ───────┬─────────── LCD GND
                                │
                                └─────────── MCP23008 VSS (Pin 9)
```

**I2C Addresses:**
- LCD: 0x27 (or 0x3F depending on module)
- MCP23008: 0x20 (when A0, A1, A2 = GND)

## LCD Display (20x4 I2C)

```
LCD Module Pins:
  VCC  ──── 5V (Pin 19) or 3.3V (Pin 1) - check module specs
  GND  ──── ESP32 GND (Pin 14, 38, or 32)
  SDA  ──── ESP32 GPIO 21 (Pin 33)
  SCL  ──── ESP32 GPIO 22 (Pin 36)
  
Backlight: Controlled by LCD module (always on)
Contrast: Adjust potentiometer on back of module
```

**Identifying LCD I2C Address:**
- Most common: 0x27
- Alternative: 0x3F
- Use I2C scanner sketch if unsure
- Change in Config.h if needed

## MCP23008 I2C GPIO Expander

```
MCP23008 Pinout:
  Pin 1  (SDA)    ──── ESP32 GPIO 21
  Pin 2  (SCL)    ──── ESP32 GPIO 22
  Pin 3  (A2)     ──── GND (address select)
  Pin 4  (A1)     ──── GND (address select)
  Pin 5  (A0)     ──── GND (address select)
  Pin 6  (RESET)  ──── 3.3V (always enabled)
  Pin 7  (NC)     ──── Not connected
  Pin 8  (INT)    ──── Not used (optional interrupt)
  Pin 9  (VSS)    ──── ESP32 GND
  Pin 10 (GP0)    ──── Button 1 (heater)
  Pin 11 (GP1)    ──── Button 2 (pump + emergency)
  Pin 12 (GP2)    ──── Button 3 (temp up)
  Pin 13 (GP3)    ──── Button 4 (temp down)
  Pin 14 (GP4)    ──── Button 5 (factory reset)
  Pin 15 (GP5)    ──── Reserved (not used)
  Pin 16 (GP6)    ──── Reserved (not used)
  Pin 17 (GP7)    ──── Reserved (not used)
  Pin 18 (VDD)    ──── 3.3V or 5V
```

**Address Configuration:**
```
A2 A1 A0 = Address
0  0  0  = 0x20 (default, all to GND)
0  0  1  = 0x21
0  1  0  = 0x22
... and so on
```

## Push Buttons

**All buttons use internal pull-ups:**
- One side of button → MCP23008 GP pin
- Other side of button → GND
- Pressed = LOW (pulled to ground)
- Released = HIGH (pulled up internally)

```
Button Wiring (example for Button 1):
  
  Button 1 (normally open)
    Terminal 1 ──── MCP23008 GP0 (Pin 10)
    Terminal 2 ──── GND

Repeat for all 5 buttons:
  Button 1 ──── GP0 (Pin 10) + GND
  Button 2 ──── GP1 (Pin 11) + GND
  Button 3 ──── GP2 (Pin 12) + GND
  Button 4 ──── GP3 (Pin 13) + GND
  Button 5 ──── GP4 (Pin 14) + GND
```

**Button Specifications:**
- Type: Momentary SPST (normally open)
- Rating: 50mA minimum
- Typical: 6mm tactile switches or panel mount buttons

## Control Outputs - Pump

**2-Channel Relay Module:**

```
Relay Module:
  VCC  ──── 5V (Pin 19)
  GND  ──── ESP32 GND (Pin 14)
  IN1  ──── ESP32 GPIO 26 (Pin 10) - pump enable
  IN2  ──── ESP32 GPIO 13 (Pin 15) - pump speed

Relay Outputs (connect to pump motor controller):
  CH1 (pump enable):
    COM  ──── Pump power source
    NO   ──── Pump motor controller input
    NC   ──── Not used
    
  CH2 (pump speed):
    COM  ──── Speed control input
    NO   ──── High speed signal
    NC   ──── Low speed signal (or leave NC unused)
```

**Pump Control Logic:**
```
GPIO 26 (Enable)  GPIO 13 (Speed)  Result
     LOW              LOW         Pump OFF
     HIGH             LOW         Pump LOW speed
     HIGH             HIGH        Pump HIGH speed
```

**Electrical Ratings:**
- Use relay rated for pump motor current
- Minimum: 10A @ 120/240VAC
- Recommended: 15A or 20A
- Consider SSR for longer life

## Control Output - Heater

**Single Relay or SSR:**

```
Option 1: Mechanical Relay
  VCC  ──── 5V (Pin 19)
  GND  ──── ESP32 GND (Pin 14)
  IN   ──── ESP32 GPIO 12 (Pin 13)
  COM  ──── Heater power source
  NO   ──── Heater input
  NC   ──── Not used

Option 2: Solid State Relay (SSR) - Recommended
  Input+ ──── ESP32 GPIO 12 (Pin 13)
  Input- ──── ESP32 GND (Pin 14)
  Load1  ──── Heater hot wire
  Load2  ──── Heater return
```

**Heater Control Logic:**
```
GPIO 12 (Pin 13)    Result
      LOW           Heater OFF
      HIGH          Heater ON
```

**⚠️ CRITICAL SAFETY:**
- Heater relay MUST be rated for full heater load
- Typical pool heaters: 5-30A @ 240VAC
- Use SSR rated 25A minimum for 20A heater
- Always include proper circuit breaker
- GFCI required for wet locations
- Installation by qualified electrician REQUIRED

## Complete System Wiring Diagram

```
┌─────────────────────────────────────────────────────────┐
│                      ESP32-WROOM                        │
│                                                         │
│  3.3V ────┬──── DS18B20 #1 VCC (w/ 4.7kΩ to DATA)       │
│           ├──── DS18B20 #2 VCC (w/ 4.7kΩ to DATA)       │
│           ├──── MCP23008 VDD                            │
│           └──── (optional) LCD VCC                      │
│                                                         │
│  GND ─────┬──── All device grounds                      │
│           ├──── All button commons                      │
│           └──── Relay module GND                        │
│                                                         │
│  GPIO 21 (Pin 33) ─┴─ I2C SDA (LCD + MCP23008)          │
│  GPIO 22 (Pin 36) ─┴─ I2C SCL (LCD + MCP23008)          │
│                                                         │
│  GPIO 35 (Pin 6)  ──── DS18B20 #1 DATA (inlet temp)     │
│  GPIO 32 (Pin 7)  ──── DS18B20 #2 DATA (heater temp)    │
│  GPIO 33 (Pin 8)  ──── DS18B20 #3 DATA (ambient temp)   │
│                                                         │
│  GPIO 26 (Pin 10) ──── Pump Enable Relay IN             │
│  GPIO 13 (Pin 15) ──── Pump Speed Relay IN              │
│  GPIO 12 (Pin 13) ──── Heater Relay/SSR IN              │
│                                                         │
└─────────────────────────────────────────────────────────┘
         │                    │                   │
         ▼                    ▼                   ▼
    ┌────────┐          ┌──────────┐       ┌──────────┐
    │  LCD   │          │ MCP23008 │       │  Relays  │
    │ 0x27   │          │   0x20   │       │ (Outputs)│
    └────────┘          └──────────┘       └──────────┘
                              │
                         (5 Buttons)
```

## Wiring Best Practices

**I2C Bus:**
1. Keep wiring under 30cm (12") if possible
2. Use twisted pair or shielded cable for longer runs
3. Add 100nF capacitors near each I2C device (optional, for noise)
4. Check for 4.7kΩ pull-ups (usually built-in to modules)
5. Test each I2C device individually before combining

**Temperature Sensors:**
1. MUST have 4.7kΩ pull-up resistor on each sensor
2. Use good quality waterproof probes for pool environment
3. Secure connections with heat shrink or waterproof connectors
4. Keep data wires away from AC power wires
5. Maximum cable length: 10 meters (30 feet)

**Buttons:**
1. Use shielded cable if buttons are far from ESP32
2. Debouncing handled in software (50ms)
3. No external pull-up resistors needed (internal in MCP23008)
4. Keep button wires separate from relay/power wires

**Power:**
1. Use adequate power supply (2A minimum for entire system)
2. Short, heavy gauge wires for power distribution
3. Common ground for all devices
4. Add 100µF capacitor near ESP32 Vin for stability
5. Separate power supply for relay coils if using many relays

**Relays:**
1. Flyback diodes across relay coils (usually built-in to modules)
2. Use opto-isolated relay modules for safety
3. Keep control wires separate from high-voltage load wires
4. Always fuse or breaker protect the load circuits
5. Follow local electrical codes

## Testing Procedure

**Before powering on:**
1. ☐ Double-check all I2C addresses set correctly
2. ☐ Verify no shorts between power and ground
3. ☐ Check temperature sensor pull-up resistors installed
4. ☐ Ensure all buttons connect to GND (not power)
5. ☐ Verify relay ratings adequate for loads
6. ☐ Test continuity of all connections

**Initial power-up:**
1. ☐ Power ESP32 only (no relays connected)
2. ☐ Check Serial Monitor at 115200 baud
3. ☐ Verify all I2C devices detected
4. ☐ Check temperature readings appear
5. ☐ Test each button individually
6. ☐ Verify LCD displays data

**Relay testing:**
1. ☐ Connect relays but NOT to pool equipment
2. ☐ Test pump enable relay clicks
3. ☐ Test pump speed relay clicks
4. ☐ Test heater relay clicks
5. ☐ Measure relay coil voltage (should be ~5V when ON)
6. ☐ Verify GPIOs go HIGH/LOW as expected

**Final integration:**
1. ☐ Have qualified electrician wire to pool equipment
2. ☐ Test all safety interlocks
3. ☐ Verify emergency shutdown works
4. ☐ Test full heating cycle
5. ☐ Confirm cooldown logic operates correctly
6. ☐ Verify pump cannot turn off while heater is on

## Cable Lengths and Wire Gauge

**Power (5V to ESP32):**
- Wire gauge: 18-22 AWG
- Maximum length: 2 meters (6 feet)

**I2C (SDA/SCL):**
- Wire gauge: 22-24 AWG
- Maximum length: 30cm (1 foot) unshielded
- Up to 3 meters (10 feet) with shielded cable

**Temperature Sensors (1-Wire):**
- Wire gauge: 22-24 AWG
- Maximum length: 10 meters (30 feet)
- Use shielded cable for runs over 3 meters

**Buttons (Digital Inputs):**
- Wire gauge: 24-26 AWG
- Maximum length: 5 meters (15 feet)

**Relay Control (GPIO Outputs):**
- Wire gauge: 22-24 AWG
- Maximum length: 3 meters (10 feet)

**High Voltage Loads (AC power):**
- Wire gauge: Per NEC/local code for amperage
- MUST be done by qualified electrician
- Separate conduit from low-voltage wiring

## Troubleshooting Wiring Issues

**No I2C devices detected:**
- Verify 3.3V power present at devices
- Check SDA and SCL not swapped
- Test I2C lines with multimeter (should idle at 3.3V)
- Add pull-up resistors if very long cables used

**Temperature always invalid:**
- Check 4.7kΩ resistor installed correctly
- Verify sensor DATA line connected to correct GPIO
- Test sensor with multimeter (should show ~10kΩ resistance)
- Try different DS18B20 sensor to rule out defect

**Buttons not responding:**
- Verify MCP23008 powered (VDD = 3.3V)
- Check A0, A1, A2 all connected to GND
- Test button continuity with multimeter
- Ensure buttons connect GP pin to GND (not power)

**Relays clicking but loads not switching:**
- Check relay contact ratings adequate
- Verify load connections to NO terminals
- Test relay contacts with multimeter
- Check for blown fuses in load circuits

**Erratic behavior / random resets:**
- Add decoupling capacitor (100µF) at ESP32 Vin
- Check all ground connections solid
- Separate relay control from sensor wires
- Use shielded cables in noisy environments

## Safety Warnings

⚠️ **DANGER: HIGH VOLTAGE**

This system interfaces with 120VAC or 240VAC pool equipment. Improper wiring can result in:
- Electric shock (potentially fatal)
- Fire hazard
- Equipment damage
- Warranty voidance

**REQUIREMENTS:**
- All high-voltage wiring MUST be performed by licensed electrician
- Follow all local electrical codes (NEC in USA)
- Use GFCI protection for all circuits near water
- Install in weatherproof enclosure rated for outdoor use
- Properly ground all equipment
- Use correct wire gauge for amperage
- Include proper overcurrent protection (breakers/fuses)
- Keep low-voltage and high-voltage wiring separate
- Label all circuits clearly
- Test with multimeter before pool equipment connection

**DO NOT:**
- Wire directly to pool equipment unless qualified
- Exceed relay voltage/current ratings
- Use extension cords for permanent installation
- Mix AC and DC circuits
- Work on live circuits
- Skip safety interlocks
- Bypass GFCI protection

**VERIFY:**
- All connections tight and secure
- No exposed conductors
- Proper strain relief on all cables
- Weatherproof sealing where needed
- Clear labeling of all circuits

When in doubt, consult a qualified professional.

## Enclosure Recommendations

**For ESP32 and Control Electronics:**
- NEMA 4X (waterproof) rated enclosure
- Minimum size: 8" x 8" x 4"
- Vented or fan-cooled to prevent heat buildup
- Separate compartments for low-voltage and line-voltage
- Clear viewing window for LCD display
- Holes for buttons with waterproof pushbuttons
- Cable glands for all wire entries

**Mounting:**
- Mount away from direct spray
- Under roof/cover if possible
- 4+ feet above ground level
- Easy access for maintenance
- Near WiFi signal for good connectivity

## Connection Summary Checklist

- [ ] ESP32 powered from 5V, 2A supply
- [ ] All grounds connected to common GND
- [ ] I2C: GPIO 21 = SDA, GPIO 22 = SCL
- [ ] LCD at address 0x27, powered, on I2C bus
- [ ] MCP23008 at address 0x20, A0/A1/A2 = GND
- [ ] DS18B20 inlet on GPIO 35 with 4.7kΩ pull-up
- [ ] DS18B20 heater on GPIO 32 with 4.7kΩ pull-up
- [ ] DS18B20 ambient on GPIO 33 with 4.7kΩ pull-up
- [ ] 5 buttons connected from GP0-GP4 to GND
- [ ] GPIO 26 to pump enable relay
- [ ] GPIO 13 to pump speed relay
- [ ] GPIO 12 to heater relay
- [ ] All relay modules powered correctly
- [ ] Flyback diodes on all relay coils
- [ ] Proper gauge wire for all connections
- [ ] Cable lengths within specified limits
- [ ] No shorts between power and ground
- [ ] High-voltage wiring by qualified electrician only
- [ ] GFCI protection on all pool circuits
- [ ] System enclosed in weatherproof enclosure

## Support

If you need help with wiring:
1. Post clear photos in support forum
2. Use multimeter to verify voltages
3. Check Serial Monitor for I2C scanner output
4. Test each subsystem individually before combining
5. Review troubleshooting section in README.md
