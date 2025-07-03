#include <U8g2lib.h>
#include <Wire.h>

// OLED Setup
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 5, /* data=*/ 4);

// Pin Definitions
const int BTN1 = 12;        // Mode/Set selection
const int BTN2 = 15;         // Start/Stop
const int POT = 34;          // Analog input
const int SPEAKER = 19;      // Audio output

// Preset Set Structure
struct PresetSet {
  int bpm;
  int numerator;
  int denominator;
};

// State Variables
enum State { 
  SET_BPM, SET_NUMERATOR, SET_DENOMINATOR, 
  SETTINGS_SET1, SETTINGS_SET2, SETTINGS_SET3, SETTINGS_SET4,
  RUNNING, PAUSED 
};

State currentState = RUNNING;  // Start at main screen
PresetSet presets[4] = {
  {120, 4, 4},  // Set 1 default
  {140, 2, 4},   // Set 2 default
  {90, 3, 4},  // Set 3 default
  {160, 4, 8}   // Set 4 default
};

int currentSet = 0;         // Current active set (0-3)
int editingSet = 0;         // Set being edited in settings (0-3)
int settingParameter = 0;   // 0=BPM, 1=numerator, 2=denominator
unsigned long lastBeat = 0;
int beatCount = 0;
bool beatIndicator = false;
unsigned long setStartTime = 0;
int measuresInCurrentSet = 0;
const int MEASURES_PER_SET = 16; // 4 measures per set

// Button tracking
unsigned long btn1PressTime = 0;
bool longPressActive = false;
bool btn2Pressed = false;
int lastPotValue = -1;

void setup() {
  pinMode(BTN1, INPUT_PULLUP);
  pinMode(BTN2, INPUT_PULLUP);
  
  // Initialize I2C with explicit pins
  Wire.begin(4, 5); // SDA=4, SCL=5
  
  // PWM setup for speaker
  ledcAttach(SPEAKER, 1000, 8); // Pin, frequency, resolution
  
  u8g2.begin();
  Serial.begin(115200);
  
  // Initial display test
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_profont12_tf);
  u8g2.drawStr(0, 12, "Metronome Ready");
  u8g2.sendBuffer();
  
  Serial.println("System initialized");
  delay(1000);
}

void loop() {
  handleButtons();
  handleMetronome();
  updateDisplay();
}

void handleButtons() {
  // Button 1 - Mode/Set selection
  if (digitalRead(BTN1) == LOW) {
    if (btn1PressTime == 0) btn1PressTime = millis();
    
    // Long press detection (>2s) - Enter settings mode
    if (!longPressActive && millis() - btn1PressTime > 2000) {
      longPressActive = true;
      if (currentState == RUNNING || currentState == PAUSED) {
        currentState = SETTINGS_SET1;
        editingSet = 0;
        settingParameter = 0;
        Serial.println("Settings mode - Set 1");
      }
    }
  } 
  else {
    if (btn1PressTime > 0) {
      // Short press action - cycle parameters in settings
      if (!longPressActive) {
        if (currentState >= SETTINGS_SET1 && currentState <= SETTINGS_SET4) {
          settingParameter = (settingParameter + 1) % 3;
          Serial.print("Parameter: ");
          Serial.println(settingParameter == 0 ? "BPM" : (settingParameter == 1 ? "Numerator" : "Denominator"));
        }
      }
      btn1PressTime = 0;
      longPressActive = false;
    }
  }

  // Button 2 - Context dependent
  bool btn2Current = (digitalRead(BTN2) == LOW);
  bool btn2LongPress = false;
  static unsigned long btn2PressTime = 0;
  
  if (btn2Current && !btn2Pressed) {
    btn2PressTime = millis();
  }
  
  if (btn2Current && millis() - btn2PressTime > 2000) {
    btn2LongPress = true;
  }
  
  if (!btn2Current && btn2Pressed) {  // Button released
    if (currentState >= SETTINGS_SET1 && currentState <= SETTINGS_SET4) {
      if (btn2LongPress) {
        // Long press - exit settings and return to main
        currentState = RUNNING;
        currentSet = 0;
        measuresInCurrentSet = 0;
        beatCount = 0;
        lastBeat = millis();
        Serial.println("Exiting settings - Starting playback");
      } else {
        // Short press - save current set and move to next
        Serial.print("Set ");
        Serial.print(editingSet + 1);
        Serial.println(" saved");
        
        editingSet++;
        if (editingSet < 4) {
          currentState = (State)(SETTINGS_SET1 + editingSet);
          settingParameter = 0;
        } else {
          // All sets configured, start playback
          currentState = RUNNING;
          currentSet = 0;
          measuresInCurrentSet = 0;
          beatCount = 0;
          lastBeat = millis();
          Serial.println("All sets configured - Starting playback");
        }
      }
    } else {
      // Main screen - start/stop toggle
      if (currentState == RUNNING) {
        currentState = PAUSED;
      } else {
        currentState = RUNNING;
        measuresInCurrentSet = 0;
        beatCount = 0;
        lastBeat = millis();
      }
    }
  }
  btn2Pressed = btn2Current;
  
  // Potentiometer value handling
  int potValue = analogRead(POT);
  
  if (currentState >= SETTINGS_SET1 && currentState <= SETTINGS_SET4) {
    // In settings mode - adjust current parameter of current set
    if (settingParameter == 0) {  // BPM
      presets[editingSet].bpm = map(potValue, 0, 4095, 40, 240);
    } 
    else if (settingParameter == 1) {  // Numerator
      presets[editingSet].numerator = map(potValue, 0, 4095, 1, 8);
    }
    else if (settingParameter == 2) {  // Denominator
      int denomIndex = map(potValue, 0, 4095, 0, 3);
      switch(denomIndex) {
        case 0: presets[editingSet].denominator = 1; break;
        case 1: presets[editingSet].denominator = 2; break;
        case 2: presets[editingSet].denominator = 4; break;
        case 3: presets[editingSet].denominator = 8; break;
      }
    }
  }
}

