# NUCLEO-H753ZI Exoskeleton Firmware Base

PlatformIO & STM32CubeMX starter project for the **STMicroelectronics NUCLEO-H753ZI** development board (Arm Cortex-M7 @ 400 MHz).

This repository provides a verified, production-ready template that integrates **STM32CubeMX code generation** with **PlatformIO**, featuring a native build for the STM32H753 chip and a fix for onboard ST-LINK/V3 flashing with modern OpenOCD versions.

---

## Hardware Specifications

| Parameter | Details |
| :--- | :--- |
| **Board** | STMicroelectronics NUCLEO-H753ZI (Nucleo-144) |
| **MCU** | STM32H753ZIT6 |
| **Core** | Arm 32-bit Cortex-M7 with double-precision FPU @ 400 MHz |
| **Flash Memory** | 2 MB (Dual Bank) |
| **SRAM** | 512 KB (up to 1 MB total bus matrix RAM) |
| **Hardware Crypto** | AES-128/192/256, DES/TDES, HASH (MD5, SHA-1, SHA-224, SHA-256), HMAC |
| **Onboard Debugger** | ST-LINK/V3 |
| **User LEDs** | **LD1 (Green):** `PB0`<br>**LD2 (Yellow):** `PE1`<br>**LD3 (Red):** `PB14` |
| **User Button** | `PC13` (Blue button) |

---

## Key Highlights

- **Native STM32H753ZI Configuration:**  
  Unlike generic fallbacks to the H743, this project compiles against `startup_stm32h753xx.s` and defines `-DSTM32H753xx`, ensuring complete interrupt vector tables for the hardware cryptographic accelerators (`CRYP_IRQHandler`, `HASH_RNG_IRQHandler`) and full CMSIS peripheral registers.

- **ST-LINK/V3 OpenOCD Fix:**  
  Includes a local board definition ([boards/nucleo_h753zi.json](boards/nucleo_h753zi.json)) that maps OpenOCD to use native SWD transport (`board/st_nucleo_h743zi.cfg`), preventing the common `Debug adapter doesn't support 'hla_swd' transport` error seen in OpenOCD 0.12+.

- **Board Support Package (BSP) Ready:**  
  Integrates ST's official Nucleo-144 BSP drivers (`Drivers/BSP/STM32H7xx_Nucleo/`), enabling clean, board-level API calls:
  ```c
  BSP_LED_Toggle(LED_GREEN);
  BSP_LED_Toggle(LED_YELLOW);
  BSP_LED_Toggle(LED_RED);
  ```

---

## Project Structure

```text
├── .gitignore
├── boards/
│   └── nucleo_h753zi.json       # Custom board config with ST-LINK/V3 OpenOCD fix
├── Core/
│   ├── Inc/                     # Application header files & BSP configuration
│   └── Src/                     # Application source files (main.c, gpio.c, etc.)
├── Drivers/
│   ├── BSP/STM32H7xx_Nucleo/    # Nucleo board support package (LEDs, button, COM)
│   ├── CMSIS/                   # ARM CMSIS definitions
│   └── STM32H7xx_HAL_Driver/    # STM32H7 HAL drivers
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

Open the repository in VS Code, then either:
- Click the **Build** button (`✓`) on the bottom status bar, or
- Run the following command in the terminal:
  ```bash
  pio run
  ```

### Flash to the Board

1. Connect the **NUCLEO-H753ZI** to your computer via the micro-USB port marked **CN1 (ST-LINK)**.
2. Click the **Upload** button (`→`) on the bottom status bar, or run:
  ```bash
  pio run -t upload
  ```

Once flashing finishes, all 3 user LEDs (**Green**, **Yellow**, and **Red**) will blink at 500 ms intervals.

---

## License

Hardware driver components and CMSIS files are copyright STMicroelectronics and ARM Limited. See individual file headers and `LICENSE` files within the `Drivers/` directory for details.

