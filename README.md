# ESP32 Alarm & Pomodoro Clock

A feature-rich digital clock with alarm and pomodoro timer functionality, built on ESP32 with Bluetooth control.

## 🎯 Features

- **Real-Time Clock (RTC)**: Accurate timekeeping with DS3231 RTC module
- **7-Segment Display**: Clear time display using TM1637 module
- **Alarm Function**: Set and manage alarms with audio alerts
- **Pomodoro Timer**: Built-in productivity timer with work/break phases
- **Bluetooth Control**: Full remote control via smartphone
- **Physical Buttons**: Manual control with 5 buttons
- **Audio Feedback**: Buzzer alerts for alarms and notifications

## 📋 Hardware Requirements

### Components
- **ESP32 Development Board**
- **DS3231 RTC Module** (I2C)
- **TM1637 4-Digit 7-Segment Display**
- **Piezo Buzzer** (or small speaker)
- **5x Push Buttons**
- **Jumper Wires**
- **Breadboard** (optional)

### Pin Connections

| Component | ESP32 Pin | Description |
|-----------|-----------|-------------|
| BTN_HOUR | GPIO 4 | Hour adjustment button |
| BTN_MIN | GPIO 5 | Minute adjustment button |
| BTN_MODE | GPIO 17 | Alarm mode button |
| BTN_POMO | GPIO 16 | Pomodoro mode button |
| BTN_RESET | GPIO 21 | Reset button |
| BUZZER | GPIO 27 | Buzzer output |
| TM1637 CLK | GPIO 18 | Display clock |
| TM1637 DIO | GPIO 19 | Display data |
| DS3231 SDA | GPIO 25 | I2C data |
| DS3231 SCL | GPIO 26 | I2C clock |

## 🔧 Software Requirements

### Libraries
- `Wire` (I2C communication)
- `RTClib` by Adafruit (v2.1.4 or later)
- `Adafruit BusIO` (v1.17.4 or later)
- `TM1637` display library
- `BluetoothSerial` (included with ESP32 core)

### Installation

1. **Install PlatformIO** (or Arduino IDE with ESP32 support)

2. **Clone or download this project**

3. **Install required libraries:**
   ```bash
   pio lib install "RTClib"
   pio lib install "Adafruit BusIO"
   pio lib install "TM1637"
   ```

4. **Upload to ESP32:**
   ```bash
   pio run -t upload
   ```

## 🎮 Physical Button Controls

### Normal Mode
- **BTN_MODE (Short Press)**: Toggle alarm on/off
- **BTN_MODE (Long Press >1s)**: Enter alarm set mode
- **BTN_POMO (Short Press)**: Start/pause pomodoro
- **BTN_POMO (Long Press >1s)**: Enter pomodoro mode
- **BTN_RESET**: Reset all timers and stop alerts

### Alarm Set Mode
- **BTN_HOUR**: Increment alarm hour (0-23)
- **BTN_MIN**: Increment alarm minute (0-59)
- **BTN_MODE (Short Press)**: Confirm and enable alarm
- **BTN_MODE (Long Press)**: Exit without saving

### Pomodoro Mode
- **BTN_HOUR**: Increment work duration (wraps at 99)
- **BTN_MIN**: Increment break duration (wraps at 99)
- **BTN_POMO (Short Press)**: Start timer
- **BTN_POMO (Long Press)**: Exit pomodoro mode

## 📱 Bluetooth Commands

### Connection
1. Enable Bluetooth on your smartphone
2. Search for device: **ESP32_Clock**
3. Connect (no pairing code required)
4. Use a Bluetooth Serial Terminal app

### Command Reference

| Command | Description | Example |
|---------|-------------|---------|
| `help` | Show all commands | `help` |
| `status` | Display current status | `status` |
| `time HH:MM:SS` | Set current time | `time 14:30:00` |
| `alarm HH:MM` | Set alarm time | `alarm 07:30` |
| `alarm on` | Enable alarm | `alarm on` |
| `alarm off` | Disable alarm | `alarm off` |
| `pomo start` | Start pomodoro timer | `pomo start` |
| `pomo stop` | Stop pomodoro timer | `pomo stop` |
| `pomo work MM` | Set work duration (1-99 min) | `pomo work 25` |
| `pomo break MM` | Set break duration (1-99 min) | `pomo break 5` |
| `reset` | Reset all timers | `reset` |

### Command Features
- ✅ Case-insensitive commands
- ✅ Real-time feedback (✓ success, ✗ error, ⚠ warning)
- ✅ Audio confirmation beeps
- ✅ Input validation

## 🔊 Audio Feedback

- **Single beep**: Setting confirmation
- **Double beep**: Alarm/timer set
- **Triple beep**: Pomodoro phase complete
- **Continuous beep**: Alarm or timer ringing

## 🎨 Display Modes

### Normal Mode
Shows current time with blinking colon (HH:MM)

### Alarm Set Mode
Blinks the alarm time being set

### Pomodoro Running
Shows countdown timer (MM:SS)

