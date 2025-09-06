# Modbus Register Map

This document outlines the Modbus register map used in the Plue Pill Relay Modbus project. The register map defines how the relay states and switch inputs are mapped to specific Modbus addresses for communication with Home Assistant.

## Register Map Overview

| Register Address | Functionality                | Description                                      |
|------------------|------------------------------|--------------------------------------------------|
| 0x0001           | Relay 1 Control              | Write 0 to turn off, Write 1 to turn on         |
| 0x0002           | Relay 2 Control              | Write 0 to turn off, Write 1 to turn on         |
| 0x0003           | Relay 3 Control              | Write 0 to turn off, Write 1 to turn on         |
| 0x0004           | Relay 4 Control              | Write 0 to turn off, Write 1 to turn on         |
| 0x0005           | Relay 5 Control              | Write 0 to turn off, Write 1 to turn on         |
| 0x0006           | Relay 6 Control              | Write 0 to turn off, Write 1 to turn on         |
| 0x0007           | Relay 7 Control              | Write 0 to turn off, Write 1 to turn on         |
| 0x0008           | Relay 8 Control              | Write 0 to turn off, Write 1 to turn on         |
| 0x0010           | Switch 1 State               | Read the state of Switch 1 (0 = OFF, 1 = ON)    |
| 0x0011           | Switch 2 State               | Read the state of Switch 2 (0 = OFF, 1 = ON)    |
| 0x0012           | Switch 3 State               | Read the state of Switch 3 (0 = OFF, 1 = ON)    |
| 0x0013           | Switch 4 State               | Read the state of Switch 4 (0 = OFF, 1 = ON)    |
| 0x0014           | Switch 5 State               | Read the state of Switch 5 (0 = OFF, 1 = ON)    |
| 0x0015           | Switch 6 State               | Read the state of Switch 6 (0 = OFF, 1 = ON)    |
| 0x0016           | Switch 7 State               | Read the state of Switch 7 (0 = OFF, 1 = ON)    |
| 0x0017           | Switch 8 State               | Read the state of Switch 8 (0 = OFF, 1 = ON)    |

## Usage

- To control a relay, write to the corresponding relay control register (0x0001 to 0x0008).
- To read the state of a switch, read from the corresponding switch state register (0x0010 to 0x0017).

This register map allows for seamless integration with Home Assistant, enabling remote control and monitoring of the relay states and switch inputs.