void handleMetronome() {
  if (currentState != RUNNING) return;
  
  // Use current preset values
  PresetSet& current = presets[currentSet];
  unsigned long interval = 60000 / current.bpm;
  
  if (millis() - lastBeat >= interval) {
    // Generate sound with higher frequency for better audibility  
    ledcWriteTone(SPEAKER, 4500); // 3500Hz tone for piezo buzzer
    delay(20);                    // 100ms duration
    ledcWrite(SPEAKER, 0);         // Stop sound
    
    beatIndicator = !beatIndicator;
    lastBeat = millis();
    beatCount = (beatCount % current.numerator) + 1;
    
    // Check if we completed a measure
    if (beatCount == 1) { // Just started new measure
      measuresInCurrentSet++;
      if (measuresInCurrentSet >= MEASURES_PER_SET) {
        currentSet = (currentSet + 1) % 4;
        measuresInCurrentSet = 0;
        Serial.print("Switching to Set ");
        Serial.println(currentSet + 1);
      }
    }
  }
}

void updateDisplay() {
  u8g2.clearBuffer();
  
  if (currentState >= SETTINGS_SET1 && currentState <= SETTINGS_SET4) {
    // Settings mode display
    u8g2.setFont(u8g2_font_6x10_tf);
    
    // Show which set we're editing
    char setStr[10];
    sprintf(setStr, "SET %d", editingSet + 1);
    u8g2.drawStr(2, 10, setStr);
    
    // Show current parameter being edited
    const char* paramNames[] = {"BPM", "BEATS", "NOTE"};
    u8g2.drawStr(60, 10, paramNames[settingParameter]);
    
    // Show current values
    PresetSet& editSet = presets[editingSet];
    
    // BPM
    char bpmStr[10];
    sprintf(bpmStr, "BPM:%d", editSet.bpm);
    u8g2.drawStr(2, 20, bpmStr);
    if (settingParameter == 0) u8g2.drawStr(45, 20, "<");
    
    // Time signature
    char timeSigStr[15];
    sprintf(timeSigStr, "SIG:%d/%d", editSet.numerator, editSet.denominator);
    u8g2.drawStr(2, 30, timeSigStr);
    if (settingParameter == 1) u8g2.drawStr(25, 30, "<");
    if (settingParameter == 2) u8g2.drawStr(35, 30, "<");
    
  } else {
    // Main screen display
    PresetSet& current = presets[currentSet];
    
    // Top row: Beat indicator and current set
    if (currentState == RUNNING) {
      u8g2.drawDisc(8, 8, beatIndicator ? 5 : 2);
    } else {
      u8g2.drawCircle(8, 8, 2);
    }
    
    // Show current set number
    u8g2.setFont(u8g2_font_6x10_tf);
    char setStr[8];
    sprintf(setStr, "S%d", currentSet + 1);
    u8g2.drawStr(20, 10, setStr);
    
    // Time signature
    char timeSigStr[8];
    sprintf(timeSigStr, "%d/%d", current.numerator, current.denominator);
    u8g2.drawStr(100, 10, timeSigStr);
    
    // Center: Large BPM value
    char bpmStr[10];
    sprintf(bpmStr, "%d", current.bpm);
    u8g2.setFont(u8g2_font_logisoso16_tn);
    int bpmWidth = u8g2.getStrWidth(bpmStr);
    u8g2.drawStr(64 - bpmWidth/2, 20, bpmStr);
    
    // BPM label
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(64 - 10, 28, "BPM");
    
    // Bottom row: State and beat count
    switch(currentState) {
      case RUNNING: 
        u8g2.drawStr(2, 32, "PLAYING");
        char beatStr[8];
        sprintf(beatStr, "%d/%d", beatCount, current.numerator);
        u8g2.drawStr(90, 32, beatStr);
        break;
      case PAUSED: 
        u8g2.drawStr(2, 32, "PAUSED"); 
        break;
    }
  }
  
  u8g2.sendBuffer();
}
