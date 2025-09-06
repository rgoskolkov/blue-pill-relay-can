# Plue Pill Relay Modbus Project

This project is designed for the Plue Pill board using the STM32Cube framework. It interfaces with an eight-channel relay module through transistor switches and allows control via Modbus protocol, making it compatible with Home Assistant.

## Project Structure

```
plue-pill-relay-modbus
├── src
│   ├── main.c               # Entry point of the application
│   ├── gpio.c               # GPIO configuration and management
│   ├── relay_driver.c       # Relay control functions
│   ├── input_driver.c       # Switch input handling
│   ├── debounce.c           # Debounce logic for switches
│   ├── modbus_rtu.c         # Modbus RTU protocol implementation
│   └── modbus_map.c         # Modbus register mapping
├── include
│   ├── gpio.h               # GPIO functions and constants
│   ├── relay_driver.h       # Relay driver functions and constants
│   ├── input_driver.h       # Input driver functions and constants
│   ├── debounce.h           # Debounce functions and constants
│   ├── modbus_rtu.h         # Modbus RTU functions and constants
│   └── board_config.h       # Board-specific configuration
├── lib
│   └── modbus-rtu
│       ├── src
│       │   ├── mb_port.c    # Platform-specific Modbus code
│       │   └── mb_rtu.c     # Core Modbus RTU implementation
│       └── library.json      # Modbus RTU library description
├── drivers
│   ├── transistor_switch.c   # Transistor switch control functions
│   └── transistor_switch.h    # Transistor switch functions and constants
├── ext
│   └── stm32cube
│       └── BluePill.ioc      # STM32CubeMX configuration file
├── test
│   └── test_main.c           # Test code for functionality verification
├── docs
│   ├── wiring.md             # Wiring diagrams and instructions
│   └── modbus_register_map.md # Modbus register map documentation
├── .vscode
│   ├── settings.json         # VS Code settings
│   └── launch.json           # Debugging configuration
├── platformio.ini            # PlatformIO configuration
└── README.md                 # Project documentation
```

## Features

- Control of an eight-channel relay module using transistor switches.
- Input handling for both momentary and toggle switches.
- Relay activation based on switch state changes.
- Modbus protocol support for integration with Home Assistant.

## Setup Instructions

1. Clone the repository to your local machine.
2. Open the project in your preferred IDE (e.g., VS Code).
3. Ensure that PlatformIO is installed and configured.
4. Connect the Plue Pill board to your computer.
5. Build the project using PlatformIO.
6. Upload the firmware to the Plue Pill board.
7. Follow the wiring instructions in `docs/wiring.md` to connect the relay module and switches.
8. Configure Home Assistant to communicate with the Modbus protocol as described in `docs/modbus_register_map.md`.

## Usage

Once the project is set up and running, toggling the switches will activate the corresponding relays. You can also control the relays through Modbus commands sent from Home Assistant.

For further details on wiring and Modbus register mapping, refer to the documentation in the `docs` folder.