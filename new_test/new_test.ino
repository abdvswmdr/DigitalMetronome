#include <U8g2lib.h>
#include <Wire.h>
#include <Preferences.h>

// OLED Setup
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 5, /* data=*/ 4);

// Pin Definitions
const int BTN1 = 12;        // Menu/Back button
const int BTN2 = 15;         // Play/Stop button
const int ENCODER_CLK = 25;  // Rotary encoder Channel A
const int ENCODER_DT = 26;   // Rotary encoder Channel B
const int ENCODER_SW = 14;   // Rotary encoder push button
const int SPEAKER = 19;      // Audio output

// Preset Set Structure
struct PresetSet {
  String name;
  int bpm;
  int numerator;
  int denominator;
};

// Custom Set Structure for variable length sequences
struct CustomSet {
  String name;
  int bpm;
  int setCount;
  int numerators[16];  // Max 16 time signatures
  int denominators[16];
};

// State Variables
enum State { 
  MAIN_MENU, PLAY_PRESETS_MENU, CREATE_NEW_MENU, SET_MANAGEMENT_MENU,
  SET_BPM, SET_COUNT, SET_TIME_SIGNATURES,
  RUNNING, PAUSED 
};

// Menu States
enum MenuState {
  MENU_MAIN, MENU_PLAY_PRESETS, MENU_CREATE_NEW, MENU_SET_MANAGEMENT
};

// Rotary Encoder Variables
volatile int encoderPos = 0;
volatile bool encoderChanged = false;
int lastEncoderPos = 0;

State currentState = MAIN_MENU;  // Start at main menu
MenuState currentMenu = MENU_MAIN;
int menuSelection = 0;

// Factory Preset Sets (built-in)
PresetSet factoryPresets[4] = {
  {"Standard 4/4", 120, 4, 4},
  {"Waltz 3/4", 90, 3, 4},
  {"Energetic 2/4", 140, 2, 4},
  {"Fast 4/8", 160, 4, 8}
};

// Current working set for creation/editing
CustomSet workingSet;
int currentTimeSignatureIndex = 0;
int timeSignatureParam = 0; // 0=numerator, 1=denominator

// EEPROM and preset management
Preferences preferences;
int maxCustomSets = 20;
int customSetCount = 0;

int currentSet = 0;         // Current active set index
int playingSetType = 0;     // 0=factory, 1=custom
int currentPlayingSet = 0;  // Index of currently playing set
unsigned long lastBeat = 0;
int beatCount = 0;
bool beatIndicator = false;
// Removed unused variables: setStartTime, measuresInCurrentSet

// Button tracking
unsigned long btn1PressTime = 0;
bool longPressActive = false;
bool btn2Pressed = false;
unsigned long btn2PressTime = 0;
bool btn2LongPress = false;
bool encoderButtonPressed = false;
unsigned long encoderButtonPressTime = 0;
bool encoderButtonLongPress = false;

// Audio tone management
unsigned long toneStartTime = 0;
bool toneActive = false;
const unsigned long TONE_DURATION = 20; // milliseconds
const int REGULAR_FREQ = 4500; // Hz for regular beats
const int ACCENT_FREQ = 3500;  // Hz for accent beats (last beat of sequence)

// Rotary Encoder Interrupt Service Routine
void IRAM_ATTR encoderISR() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  
  // Debounce - ignore if too soon
  if (interruptTime - lastInterruptTime < 5) return;
  
  // Read both pins
  bool clkState = digitalRead(ENCODER_CLK);
  bool dtState = digitalRead(ENCODER_DT);
  
  // Determine direction
  if (clkState != dtState) {
    encoderPos++;  // Clockwise
  } else {
    encoderPos--;  // Counter-clockwise
  }
  
  encoderChanged = true;
  lastInterruptTime = interruptTime;
}

