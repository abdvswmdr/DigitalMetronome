# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32-based Digital Metronome with rotary encoder interface. The project has evolved from a simple potentiometer-based system to a sophisticated menu-driven metronome with preset management, non-volatile storage, and advanced audio features including accent beats.

## Build and Development Commands

### Compilation and Upload
```bash
# Compile the sketch
cd new_test/ && arduino-cli compile --fqbn esp32:esp32:esp32 .

# Upload to connected ESP32
arduino-cli upload -p [PORT] --fqbn esp32:esp32:esp32 .

# List available ports
arduino-cli board list
```

### Prerequisites Installation
- Install U8g2 library: `arduino-cli lib install U8g2`
- ESP32 board package should be installed via Arduino IDE or CLI

### Known Build Issues
- **esptool path issue**: If compilation fails with "esptool: Is a directory", the esptool executable may need to be copied from version 5.0.dev1 to 4.5.1 in the Arduino packages directory
- **Memory optimization**: Custom set arrays are limited to 16 elements to prevent memory issues

## Architecture and Code Structure

### Core System Architecture
The metronome operates on a state machine pattern with five main subsystems:

1. **Input Handling**: Three-button interface (rotary encoder + 2 physical buttons)
2. **Menu System**: Hierarchical navigation for presets and settings
3. **Audio Engine**: Non-blocking PWM-based tone generation with accent beats
4. **Display System**: 128x32 OLED-based UI with optimized two-column layouts
5. **Storage System**: ESP32 NVS-based preset persistence using Preferences library

### State Management
- **Main States**: `MAIN_MENU`, `PLAY_PRESETS_MENU`, `SET_BPM`, `SET_COUNT`, `SET_TIME_SIGNATURES`, `RUNNING`, `PAUSED`
- **State transitions** are driven by encoder input and button presses
- **Interrupt-driven encoder**: Uses `encoderISR()` attached to GPIO25 with 5ms debouncing

### Key Data Structures
```cpp
struct PresetSet {          // Factory presets (4 built-in)
  String name;
  int bpm;
  int numerator;
  int denominator;
};

struct CustomSet {          // User-created sequences  
  String name;
  int bpm;
  int setCount;             // 1-16 time signatures
  int numerators[16];
  int denominators[16];
};
```

### Hardware Interface
**Current Pin Configuration (Rotary Encoder System):**
- GPIO25: Encoder Channel A (CLK) - interrupt-driven
- GPIO26: Encoder Channel B (DT) 
- GPIO14: Encoder push button
- GPIO12: Menu/Back button (BTN1)
- GPIO15: Play/Stop button (BTN2)
- GPIO19: PWM speaker output (simple buzzer + resistor)
- GPIO4/5: I2C for OLED (SDA/SCL)

**Legacy Documentation**: README.md still references old potentiometer setup - the actual implementation uses rotary encoder

### Critical Implementation Details

#### Non-Blocking Audio Engine
The audio system uses a timer-based approach to prevent system blocking:
- **Non-blocking tone generation**: Uses `toneActive` flag and `millis()` timing
- **Accent beats**: Different frequencies for regular beats (4500Hz) vs accent beats (3500Hz)
- **Tone duration**: 20ms controlled by `TONE_DURATION` constant
- **Accent logic**: Last beat of sequence gets lower frequency for emphasis

#### Encoder Workflow Logic
The metronome implements a **BPM-first workflow**:
1. Set BPM (40-240) for entire sequence
2. Set count of time signatures (1-16)
3. Configure individual time signatures:
   - Clockwise rotation: numerator (1-16, loops back to 1)
   - Counterclockwise rotation: denominator (2→4→8→16→2 cyclic)
   - Encoder push: confirm and advance

#### Memory Management and ESP32 NVS
- **ESP32 NVS Storage**: Up to 50 custom sets using Preferences library (increased from 20)
- **Memory capacity**: ~24KB NVS partition available, each set uses ~156 bytes
- **Theoretical maximum**: ~153 custom sets possible, current limit is conservative
- **Key format**: `"set_N_name"`, `"set_N_bpm"`, `"set_N_count"`, `"set_N_num_I"`, `"set_N_den_I"`
- **Auto-save**: Custom sets automatically saved when created

#### Display System Optimization
Optimized for 128x32 pixel constraint:
- **Two-column layouts**: Main menu uses left/right columns for space efficiency
- **Small fonts**: `u8g2_font_4x6_tf` for most text to prevent overflow
- **Text truncation**: Automatic truncation for long preset names
- **Priority display**: "654321 P1" custom set appears first in preset menu

### Special Features

#### Piano Score Integration
The system includes a pre-programmed custom set for the piano piece "654321":
- **Auto-creation**: `create654321Set()` function runs on startup
- **Musical data**: BPM 200, time signatures 6/8→5/8→4/8→3/8→2/8→1/8
- **Menu priority**: Appears first in preset selection for easy access
- **One-time creation**: Only creates if it doesn't already exist

#### Button Input Architecture
Three-level input handling:
1. **Raw input**: `handleButtons()` coordinates all inputs
2. **Button-specific**: `handleBtn1()`, `handleBtn2()`, `handleEncoderButton()`
3. **Action processing**: Context-aware handlers based on current state

## Development Notes

### Testing the System
- **Serial output**: Extensive logging for state transitions and user actions
- **Memory usage**: ~27% program storage, ~7% dynamic memory  
- **Display testing**: 2-second startup screen confirms OLED functionality
- **Audio testing**: Non-blocking system allows responsive UI during playback

### Recent Enhancements Completed
- ✅ Non-blocking audio engine (replaced blocking `delay(20)`)
- ✅ Accent beat system (different frequencies for emphasis)
- ✅ Two-column menu layouts optimized for 128x32 display
- ✅ Increased custom set limit from 20 to 50
- ✅ Piano score integration ("654321" piece)
- ✅ Font size optimization for small screen

### Configuration Constants
Key values that may need adjustment:
- `maxCustomSets = 50`: NVS storage limit (can be increased up to ~153)
- `TONE_DURATION = 20`: Audio tone duration in milliseconds
- `REGULAR_FREQ = 4500`: Hz for regular beats (safe for buzzer + resistor)
- `ACCENT_FREQ = 3500`: Hz for accent beats (lower frequency for emphasis)
- `5ms debouncing`: In encoder ISR
- `16 max time signatures`: Memory optimization limit per custom set

### Hardware Considerations
- **Simple buzzer setup**: Designed for basic buzzer + resistor, frequency range 3500-4500Hz
- **128x32 OLED**: All UI elements designed for this specific resolution
- **ESP32 NVS**: Uses built-in non-volatile storage, not external EEPROM