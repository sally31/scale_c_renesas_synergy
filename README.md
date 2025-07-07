# Renesas Synergy Digital Scale Project

## Overview

This project implements a digital scale using the Renesas Synergy platform with a load cell and the HX711 ADC module.
The measured weight data is transmitted over UART serial communication, which can be monitored on a PC using Tera Term.
This project is a practical reference for integrating sensors and human-machine interfaces with Renesas microcontrollers.


---

## Hardware Environment

- **MCU**: R7FS5D97 (Synergy S5D9 Group)
- **Board**: PK-S5D9 (Renesas Synergy Promotion Kit)
- **External modules**:
  - HX711 load cell amplifier
  - Load cell sensor
  - USB to TTL adapter (DSD TECH SH-U09C2 with FTDI FT232RL)

---

## Features

- Weight measurement with load cell
- UART serial data output for debugging/logging

---

## Serial Communication

For data logging and debugging, the project uses a USB to TTL serial adapter:

- **Adapter**: DSD TECH SH-U09C2
- **Chipset**: FTDI FT232RL
- **Connection**:
  - TXD, RXD, GND
  - 3.3V logic level (match MCU levels)
  - Baudrate: 9600 bps
- Recommended terminal software: **Tera Term**
  - [Tera Term Download](https://osdn.net/projects/ttssh2/releases/)
  - Use Tera Term to view the weight data sent from the MCU via UART.

---

## How to Use

1. Connect the load cell and HX711 to the MCU
2. Connect the LCD and the tare button
3. Connect the FTDI USB serial adapter if you need UART output
4. Open Tera Term on your PC to monitor the serial data
5. Build the project with e² studio　Synergy
6. Write the program to the board
7. Power on and start measuring