void setup() {
  // Initialize button pins
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  
  // Initialize rotary encoder pins
  pinMode(ENCODER_CLK, INPUT);
  pinMode(ENCODER_DT, INPUT);
  pinMode(ENCODER_SW, INPUT);
  
  // Attach interrupt for rotary encoder
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);
  
  // Initialize I2C with explicit pins
  Wire.begin(4, 5); // SDA=4, SCL=5
  
  // PWM setup for speaker
  ledcAttach(SPEAKER, 3000, 8); // Pin, frequency, resolution
  
  // Initialize preferences for EEPROM
  preferences.begin("metronome", false);
  
  u8g2.begin();
  Serial.begin(115200);
  
  // Initialize working set
  workingSet.name = "New Set";
  workingSet.bpm = 120;
  workingSet.setCount = 1;
  workingSet.numerators[0] = 4;
  workingSet.denominators[0] = 4;
  
  // Load custom set count from EEPROM
  customSetCount = preferences.getInt("customCount", 0);
  
  // Initial display test
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 12, "Enhanced Metronome");
  u8g2.drawStr(0, 24, "v2.0 - Encoder Ready");
  u8g2.sendBuffer();
  
  Serial.println("Enhanced Metronome System initialized");
  delay(2000);
}

void loop() {
  handleButtons();
  handleMetronome();
  updateDisplay();
  delay(10); // Small delay to prevent overwhelming the system
}

void handleButtons() {
  handleEncoderButton();
  handleBtn1();
  handleBtn2();
  handleEncoderRotation();
}

void handleEncoderButton() {
  bool currentPressed = (digitalRead(ENCODER_SW) == LOW);
  
  if (currentPressed && !encoderButtonPressed) {
    encoderButtonPressTime = millis();
  }
  
  if (currentPressed && millis() - encoderButtonPressTime > 1000) {
    encoderButtonLongPress = true;
  }
  
  if (!currentPressed && encoderButtonPressed) {
    if (encoderButtonLongPress) {
      // Long press actions
      Serial.println("Encoder long press");
    } else {
      // Short press actions - Select/Confirm
      handleEncoderSelect();
    }
    encoderButtonLongPress = false;
  }
  encoderButtonPressed = currentPressed;
}

void handleBtn1() {
  // BTN1 - Menu/Back button
  if (digitalRead(BTN1) == LOW) {
    if (btn1PressTime == 0) btn1PressTime = millis();
    
    if (!longPressActive && millis() - btn1PressTime > 2000) {
      longPressActive = true;
      handleBtn1LongPress();
    }
  } else {
    if (btn1PressTime > 0) {
      if (!longPressActive) {
        handleBtn1ShortPress();
      }
      btn1PressTime = 0;
      longPressActive = false;
    }
  }
}

void handleBtn2() {
  // BTN2 - Play/Stop button
  bool currentPressed = (digitalRead(BTN2) == LOW);
  
  if (currentPressed && !btn2Pressed) {
    btn2PressTime = millis();
  }
  
  if (currentPressed && millis() - btn2PressTime > 2000) {
    btn2LongPress = true;
  }
  
  if (!currentPressed && btn2Pressed) {
    if (btn2LongPress) {
      handleBtn2LongPress();
    } else {
      handleBtn2ShortPress();
    }
    btn2LongPress = false;
  }
  btn2Pressed = currentPressed;
}

void handleEncoderRotation() {
  if (encoderChanged) {
    encoderChanged = false;
    int change = encoderPos - lastEncoderPos;
    lastEncoderPos = encoderPos;
    
    if (change != 0) {
      processEncoderChange(change);
    }
  }
}

void processEncoderChange(int change) {
  switch (currentState) {
    case MAIN_MENU:
      navigateMenu(change);
      break;
    case PLAY_PRESETS_MENU:
      navigatePlayPresets(change);
      break;
    case SET_BPM:
      adjustBPM(change);
      break;
    case SET_COUNT:
      adjustSetCount(change);
      break;
    case SET_TIME_SIGNATURES:
      adjustTimeSignature(change);
      break;
    default:
      break;
  }
}

