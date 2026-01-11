#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <TM1637Display.h>
#include <BluetoothSerial.h>

// Pin definitions
#define BTN_HOUR   4
#define BTN_MIN    5
#define BTN_MODE   17
#define BTN_POMO   16
#define BTN_RESET  21
#define BUZZER_PIN 27
#define TM_CLK     18
#define TM_DIO     19
#define I2C_SDA    25
#define I2C_SCL    26

// Buzzer configuration
#define BUZZER_CHANNEL 0
#define BUZZER_FREQ    2000
#define BUZZER_RES     8

// Timing constants
#define DEBOUNCE_MS       200
#define BLINK_INTERVAL_MS 500
#define DEBUG_INTERVAL_MS 5000
#define LOOP_DELAY_MS     50
#define LONG_PRESS_MS     1000

// Display brightness
#define DISPLAY_BRIGHTNESS 0x0f

// Object instances
RTC_DS3231 rtc;
TM1637Display display(TM_CLK, TM_DIO);
BluetoothSerial SerialBT;

// Forward declarations
void beep(uint16_t ms);
void beepPattern(uint8_t count, uint16_t duration, uint16_t gap);
void stopAllAlerts();

// Alarm state
struct AlarmState {
  int hour = 0;
  int minute = 0;
  bool enabled = false;
  bool setMode = false;
  bool ringing = false;
  bool rangThisMinute = false;
} alarmState;

// Pomodoro state
struct PomoState {
  bool mode = false;
  bool running = false;
  bool workPhase = true;
  bool ringing = false;
  int workMin = 1;
  int breakMin = 1;
  unsigned long startMillis = 0;
  unsigned long duration = 0;
} pomo;

// Button state
struct ButtonState {
  unsigned long modePressTime = 0;
  unsigned long pomoPressTime = 0;
  bool modePressed = false;
  bool pomoPressed = false;
  unsigned long lastHourPress = 0;
  unsigned long lastMinPress = 0;
} buttons;

// Display state
struct DisplayState {
  unsigned long lastBlinkTime = 0;
  bool blinkState = true;
} displayState;

// Timing
int lastMinute = -1;
unsigned long lastDebugPrint = 0;

// ==================== HELPER FUNCTIONS ====================

