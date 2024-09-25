# Smart Lock System

This project is a C++-based smart lock system developed on an ESP32 board. It integrates RFID, LCD, I2C, a keypad, and a custom website for both local and remote control. The smart lock provides security through RFID scanning and password entry, with user interaction allowed using an LCD. Remote control is managed via a custom-built website.

## Features

- **RFID Control**: Unlock the system using RFID tags.
- **Password Input via Keypad**: Enter a password using a 4x3 keypad.
- **LCD Display**: User interaction and system messages are displayed on an LCD using the I2C protocol.
- **Remote Control via Website**: A custom website allows for remote control of the lock.

> Note: The system was planned to include an internal button for control from inside, but this feature was not implemented due to time constraints.

## Components Used

- **ESP32 Board**: The central microcontroller for the system.
- **RFID Module**: Used to scan RFID tags for access.
- **4x3 Keypad**: Allows users to input passwords.
- **LCD Display with I2C**: Provides user feedback and displays system states.
- **Custom Website**: Enables remote control and monitoring.

## How It Works

1. **RFID Scanning**: Users can unlock the system using registered RFID tags.
2. **Password Entry**: Users can also unlock the system by entering a password using the keypad.
3. **User Interaction**: The LCD screen provides feedback and displays system prompts.
4. **Remote Control**: The custom website enables users to control the lock remotely.

## Planned Features

- **Internal Button**: A button for internal control was planned but not added due to time limitations.

## How to Run

1. Flash the ESP32 with the project code.
2. Connect the hardware components (RFID, LCD, keypad) to the ESP32 board.
3. Use the custom website for remote control (instructions for the website can be found in the repository).
4. Monitor user interaction via the LCD display.

## Requirements

- ESP32 development board
- RFID module
- 4x3 Keypad
- I2C-enabled LCD display
- Web hosting for the custom control website
- Arduino IDE with ESP32 board support


