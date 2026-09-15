# NUCLEO-H753ZI Exoskeleton Firmware Base & BNO085 SPI Driver

Production-ready firmware template and high-speed **BNO085 9-DOF IMU Sensor Hub SPI Driver** for the **STMicroelectronics NUCLEO-H753ZI** development board (Arm Cortex-M7 @ 400 MHz), built with STM32CubeMX HAL and PlatformIO.

This project implements the official **CEVA / Hillcrest Laboratories SHTP (Sensor Hub Transport Protocol)** over full-duplex SPI, enabling high-rate, low-latency streaming of **Game Rotation Vector Quaternions** and real-time **Euler Angles (Yaw, Pitch, Roll)**.

---

## Hardware Overview

| Component | Specifications |
| :--- | :--- |
| **Development Board** | STMicroelectronics NUCLEO-H753ZI (Nucleo-144) |
| **Microcontroller** | STM32H753ZIT6 (Cortex-M7 @ 400 MHz, 2 MB Flash, 512 KB SRAM) |
| **IMU Sensor** | Adafruit 9-DOF Orientation IMU Fusion Breakout - BNO085 (Product #4754) |
| **Bus Interface** | SPI1 (Mode 3: CPOL=1, CPHA=1, Full-Duplex TransmitReceive) |
| **Debug Serial** | USART6 (115200 8-N-1 on PC6/PC7) |

---

## BNO085 SPI Wiring Guide

Connect the Adafruit BNO085 breakout directly to the Nucleo-144 Arduino Zio female headers as shown below:

| Adafruit BNO085 Pin | Function | Nucleo Pin | Header Location | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **VIN** | Power Supply | **`3V3`** | CN8 pin 7 | 3.3V power |
| **GND** | Ground | **`GND`** | CN8 pin 11 | Common ground |
| **SCL** | SPI Clock | **`PA5` (D13)** | CN7 pin 10 | SPI1 SCK |
| **SDA** | SPI MISO | **`PA6` (D12)** | CN7 pin 12 | SPI1 MISO (Sensor Data Out) |
| **DI** | SPI MOSI | **`PB5` (D11)** | CN7 pin 14 | SPI1 MOSI (Data In to sensor) |
| **CS** | Chip Select | **`PD14` (D10)**| CN7 pin 16 | Active LOW GPIO output |
| **RST** | Hardware Reset | **`PD15` (D9)** | CN7 pin 18 | Active LOW GPIO output |
| **INT** | Host Interrupt | **`PF3` (D8)**  | CN7 pin 20 | Active LOW GPIO input (Pull-up) |
| **P0** | Protocol / WAKE | **`PG12` (D7)** | CN10 pin 2 | Active LOW Wake pulse (Init HIGH) |
| **P1** | Protocol Select | **`3V3`** | CN8 pin 7 | Tied permanently to 3.3V for SPI |

> [!IMPORTANT]
> **Mode Selection:** For SPI communication, both `P0` (WAKE) and `P1` must be HIGH during reset. The firmware initializes `PG12` (D7) HIGH on startup, so the sensor enters SPI mode cleanly.
>
> **Nucleo Pin Mapping Note:** On the NUCLEO-H753ZI, Arduino **D11** is physically routed to **PB5** (not PA7). Outgoing SPI1 MOSI is mapped to PB5 via `GPIO_AF5_SPI1`.

### External Serial Monitor (USART6)

Debug log and orientation data are output via **USART6**:

| Nucleo Pin | Arduino Label | Location | Serial Adapter Pin |
| :--- | :--- | :--- | :--- |
| **PC6** | **D1** (TX) | CN8 pin 2 | Connect to USB-to-TTL **RX** |
| **PC7** | **D0** (RX) | CN8 pin 4 | Connect to USB-to-TTL **TX** |
| **GND** | **GND** | CN8 pin 11 | Connect to USB-to-TTL **GND** |

---

## Driver Implementation Details

The driver ([Core/Src/bno085.c](Core/Src/bno085.c), [Core/Inc/bno085.h](Core/Inc/bno085.h)) implements the CEVA SHTP specification:

1. **Hardware Reset & Boot Drain:**
   - Pulses `RST` LOW for 15 ms.
   - Drains the initial startup packets:
     - SHTP Advertisement packet (CH0, 276 bytes)
     - Executable reset packet (CH1, 5 bytes)
     - SH-2 System Initialized notification `0xF1` (CH2, 20 bytes)
   - Waits until the interrupt line settles into idle.

2. **Official CEVA Host-Write Sequence:**
   ```text
   Assert WAKE LOW -> Wait HINTN LOW -> Assert CS LOW -> Deassert WAKE HIGH -> Full-Duplex SPI -> Deassert CS HIGH -> Wait HINTN HIGH
   ```
   This ensures the sensor wakes from deep sleep, acknowledges readiness via `HINTN`, and accepts the command without dropping bytes.

3. **Sensor Configuration:**
   - Transmits a 21-byte `Set Feature Command` (`0xFD`) on Channel 2 for **Game Rotation Vector** (Report `0x08`).
   - Configures a 5,000 µs interval (200 Hz update rate) with zero batch latency.

4. **Packet Parser & Math:**
   - Detects the `0xFB` Base Timestamp record prepended to sensor hub packets.
   - Extracts the 16-bit Q14 fixed-point quaternion components ($q_r, q_i, q_j, q_k$).
   - Converts fixed-point values to normalized unit quaternions ($1 \text{ LSB} = 1/16384$).
   - Computes standard Euler angles (**Yaw**, **Pitch**, **Roll**) in degrees.

---

## Project Structure

```text
├── .gitignore
├── boards/
│   └── nucleo_h753zi.json       # Custom board config with ST-LINK/V3 OpenOCD fix
├── Core/
│   ├── Inc/
│   │   ├── bno085.h             # BNO085 driver API & struct definitions
│   │   ├── main.h               # Pin assignments & peripheral handles
│   │   └── ...
│   └── Src/
│       ├── bno085.c             # SHTP SPI protocol & quaternion decoder
│       ├── main.c               # Application entry point & measurement loop
│       ├── gpio.c               # GPIO clock & pin initialization (CS, RST, INT, WAKE)
│       ├── spi.c                # SPI1 peripheral configuration (Mode 3, PB5 MOSI)
│       └── usart.c              # USART6 configuration (115200 baud)
├── nucleo-h753zi-blink.ioc      # STM32CubeMX project file
├── platformio.ini               # PlatformIO build configuration
└── README.md
```

---

## Getting Started

### Prerequisites

- [VS Code](https://code.visualstudio.com/)
- [PlatformIO IDE Extension](https://platformio.org/install/ide?install=vscode)
- [Git](https://git-scm.com/)

### Build the Firmware

Compile the project using the PlatformIO CLI or status bar icon:
```bash
pio run
```

### Upload to NUCLEO-H753ZI

Connect the board via the ST-LINK USB port (CN1) and run:
```bash
pio run --target upload
```

### Monitor Serial Stream

Open the terminal on your external USB-to-TTL COM port (e.g. `COM11`):
```bash
pio device monitor --port COM11 --baud 115200
```

### Sample Live Output

```text
========================================
   BNO085 Stage 2: 200Hz Quaternion Stream
========================================
[1/3] Hardware Resetting BNO085...
[2/3] Draining boot packets until SH-2 is initialized...
  -> Boot packet: 276 bytes on CH0 (ID: 0x00)
  -> Boot packet: 5 bytes on CH1 (ID: 0x01)
  -> Boot packet: 20 bytes on CH2 (ID: 0xF1)
  [OK] SH-2 System Initialized (0xF1 received)!
[3/3] Sending Set Feature Command (200Hz Game Rotation Vector)...
[CMD] Sending Set Feature Command (200Hz)...
  [TX OK] Sent 21 bytes on CH2 (sensor replied 0 bytes on CH0)
[OK] Set Feature Command Transmitted!
[READY] Polling for sensor data...
Quat: [+0.9999, +0.0046, -0.0106, +0.0000] | YPR: [  -0.0,   -1.2,   +0.5] deg
Quat: [+0.9999, +0.0047, -0.0106, +0.0000] | YPR: [  -0.0,   -1.2,   +0.5] deg
Quat: [+0.9999, +0.0047, -0.0107, +0.0000] | YPR: [  -0.0,   -1.2,   +0.5] deg
```

---

## License

Hardware driver components and CMSIS files are copyright STMicroelectronics and ARM Limited. SHTP protocol definitions based on Hillcrest Laboratories / CEVA Inc. specifications.
