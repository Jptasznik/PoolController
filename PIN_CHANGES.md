# Pin Assignment Changes - Summary

## Overview
Updated GPIO pin assignments to use ADC1 channels for temperature sensors (which work with WiFi active).

## Pin Changes

### Temperature Sensors (Changed to ADC1)

| Sensor | Old Pin | New Pin | Notes |
|--------|---------|---------|-------|
| Inlet Temperature | Pin 9 / GPIO 25 (ADC2_8) | **Pin 6 / GPIO 35 (ADC1_7)** | Now uses ADC1 - works with WiFi |
| Heater Temperature | Pin 10 / GPIO 26 (ADC2_9) | **Pin 7 / GPIO 32 (ADC1_4)** | Now uses ADC1 - works with WiFi |
| Ambient Temperature | Pin 8 / GPIO 33 (ADC1_5) | **Pin 8 / GPIO 33 (ADC1_5)** | No change - already ADC1 |

### Control Outputs

| Control | Old Pin | New Pin | Notes |
|---------|---------|---------|-------|
| Pump Enable | Pin 11 / GPIO 27 (ADC2_7) | **Pin 10 / GPIO 26 (ADC2_9)** | Freed up GPIO 27 |
| Pump Speed | Pin 15 / GPIO 13 (ADC2_4) | **Pin 15 / GPIO 13 (ADC2_4)** | No change |
| Heater Relay | Pin 13 / GPIO 12 (ADC2_5) | **Pin 13 / GPIO 12 (ADC2_5)** | No change |

### Unchanged Pins

| Function | Pin |
|----------|-----|
| I2C SDA | GPIO 21 |
| I2C SCL | GPIO 22 |

## Benefits of Changes

### ✅ All Temperature Sensors Now on ADC1
**Previous:** 2 sensors on ADC2 (GPIO 25, 26)  
**Now:** All 3 sensors on ADC1 (GPIO 35, 32, 33)

**Benefit:** Future option to use analog input on temperature sensor pins while WiFi is active!

### ✅ More Flexible Pin Usage
- GPIO 25 (ADC2_8, DAC1) - **Now available**
- GPIO 27 (ADC2_7) - **Now available**

**New Available Pins:** 2 additional GPIOs freed up for expansion

