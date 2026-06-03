# Smart Lock System

A smart lock system built on ESP32 microcontroller, integrating RFID, keypad input, custom web app for control, and LCD feedback.

## Features

- **RFID Control**: Unlock the system using RFID tags
- **Password Input**: Enter a PIN using a 4x3 keypad
- **LCD Display**: Real-time user feedback via I2C LCD
- **Remote Control**: Web-based lock/unlock interface
- **Emergency Button**: Physical override for manual unlock
- **Password Management**: Change master password via keypad

## Components Used

- **ESP32 Board**: Central microcontroller
- **MFRC522 RFID Module**: Card-based authentication
- **4x3 Keypad**: PIN entry interface
- **LCD Display (20x4)**: I2C-based user display
- **LED Indicators**: Green (Unlock) and Red (Lock) status
- **Door Lock Relay**: GPIO-controlled lock mechanism

## Hardware Setup

### Pin Configuration

| Component | Function | ESP32 Pin |
|-----------|----------|-----------|
| Green LED | Unlock indicator | GPIO 32 |
| Red LED | Lock indicator | GPIO 25 |
| Lock Relay | Door lock control | GPIO 14 |
| Emergency Button | Manual unlock | GPIO 33 |
| RFID SS | SPI Chip Select | GPIO 5 |
| RFID RST | RFID Reset | GPIO 27 |
| I2C SDA | Keypad & LCD | GPIO 21 |
| I2C SCL | Keypad & LCD | GPIO 22 |
| SPI MOSI | SPI Data Out | GPIO 23 |
| SPI MISO | SPI Data In | GPIO 19 |
| SPI CLK | SPI Clock | GPIO 18 |

### I2C Addresses

- Keypad (PCF8574): `0x20`
- LCD Display: `0x27` (or `0x3F`)

## Installation

1. Install ESP32 board support in Arduino IDE
2. Install required libraries:
   - Keypad_I2C
   - LiquidCrystal_I2C
   - MFRC522
3. Open `source/Main.ino` and update WiFi credentials (lines 84-85)
4. Upload to ESP32

## Configuration

### WiFi Credentials
Edit lines 84-85 in `Main.ino`:
```cpp
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
```

### Default Passwords
- Primary: `0000`
- Secondary: `1234`

Change in lines 31-32 of `Main.ino`

## Web API Endpoints

| Endpoint | Method | Action |
|----------|--------|--------|
| `/` | GET | Display control dashboard |
| `/lock` | GET | Lock the door |
| `/unlock` | GET | Unlock the door |

**Access**: `http://<ESP32_IP>/`

## Usage

### Authentication Methods

1. **RFID Card**: Hold registered card to reader (~4cm)
2. **Keypad PIN**: Enter 4-digit password + `#` to confirm
3. **Web Interface**: Click Lock/Unlock button on dashboard
4. **Emergency Button**: Press physical button for manual override

### Password Change

1. Enter any key to start
2. Type `*#` (asterisk + hash) to enter password change mode
3. Enter old password + `#`
4. Enter new password + `#`
5. Green LED confirms successful change

## Security Notes

**Current Implementation (Development)**:
- Passwords stored in plain text
- No authentication on web API
- RFID UIDs easily cloned

 **For Production**:
- Use encrypted password storage (SHA-256)
- Add API token authentication
- Implement attempt rate limiting
- Use secure WiFi with credentials in EEPROM

## Troubleshooting

| Issue | Solution |
|-------|----------|
| WiFi fails to connect | Check SSID/password, try 2.4GHz network |
| LCD shows gibberish | Verify I2C address (0x27 or 0x3F) |
| RFID not detected | Check SPI pins (GPIO 18,19,23), verify module power |
| Keypad not working | Verify I2C connection, check PCF8574 address (0x20) |
| LED not lighting | Check LED polarity and GPIO pin assignment |

## Requirements

- Arduino IDE with ESP32 support
- ESP32 Dev Board
- MFRC522 RFID Module
- 4x3 Keypad with I2C PCF8574 module
- 20x4 I2C LCD Display
- 2x LEDs (220Ω resistors)
- Door lock relay module