void handleEncoderSelect() {
  switch (currentState) {
    case MAIN_MENU:
      selectMainMenuItem();
      break;
    case PLAY_PRESETS_MENU:
      selectPresetToPlay();
      break;
    case SET_BPM:
      confirmBPM();
      break;
    case SET_COUNT:
      confirmSetCount();
      break;
    case SET_TIME_SIGNATURES:
      confirmTimeSignature();
      break;
    default:
      break;
  }
}

void handleBtn1ShortPress() {
  // Menu/Back functionality
  switch (currentState) {
    case RUNNING:
    case PAUSED:
      currentState = MAIN_MENU;
      menuSelection = 0;
      break;
    case PLAY_PRESETS_MENU:
    case CREATE_NEW_MENU:
    case SET_MANAGEMENT_MENU:
      currentState = MAIN_MENU;
      menuSelection = 0;
      break;
    case SET_BPM:
    case SET_COUNT:
    case SET_TIME_SIGNATURES:
      currentState = MAIN_MENU;
      menuSelection = 0;
      break;
    default:
      break;
  }
}

void handleBtn1LongPress() {
  // Quick access to main menu from any state
  currentState = MAIN_MENU;
  menuSelection = 0;
}

void handleBtn2ShortPress() {
  // Play/Stop functionality
  if (currentState == RUNNING) {
    currentState = PAUSED;
  } else if (currentState == PAUSED) {
    currentState = RUNNING;
    lastBeat = millis();
  }
}

void handleBtn2LongPress() {
  // Quick create mode - immediate custom set creation
  currentState = SET_BPM;
  workingSet.bpm = 120;
  workingSet.setCount = 1;
  workingSet.numerators[0] = 4;
  workingSet.denominators[0] = 4;
  Serial.println("Quick create mode - Set BPM");
}

void handleMetronome() {
  if (currentState != RUNNING) return;
  
  // Handle tone turn-off (non-blocking)
  if (toneActive && millis() - toneStartTime >= TONE_DURATION) {
    ledcWrite(SPEAKER, 0);
    toneActive = false;
  }
  
  // Determine current playing set based on type
  int currentBPM, currentNumerator, currentDenominator;
  
  if (playingSetType == 0) { // Factory preset
    currentBPM = factoryPresets[currentSet].bpm;
    currentNumerator = factoryPresets[currentSet].numerator;
    currentDenominator = factoryPresets[currentSet].denominator;
  } else { // Custom set
    currentBPM = workingSet.bpm;
    currentNumerator = workingSet.numerators[currentSet];
    currentDenominator = workingSet.denominators[currentSet];
  }
  
  unsigned long interval = 60000 / currentBPM;
  
  if (millis() - lastBeat >= interval) {
    // Determine if this is the last beat of the current sequence
    bool isLastBeatOfSequence = false;
    
    if (playingSetType == 0) {
      // Factory preset - last beat before cycling to next preset
      isLastBeatOfSequence = (beatCount == currentNumerator);
    } else {
      // Custom set - last beat of the last time signature in the sequence
      bool isLastSet = (currentSet == workingSet.setCount - 1);
      bool isLastBeatOfSet = (beatCount == currentNumerator);
      isLastBeatOfSequence = isLastSet && isLastBeatOfSet;
    }
    
    // Generate sound with accent for last beat of sequence
    int frequency = isLastBeatOfSequence ? ACCENT_FREQ : REGULAR_FREQ;
    ledcWriteTone(SPEAKER, frequency);
    toneStartTime = millis();
    toneActive = true;
    
    beatIndicator = !beatIndicator;
    lastBeat = millis();
    beatCount = (beatCount % currentNumerator) + 1;
    
    // Check if we completed a measure - seamless transition
    if (beatCount == 1) { // Just started new measure
      if (playingSetType == 0) {
        // Factory preset cycling
        currentSet = (currentSet + 1) % 4;
      } else {
        // Custom set progression
        currentSet = (currentSet + 1) % workingSet.setCount;
      }
      Serial.print("Switching to Set ");
      Serial.println(currentSet + 1);
    }
  }
}

// Menu Navigation Functions
void navigateMenu(int change) {
  switch (currentState) {
    case MAIN_MENU:
      menuSelection = constrain(menuSelection + change, 0, 3);
      break;
    case PLAY_PRESETS_MENU:
      menuSelection = constrain(menuSelection + change, 0, 3 + customSetCount);
      break;
    default:
      break;
  }
}

