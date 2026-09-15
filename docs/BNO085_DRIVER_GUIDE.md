# BNO085 / BNO080 STM32 SPI Driver Integration Guide

A portable, bare-metal / HAL-based C driver for the **CEVA / Hillcrest Laboratories BNO085 & BNO080 9-DOF IMU Sensor Hub** over high-speed SPI using the **Sensor Hub Transport Protocol (SHTP)**.

This driver is completely independent of pin labels, boards, or specific STM32 families. It works out-of-the-box on **STM32H7, STM32F4, STM32F7, STM32G4, STM32L4**, or any other STM32 microcontroller with a hardware SPI peripheral.

---

## 1. Hardware & Wiring Requirements

The BNO085 requires **8 physical connections** to your STM32 microcontroller:

```text
               +-------------------+
               |  Adafruit BNO085  |
               |  Breakout Board   |
               +-------------------+
                 | | | | | | | | |
                 | | | | | | | | +-- [P1]   --> Tied to 3.3V rail (SPI Mode Select)
                 | | | | | | | +---- [P0]   --> Any MCU GPIO Output (WAKE / PS0)
                 | | | | | | +------ [INT]  --> Any MCU GPIO Input Pull-up (HINTN)
                 | | | | | +-------- [RST]  --> Any MCU GPIO Output (Hardware Reset)
                 | | | | +---------- [CS]   --> Any MCU GPIO Output (Chip Select)
                 | | | +------------ [DI]   --> MCU SPI MOSI (Data In to sensor)
                 | | +-------------- [SDA]  --> MCU SPI MISO (Data Out from sensor)
                 | +---------------- [SCL]  --> MCU SPI SCK  (SPI Clock)
                 +------------------ [VIN]  --> 3.3V Power & GND to Ground
```

### Pin Roles Explained

| BNO085 Pin | Connection Type | Description |
| :--- | :--- | :--- |
| **`VIN`** | 3.3V Supply | 3.3V regulated power rail |
| **`GND`** | Ground | Common system ground |
| **`SCL`** | MCU SPI SCK | SPI serial clock line |
| **`SDA`** | MCU SPI MISO | SPI Master In / Slave Out (Sensor data to MCU) |
| **`DI`** | MCU SPI MOSI | SPI Master Out / Slave In (Commands to sensor) |
| **`CS`** | GPIO Output | Active-LOW Software Chip Select |
| **`RST`** | GPIO Output | Active-LOW Hardware Reset |
| **`INT`** | GPIO Input (**Pull-up**) | Active-LOW Host Interrupt signal (`HINTN`). Sensor pulls this LOW when it has a packet or acknowledges a wake event. |
| **`P0`** | GPIO Output | **Dual-purpose WAKE / PS0 pin**: Must be HIGH at power-on to boot into SPI mode; pulsed LOW during runtime to wake the sensor from sleep. |
| **`P1`** | Tied to **3.3V** | Fixed protocol select (`HIGH` = SPI). |

---

## 2. STM32CubeMX Peripheral Configuration

Open your `.ioc` file in STM32CubeMX and configure the peripherals:

### A. SPI Configuration (SPI1, SPI2, etc.)
* **Mode**: `Full-Duplex Master`
* **Hardware NSS Signal**: `Disable` *(We manage CS manually via software for precise SHTP framing)*
* **Data Size**: `8 Bits`
* **First Bit**: `MSB First`
* **Clock Polarity (CPOL)**: **`High` (1)**
* **Clock Phase (CPHA)**: **`2 Edge` (1)**
* **Prescaler**: Adjust so the resulting baud rate is **$\le 3.0\text{ MHz}$** *(BNO085 max SPI clock is 3.0 MHz; 1.0 to 2.5 MHz is optimal)*.

> [!IMPORTANT]
> **SPI Mode 3 is Mandatory:** The BNO085 requires SPI Mode 3 (`CPOL = High`, `CPHA = 2 Edge`). Mode 0 will result in corrupted packet headers.

### B. GPIO Configuration (4 Control Pins)
Configure any 4 free pins on your microcontroller. **No specific User Labels are required!**

1. **Chip Select (CS)**:
   - Mode: `Output Push-Pull`
   - Pull: `No pull`
   - Initial Output Level: **`High`**
   - Speed: `Medium` or `High`
2. **Reset (RST)**:
   - Mode: `Output Push-Pull`
   - Pull: `No pull`
   - Initial Output Level: **`High`**
