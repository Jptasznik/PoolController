# Watchdog Timeout Fix - Root Cause Analysis

## Problem

Watchdog timeouts occurring in TemperatureTask, ControlTask, and loopTask:

```
E (27596) task_wdt: Task watchdog got triggered. The following tasks/users did not reset the watchdog in time:
E (27596) task_wdt:  - ControlTask (CPU 1)
E (27596) task_wdt:  - loopTask (CPU 1)
E (27596) task_wdt: Tasks currently running:
E (27596) task_wdt: CPU 1: TemperatureTask
```

## Root Cause

**TemperatureTask was stuck in `enterFailsafeMode()` performing blocking network I/O operations** when sensor failures were detected.

### Call Chain That Caused Timeout:

1. `TemperatureSensors.cpp:117` - Sensor failure detected
2. `enterFailsafeMode("CRITICAL_SENSOR_FAILURE")` called
3. Inside `StateManager.cpp:enterFailsafeMode()`:
   - Line 325: `saveHeaterEnabled(false)` - **NVS flash write (100-500ms)**
   - Line 331: `updateLCDDisplay()` - **I2C communication (100-200ms)**
   - Line 332: `syncHeaterToBlynk()` - **Network I/O (can block for SECONDS)**
   - Line 333: `syncPumpToBlynk()` - **Network I/O (can block for SECONDS)**
   - Line 334: `syncHeaterToSinric()` - **Network I/O (can block for SECONDS)**
   - Line 335: `syncPumpToSinric()` - **Network I/O (can block for SECONDS)**
4. **No watchdog reset during these operations**
5. **Total time > 15 seconds** → Watchdog timeout

### Why Network I/O Is Slow:

- `Blynk.virtualWrite()` involves TCP socket operations
- `SinricPro.sendEvent()` involves HTTPS POST requests
- WiFi latency, retries, timeouts can add 5-10+ seconds
- If Blynk/Sinric servers are slow or unreachable, blocking can exceed watchdog timeout

### Why This Affected Multiple Tasks:

- **TemperatureTask** - Stuck in blocking operations (15s watchdog)
- **ControlTask** - Starved by scheduler, couldn't reset watchdog (180s watchdog)
- **loopTask** - Starved by scheduler, couldn't reset watchdog (30s watchdog)

## Solution

**Removed all blocking network and LCD calls from state setter functions** that are called from FreeRTOS tasks.

### Functions Fixed:

1. ✅ `enterFailsafeMode()` - **Critical fix** - removed network sync and LCD update
2. ✅ `triggerEmergencyShutdown()` - removed network sync calls
3. ✅ `setHeaterEnabledState()` - removed network sync and LCD update
4. ✅ `setPumpState()` - removed network sync and LCD update
5. ✅ `setTargetTemperature()` - removed network sync and LCD update
6. ✅ `clearEmergencyShutdown()` - removed LCD update
7. ✅ `clearFailsafeMode()` - removed LCD update
8. ✅ `clearOverheatLockout()` - removed LCD update
9. ✅ `startCooldown()` - removed LCD update

### How Updates Now Work:

**Before (Synchronous - caused timeouts):**
```
Sensor failure → enterFailsafeMode() → updateLCDDisplay() + Blynk sync + Sinric sync
                                       ↑ All blocking, takes 5-15 seconds
```

**After (Asynchronous - non-blocking):**
```
Sensor failure → enterFailsafeMode() → set state.inFailsafeMode = true
                                       ↑ Fast, returns immediately

Main loop (every 2s)  → if (state changed) → Blynk.virtualWrite()  ← Non-blocking context
Main loop (every 30s) → if (state changed) → Sinric.sendEvent()    ← Non-blocking context
LCD Task (every 1s)   → updateLCDDisplay()                         ← Dedicated task
```

### Why This Works:

1. **State changes are immediate** - hardware GPIO updates happen instantly
2. **Network sync is deferred** - main loop handles cloud updates asynchronously
3. **LCD updates are automatic** - dedicated LCD task runs every second
4. **No blocking in tasks** - FreeRTOS tasks return quickly, reset watchdog on time
5. **User sees updates within 1-2 seconds** - acceptable latency for UI

## Testing Recommendations

1. **Test sensor failure scenarios:**
   - Disconnect temperature sensors
   - Verify failsafe mode activates immediately (hardware)
   - Verify Blynk/Sinric updates within 2-30 seconds (UI)

2. **Monitor watchdog timeouts:**
   ```
   Serial Monitor @ 115200 baud
   Look for: "task_wdt: Task watchdog got triggered"
   Should see: No watchdog timeouts
   ```

3. **Test all state changes:**
   - Physical buttons → immediate local response
   - Blynk dashboard → should sync back within 2s
   - Alexa commands → should sync back within 30s

4. **Network failure testing:**
   - Disconnect WiFi
   - Trigger sensor failure
   - System should enter failsafe WITHOUT timeout
   - Reconnect WiFi → updates should sync

## Additional Notes

- **Blynk timer** syncs every 2 seconds (`BLYNK_UPDATE_INTERVAL`)
- **Sinric timer** syncs every 30 seconds (`SINRIC_TEMP_UPDATE_INTERVAL`)
- **LCD task** updates every 1 second (`LCD_UPDATE_INTERVAL`)
- **Forward declarations** for sync functions remain (needed for linking)
- **Main loop still calls** sync functions on timers (untouched)

## Files Modified

- `StateManager.cpp` - Removed blocking calls from 9 state setter functions

## Commit Message

```
Fix watchdog timeouts by removing blocking network I/O from state setters

Root cause: TemperatureTask stuck in enterFailsafeMode() performing
synchronous Blynk/Sinric network operations that blocked for >15 seconds,
causing watchdog timeout.

Solution: Removed all blocking network sync calls and LCD updates from
state setter functions. Cloud sync now happens asynchronously in main
loop on timer intervals (2s Blynk, 30s Sinric). LCD updates happen
automatically via dedicated LCD task (1s interval).

This ensures FreeRTOS tasks complete quickly and reset watchdog timers,
while still providing timely UI updates (1-2s latency acceptable).

Fixes: watchdog timeouts in TemperatureTask, ControlTask, loopTask
```

## References

- CLAUDE.md - Section 6: FreeRTOS Multitasking Architecture
- CLAUDE.md - Section 3: Architecture Patterns (Centralized State Management)
- Config.h - Watchdog timeout definitions
- TaskManager.cpp - Task implementations with watchdog reset calls