void navigatePlayPresets(int change) {
  menuSelection = constrain(menuSelection + change, 0, 3 + customSetCount);
}

void selectMainMenuItem() {
  switch (menuSelection) {
    case 0: // Play Presets
      currentState = PLAY_PRESETS_MENU;
      menuSelection = 0;
      break;
    case 1: // Create New Set
      currentState = SET_BPM;
      workingSet.bpm = 120;
      workingSet.setCount = 1;
      workingSet.numerators[0] = 4;
      workingSet.denominators[0] = 4;
      break;
    case 2: // Set Management
      currentState = SET_MANAGEMENT_MENU;
      menuSelection = 0;
      break;
    case 3: // Quick Play (Last Used)
      startPlayback();
      break;
  }
}

void selectPresetToPlay() {
  if (menuSelection < 4) {
    // Factory preset selected
    playingSetType = 0;
    currentSet = menuSelection;
    startPlayback();
  } else {
    // Custom set selected
    playingSetType = 1;
    currentSet = 0;
    loadCustomSet(menuSelection - 4);
    startPlayback();
  }
}

void adjustBPM(int change) {
  workingSet.bpm = constrain(workingSet.bpm + change, 40, 240);
}

void adjustSetCount(int change) {
  workingSet.setCount = constrain(workingSet.setCount + change, 1, 16);
}

void adjustTimeSignature(int change) {
  if (change > 0) {
    // Clockwise - adjust numerator
    workingSet.numerators[currentTimeSignatureIndex]++;
    if (workingSet.numerators[currentTimeSignatureIndex] > 16) {
      workingSet.numerators[currentTimeSignatureIndex] = 1;
    }
  } else {
    // Counter-clockwise - adjust denominator with looping
    int currentDenom = workingSet.denominators[currentTimeSignatureIndex];
    switch (currentDenom) {
      case 2: workingSet.denominators[currentTimeSignatureIndex] = 16; break;
      case 4: workingSet.denominators[currentTimeSignatureIndex] = 2; break;
      case 8: workingSet.denominators[currentTimeSignatureIndex] = 4; break;
      case 16: workingSet.denominators[currentTimeSignatureIndex] = 8; break;
      default: workingSet.denominators[currentTimeSignatureIndex] = 4; break;
    }
  }
}

void confirmBPM() {
  currentState = SET_COUNT;
  Serial.print("BPM set to: ");
  Serial.println(workingSet.bpm);
}

void confirmSetCount() {
  currentState = SET_TIME_SIGNATURES;
  currentTimeSignatureIndex = 0;
  // Initialize all time signatures with default values
  for (int i = 0; i < workingSet.setCount; i++) {
    workingSet.numerators[i] = 4;
    workingSet.denominators[i] = 4;
  }
  Serial.print("Set count: ");
  Serial.println(workingSet.setCount);
}

void confirmTimeSignature() {
  currentTimeSignatureIndex++;
  if (currentTimeSignatureIndex >= workingSet.setCount) {
    // All time signatures set, auto-save and play
    if (customSetCount < maxCustomSets) {
      saveCustomSet(customSetCount);
      Serial.println("Set saved automatically.");
    }
    currentSet = 0;
    playingSetType = 1;
    startPlayback();
  }
}

void startPlayback() {
  currentState = RUNNING;
  beatCount = 0;
  lastBeat = millis();
  Serial.println("Starting playback");
}


void updateDisplay() {
  u8g2.clearBuffer();
  
  switch (currentState) {
    case MAIN_MENU:
      displayMainMenu();
      break;
    case PLAY_PRESETS_MENU:
      displayPlayPresetsMenu();
      break;
    case SET_BPM:
      displayBPMSetting();
      break;
    case SET_COUNT:
      displaySetCountSetting();
      break;
    case SET_TIME_SIGNATURES:
      displayTimeSignatureSetting();
      break;
    case RUNNING:
    case PAUSED:
      displayPlayback();
      break;
    default:
      break;
  }
  
  u8g2.sendBuffer();
}