3. **Wake (WAKE / P0)**:
   - Mode: `Output Push-Pull`
   - Pull: `No pull`
   - Initial Output Level: **`High`** *(Ensures sensor boots into SPI mode)*
4. **Interrupt (INT / HINTN)**:
   - Mode: `Input mode`
   - Pull: **`Pull-up`** *(Mandatory: HINTN is an open-drain/active-low signal)*

---

## 3. Adding Driver Files to Your Project

1. Copy **`bno085.h`** into your project's header directory (e.g., `Core/Inc/`).
2. Copy **`bno085.c`** into your project's source directory (e.g., `Core/Src/`).

### Enable Floating-Point `printf` (If printing orientation angles)
If you are using GCC / PlatformIO / STM32CubeIDE with `newlib-nano`, standard `printf("%f")` is disabled by default to save flash. Enable it by adding the linker flag:

* **PlatformIO (`platformio.ini`)**:
  ```ini
  build_flags = -Wl,-u,_printf_float
  ```
* **STM32CubeIDE**:
  Project Properties $\rightarrow$ C/C++ Build $\rightarrow$ Settings $\rightarrow$ Tool Settings $\rightarrow$ MCU GCC Linker $\rightarrow$ Miscellaneous $\rightarrow$ Check *"Use float with printf from newlib-nano"*.

---

## 4. Code Integration Example

Here is a minimal, complete `main.c` example:

```c
#include "main.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

// 1. Include the BNO085 driver
#include "bno085.h"
#include <stdio.h>

// Handle for our sensor instance
BNO085_t imu;

// Redirect printf to your preferred UART peripheral (e.g. huart1, huart2, huart6)
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart6, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_USART6_UART_Init();

    printf("\r\n--- BNO085 Initialization ---\r\n");

    // 2. Initialize device handle with your chosen GPIO ports & pins
    // (Replace GPIO ports and pin numbers with your actual hardware pins)
    BNO085_Init(&imu, &hspi1,
                GPIOD, GPIO_PIN_14,  // CS Pin
                GPIOD, GPIO_PIN_15,  // RST Pin
                GPIOF, GPIO_PIN_3,   // INT / HINTN Pin
                GPIOG, GPIO_PIN_12); // WAKE / P0 Pin

    // 3. Hardware reset the sensor
    printf("[1/3] Hardware Resetting...\r\n");
    BNO085_HardwareReset(&imu);
    HAL_Delay(300);

    // 4. Drain initial boot packets until SH-2 firmware is initialized (0xF1)
    printf("[2/3] Waiting for SH-2 Initialization...\r\n");
    uint8_t boot_buf[300];
    uint32_t drain_start = HAL_GetTick();
    bool sh2_initialized = false;

    while ((HAL_GetTick() - drain_start) < 2000) {
        if (HAL_GPIO_ReadPin(imu.hintn_port, imu.hintn_pin) == GPIO_PIN_RESET) {
            uint16_t plen = BNO085_ReadPacket(&imu, boot_buf, sizeof(boot_buf));
            if (plen > 0) {
                if (boot_buf[2] == 2 && plen >= 5 && boot_buf[4] == 0xF1) {
                    printf("  -> SH-2 Initialized (0xF1 received)!\r\n");
                    sh2_initialized = true;
                }
                drain_start = HAL_GetTick();
            }
        } else if (sh2_initialized && ((HAL_GetTick() - drain_start) > 100)) {
            break; // All boot packets drained and bus is idle
        }
    }

    // 5. Send Set Feature Command to enable 200 Hz Game Rotation Vector (Report 0x08)
    printf("[3/3] Enabling 200Hz Game Rotation Vector...\r\n");
    BNO085_EnableGameRotationVector(&imu);

    printf("Initialization Complete! Streaming data...\r\n");

    // 6. Main measurement loop
    while (1) {
        // Reads incoming SHTP packets when INT is asserted & decodes quaternions
        BNO085_Stage2_PollData(&imu);
    }
}
```

---

## 5. Driver Architecture & API Reference

### Data Structures

```c
typedef struct {
    SPI_HandleTypeDef   *hspi;
    GPIO_TypeDef        *cs_port;
    uint16_t            cs_pin;
    GPIO_TypeDef        *rst_port;
    uint16_t            rst_pin;
    GPIO_TypeDef        *hintn_port;
    uint16_t            hintn_pin;
    GPIO_TypeDef        *wake_port;
    uint16_t            wake_pin;
} BNO085_t;
```

### Core API Functions

