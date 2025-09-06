# Wiring Diagram and Instructions for Plue Pill Relay Modbus Project

## Wiring Overview

This document provides the necessary wiring instructions to connect the Plue Pill board to an eight-channel relay module and the associated switches. The connections are made using transistor switches to control the relays.

### Components Required

- Plue Pill board (STM32F103C8T6)
- Eight-channel relay module
- NPN transistors (e.g., 2N3904)
- Resistors (1kΩ for base resistors)
- Push-button switches or toggle switches
- Jumper wires
- Breadboard (optional)

### Wiring Diagram

1. **Relay Module Connections:**
   - Connect the VCC pin of the relay module to the 5V pin on the Plue Pill board.
   - Connect the GND pin of the relay module to the GND pin on the Plue Pill board.
   - Connect the IN1 to IN8 pins of the relay module to the collector of the corresponding NPN transistors (Q1 to Q8).

2. **Transistor Switch Connections:**
   - Connect the emitter of each NPN transistor (Q1 to Q8) to GND.
   - Connect a 1kΩ resistor from the base of each transistor to the corresponding GPIO pin on the Plue Pill board (e.g., PA0 to PA7).
   - Ensure that the GPIO pins are configured as output in the firmware.

3. **Switch Connections:**
   - Connect one terminal of each switch to GND.
   - Connect the other terminal of each switch to the corresponding GPIO pin on the Plue Pill board (e.g., PB0 to PB7).
   - Configure the GPIO pins connected to the switches as input with pull-up resistors enabled.

### Example Connections

| Relay Channel | Relay IN Pin | Transistor GPIO Pin | Switch GPIO Pin |
|---------------|--------------|---------------------|------------------|
| Relay 1       | IN1          | PA0                 | PB0              |
| Relay 2       | IN2          | PA1                 | PB1              |
| Relay 3       | IN3          | PA2                 | PB2              |
| Relay 4       | IN4          | PA3                 | PB3              |
| Relay 5       | IN5          | PA4                 | PB4              |
| Relay 6       | IN6          | PA5                 | PB5              |
| Relay 7       | IN7          | PA6                 | PB6              |
| Relay 8       | IN8          | PA7                 | PB7              |

### Notes

- Ensure all connections are secure to prevent intermittent issues.
- Verify the transistor orientation: the flat side should face the base resistor.
- The relay module should be powered according to its specifications; typically, it operates at 5V.

By following these instructions, you will successfully wire the Plue Pill board to control the relay module using switches and Modbus communication.