void displayMainMenu() {
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0, 8, "MAIN MENU");
  
  const char* menuItems[] = {"Play Presets", "Create New", "Manage Sets", "Quick Play"};
  
  // Two-column layout for better space utilization
  for (int i = 0; i < 4; i++) {
    int col = i % 2;
    int row = i / 2;
    int x = col * 64;
    int y = 16 + row * 8;
    
    if (i == menuSelection) {
      u8g2.drawStr(x, y, "> ");
    }
    u8g2.drawStr(x + 8, y, menuItems[i]);
  }
}

void displayPlayPresetsMenu() {
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0, 8, "SELECT PRESET");
  
  // Show factory presets first
  for (int i = 0; i < 4; i++) {
    int y = 16 + i * 6;
    if (i == menuSelection) {
      u8g2.drawStr(0, y, "> ");
    }
    // Truncate long names to fit screen
    String name = factoryPresets[i].name;
    if (name.length() > 15) {
      name = name.substring(0, 15);
    }
    u8g2.drawStr(8, y, name.c_str());
  }
  
  // Show custom sets if any exist
  if (customSetCount > 0) {
    u8g2.drawStr(64, 16, "CUSTOM:");
    for (int i = 0; i < min(customSetCount, 3); i++) {
      int customIndex = i + 4;
      int y = 22 + i * 6;
      if (customIndex == menuSelection) {
        u8g2.drawStr(64, y, "> ");
      }
      String customName = getCustomSetName(i);
      if (customName.length() > 7) {
        customName = customName.substring(0, 7);
      }
      u8g2.drawStr(72, y, customName.c_str());
    }
  }
}

void displayBPMSetting() {
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0, 8, "SET BPM");
  
  char bpmStr[10];
  sprintf(bpmStr, "%d", workingSet.bpm);
  u8g2.setFont(u8g2_font_logisoso16_tn);
  u8g2.drawStr(32, 22, bpmStr);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0, 32, "Rotate to adjust");
}

void displaySetCountSetting() {
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0, 8, "SET COUNT");
  
  char countStr[10];
  sprintf(countStr, "%d", workingSet.setCount);
  u8g2.setFont(u8g2_font_logisoso16_tn);
  u8g2.drawStr(32, 22, countStr);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0, 32, "Rotate to adjust");
}

void displayTimeSignatureSetting() {
  u8g2.setFont(u8g2_font_4x6_tf);
  char headerStr[16];
  sprintf(headerStr, "SET %d/%d", currentTimeSignatureIndex + 1, workingSet.setCount);
  u8g2.drawStr(0, 8, headerStr);
  
  char timeSigStr[10];
  sprintf(timeSigStr, "%d/%d", workingSet.numerators[currentTimeSignatureIndex], workingSet.denominators[currentTimeSignatureIndex]);
  u8g2.setFont(u8g2_font_logisoso16_tn);
  u8g2.drawStr(32, 22, timeSigStr);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(0, 32, "CW:beats CCW:note");
}