### ✅ Better Hardware Organization
- All temperature inputs use ADC1 (WiFi-compatible)
- All relay outputs use ADC2 (don't need ADC anyway)
- Clean separation of sensor vs. control pins

## Wiring Changes Required

### DS18B20 Temperature Sensors

**Sensor #1 - Inlet Temperature:**
```
OLD: Yellow wire → GPIO 25 (Pin 9)
NEW: Yellow wire → GPIO 35 (Pin 6)
```

**Sensor #2 - Heater Temperature:**
```
OLD: Yellow wire → GPIO 26 (Pin 10)
NEW: Yellow wire → GPIO 32 (Pin 7)
```

**Sensor #3 - Ambient Temperature:**
```
OLD: Yellow wire → GPIO 33 (Pin 8)
NEW: Yellow wire → GPIO 33 (Pin 8)  (No change)
```

### Pump Enable Relay

**Pump Enable Control:**
```
OLD: Relay IN1 → GPIO 27 (Pin 11)
NEW: Relay IN1 → GPIO 26 (Pin 10)
```

**Pump Speed Control:**
```
OLD: Relay IN2 → GPIO 13
NEW: Relay IN2 → GPIO 13  (No change)
```

### Heater Relay

**Heater Control:**
```
OLD: Relay IN → GPIO 12
NEW: Relay IN → GPIO 12  (No change)
```

## ADC Channel Comparison

### Before Pin Changes

**ADC1 (works with WiFi):**
- GPIO 33 ✓ Ambient temp
- GPIO 32 ○ Available
- GPIO 35 ○ Available
- GPIO 34, 36, 39 ○ Available

**ADC2 (disabled with WiFi):**
- GPIO 25 ✓ Inlet temp
- GPIO 26 ✓ Heater temp
- GPIO 27 ✓ Pump enable
- GPIO 13 ✓ Pump speed
- GPIO 12 ✓ Heater relay

### After Pin Changes

**ADC1 (works with WiFi):**
- GPIO 35 ✓ Inlet temp
- GPIO 32 ✓ Heater temp
- GPIO 33 ✓ Ambient temp
- GPIO 34, 36, 39 ○ Available

**ADC2 (disabled with WiFi):**
- GPIO 26 ✓ Pump enable
- GPIO 13 ✓ Pump speed
- GPIO 12 ✓ Heater relay
- GPIO 25, 27 ○ **Now available**
- GPIO 4, 14, 15 ○ Available

## Pin Characteristics

### New Temperature Sensor Pins

**GPIO 35 (Inlet Temperature):**
- ADC1_7 channel
- **Input-only** (no output capability)
- No internal pull-up (external 4.7kΩ resistor required anyway)
- Works perfectly for DS18B20 1-Wire sensor
- ADC available with WiFi active

**GPIO 32 (Heater Temperature):**
- ADC1_4 channel
- Bidirectional (input/output)
- Touch capable (T9)
- Works perfectly for DS18B20 1-Wire sensor
- ADC available with WiFi active

**GPIO 33 (Ambient Temperature):**
- ADC1_5 channel
- Bidirectional (input/output)
- Touch capable (T8)
- Works perfectly for DS18B20 1-Wire sensor
- ADC available with WiFi active

### New Pump Enable Pin

**GPIO 26 (Pump Enable Relay):**
- ADC2_9 channel
- Bidirectional (input/output)
- Has DAC2 (digital-to-analog converter)
- Works perfectly for relay control
- ADC not usable with WiFi (but not needed for relay)

## Newly Available Pins

### GPIO 25
- ADC2_8 + DAC1
- Bidirectional I/O
- Digital-to-analog output capability
- **Good for:** Additional relay, LED, sensor

### GPIO 27
- ADC2_7
- Bidirectional I/O
- Touch capable (T7)
- **Good for:** Additional relay, LED, sensor

## Files Updated

All documentation has been updated to reflect the new pin assignments:

1. ✅ Config.h - Pin definitions
2. ✅ README.md - Hardware documentation
3. ✅ INSTALL.md - Setup instructions
4. ✅ QUICK_REFERENCE.md - Quick reference
5. ✅ WIRING_GUIDE.md - Wiring diagrams
6. ✅ ESP32_PIN_REFERENCE.md - Complete pin reference
7. ✅ PIN_STATUS_SUMMARY.md - Pin usage summary
8. ✅ 3RD_SENSOR_ADDITION.md - Third sensor guide
9. ✅ This file - PIN_CHANGES.md

## Migration Checklist

If you have an existing build, follow these steps:

- [ ] Power off ESP32 and disconnect from pool equipment
- [ ] Move inlet temp sensor from GPIO 25 to GPIO 35
- [ ] Move heater temp sensor from GPIO 26 to GPIO 32
- [ ] Verify ambient temp sensor still on GPIO 33 (no change)
- [ ] Move pump enable relay from GPIO 27 to GPIO 26
- [ ] Verify pump speed relay still on GPIO 13 (no change)
- [ ] Verify heater relay still on GPIO 12 (no change)
- [ ] Upload new code to ESP32
- [ ] Test all sensors read correctly (Serial Monitor)
- [ ] Test all relays operate correctly
- [ ] Test system with actual pool equipment

## Testing

After updating wiring and code:

**Serial Monitor (115200 baud):**
```
Expected output:
"Inlet sensor devices: 1"     ← Should detect on GPIO 35
"Heater sensor devices: 1"    ← Should detect on GPIO 32
"Ambient sensor devices: 1"   ← Should detect on GPIO 33
"Inlet: XX.XF  Heater: XX.XF  Ambient: XX.XF"
```

**Test Commands:**
1. Press pump button → Relay on GPIO 26 should activate
2. Press heater button → Relay on GPIO 12 should activate
3. Check Blynk dashboard → All temps should update
4. Use Alexa voice control → Verify commands work

## Notes

### Why These Changes?

**ADC Compatibility:**
- ESP32's ADC2 cannot be used when WiFi is active
- Temperature sensors were on ADC2 (GPIO 25, 26)
- Moving to ADC1 allows future analog reading capability
- All temp sensors now on ADC1 = consistent, WiFi-compatible

**Pin Availability:**
- GPIO 25 and 27 are now available
- Better separation: sensors on ADC1, controls on ADC2
- More flexibility for future expansion

### Input-Only Pin Note

GPIO 35 is "input-only" which means:
- ✅ Perfect for reading sensors (DS18B20)
- ❌ Cannot drive outputs (not needed)
- The 4.7kΩ pull-up resistor provides the necessary voltage

This is actually ideal for a temperature sensor - the pin can only read, not accidentally drive.

## Expansion Options with Freed Pins

### GPIO 25 (Now Available)
- DAC output (0-3.3V analog)
- Can drive PWM
- Good for: Dimming LED, analog control signal, additional relay

### GPIO 27 (Now Available)
- Touch sensor capable
- Good for: Additional relay, capacitive button, sensor input

## Summary

**Old Configuration:**
- 2 temp sensors on ADC2 (won't work with WiFi for analog)
- 1 temp sensor on ADC1 (works with WiFi)
- 3 control outputs using various pins

**New Configuration:**
- **All 3 temp sensors on ADC1** (WiFi-compatible)
- 3 control outputs on efficient pins
- **2 additional GPIOs freed for expansion**
- Better organized pin usage

The new pin assignment is more logical, more flexible, and future-proof! 🎯
