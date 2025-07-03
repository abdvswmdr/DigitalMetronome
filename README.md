# ESP32 Digital Metronome

A professional digital metronome built with ESP32 microcontroller, featuring four customizable preset sets, OLED display, and intuitive button controls.

## 🎵 Features

- **Four Preset Sets**: Configure and cycle through 4 different tempo/time signature combinations
- **OLED Display**: Clear visual feedback with beat indicator and current settings
- **Intuitive Controls**: Two-button navigation with potentiometer adjustment
- **Automatic Set Cycling**: Seamlessly transitions between sets during playback
- **Professional Audio**: Optimized piezo buzzer output for clear beat indication
- **Real-time Configuration**: Adjust BPM and time signatures on-the-fly

## 🔧 Hardware Requirements

### Components
- **ESP32 Development Board** (currently ESP32, future: XIAO ESP32-S3)
- **SSD1306 OLED Display** (128x32 pixels, I2C)
- **Piezo Buzzer** (with optional 47-100Ω resistor)
- **2x Push Buttons** (with pull-up resistors)
- **1x Potentiometer** (BK10, future: Rotary Encoder)
- **Breadboard and jumper wires**

### Pin Connections

| Component | ESP32 Pin | Description |
|-----------|-----------|-------------|
| OLED SDA | GPIO 4 | I2C Data |
| OLED SCL | GPIO 5 | I2C Clock |
| Button 1 | GPIO 12 | Mode/Settings (with pull-up) |
| Button 2 | GPIO 15 | Start/Stop/Save (with pull-up) |
| Potentiometer | GPIO 34 | Analog input for value adjustment |
| Piezo Buzzer | GPIO 19 | Audio output |

## 🚀 Getting Started

### Prerequisites
1. **Arduino IDE** with ESP32 board package
2. **U8g2 Library** for OLED display
   ```
   Tools → Manage Libraries → Search "U8g2" → Install
   ```

### Installation
1. **Clone the repository:**
   ```bash
   git clone <repository-url>
   cd esp32-digital-metronome
   ```

2. **Open Arduino IDE:**
   - File → Open → Select `new_test/new_test.ino`

3. **Configure ESP32:**
   - Tools → Board → ESP32 Arduino → ESP32 Dev Module
   - Tools → Port → Select your ESP32 port

4. **Upload the code:**
   - Click Upload button or Ctrl+U

## 🎮 How to Use

### Main Screen
- **Display shows:** Current BPM, time signature, set number (S1-S4), and beat indicator
- **Button 2 (Short Press):** Start/Stop metronome
- **Button 1 (Long Press):** Enter settings mode

### Settings Mode
The metronome allows you to configure 4 preset sets, each with custom BPM and time signature:

1. **Enter Settings:**
   - Long press Button 1 (2+ seconds) from main screen

2. **Navigate Parameters:**
   - Short press Button 1 to cycle: BPM → Beats → Notes → BPM...
   - Turn potentiometer to adjust selected parameter

3. **Save and Continue:**
   - Short press Button 2 to save current set and move to next
   - Repeat for all 4 sets

4. **Exit Settings:**
   - Long press Button 2 to exit and start playback
   - Or complete all 4 sets to auto-start

### Parameter Ranges
- **BPM:** 40-240 beats per minute
- **Numerator (Beats):** 1-8 beats per measure
- **Denominator (Notes):** 1, 2, 4, 8 (whole, half, quarter, eighth notes)

## 🎼 Default Preset Sets

| Set | BPM | Time Signature | Style |
|-----|-----|----------------|-------|
| 1 | 120 | 4/4 | Standard pop/rock |
| 2 | 140 | 2/4 | Energetic, simple |
| 3 | 90 | 3/4 | Waltz feel |
| 4 | 160 | 4/8 | Fast eighth note feel |

## 🔀 Set Cycling Behavior

- Each set plays for exactly **16 measures** before switching
- Sets cycle automatically: Set 1 → Set 2 → Set 3 → Set 4 → Set 1...
- Seamless transitions maintain musical timing
- Current set displayed as "S1", "S2", "S3", or "S4"

## 🛠 Development

### Project Structure
```
esp32-digital-metronome/
├── new_test/
│   └── new_test.ino          # Main Arduino sketch
├── README.md                 # This file
└── .git/                     # Git repository
```

### Development Workflow
1. **Create feature branches** for new functionality
2. **Test thoroughly** before merging to main
3. **Document changes** in commit messages
4. **Update README** for significant feature additions

### Git Commands for Team

#### First Time Setup
```bash
git clone <repository-url>
cd esp32-digital-metronome
```

#### Daily Workflow
```bash
# Get latest changes
git pull origin main

# Create new branch for your feature
git checkout -b feature/your-feature-name

# Make your changes, then:
git add .
git commit -m "Add: describe your changes"
git push origin feature/your-feature-name
```

#### Common Commands
```bash
git status                    # Check what files changed
git log --oneline            # See recent commits
git checkout main            # Switch to main branch
git branch -a                # List all branches
```

## 🔧 Configuration

### Hardware Tuning
- **Buzzer Volume:** Adjust frequency (2500-4500Hz) in code for optimal volume
- **Resistor Value:** Try 47Ω or direct connection if volume too low
- **Power Source:** Use ESP32 USB port for maximum current output

### Software Parameters
Key constants you can modify in the code:
```cpp
const int MEASURES_PER_SET = 16;        // Measures per set
const int BUZZER_FREQUENCY = 4500;     // Buzzer frequency (Hz)
const int BUZZER_DURATION = 20;        // Buzzer duration (ms)
```

## 🐛 Troubleshooting

### Common Issues
1. **No sound from buzzer:**
   - Check wiring and resistor value
   - Try different frequencies (2500-4500Hz)
   - Use ESP32 USB power instead of FTDI

2. **OLED not displaying:**
   - Verify I2C connections (SDA=4, SCL=5)
   - Check power supply (3.3V)

3. **Buttons not responding:**
   - Ensure pull-up resistors are connected
   - Check for loose connections

4. **Compilation errors:**
   - Install U8g2 library
   - Select correct ESP32 board type

## 🤝 Contributing

### For Team Members
1. **Always create branches** for new features
2. **Test your changes** thoroughly before pushing
3. **Write clear commit messages**
4. **Ask for help** if you're stuck with Git commands

### Branch Naming Convention
- `feature/description` - New features
- `bugfix/description` - Bug fixes
- `experiment/description` - Testing new ideas

## 📋 Future Enhancements

- [ ] Switch to XIAO ESP32-S3 hardware
- [ ] Replace potentiometer with rotary encoder
- [ ] Add push button functionality to encoder
- [ ] Implement volume control
- [ ] Add tap tempo feature
- [ ] Save settings to EEPROM
- [ ] Add more time signature options
- [ ] Implement accent beats (strong/weak beats)

## 📄 License

This project is developed for educational and professional use. Please respect the collaborative nature of this project.

---

**Note:** This is an active development project. Features and documentation may change as we iterate and improve the design.