#### `void BNO085_Init(dev, hspi, cs_port, cs_pin, rst_port, rst_pin, hintn_port, hintn_pin, wake_port, wake_pin)`
Associates hardware pins and the SPI peripheral with the `BNO085_t` device handle. Sets initial states (`CS` = High, `RST` = High, `WAKE` = High).

#### `void BNO085_HardwareReset(BNO085_t *dev)`
Drives the hardware `RST` line LOW for 15 ms, then restores it HIGH to reset the sensor coprocessor.

#### `uint16_t BNO085_ReadPacket(BNO085_t *dev, uint8_t *buffer, uint16_t buffer_size)`
Checks if `HINTN` is asserted (LOW). If asserted:
1. Asserts `CS` LOW.
2. Reads the 4-byte SHTP header to obtain packet length and channel.
3. Reads the payload bytes.
4. Deasserts `CS` HIGH.
Returns the total bytes read (header + payload), or 0 if no data is pending.

#### `bool BNO085_SendPacket(BNO085_t *dev, uint8_t *tx_buf, uint16_t len)`
Executes the official CEVA / Hillcrest SHTP host-write handshake:
1. Asserts `WAKE` LOW to request bus access.
2. Waits for sensor to assert `HINTN` LOW acknowledging wakeup.
3. Asserts `CS` LOW.
4. Deasserts `WAKE` HIGH.
5. Exchanges SHTP header & payload via full-duplex `HAL_SPI_TransmitReceive`.
6. Deasserts `CS` HIGH.
7. Waits for `HINTN` to return HIGH (bus idle).

#### `void BNO085_EnableGameRotationVector(BNO085_t *dev)`
Formats and sends a 21-byte SHTP `Set Feature Command` (`0xFD`) requesting **Game Rotation Vector** (Report `0x08`) at a 5,000 µs interval (200 Hz).

#### `void BNO085_Stage2_PollData(BNO085_t *dev)`
Polls for incoming sensor reports on Channel 3. Automatically strips the `0xFB` Base Timestamp record, decodes the Q14 fixed-point quaternion ($q_r, q_i, q_j, q_k$), computes Euler angles (Yaw, Pitch, Roll in degrees), and outputs to console.

---

## 6. Quaternion & Angle Math

The BNO085 reports rotation vector values as **16-bit signed integers in Q14 fixed-point format**:

$$\text{Scale Factor} = \frac{1}{2^{14}} = \frac{1}{16384} \approx 0.000061035$$

$$q_r = \frac{\text{raw\_r}}{16384.0}, \quad q_i = \frac{\text{raw\_i}}{16384.0}, \quad q_j = \frac{\text{raw\_j}}{16384.0}, \quad q_k = \frac{\text{raw\_k}}{16384.0}$$

### Converting to Euler Angles (Tait-Bryan Z-Y-X):

$$\text{Yaw} = \operatorname{atan2}\left(2(q_r q_k + q_i q_j),\, 1 - 2(q_j^2 + q_k^2)\right) \times \frac{180}{\pi}$$

$$\text{Pitch} = \operatorname{asin}\left(2(q_r q_j - q_k q_i)\right) \times \frac{180}{\pi}$$

$$\text{Roll} = \operatorname{atan2}\left(2(q_r q_i + q_j q_k),\, 1 - 2(q_i^2 + q_j^2)\right) \times \frac{180}{\pi}$$

---

## 7. Troubleshooting & FAQs

### 1. `Wake-up timeout / HINTN did not go LOW`
* **Check the WAKE (P0) wire:** The MCU must be able to drive `P0` LOW. Ensure `P0` is NOT tied to 3.3V or GND.
* **Verify `P0` initial state:** `P0` must start HIGH at power-on so the sensor knows to start in SPI mode instead of I2C.

### 2. Header invalid / All zeroes returned
* **Check SPI Mode:** Verify your SPI peripheral is in **Mode 3** (`CPOL = High`, `CPHA = 2 Edge`).
* **Check SPI Prescaler:** If the SPI clock is $> 3\text{ MHz}$, communication will fail. Lower your SPI clock frequency.
* **Verify MOSI pin:** Ensure your MCU pin matches the physical header pin (e.g., on NUCLEO-H753ZI, Arduino D11 is `PB5`, not `PA7`).

### 3. Missing or blank `printf` output
* In GCC ARM embedded projects, floating-point `printf` is disabled by default. Ensure `-Wl,-u,_printf_float` is added to your compiler linker flags.