void sendBTStatus() {
  DateTime now = rtc.now();
  SerialBT.println("\n=== STATUS ===");
  SerialBT.printf("Time: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
  SerialBT.printf("Alarm: %02d:%02d | Enabled:%d Ringing:%d\n",
                alarmState.hour, alarmState.minute, alarmState.enabled, alarmState.ringing);
  SerialBT.printf("Pomodoro: Running:%d Phase:%s Work:%dmin Break:%dmin\n",
                pomo.running, pomo.workPhase ? "WORK" : "BREAK", pomo.workMin, pomo.breakMin);
}

void handleBluetoothCommands() {
  if (!SerialBT.available()) return;
  
  String cmd = SerialBT.readStringUntil('\n');
  cmd.trim();
  cmd.toLowerCase();
  
  if (cmd == "status") {
    sendBTStatus();
  }
  else if (cmd == "help") {
    SerialBT.println("\n=== BLUETOOTH COMMANDS ===");
    SerialBT.println("status - Show current status");
    SerialBT.println("time HH:MM:SS - Set current time");
    SerialBT.println("alarm HH:MM - Set alarm time");
    SerialBT.println("alarm on - Enable alarm");
    SerialBT.println("alarm off - Disable alarm");
    SerialBT.println("pomo start - Start pomodoro");
    SerialBT.println("pomo stop - Stop pomodoro");
    SerialBT.println("pomo work MM - Set work duration");
    SerialBT.println("pomo break MM - Set break duration");
    SerialBT.println("reset - Reset all timers");
    SerialBT.println("help - Show this message");
  }
  else if (cmd.startsWith("time ")) {
    // Format: time HH:MM:SS
    int h, m, s;
    if (sscanf(cmd.c_str(), "time %d:%d:%d", &h, &m, &s) == 3) {
      if (h >= 0 && h < 24 && m >= 0 && m < 60 && s >= 0 && s < 60) {
        DateTime now = rtc.now();
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, s));
        SerialBT.printf("✓ Time set to %02d:%02d:%02d\n", h, m, s);
        beepPattern(2, 100, 100);
      } else {
        SerialBT.println("✗ Invalid time format");
      }
    } else {
      SerialBT.println("✗ Use format: time HH:MM:SS");
    }
  }
  else if (cmd.startsWith("alarm ")) {
    String subcmd = cmd.substring(6);
    subcmd.trim();
    
    if (subcmd == "on") {
      alarmState.enabled = true;
      SerialBT.println("✓ Alarm enabled");
      beep(200);
    }
    else if (subcmd == "off") {
      alarmState.enabled = false;
      alarmState.ringing = false;
      SerialBT.println("✓ Alarm disabled");
      beep(100);
    }
    else {
      // Format: alarm HH:MM
      int h, m;
      if (sscanf(subcmd.c_str(), "%d:%d", &h, &m) == 2) {
        if (h >= 0 && h < 24 && m >= 0 && m < 60) {
          alarmState.hour = h;
          alarmState.minute = m;
          alarmState.enabled = true;
          SerialBT.printf("✓ Alarm set to %02d:%02d\n", h, m);
          beepPattern(2, 150, 100);
        } else {
          SerialBT.println("✗ Invalid time values");
        }
      } else {
        SerialBT.println("✗ Use format: alarm HH:MM");
      }
    }
  }
  else if (cmd.startsWith("pomo ")) {
    String subcmd = cmd.substring(5);
    subcmd.trim();
    
    if (subcmd == "start") {
      if (!pomo.running) {
        pomo.running = true;
        pomo.startMillis = millis();
        pomo.duration = (pomo.workPhase ? pomo.workMin : pomo.breakMin) * 60000UL;
        SerialBT.printf("✓ Pomodoro started: %s (%d min)\n", 
                       pomo.workPhase ? "WORK" : "BREAK",
                       pomo.workPhase ? pomo.workMin : pomo.breakMin);
        beepPattern(2, 100, 100);
      } else {
        SerialBT.println("⚠ Pomodoro already running");
      }
    }
    else if (subcmd == "stop") {
      pomo.running = false;
      pomo.ringing = false;
      SerialBT.println("✓ Pomodoro stopped");
      beep(100);
    }
    else if (subcmd.startsWith("work ")) {
      int mins = subcmd.substring(5).toInt();
      if (mins > 0 && mins <= 99) {
        pomo.workMin = mins;
        SerialBT.printf("✓ Work duration set to %d min\n", mins);
        beep(100);
      } else {
        SerialBT.println("✗ Value must be 1-99 minutes");
      }
    }
    else if (subcmd.startsWith("break ")) {
      int mins = subcmd.substring(6).toInt();
      if (mins > 0 && mins <= 99) {
        pomo.breakMin = mins;
        SerialBT.printf("✓ Break duration set to %d min\n", mins);
        beep(100);
      } else {
        SerialBT.println("✗ Value must be 1-99 minutes");
      }
    }
    else {
      SerialBT.println("✗ Unknown pomo command. Try: start, stop, work MM, break MM");
    }
  }
  else if (cmd == "reset") {
    stopAllAlerts();
    SerialBT.println("✓ All timers reset");
    beep(200);
  }
  else {
    SerialBT.println("✗ Unknown command. Type 'help' for commands");
  }
}

void beep(uint16_t ms) {
  ledcWrite(BUZZER_CHANNEL, 128);
  delay(ms);
  ledcWrite(BUZZER_CHANNEL, 0);
}

void beepPattern(uint8_t count, uint16_t duration, uint16_t gap) {
  for (uint8_t i = 0; i < count; i++) {
    beep(duration);
    if (i < count - 1) delay(gap);
  }
}

void stopAllAlerts() {
  alarmState.enabled = false;
  alarmState.ringing = false;
  alarmState.setMode = false;
  pomo.running = false;
  pomo.ringing = false;
  pomo.mode = false;
  pomo.workMin = 1;       // Reset to default
  pomo.breakMin = 1;      // Reset to default
  pomo.workPhase = true;  // Reset to work phase
  ledcWrite(BUZZER_CHANNEL, 0);
}