void displayPlayback() {
  // Determine current playing values
  int currentBPM, currentNumerator, currentDenominator;
  String setName;
  
  if (playingSetType == 0) {
    currentBPM = factoryPresets[currentSet].bpm;
    currentNumerator = factoryPresets[currentSet].numerator;
    currentDenominator = factoryPresets[currentSet].denominator;
    setName = factoryPresets[currentSet].name;
  } else {
    currentBPM = workingSet.bpm;
    currentNumerator = workingSet.numerators[currentSet];
    currentDenominator = workingSet.denominators[currentSet];
    setName = workingSet.name;
  }
  
  // Beat indicator (left side)
  if (currentState == RUNNING) {
    u8g2.drawDisc(6, 6, beatIndicator ? 4 : 2);
  } else {
    u8g2.drawCircle(6, 6, 2);
  }
  
  // Set info (top row)
  u8g2.setFont(u8g2_font_4x6_tf);
  char setStr[8];
  sprintf(setStr, "S%d", currentSet + 1);
  u8g2.drawStr(16, 8, setStr);
  
  // Time signature
  char timeSigStr[8];
  sprintf(timeSigStr, "%d/%d", currentNumerator, currentDenominator);
  u8g2.drawStr(32, 8, timeSigStr);
  
  // BPM (center, prominent)
  char bpmStr[10];
  sprintf(bpmStr, "%d", currentBPM);
  u8g2.setFont(u8g2_font_logisoso16_tn);
  int bpmWidth = u8g2.getStrWidth(bpmStr);
  u8g2.drawStr(64 - bpmWidth/2, 20, bpmStr);
  
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(64 - 8, 26, "BPM");
  
  // Status and beat count (bottom row)
  switch(currentState) {
    case RUNNING: 
      u8g2.drawStr(2, 32, "PLAY");
      char beatStr[8];
      sprintf(beatStr, "%d/%d", beatCount, currentNumerator);
      u8g2.drawStr(90, 32, beatStr);
      break;
    case PAUSED: 
      u8g2.drawStr(2, 32, "PAUSE"); 
      break;
  }
}

// EEPROM Preset Management Functions
void saveCustomSet(int index) {
  String baseKey = "set_" + String(index);
  
  preferences.putString((baseKey + "_name").c_str(), workingSet.name);
  preferences.putInt((baseKey + "_bpm").c_str(), workingSet.bpm);
  preferences.putInt((baseKey + "_count").c_str(), workingSet.setCount);
  
  // Save time signatures
  for (int i = 0; i < workingSet.setCount; i++) {
    preferences.putInt((baseKey + "_num_" + String(i)).c_str(), workingSet.numerators[i]);
    preferences.putInt((baseKey + "_den_" + String(i)).c_str(), workingSet.denominators[i]);
  }
  
  // Update custom set count if this is a new set
  if (index >= customSetCount) {
    customSetCount = index + 1;
    preferences.putInt("customCount", customSetCount);
  }
  
  Serial.print("Custom set saved: ");
  Serial.println(workingSet.name);
}

void loadCustomSet(int index) {
  String baseKey = "set_" + String(index);
  
  workingSet.name = preferences.getString((baseKey + "_name").c_str(), "Custom Set");
  workingSet.bpm = preferences.getInt((baseKey + "_bpm").c_str(), 120);
  workingSet.setCount = preferences.getInt((baseKey + "_count").c_str(), 1);
  
  // Load time signatures
  for (int i = 0; i < workingSet.setCount; i++) {
    workingSet.numerators[i] = preferences.getInt((baseKey + "_num_" + String(i)).c_str(), 4);
    workingSet.denominators[i] = preferences.getInt((baseKey + "_den_" + String(i)).c_str(), 4);
  }
  
  Serial.print("Custom set loaded: ");
  Serial.println(workingSet.name);
}

void deleteCustomSet(int index) {
  String baseKey = "set_" + String(index);
  
  preferences.remove((baseKey + "_name").c_str());
  preferences.remove((baseKey + "_bpm").c_str());
  preferences.remove((baseKey + "_count").c_str());
  
  // Remove time signatures
  for (int i = 0; i < 16; i++) {
    preferences.remove((baseKey + "_num_" + String(i)).c_str());
    preferences.remove((baseKey + "_den_" + String(i)).c_str());
  }
  
  // Shift remaining sets down
  for (int i = index; i < customSetCount - 1; i++) {
    loadCustomSet(i + 1);
    saveCustomSet(i);
  }
  
  customSetCount--;
  preferences.putInt("customCount", customSetCount);
  
  Serial.print("Custom set deleted: ");
  Serial.println(index);
}

bool hasCustomSet(int index) {
  if (index >= customSetCount) return false;
  String baseKey = "set_" + String(index);
  return preferences.isKey((baseKey + "_name").c_str());
}

String getCustomSetName(int index) {
  if (!hasCustomSet(index)) return "";
  String baseKey = "set_" + String(index);
  return preferences.getString((baseKey + "_name").c_str(), "Custom Set");
}
