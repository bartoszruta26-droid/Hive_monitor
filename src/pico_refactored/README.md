# ApiaryGuard Pico - Refactored Firmware

## Overview
Refactored version of the ApiaryGuard firmware for Raspberry Pi Pico (RP2040) with modular architecture addressing the critical issues in the original monolithic code.

## Key Improvements

### 1. Modular Architecture
- **Main file**: `apiaryguard_pico.ino` (~100 lines) - Entry point only
- **Config**: `include/config.h` - Pin definitions and constants
- **Sensors**: `src/sensors.cpp` + `include/sensors.h` - Sensor detection and reading
- **HX711**: `src/hx711.cpp` + `include/hx711.h` - Weight sensor with robust implementation
- **Network**: `src/network.cpp` + `include/network.h` - Ethernet W6100 with DHCP fallback
- **Audio**: `include/audio_analysis.h` - Audio FFT analysis (stub)
- **Weight**: `include/weight_analysis.h` - Weight trend analysis (stub)
- **Air Quality**: `include/air_quality.h` - SGP41 processing (stub)
- **Radar**: `include/radar_analysis.h` - LD2410B processing (stub)
- **Effectors**: `src/effectors.cpp` + `include/effectors.h` - PWM and relay control

### 2. Fixed Critical Issues

#### HX711 Implementation
- ✅ Added timeout handling (50ms) to prevent hanging
- ✅ Proper error logging with rate limiting
- ✅ Watchdog feed during wait loop
- ✅ Stable timing with `delayMicroseconds(1)` for RP2040 at 133MHz

#### Memory Management
- ✅ Reduced structure sizes (simplified from 100+ params to essential ones)
- ✅ Removed large static buffers where possible
- ✅ Stack-safe design with smaller local variables

#### Network
- ✅ DHCP fallback mechanism (tries DHCP first, falls back to static IP)
- ✅ Proper `Ethernet.maintain()` calls in main loop
- ✅ Link status checking

#### Sensor Handling
- ✅ Dynamic sensor detection at startup
- ✅ Conditional reading (only active sensors)
- ✅ Error counting and recovery
- ✅ DHT read retries (3 attempts)

#### Watchdog
- ✅ Enabled with 8-second timeout
- ✅ Fed in main loop and long-wait operations

#### Pin Conflicts
- ✅ Fixed GPIO26/28 conflict (RELAY_CH6 moved to GPIO28)

### 3. Compilation Benefits
- Original: Single 4700-line file - slow compilation, hard debugging
- Refactored: Multiple small files - fast incremental compilation, easy debugging

## File Structure
```
pico_refactored/
├── apiaryguard_pico.ino    # Main entry point
├── include/
│   ├── config.h            # Pin definitions, constants
│   ├── sensors.h           # Sensor management
│   ├── hx711.h             # Weight sensor
│   ├── network.h           # Ethernet/W6100
│   ├── audio_analysis.h    # Audio FFT
│   ├── weight_analysis.h   # Weight trends
│   ├── air_quality.h       # SGP41 IAQ
│   ├── radar_analysis.h    # LD2410B
│   └── effectors.h         # PWM/relays
└── src/
    ├── sensors.cpp         # Sensor implementation
    ├── hx711.cpp           # HX711 reading
    ├── network.cpp         # Network stack
    └── effectors.cpp       # Actuator control
```

## Usage

### Arduino IDE Setup
1. Install RP2040 board support
2. Install required libraries:
   - Ethernet (for W6100)
   - DHT sensor library
   - Adafruit SGP41 library
3. Open `apiaryguard_pico.ino`
4. Select board: "Raspberry Pi Pico"
5. Upload

### Configuration
Edit `include/config.h` to change:
- Pin assignments
- Network settings (IP, gateway)
- Analysis parameters

## Migration from Original
The refactored code maintains API compatibility with the original where possible. Global variables are declared as `extern` and shared between modules.

## Remaining Work
The following modules need full implementation (currently stubs):
- Full FFT audio analysis
- Complete weight trend analysis (60+ params reduced to essentials)
- Air quality IAQ calculation
- Radar anomaly detection

These can be added incrementally without breaking the core functionality.

## Testing
1. Verify sensor detection in Serial Monitor
2. Check Ethernet connectivity (DHCP or static)
3. Test HTTP endpoint at `http://<ip>:8080/`
4. Verify UDP data transmission to RPi

## License
Same as original project