void printDebugInfo(const DateTime& now) {
  Serial.printf("\n=== STATUS ===\n");
  Serial.printf("Time: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
  Serial.printf("Alarm: %02d:%02d | Enabled:%d SetMode:%d Ringing:%d\n",
                alarmState.hour, alarmState.minute, alarmState.enabled, alarmState.setMode, alarmState.ringing);
  Serial.printf("Pomodoro: Mode:%d Running:%d Phase:%s Ringing:%d\n",
                pomo.mode, pomo.running, pomo.workPhase ? "WORK" : "BREAK", pomo.ringing);
  if (pomo.mode) {
    Serial.printf("Pomo Times: Work:%dmin Break:%dmin\n", pomo.workMin, pomo.breakMin);
  }
}

// ==================== BUTTON HANDLERS ====================

void handleResetButton() {
  if (digitalRead(BTN_RESET) == LOW) {
    stopAllAlerts();
    beep(200);
    Serial.println("*** RESET: All timers stopped ***");
    delay(500);
  }
}

void handleModeButton() {
  // Detect button press
  if (digitalRead(BTN_MODE) == LOW && !buttons.modePressed) {
    buttons.modePressed = true;
    buttons.modePressTime = millis();
    return;
  }

  // Handle button release
  if (digitalRead(BTN_MODE) == HIGH && buttons.modePressed) {
    unsigned long pressDuration = millis() - buttons.modePressTime;

    if (pressDuration > LONG_PRESS_MS) {
      // Long press: Toggle alarm set mode
      alarmState.setMode = !alarmState.setMode;
      pomo.mode = false;
      beep(100);
      Serial.println(alarmState.setMode ? "→ Alarm Set Mode" : "← Normal Mode");
    } else {
      // Short press: Handle based on current state
      if (alarmState.ringing) {
        alarmState.ringing = false;
        alarmState.enabled = false;
        ledcWrite(BUZZER_CHANNEL, 0);
        Serial.println("✓ Alarm stopped");
      } else if (alarmState.setMode) {
        alarmState.enabled = true;
        alarmState.setMode = false;
        beepPattern(2, 150, 100);
        Serial.printf("✓ Alarm set for %02d:%02d\n", alarmState.hour, alarmState.minute);
      } else {
        alarmState.enabled = !alarmState.enabled;
        beep(alarmState.enabled ? 200 : 100);
        Serial.println(alarmState.enabled ? "✓ Alarm ON" : "○ Alarm OFF");
      }
    }
    buttons.modePressed = false;
  }
}

void handlePomoButton() {
  // Detect button press
  if (digitalRead(BTN_POMO) == LOW && !buttons.pomoPressed) {
    buttons.pomoPressed = true;
    buttons.pomoPressTime = millis();
    return;
  }

  // Handle button release
  if (digitalRead(BTN_POMO) == HIGH && buttons.pomoPressed) {
    unsigned long pressDuration = millis() - buttons.pomoPressTime;

    if (pressDuration > LONG_PRESS_MS) {
      // Long press: Toggle pomodoro mode
      pomo.mode = !pomo.mode;
      alarmState.setMode = false;
      beep(100);
      Serial.println(pomo.mode ? "→ Pomodoro Mode" : "← Normal Mode");
    } else {
      // Short press: Handle based on current state
      if (pomo.ringing) {
        pomo.ringing = false;
        ledcWrite(BUZZER_CHANNEL, 0);
        Serial.println("✓ Pomodoro alert stopped");
      } else if (pomo.mode && !pomo.running) {
        pomo.running = true;
        pomo.startMillis = millis();
        pomo.duration = (pomo.workPhase ? pomo.workMin : pomo.breakMin) * 60000UL;
        beepPattern(2, 100, 100);
        Serial.printf("▶ Pomodoro started: %s (%d min)\n", 
                     pomo.workPhase ? "WORK" : "BREAK",
                     pomo.workPhase ? pomo.workMin : pomo.breakMin);
      } else if (pomo.running) {
        pomo.running = false;
        beep(100);
        Serial.println("⏸ Pomodoro paused");
      }
    }
    buttons.pomoPressed = false;
  }
}

void handleAdjustmentButtons() {
  unsigned long now = millis();

  if (alarmState.setMode) {
    // Adjust alarm time
    if (digitalRead(BTN_HOUR) == LOW && now - buttons.lastHourPress > DEBOUNCE_MS) {
      alarmState.hour = (alarmState.hour + 1) % 24;
      buttons.lastHourPress = now;
      Serial.printf("Alarm hour: %02d\n", alarmState.hour);
    }
    if (digitalRead(BTN_MIN) == LOW && now - buttons.lastMinPress > DEBOUNCE_MS) {
      alarmState.minute = (alarmState.minute + 1) % 60;
      buttons.lastMinPress = now;
      Serial.printf("Alarm minute: %02d\n", alarmState.minute);
    }
  } else if (pomo.mode && !pomo.running) {
    // Adjust pomodoro durations (wrap to 0 after 99)
    if (digitalRead(BTN_HOUR) == LOW && now - buttons.lastHourPress > DEBOUNCE_MS) {
      pomo.workMin++;
      if (pomo.workMin > 99) pomo.workMin = 0;  // Wrap to 0
      buttons.lastHourPress = now;
      Serial.printf("Work time: %d min\n", pomo.workMin);
    }
    if (digitalRead(BTN_MIN) == LOW && now - buttons.lastMinPress > DEBOUNCE_MS) {
      pomo.breakMin++;
      if (pomo.breakMin > 99) pomo.breakMin = 0;  // Wrap to 0
      buttons.lastMinPress = now;
      Serial.printf("Break time: %d min\n", pomo.breakMin);
    }
  }
}

// ==================== DISPLAY FUNCTIONS ====================

void updateDisplay(const DateTime& now) {
  if (alarmState.setMode) {
    // Blink alarm time being set
    if (millis() - displayState.lastBlinkTime > BLINK_INTERVAL_MS) {
      displayState.blinkState = !displayState.blinkState;
      displayState.lastBlinkTime = millis();
    }
    
    if (displayState.blinkState) {
      display.showNumberDecEx(alarmState.hour * 100 + alarmState.minute, 0b01000000, true);
    } else {
      display.clear();
    }
  } else if (pomo.mode) {
    // Show pomodoro timer
    if (pomo.running) {
      unsigned long elapsed = millis() - pomo.startMillis;
      if (elapsed < pomo.duration) {
        unsigned long remainSec = (pomo.duration - elapsed) / 1000;
        int m = remainSec / 60;
        int s = remainSec % 60;
        display.showNumberDecEx(m * 100 + s, 0b01000000, true);
      } else {
        display.showNumberDecEx(0, 0b01000000, true);
      }
    } else {
      // Show work duration when not running
      display.showNumberDecEx(pomo.workMin * 100, 0, true);
    }
  } else {
    // Show current time
    display.showNumberDecEx(now.hour() * 100 + now.minute(), 0b01000000, true);
  }
}

// ==================== ALARM & TIMER LOGIC ====================

void checkAlarmTrigger(const DateTime& now) {
  // Reset the "rang this minute" flag when minute changes
  if (now.minute() != lastMinute) {
    lastMinute = now.minute();
    alarmState.rangThisMinute = false;
  }

  // Trigger alarm if conditions met
  if (alarmState.enabled && 
      now.hour() == alarmState.hour && 
      now.minute() == alarmState.minute && 
      !alarmState.rangThisMinute) {
    alarmState.rangThisMinute = true;
    alarmState.ringing = true;
    Serial.println("⏰ ALARM RINGING!");
  }
}

void checkPomodoroComplete() {
  if (pomo.running && millis() - pomo.startMillis >= pomo.duration) {
    pomo.running = false;
    pomo.ringing = true;
    pomo.workPhase = !pomo.workPhase;
    beepPattern(3, 150, 150);
    Serial.printf("✓ Pomodoro complete! Next: %s\n", 
                 pomo.workPhase ? "WORK" : "BREAK");
  }
}

void updateBuzzer() {
  if (alarmState.ringing || pomo.ringing) {
    ledcWrite(BUZZER_CHANNEL, 128);
  } else {
    ledcWrite(BUZZER_CHANNEL, 0);
  }
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configure pins
  pinMode(BTN_HOUR, INPUT_PULLUP);
  pinMode(BTN_MIN, INPUT_PULLUP);
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_POMO, INPUT_PULLUP);
  pinMode(BTN_RESET, INPUT_PULLUP);

  // Initialize I2C and RTC
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!rtc.begin()) {
    Serial.println("ERROR: RTC not found!");
    while (1) delay(1000);
  }

  // Initialize display
  display.setBrightness(DISPLAY_BRIGHTNESS);

  // Initialize buzzer
  ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, BUZZER_RES);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

  // Initialize Bluetooth
  if (!SerialBT.begin("ESP32_Clock")) {
    Serial.println("ERROR: Bluetooth failed to start!");
  } else {
    Serial.println("Bluetooth started: ESP32_Clock");
  }

  // Startup sequence
  Serial.println("\n╔════════════════════════════════╗");
  Serial.println("║ ALARM + POMODORO CLOCK v2.0    ║");
  Serial.println("║ Bluetooth: ESP32_Clock         ║");
  Serial.println("╚════════════════════════════════╝");
  beepPattern(2, 200, 200);
}

// ==================== MAIN LOOP ====================

void loop() {
  DateTime now = rtc.now();

  // Handle Bluetooth commands
  handleBluetoothCommands();

  // Periodic debug output
  if (millis() - lastDebugPrint > DEBUG_INTERVAL_MS) {
    lastDebugPrint = millis();
    printDebugInfo(now);
  }

  // Handle all buttons
  handleResetButton();
  handleModeButton();
  handlePomoButton();
  handleAdjustmentButtons();

  // Update display
  updateDisplay(now);

  // Check triggers
  checkAlarmTrigger(now);
  checkPomodoroComplete();

  // Update buzzer state
  updateBuzzer();

  delay(LOOP_DELAY_MS);
}
