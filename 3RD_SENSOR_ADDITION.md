# 3rd Temperature Sensor Addition - Summary

## Overview
Added a 3rd DS18B20 temperature sensor to monitor ambient/outdoor temperature and log it to Blynk.

## Hardware Changes

### New GPIO Pin
- **GPIO 33** - Ambient/outdoor temperature sensor (DS18B20)

### Wiring (same as other sensors)
```
DS18B20 Sensor #3:
  Red (VCC)    → ESP32 3.3V
  Black (GND)  → ESP32 GND
  Yellow (DATA)→ ESP32 GPIO 33
  
  4.7kΩ pull-up resistor between Yellow (DATA) and Red (VCC)
```

## Software Changes

### Config.h
- Added: `#define TEMP_AMBIENT_PIN 33`
- Added: `#define VPIN_AMBIENT_TEMP V10`

### StateManager.h
- Added: `float ambientTemp` to SystemState
- Added: `bool ambientTempValid` to SystemState

### StateManager.cpp
- Initialize ambientTemp to 0.0
- Initialize ambientTempValid to false

### TemperatureSensors.cpp
- Added OneWire instance: `ambientWire(TEMP_AMBIENT_PIN)`
- Added DallasTemperature instance: `ambientSensor(&ambientWire)`
- Modified `initTemperatureSensors()` to initialize 3rd sensor
- Modified `readTemperatures()` to read 3rd sensor
- Added logging for ambient temperature

### PoolController.ino
- Modified `syncTemperaturesToBlynk()` to send V10 value

## Blynk Configuration

### New Virtual Pin
**V10: Ambient Temperature**
- Type: Display (read-only)
- Unit: °F
- Updates: Every 2 seconds (with other temps)

### Setup in Blynk Dashboard
1. Go to your Blynk template
2. Add new Datastream:
   - Name: "Ambient Temperature"
   - Pin: V10
   - Data Type: Double
   - Units: °F
   - Min: 0, Max: 120
3. Add widget to display V10 on dashboard

## Usage

### Purpose
- Monitor outdoor/ambient temperature
- Compare pool temperature to outdoor conditions
- Track temperature trends
- Optional: Use for smart heating decisions (future enhancement)

### Serial Monitor Output
Temperature readings now show all 3 sensors:
```
Inlet: 78.5°F  Heater: 79.2°F  Ambient: 65.3°F
```

### Blynk Dashboard
The ambient temperature will automatically appear on V10 when:
- Sensor is connected and working
- WiFi is connected
- Blynk is connected
- Reading is valid (between 32-104°F)

## Testing Checklist

- [ ] Verify 4.7kΩ pull-up resistor installed
- [ ] Connect sensor to GPIO 33
- [ ] Upload updated code
- [ ] Check Serial Monitor (115200 baud)
- [ ] Verify "Ambient sensor devices: 1" message
- [ ] Verify temperature readings appear
- [ ] Check Blynk dashboard shows V10 value
- [ ] Disconnect sensor and verify "disconnected" warning

## Troubleshooting

**"Ambient sensor devices: 0"**
- Check wiring to GPIO 33
- Verify 4.7kΩ pull-up resistor installed
- Test sensor on different GPIO to rule out defect

**"WARNING: Ambient sensor disconnected"**
- Check all 3 wire connections
- Verify sensor power (3.3V)
- Check for damaged sensor

**Temperature shows on Serial but not Blynk**
- Verify V10 configured in Blynk template
- Check Blynk connection status
- Ensure widget added to dashboard

**Temperature out of range**
- Sensor reading below 32°F or above 104°F
- Check sensor placement (extreme temperatures)
- Verify sensor is not damaged

## Future Enhancements

Possible uses for the ambient temperature data:

1. **Smart Heating Logic**
   - Don't heat if ambient temp is too cold (wastes energy)
   - Adjust target based on outdoor conditions

2. **Historical Tracking**
   - Log temperature trends
   - Analyze heating efficiency vs outdoor temp
   - Generate reports

3. **Alerts**
   - Notify if pool temp drops too close to ambient
   - Alert if outdoor temp drops below freezing

4. **Display on LCD**
   - Could show on line 4 instead of connection status
   - Or rotate display to show all temps

5. **Alexa Integration**
   - Add as separate temperature sensor
   - Voice query: "What's the outdoor temperature?"

## Files Modified

1. Config.h - Added GPIO and Blynk pin definitions
2. StateManager.h - Added state variables
3. StateManager.cpp - Initialize new variables
4. TemperatureSensors.cpp - Read 3rd sensor
5. PoolController.ino - Sync to Blynk
6. README.md - Updated documentation
7. INSTALL.md - Updated setup instructions
8. QUICK_REFERENCE.md - Updated reference
9. WIRING_GUIDE.md - Added wiring diagram

## Hardware Summary

Total DS18B20 sensors: **3**
1. GPIO 25 - Inlet temperature (pool control)
2. GPIO 26 - Heater outlet temperature (pool control)
3. GPIO 33 - Ambient/outdoor temperature (monitoring only)

Each requires:
- 3.3V power
- Ground
- 4.7kΩ pull-up resistor

## Notes

- The ambient sensor is for **monitoring only**
- It does NOT affect heater control logic
- It does NOT affect safety interlocks
- It's purely for data logging and display
- Can be disconnected without affecting pool operation
- Future firmware could use it for advanced features
