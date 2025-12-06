# Relay Interlock Logic - Hardware Design

## Overview

This document describes the hardware interlock logic added to ensure only **one relay is active at a time** for pump control. The design uses TTL logic gates (SN74LS08N quad AND gate and SN74LS04N hex inverter) to create a mutually exclusive relay control scheme.

## Problem Statement

The original design could theoretically have both pump relays energized simultaneously if software had a bug or the ESP32 entered an undefined state. This modification ensures **hardware-level mutual exclusion** - only one relay can be ON at any given time.

## Logic Design

### Control Signals from ESP32

- **PUMP_ENABLE** (GPIO 26, Pin 10): Master enable signal
- **PUMP_SPEED** (GPIO 13, Pin 15): Speed selector (LOW = low speed, HIGH = high speed)

### Desired States

| PUMP_ENABLE | PUMP_SPEED | RELAY_LOW | RELAY_HIGH | Pump State |
|-------------|------------|-----------|------------|------------|
| LOW         | X (don't care) | OFF  | OFF        | Pump OFF   |
| HIGH        | LOW        | ON        | OFF        | Pump LOW   |
| HIGH        | HIGH       | OFF       | ON         | Pump HIGH  |

### Boolean Logic Equations

```
RELAY_LOW  = PUMP_ENABLE AND (NOT PUMP_SPEED)
RELAY_HIGH = PUMP_ENABLE AND PUMP_SPEED
```

**Key Property:** RELAY_LOW and RELAY_HIGH are mutually exclusive - they can never both be HIGH simultaneously.

**Proof:**
- If PUMP_SPEED = LOW: RELAY_LOW = HIGH, RELAY_HIGH = LOW
- If PUMP_SPEED = HIGH: RELAY_LOW = LOW, RELAY_HIGH = HIGH

## Circuit Schematic

### Components Required

1. **SN74LS04N** - Hex Inverter (14-pin DIP)
2. **SN74LS08N** - Quad 2-input AND Gate (14-pin DIP)
3. **0.1µF ceramic capacitors** (2) - for power supply decoupling
4. **10kΩ resistors** (2) - optional pull-down resistors on ESP32 outputs

### IC Pinouts

#### SN74LS04N (Hex Inverter)
```
        +--------+
   1A --|1    14|-- VCC (5V)
   1Y --|2    13|-- 6A
   2A --|3    12|-- 6Y
   2Y --|4    11|-- 5A
   3A --|5    10|-- 5Y
   3Y --|6     9|-- 4A
  GND --|7     8|-- 4Y
        +--------+

We only use gate 1:
  Pin 1 (1A): PUMP_SPEED input
  Pin 2 (1Y): NOT_SPEED output
```

#### SN74LS08N (Quad AND Gate)
```
        +--------+
   1A --|1    14|-- VCC (5V)
   1B --|2    13|-- 4B
   1Y --|3    12|-- 4A
   2A --|4    11|-- 4Y
   2B --|5    10|-- 3B
   2Y --|6     9|-- 3A
  GND --|7     8|-- 3Y
        +--------+

We use gates 1 and 2:
  Gate 1 (RELAY_LOW logic):
    Pin 1 (1A): PUMP_ENABLE input
    Pin 2 (1B): NOT_SPEED input (from inverter)
    Pin 3 (1Y): RELAY_LOW output

  Gate 2 (RELAY_HIGH logic):
    Pin 4 (2A): PUMP_ENABLE input
    Pin 5 (2B): PUMP_SPEED input
    Pin 6 (2Y): RELAY_HIGH output
```

### Wiring Diagram

```
ESP32                                    Logic Gates                           Relays
┌──────────────┐
│              │
│  GPIO 26 ────┼──────┬─────────────────┬──────────────────────┐
│ (ENABLE)     │      │                 │                      │
│              │      │            ┌────▼────┐            ┌────▼────┐
│  GPIO 13 ────┼──────┼────────┬───┤ 1A  1Y  ├────────────┤ 2A  2Y  ├──┐
│ (SPEED)      │      │        │   │  SN74   │            │  SN74   │  │
│              │      │    ┌───▼──►│ LS08N   │            │ LS08N   │  │
│              │      │    │   │   │ (AND)   │            │ (AND)   │  │
│              │      │    │   │   └─────────┘            └─────────┘  │
│              │      │    │   │        │                      │        │
│              │      │    │   │        │ RELAY_LOW            │        │
│  GND ────────┼──────┼────┼───┼────────┼──────────────────────┼────────┼──────┐
│              │      │    │   │        │                      │        │      │
└──────────────┘      │    │   │        │                      │        │      │
                      │    │   │   ┌────▼────┐                │        │      │
                   [10kΩ] │   │   │  Relay  │                │        │      │
                    (PD)  │   │   │ Module  │                │        │      │
                      │    │   │   │ Channel │                │        │      │
                      ▼    │   │   │   1     │                │        │      │
                     GND   │   │   │  (LOW)  │                │        │      │
                           │   │   └─────────┘                │        │      │
                      [10kΓ] │                                │        │      │
                       (PD) │                                 │        │      │
                        │   │   ┌───────────┐                │        │      │
                        ▼   │   │    1A      │                │        │      │
                       GND  └───┤    1Y      │────────────────┘        │      │
                               │   SN74LS04N │                         │      │
                               │  (INVERTER) │                         │      │
                               └─────────────┘                         │      │
                                    NOT_SPEED                          │      │
                                                                        │      │
                                                             RELAY_HIGH │      │
                                                                   ┌────▼────┐ │
                                                                   │  Relay  │ │
                                                                   │ Module  │ │
                                                                   │ Channel │ │
                                                                   │   2     │ │
                                                                   │ (HIGH)  │ │
                                                                   └─────────┘ │
                                                                               │
Power Supply:                                                                  │
  5V ────────┬────────────────────┬───────────────────────┐                   │
             │                    │                       │                   │
        SN74LS04N              SN74LS08N            Relay Modules             │
         Pin 14                 Pin 14                  VCC                   │
           (VCC)                (VCC)                                         │
             │                    │                       │                   │
          [0.1µF]              [0.1µF]                    │                   │
             │                    │                       │                   │
  GND ──────┴────────────────────┴───────────────────────┴───────────────────┘
          (Decoupling             (Decoupling
          capacitor)              capacitor)
```

### Connection Table

| Signal Name    | From              | To                           | Wire Color | Notes                          |
|----------------|-------------------|------------------------------|------------|--------------------------------|
| PUMP_ENABLE    | ESP32 GPIO 26     | SN74LS08N Pin 1 (1A)        | Orange     | Direct connection              |
| PUMP_ENABLE    | ESP32 GPIO 26     | SN74LS08N Pin 4 (2A)        | Orange     | Jumper from pin 1              |
| PUMP_SPEED     | ESP32 GPIO 13     | SN74LS04N Pin 1 (1A)        | Yellow     | To inverter                    |
| PUMP_SPEED     | ESP32 GPIO 13     | SN74LS08N Pin 5 (2B)        | Yellow     | Direct to AND gate             |
| NOT_SPEED      | SN74LS04N Pin 2   | SN74LS08N Pin 2 (1B)        | Green      | Inverter output to AND gate    |
| RELAY_LOW      | SN74LS08N Pin 3   | Relay Module CH1 IN         | Blue       | Low speed relay control        |
| RELAY_HIGH     | SN74LS08N Pin 6   | Relay Module CH2 IN         | Purple     | High speed relay control       |
| +5V            | Power Supply      | SN74LS04N Pin 14            | Red        | Power for inverter             |
| +5V            | Power Supply      | SN74LS08N Pin 14            | Red        | Power for AND gate             |
| +5V            | Power Supply      | Relay Module VCC            | Red        | Power for relays               |
| GND            | ESP32 GND         | SN74LS04N Pin 7             | Black      | Common ground                  |
| GND            | ESP32 GND         | SN74LS08N Pin 7             | Black      | Common ground                  |
| GND            | ESP32 GND         | Relay Module GND            | Black      | Common ground                  |

## Physical Implementation

### Breadboard Layout (for testing)

```
 5V Rail ─────────────────────────────────────────────────────────────────
   │
   ├─────┬──────── VCC (Pin 14) SN74LS04N
   │     │
   │   [0.1µF]
   │     │
   │     └──── GND
   │
   ├─────┬──────── VCC (Pin 14) SN74LS08N
   │     │
   │   [0.1µF]
   │     │
   │     └──── GND
   │
   └──────────── Relay Module VCC

GND Rail ────────────────────────────────────────────────────────────────
```

### PCB Layout Recommendations

For a permanent installation, consider designing a small PCB with:

1. **Component placement:**
   - Logic ICs near ESP32 connection
   - Decoupling capacitors directly adjacent to each IC
   - Relay connections on opposite edge

2. **Trace routing:**
   - Keep PUMP_ENABLE and PUMP_SPEED traces short
   - Use ground plane for noise immunity
   - Separate high-current relay traces from logic traces

3. **Connectors:**
   - 3-pin header for ESP32 signals (ENABLE, SPEED, GND)
   - 2-pin screw terminals for each relay output
   - 2-pin power input (5V, GND)

## Power Considerations

### Supply Voltage

- **Logic ICs:** 5V ± 0.25V (4.75V - 5.25V)
- **ESP32 outputs:** 3.3V logic levels

**Important:** SN74LS series TTL chips recognize 3.3V as logic HIGH (minimum HIGH input = 2.0V), so ESP32 outputs are compatible.

### Current Consumption

| Component      | Typical Current | Maximum Current |
|----------------|-----------------|-----------------|
| SN74LS04N      | 1.6 mA/gate     | 10 mA total     |
| SN74LS08N      | 2.0 mA/gate     | 15 mA total     |
| **Total Logic**| **~5 mA**       | **25 mA**       |
| Relay Module   | 15-30 mA/relay  | 60 mA           |
| **Grand Total**| **~40 mA**      | **85 mA**       |

A 5V, 500mA power supply is more than adequate.

### Pull-down Resistors (Optional)

Adding 10kΩ pull-down resistors on PUMP_ENABLE and PUMP_SPEED ensures defined logic LOW state during ESP32 boot:

```
GPIO 26 ────┬──── Logic circuit
            │
          [10kΩ]
            │
           GND

GPIO 13 ────┬──── Logic circuit
            │
          [10kΩ]
            │
           GND
```

**Benefit:** Prevents relays from clicking during ESP32 startup/reset.

## Testing Procedure

### Bench Testing (before connecting pump)

1. **Power-up test:**
   ```
   - Apply 5V to logic ICs
   - Verify VCC = 5V at pins 14 of both ICs
   - Verify GND at pins 7 of both ICs
   ```

2. **Logic level test with multimeter:**
   ```
   Set PUMP_ENABLE = LOW, PUMP_SPEED = LOW
     → Verify RELAY_LOW = LOW
     → Verify RELAY_HIGH = LOW

   Set PUMP_ENABLE = HIGH, PUMP_SPEED = LOW
     → Verify RELAY_LOW = HIGH (should see ~3.3-5V)
     → Verify RELAY_HIGH = LOW

   Set PUMP_ENABLE = HIGH, PUMP_SPEED = HIGH
     → Verify RELAY_LOW = LOW
     → Verify RELAY_HIGH = HIGH (should see ~3.3-5V)
   ```

3. **Relay click test:**
   ```
   Manually toggle PUMP_ENABLE and PUMP_SPEED
   Listen for relay clicks
   Verify only ONE relay clicks at a time
   ```

4. **Mutual exclusion verification:**
   ```
   Use LED indicators or multimeter
   Confirm RELAY_LOW and RELAY_HIGH never both HIGH
   Test all 4 input combinations
   ```

### Truth Table Verification

| ENABLE | SPEED | NOT_SPEED | RELAY_LOW | RELAY_HIGH | Expected State |
|--------|-------|-----------|-----------|------------|----------------|
| 0      | 0     | 1         | 0         | 0          | ✓ Both OFF     |
| 0      | 1     | 0         | 0         | 0          | ✓ Both OFF     |
| 1      | 0     | 1         | 1         | 0          | ✓ Only LOW ON  |
| 1      | 1     | 0         | 0         | 1          | ✓ Only HIGH ON |

### Integration Testing

After confirming logic works correctly:

1. Connect to relay module
2. Test relay switching without pump connected
3. Verify relay contacts switch correctly
4. Connect to pump motor controller
5. Test full operation under supervision

## Safety Features

### Hardware-Level Mutual Exclusion

This circuit provides **fail-safe** protection:

- **No software bugs can cause both relays ON** - physically impossible
- **Clean transitions** - switching between states has no overlap
- **Predictable behavior** - simple combinational logic, no race conditions

### Advantages Over Software-Only Control

| Software Control | Hardware Interlock |
|------------------|-------------------|
| Relies on code correctness | Physically enforced |
| Can fail during ESP32 reset | Always active |
| Timing-dependent | Instantaneous |
| Requires testing all code paths | One-time verification |

## Bill of Materials (BOM)

| Qty | Part Number   | Description                  | Supplier   | Est. Cost |
|-----|---------------|------------------------------|------------|-----------|
| 1   | SN74LS04N     | Hex Inverter, 14-DIP         | Digi-Key   | $0.50     |
| 1   | SN74LS08N     | Quad AND Gate, 14-DIP        | Digi-Key   | $0.50     |
| 2   | 0.1µF ceramic | Decoupling capacitor         | Digi-Key   | $0.10     |
| 2   | 10kΩ resistor | Pull-down resistor (optional)| Digi-Key   | $0.05     |
| 1   | Breadboard    | For prototyping              | Digi-Key   | $5.00     |
| -   | Jumper wires  | 22 AWG solid core            | -          | $2.00     |
|     |               |                              | **Total:** | **~$8**   |

## Alternative IC Options

If SN74LS series not available:

| Original  | Alternative   | Notes                                    |
|-----------|---------------|------------------------------------------|
| SN74LS04N | CD4049        | CMOS inverter, slower but works          |
| SN74LS04N | 74HC04        | CMOS, faster, lower power                |
| SN74LS08N | CD4081        | CMOS quad AND, slower                    |
| SN74LS08N | 74HC08        | CMOS, faster, lower power                |

**Important:** If using HC series (High-speed CMOS), ensure 3.3V compatibility. Most HC parts work with 3.3V logic.

## Troubleshooting

### Both relays energize simultaneously

**Possible causes:**
- Wiring error - double-check connections
- Faulty logic IC - replace
- Power supply issue - check voltage

**Debug steps:**
1. Disconnect relay inputs
2. Measure logic outputs with multimeter
3. Verify truth table
4. Check for solder bridges or shorts

### Relays don't energize at all

**Possible causes:**
- No power to logic ICs
- ESP32 outputs stuck LOW
- Bad relay module

**Debug steps:**
1. Check 5V present at IC pin 14
2. Measure ESP32 GPIO outputs (should be 3.3V when HIGH)
3. Test relay module with direct 5V signal

### Intermittent operation

**Possible causes:**
- Poor connections
- Inadequate power supply
- Missing decoupling capacitors

**Debug steps:**
1. Solder connections instead of breadboard
2. Add/replace decoupling caps
3. Check power supply voltage under load

## Software Considerations

### No Code Changes Required

The ESP32 code **does not need modification** - the control logic remains the same:

```cpp
// In PumpControl.cpp - unchanged
void applyPumpState() {
  switch (state.pumpState) {
    case PUMP_OFF:
      digitalWrite(PUMP_ENABLE_PIN, LOW);
      digitalWrite(PUMP_SPEED_PIN, LOW);
      break;

    case PUMP_LOW:
      digitalWrite(PUMP_ENABLE_PIN, HIGH);
      digitalWrite(PUMP_SPEED_PIN, LOW);
      break;

    case PUMP_HIGH:
      digitalWrite(PUMP_ENABLE_PIN, HIGH);
      digitalWrite(PUMP_SPEED_PIN, HIGH);
      break;
  }
}
```

The hardware logic gates simply intercept the signals and enforce mutual exclusion.

### Startup Behavior

During ESP32 boot:
- GPIOs default to INPUT mode (high impedance)
- Pull-down resistors ensure LOGICs see LOW
- Both relays remain OFF until `initPumpControl()` runs
- First state is PUMP_OFF (both outputs LOW)

## Maintenance

### Periodic Checks

- **Monthly:** Verify both relays clicking properly
- **Quarterly:** Test all 3 states (OFF, LOW, HIGH)
- **Annually:** Clean contacts if using mechanical relays

### IC Replacement

If a logic IC fails:
1. Power off system
2. Remove old IC (or cut legs if soldered)
3. Install new IC in correct orientation (notch/dot = Pin 1)
4. Verify pinout matches datasheet
5. Test before reconnecting pump

## Documentation References

- **SN74LS04N Datasheet:** [Texas Instruments](https://www.ti.com/lit/ds/symlink/sn74ls04.pdf)
- **SN74LS08N Datasheet:** [Texas Instruments](https://www.ti.com/lit/ds/symlink/sn74ls08.pdf)
- **ESP32 Datasheet:** [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)

## Revision History

| Version | Date       | Changes                              |
|---------|------------|--------------------------------------|
| 1.0     | 2025-12-06 | Initial relay interlock design       |

---

**Author:** Pool Controller Development Team
**Last Updated:** 2025-12-06
