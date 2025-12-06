# ESP32 Pool Controller - Power Specifications

## Overview

This document provides comprehensive power consumption data for the ESP32 Pool Controller system, including both the low-voltage control system and the high-voltage pool equipment circuits.

## Table of Contents
1. [Control System Power (5V DC)](#control-system-power-5v-dc)
2. [Pool Equipment Power (120V AC)](#pool-equipment-power-120v-ac)
3. [Peak Power Scenarios](#peak-power-scenarios)
4. [Electrical Circuit Requirements](#electrical-circuit-requirements)
5. [Power Supply Recommendations](#power-supply-recommendations)
6. [Safety Considerations](#safety-considerations)

---

## Control System Power (5V DC)

The ESP32 controller and all peripheral devices operate on **5V DC** power supplied via USB or dedicated 5V power adapter.

### Component Power Consumption

| Component | Quantity | Current Draw (mA) | Power (W) | Notes |
|-----------|----------|-------------------|-----------|-------|
| **ESP32-WROOM Dev Board** | 1 | 80-260 | 0.4-1.3 | Active WiFi increases current |
| **DS18B20 Temp Sensors** | 3 | 1.5 each | 0.0225 total | Inlet, heater, ambient sensors |
| **20×4 LCD with I2C** | 1 | 60-80 | 0.3-0.4 | Backlight ON |
| **MCP23008 GPIO Expander** | 1 | 1 | 0.005 | I2C interface |
| **SN74LS08N AND Gate IC** | 1 | 4 | 0.02 | 2 gates used, hardware interlock |
| **SN74LS04N Inverter IC** | 1 | 1.6 | 0.008 | 1 gate used, hardware interlock |
| **SRD-03VDC-SL-C Relays** | 3 max | 90 each | 0.27 each | Coil energized |
| **Pull-up Resistors (4.7kΩ)** | 3 | <1 | <0.01 | For 1-Wire sensors |
| **Push Buttons** | 5 | Negligible | ~0 | Passive components |

### Control System Power Summary

| Operating Mode | Current Draw | Power Consumption |
|----------------|--------------|-------------------|
| **Idle** (WiFi connected, no relays) | ~150 mA | 0.75 W |
| **Typical** (WiFi active, 2 relays ON) | ~430 mA | 2.15 W |
| **Peak** (WiFi active, all 3 relays ON) | ~520 mA | 2.60 W |

### Recommended Control System Power Supply

**Minimum:** 5V @ 1A (5W)
**Recommended:** 5V @ 2A (10W)
**Type:** USB Type-A or barrel connector (5.5mm x 2.1mm)

**Why 2A?** Provides 4× safety margin above peak consumption, ensuring stable operation during WiFi bursts and relay switching transients.

**Acceptable Power Sources:**
- Quality USB wall adapter (2A rated)
- USB power bank (for testing)
- Buck converter from 12V source (LM2596 or similar)
- PC USB port (1A minimum, 2A preferred)

---

## Pool Equipment Power (120V AC)

The pool pump and heater operate on **120V AC** power and are controlled via relay modules.

### Pool Equipment Specifications

#### Pool Pump (1 HP, 2-Speed)

| Speed Mode | Voltage | Current Draw | Power Consumption | Use Case |
|------------|---------|--------------|-------------------|----------|
| **OFF** | - | 0 AMP | 0 W | Not circulating |
| **LOW Speed** | 120 VAC | 2.2 AMP | 264 W | Circulation, cooldown |
| **HIGH Speed** | 120 VAC | 9.5 AMP | 1,140 W | Heating, filtration |

**Notes:**
- Inrush current can be 2-3× running current for ~1 second during startup
- LOW speed sufficient for circulation and cooldown after heating
- HIGH speed required during active heating for proper flow rate

#### Pool Heater (Gas with Electronic Ignition)

| Component | Voltage | Current Draw | Power Consumption | Notes |
|-----------|---------|--------------|-------------------|-------|
| **Control Circuit** | 120 VAC | 4.0 AMP | 480 W | Ignition, gas valve, blower |
| **Gas Consumption** | - | - | Variable | Depends on BTU rating |

**Notes:**
- Controller power is for electrical components only
- Gas consumption (BTU/hr) is separate from electrical power
- Typical gas pool heaters: 100,000-400,000 BTU/hr
- Electronic ignition eliminates standing pilot (saves gas)

#### High-Power Relay Interlock (G7L-2A)

| Component | Voltage | Current Draw | Power Consumption | Notes |
|-----------|---------|--------------|-------------------|-------|
| **G7L-2A-BUBJ-CB-AC100/120** | 120 VAC | 17 mA each | 2.0 VA each | 2 relays, controlled by SRD relays |

**Notes:**
- These relays are triggered by the SRD-03VDC-SL-C relays via hardware interlock logic
- Provide hardware-level mutual exclusion for pump control
- 25A contact rating handles high-power pump loads
- Coil power: 2.0 VA (1.7-2.5 VA range)

---

## Peak Power Scenarios

### Scenario 1: Idle State
**Pool Equipment:** All OFF
**Controller:** WiFi connected, LCD backlight ON

| System | Power Draw |
|--------|------------|
| Controller (5V DC) | 0.75 W |
| Pool Equipment (120V AC) | 0 W |
| **Total** | **0.75 W** |

**Current:** 150 mA @ 5V DC, 0 AMP @ 120V AC

---

### Scenario 2: Pump Only (Low Speed)
**Pool Equipment:** Pump LOW
**Controller:** Active with 1 relay energized

| System | Power Draw |
|--------|------------|
| Controller (5V DC) | 2.06 W |
| Pump Low Speed (120V AC) | 264 W |
| G7L Relay Coil (120V AC) | 2.0 W |
| **Total** | **268 W** |

**Current:** 412 mA @ 5V DC, 2.22 AMP @ 120V AC

---

### Scenario 3: Pump Only (High Speed)
**Pool Equipment:** Pump HIGH
**Controller:** Active with 1 relay energized

| System | Power Draw |
|--------|------------|
| Controller (5V DC) | 2.06 W |
| Pump High Speed (120V AC) | 1,140 W |
| G7L Relay Coil (120V AC) | 2.0 W |
| **Total** | **1,144 W** |

**Current:** 412 mA @ 5V DC, 9.52 AMP @ 120V AC

---

### Scenario 4: PEAK POWER (Heating Mode)
**Pool Equipment:** Pump HIGH + Heater ON
**Controller:** Active with 2 relays energized (pump + heater)

| System | Power Draw |
|--------|------------|
| Controller (5V DC) | 2.60 W |
| Pump High Speed (120V AC) | 1,140 W |
| Heater Control (120V AC) | 480 W |
| G7L Relay Coil x2 (120V AC) | 4.0 W |
| **Total** | **1,627 W** |

**Current:** 520 mA @ 5V DC, **13.53 AMP @ 120V AC**

**This is the MAXIMUM simultaneous electrical load.**

---

## Electrical Circuit Requirements

### 120V AC Circuit for Pool Equipment

**Required Circuit Breaker:** 20 AMP (minimum 15 AMP)
**Wire Gauge:** 12 AWG copper (minimum)
**Recommended:** Dedicated 20A circuit
**Outlet Type:** GFCI-protected outlet (required for wet locations)

#### Circuit Loading Analysis

| Scenario | Load (AMP) | % of 15A Circuit | % of 20A Circuit | Status |
|----------|------------|------------------|------------------|--------|
| Idle | 0 | 0% | 0% | ✓ Safe |
| Pump Low | 2.22 | 15% | 11% | ✓ Safe |
| Pump High | 9.52 | 63% | 48% | ✓ Safe |
| **Peak (Pump+Heater)** | **13.53** | **90%** | **68%** | ⚠️ See notes |

**Notes:**
- **15A Circuit:** Peak load at 90% is acceptable for continuous duty per NEC (80% rule allows 12A continuous)
- **20A Circuit:** RECOMMENDED - Peak load at 68% provides comfortable safety margin
- **Inrush Current:** Pump startup can briefly spike to 20-30 AMP - 20A breaker handles this better
- **Code Compliance:** Check local electrical codes for pool equipment requirements

### Recommended Electrical Installation

```
Main Panel
    │
    ├─ 20A Breaker (120V AC) ────┬─ Pool Pump (via Relay)
    │                             └─ Heater Control (via Relay)
    │
    └─ Standard 15A Circuit ──────── ESP32 Controller (5V USB Adapter)
```

**Important:** Pool equipment and controller should be on separate circuits for reliability and code compliance.

---

## Power Supply Recommendations

### For ESP32 Controller (5V DC)

**Option 1: USB Wall Adapter (Recommended)**
- Voltage: 5V DC
- Current: 2A minimum (2.5A preferred)
- Type: USB Type-A output
- Quality: UL/ETL listed, regulated output
- Example: Anker, Aukey, or similar name-brand adapter

**Option 2: Buck Converter from 12V**
- Input: 12V DC (from existing 12V power supply)
- Output: 5V @ 2A
- Module: LM2596 or MP2307 based
- Efficiency: >90%
- Use case: If 12V already available in enclosure

**Option 3: Industrial DIN Rail Power Supply**
- Input: 120V AC
- Output: 5V @ 3A
- Type: DIN rail mount
- Example: Mean Well HDR-15-5
- Use case: Professional installations in control panels

### For Pool Equipment (120V AC)

**Circuit Breaker:** 20 AMP GFCI
**Wire Size:** 12 AWG copper (20A rated)
**Conduit:** Weatherproof as required by code
**Outlet:** Outdoor-rated GFCI outlet or direct-wired to equipment

---

## Safety Considerations

### Electrical Safety

**CRITICAL - Always follow these safety rules:**

1. **Low Voltage Separation**
   - NEVER connect 120V AC directly to ESP32 or any 5V component
   - Relays provide complete electrical isolation between 5V control and 120V power
   - Maintain physical separation between low-voltage and high-voltage wiring

2. **Proper Grounding**
   - All metal enclosures must be grounded
   - Pool equipment grounding per NEC Article 680
   - GFCI protection required for all pool-related circuits

3. **Circuit Protection**
   - 20A breaker recommended for pool equipment circuit
   - Fuse or breaker for 5V power supply input
   - Relays rated for actual load (10A+ relays specified)

4. **Wet Location Protection**
   - All 120V connections in weatherproof enclosures (NEMA 3R minimum)
   - ESP32 controller in splash-proof enclosure
   - GFCI protection mandatory

5. **Relay Safety**
   - SRD-03VDC-SL-C relays rated for 10A @ 250V AC
   - Adequate for pump (9.5A) and heater (4A) loads
   - NO/NC contacts properly identified
   - Screw terminals secure with no exposed conductors

### Operational Safety

**Pump-Heater Interlock:**
- System REQUIRES pump running when heater is ON
- Prevents heater dry-fire damage
- Cannot be overridden via software or manual control

**Cooldown Protection:**
- Pump continues 15 minutes after heater shutdown
- Ensures heat dissipation
- Automatic safety feature

**Power Failure Recovery:**
- System defaults to SAFE state (all OFF) on power loss
- Settings restored from flash on reboot
- Manual re-enable required for heater after power failure

### Installation Warnings

⚠️ **WARNING:** Electrical installation must be performed by qualified electrician
⚠️ **WARNING:** Follow all local electrical codes and regulations
⚠️ **WARNING:** Pool equipment installation subject to NEC Article 680
⚠️ **WARNING:** GFCI protection required - can save lives
⚠️ **WARNING:** Gas heater installation requires licensed professional

---

## Power Consumption Summary Table

### Complete System Overview

| Operating Scenario | 5V DC Current | 5V DC Power | 120V AC Current | 120V AC Power | Total Power |
|-------------------|---------------|-------------|-----------------|---------------|-------------|
| **Idle** | 150 mA | 0.75 W | 0 A | 0 W | 0.75 W |
| **Controller Only** | 520 mA | 2.60 W | 0 A | 0 W | 2.60 W |
| **Pump Low** | 412 mA | 2.06 W | 2.22 A | 266 W | 268 W |
| **Pump High** | 412 mA | 2.06 W | 9.52 A | 1,142 W | 1,144 W |
| **Heater Only** | 340 mA | 1.70 W | 4.02 A | 482 W | 484 W |
| **PEAK (Pump+Heater)** | 520 mA | 2.60 W | 13.53 A | 1,624 W | 1,627 W |

### Annual Energy Consumption Estimates

Based on typical pool operation patterns:

**Assumptions:**
- 6 months pool season (180 days)
- Pump: 8 hours/day average (mix of LOW/HIGH)
- Heater: 2 hours/day average (peak season)
- Controller: 24/7/365

| Component | Daily Usage | Daily Energy | Annual Energy | Annual Cost* |
|-----------|-------------|--------------|---------------|--------------|
| **Controller** | 24 hrs | 0.062 kWh | 22.7 kWh | $2.72 |
| **Pump** | 8 hrs avg | 5.6 kWh | 1,008 kWh | $121.00 |
| **Heater** | 2 hrs avg | 0.97 kWh | 175 kWh | $21.00 |
| **Total** | - | 6.7 kWh | **1,207 kWh** | **$144.72** |

*Based on $0.12/kWh average residential rate. Actual costs vary by location and rate structure.

**Notes:**
- Controller uses negligible energy (~$2.68/year)
- Pump is primary energy consumer
- Gas heater fuel cost not included (separate from electrical)
- Using LOW speed when possible significantly reduces pump energy

---

## Voltage Drop Calculations

### For Long Wire Runs (120V AC Circuit)

If pool equipment is distant from breaker panel, calculate voltage drop:

**Formula:** VD = (2 × K × I × L) / CM

Where:
- VD = Voltage Drop (volts)
- K = 12.9 (copper constant)
- I = Current (amps)
- L = One-way distance (feet)
- CM = Circular Mils (wire size)

**12 AWG Copper:** 6,530 CM

**Example:** 50 feet to pool equipment, 13.5A load
- VD = (2 × 12.9 × 13.5 × 50) / 6,530
- VD = 3.4 volts
- Voltage at load = 116.6V (97.2% of source)
- **Acceptable** (NEC allows 3% drop on branch circuits)

**Recommendation:** Keep wire runs under 75 feet with 12 AWG for 13.5A load.

---

## Emergency Power Considerations

### Battery Backup Options

**Controller Only (for monitoring/WiFi during outage):**
- USB power bank: 10,000 mAh @ 5V
- Runtime: ~19 hours at typical load (422 mA)
- Cost: $20-40
- Use case: Keep monitoring active during brief outages

**NOT RECOMMENDED:** Battery backup for pool equipment (pump/heater) due to high power requirements.

### Generator Sizing

**Minimum Generator:** 2,000W continuous
**Recommended Generator:** 3,000W continuous
**Type:** Inverter generator (clean sine wave for electronics)

**Reason:** Accounts for pump inrush current and provides headroom for other loads.

---

## Thermal Management

### Heat Dissipation

**Controller Components:**
- ESP32: ~1W heat dissipation (WiFi active)
- Relays: ~0.27W each when energized
- Voltage regulator: ~0.5W (if using buck converter)

**Cooling Requirements:**
- Passive cooling sufficient for controller
- Enclosure ventilation recommended
- Avoid direct sunlight on enclosure
- Operating temperature: 0-40°C (32-104°F)

**Relay Mounting:**
- Adequate spacing for heat dissipation
- Screw terminals torqued per specifications
- Verify contacts not overheating during operation

---

## Testing and Verification

### Power Consumption Testing

**Equipment Needed:**
- Multimeter with current measurement
- Kill-A-Watt or similar power meter
- USB current/voltage meter (for 5V system)

**Test Procedure:**

1. **Controller Power Test:**
   - Insert USB power meter between adapter and ESP32
   - Verify idle current: ~140-180 mA
   - Activate all relays, verify peak: ~500-550 mA

2. **Pool Equipment Power Test:**
   - Use Kill-A-Watt on 120V circuit
   - Measure pump LOW: should be ~2.2A, ~260W
   - Measure pump HIGH: should be ~9.5A, ~1,140W
   - Measure pump+heater: should be ~13.5A, ~1,620W

3. **Voltage Drop Test:**
   - Measure voltage at breaker panel: should be ~120V
   - Measure voltage at pump during HIGH operation
   - Difference should be <4V (3% max drop)

**Acceptance Criteria:**
- ✓ All measurements within ±10% of specifications
- ✓ No excessive heat at connections
- ✓ GFCI does not trip during normal operation
- ✓ Relays switch cleanly without arcing

---

## Compliance and Regulations

### Applicable Codes and Standards

**National Electrical Code (NEC):**
- Article 680: Swimming Pools, Fountains, and Similar Installations
- Section 680.6: Grounding requirements
- Section 680.21: Motors (pump requirements)
- Section 680.26: Equipotential bonding

**UL Standards:**
- UL 1081: Swimming Pool Pumps, Filters, and Chlorinators
- UL 1563: Electric Signs
- UL 61010: Safety Requirements for Electrical Equipment

**Local Codes:**
- Always follow local electrical codes (may be more restrictive than NEC)
- Permit and inspection may be required
- Licensed electrician may be mandatory

---

## Troubleshooting Power Issues

### Controller Not Powering Up

**Check:**
- [ ] USB adapter plugged in and working (test with phone)
- [ ] USB cable not damaged
- [ ] Correct polarity if using barrel connector
- [ ] Voltage at ESP32: should be 4.75-5.25V
- [ ] No short circuits on board

### Pump/Heater Not Activating

**Check:**
- [ ] 120V present at outlet (test with multimeter)
- [ ] GFCI not tripped (reset if needed)
- [ ] Breaker not tripped
- [ ] Relay clicking when activated (listen/feel)
- [ ] Wiring to relay terminals secure
- [ ] Relay contacts not welded/stuck

### Intermittent Operation

**Check:**
- [ ] Voltage drop under load (measure at equipment)
- [ ] Loose connections heating up
- [ ] Relay contacts degrading (resistance increasing)
- [ ] Power supply undersized (voltage sag under load)
- [ ] WiFi causing ESP32 brown-out (increase 5V supply current)

### GFCI Nuisance Tripping

**Common Causes:**
- Moisture in connections
- Damaged insulation
- Ground fault in equipment
- GFCI too sensitive (replace with proper rating)

**DO NOT bypass GFCI - find and fix the fault!**

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.1 | 2025-12-06 | Add SN74LS08N and SN74LS04N logic ICs; Add G7L-2A relay specifications; Update all power totals |
| 1.0 | 2024-12-06 | Initial release with complete power specifications |

---

## Summary

**Control System:** 2.60W peak @ 5V (520 mA)
**Pool Equipment:** 1,624W peak @ 120V (13.53 AMP)
**Total Peak Power:** 1,627W

**Required Infrastructure:**
- 5V @ 2A USB power supply for controller
- 20A @ 120V circuit for pool equipment
- GFCI protection mandatory
- Professional installation recommended

**Safety:** Multiple interlocks ensure safe operation. Never bypass safety features.

---

**Questions or concerns about power requirements?**
Consult a licensed electrician for installation planning and code compliance.