### Pomodoro Setup
Shows work duration when not running

## 📖 Usage Examples

### Setting Up Morning Alarm
1. Long press BTN_MODE (enter alarm set mode)
2. Press BTN_HOUR repeatedly to set hour (e.g., 07)
3. Press BTN_MIN repeatedly to set minute (e.g., 30)
4. Short press BTN_MODE to confirm
5. Display stops blinking - alarm is set for 07:30

**Or via Bluetooth:**
```
alarm 07:30
```

### Using Pomodoro Timer
1. Long press BTN_POMO (enter pomodoro mode)
2. Press BTN_HOUR to set work time (e.g., 25 minutes)
3. Press BTN_MIN to set break time (e.g., 5 minutes)
4. Short press BTN_POMO to start
5. Timer counts down, buzzer sounds when phase completes
6. Automatically switches between work/break phases

**Or via Bluetooth:**
```
pomo work 25
pomo break 5
pomo start
```

### Quick Time Set via Bluetooth
```
time 09:15:00
status
```

## 🔄 Default Settings

- **Pomodoro Work**: 1 minute (for testing)
- **Pomodoro Break**: 1 minute (for testing)
- **Display Brightness**: Maximum (15/15)
- **Buzzer Frequency**: 2000 Hz
- **Bluetooth Name**: ESP32_Clock

## 🐛 Troubleshooting

### RTC Not Found
- Check I2C connections (SDA=25, SCL=26)
- Verify DS3231 module has power
- Ensure Wire.begin() pins match your wiring

### Display Not Working
- Verify TM1637 connections (CLK=18, DIO=19)
- Check display power supply (3.3V or 5V)
- Try adjusting brightness in code

### Bluetooth Won't Connect
- Ensure ESP32 is powered on
- Check if Bluetooth is enabled on phone
- Try forgetting device and reconnecting
- Verify no other device is connected

### Buttons Not Responding
- Check pull-up resistors or INPUT_PULLUP mode
- Verify button wiring (button between GPIO and GND)
- Check debounce timing if buttons are too sensitive

### Buzzer Not Working
- Verify buzzer polarity (if polarized)
- Check GPIO 27 connection
- Ensure LEDC channel is properly configured
- Try different buzzer frequency in code

## 📝 Code Structure

```
main.cpp
├── Pin Definitions
├── Object Instances
├── State Structures
│   ├── AlarmState
│   ├── PomoState
│   ├── ButtonState
│   └── DisplayState
├── Helper Functions
│   ├── beep()
│   ├── beepPattern()
│   ├── stopAllAlerts()
│   ├── sendBTStatus()
│   └── handleBluetoothCommands()
├── Button Handlers
│   ├── handleResetButton()
│   ├── handleModeButton()
│   ├── handlePomoButton()
│   └── handleAdjustmentButtons()
├── Display Functions
│   └── updateDisplay()
├── Alarm & Timer Logic
│   ├── checkAlarmTrigger()
│   ├── checkPomodoroComplete()
│   └── updateBuzzer()
└── Main Functions
    ├── setup()
    └── loop()
```

## 🎓 How It Works

### Alarm System
- Checks current time against alarm time every minute
- Uses `rangThisMinute` flag to prevent multiple triggers
- Continuous buzzer until stopped manually
- Automatically disables after being stopped

### Pomodoro Timer
- Uses `millis()` for accurate timing
- Tracks elapsed time since start
- Automatically switches between work/break phases
- Displays countdown in MM:SS format
- Can be paused and resumed

### Button Debouncing
- 200ms debounce delay for adjustment buttons
- Long press detection (>1000ms) for mode switching
- State tracking to prevent multiple triggers

### Bluetooth Protocol
- Simple text-based commands
- Line-terminated messages (`\n`)
- Immediate response feedback
- Non-blocking command processing

## 🔐 Security Note

This implementation uses classic Bluetooth (SPP) without authentication. For production use, consider:
- Adding PIN authentication
- Implementing command authentication
- Using BLE with encryption
- Adding access control

## 📜 License

This project is open source. Feel free to modify and distribute.

## 🤝 Contributing

Contributions are welcome! Areas for improvement:
- Add WiFi time synchronization (NTP)
- Implement multiple alarms
- Add sleep mode for power saving
- Create mobile app for better control
- Add EEPROM storage for settings persistence
- Implement custom alarm sounds

## 📧 Support

For issues or questions:
1. Check the troubleshooting section
2. Review pin connections
3. Verify library versions
4. Check Serial Monitor output for debug info

## 🎉 Version History

### v2.0 (Current)
- ✨ Added Bluetooth control
- ✨ Improved code structure with structs
- ✨ Enhanced button handling
- ✨ Better audio feedback patterns
- ✨ Wrap-around for pomodoro timer settings
- 🐛 Fixed naming conflicts with system functions
- 🐛 Reset button now properly resets pomodoro

### v1.0
- Initial release with basic alarm and pomodoro functionality

---

**Made with ❤️ for productivity enthusiasts**
