
<img width="707" height="598" alt="screenshot 2025-10-07 162114" src="https://github.com/user-attachments/assets/a441b795-f72d-4563-ba61-77b1b2109a6c" />

![IMG_2819](https://github.com/user-attachments/assets/f45dfd88-631c-4bb2-9020-1739fe4b1769)
![IMG_2818](https://github.com/user-attachments/assets/e90b2215-6fda-4452-a082-e59af2779a2e)
![IMG_2820](https://github.com/user-attachments/assets/f55058b9-d4b9-47f0-ae0e-e10b9d106f0c)

# Renesas Synergy Digital Scale Project

## Overview

This project implements a digital scale using the Renesas Synergy platform with a load cell and the HX711 ADC module.
The measured weight data is transmitted over UART serial communication, which can be monitored on a PC using Tera Term.
This project is a practical reference for integrating sensors and human-machine interfaces with Renesas microcontrollers.
An **Adafruit 24LC32 I2C EEPROM (32kbit / 4KB)** is included to store calibration data and offset values, and user settings so they are retained across power cycles.

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

## EEPROM (Adafruit 24LC32 I2C)

This project uses the **Adafruit 24LC32 I2C EEPROM breakout** (32kbit = 4KB).  
It is a small non-volatile memory chip that stores data permanently using the I²C interface.

- Capacity: **32kbit (4KB)**
- Interface: **I²C (address 0x50–0x57)**
- Voltage: **2.5V – 5.5V**

---
## How to Use

1. Connect the load cell and HX711 to the MCU
2. Connect the FTDI USB serial adapter if you need UART output
3. Open Tera Term on your PC to monitor the serial data
4. Build the project with e² studio　Synergy
5. Write the program to the board
6. Power on and start measuring
