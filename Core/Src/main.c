/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bno085.h"
#include "fsr.h"
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
BNO085_t bno1;
BNO085_t bno2;
BNO085_t bno3;
FSR_HandleTypeDef fsr1;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern UART_HandleTypeDef huart3;
int _write(int file, char *ptr, int len) {
    /* printf → USART3 (TX=PD8) → USB-TTL → COM17 */
    HAL_UART_Transmit(&huart3, (uint8_t*)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* Helper function to safely reset and drain multiple BNO085 sensors on shared RST pin */
void BNO085_MultiReset(BNO085_t *imus[], uint8_t count, uint32_t timeout_ms) {
    printf("[1/3] Pulsing shared hardware reset line (PD15)...\r\n");
    HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(15);
    HAL_GPIO_WritePin(BNO_RST_GPIO_Port, BNO_RST_Pin, GPIO_PIN_SET);

    for (uint8_t i = 0; i < count; i++) {
        printf("  -> Syncing & draining boot packets for IMU #%u...\r\n", i + 1);
        uint32_t start = HAL_GetTick();
        while (HAL_GPIO_ReadPin(imus[i]->hintn_port, imus[i]->hintn_pin) == GPIO_PIN_SET) {
            if ((HAL_GetTick() - start) > timeout_ms) {
                printf("  [WARN] IMU #%u did not respond (INT pin stayed HIGH). Is it wired?\r\n", i + 1);
                break;
            }
        }

        uint8_t boot_buf[300];
        uint32_t drain_start = HAL_GetTick();
        while ((HAL_GetTick() - drain_start) < 1500) {
            if (HAL_GPIO_ReadPin(imus[i]->hintn_port, imus[i]->hintn_pin) == GPIO_PIN_RESET) {
                BNO085_ReadPacket(imus[i], boot_buf, sizeof(boot_buf));
                drain_start = HAL_GetTick();
            } else if ((HAL_GetTick() - drain_start) > 100) {
                break;
            }
        }
    }
    HAL_Delay(50);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_ADC1_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  printf("\r\n========================================\r\n");
  printf("     BNO085 MULTI-IMU INITIALIZATION    \r\n");
  printf("========================================\r\n");

  /* 1. Link BNO085 handles to SPI1 and their respective CubeMX pins */
  BNO085_Init(&bno1, &hspi1,
              CS_BNO1_GPIO_Port, CS_BNO1_Pin,
              BNO_RST_GPIO_Port, BNO_RST_Pin,
              INT_BNO1_GPIO_Port, INT_BNO1_Pin,
              WAKE_BNO1_GPIO_Port, WAKE_BNO1_Pin);

  BNO085_Init(&bno2, &hspi1,
              CS_BNO2_GPIO_Port, CS_BNO2_Pin,
              BNO_RST_GPIO_Port, BNO_RST_Pin,
              INT_BNO2_GPIO_Port, INT_BNO2_Pin,
              WAKE_BNO2_GPIO_Port, WAKE_BNO2_Pin);

  BNO085_Init(&bno3, &hspi1,
              CS_BNO3_GPIO_Port, CS_BNO3_Pin,
              BNO_RST_GPIO_Port, BNO_RST_Pin,
              INT_BNO3_GPIO_Port, INT_BNO3_Pin,
              WAKE_BNO3_GPIO_Port, WAKE_BNO3_Pin);

  /* 2. Synchronous hardware reset: pulse shared RST (PD15) and drain each connected sensor */
  BNO085_t *active_imus[2] = { &bno1, &bno2 };
  BNO085_MultiReset(active_imus, 2, 500);

  /* 3. Enable 100 Hz Game Rotation Vector on IMU1 */
  printf("[2/3] Enabling Game Rotation Vector on IMU1...\r\n");
  if (BNO085_EnableGameRotationVector(&bno1, 10) == BNO085_OK) {
      printf("[OK] IMU1: 100 Hz Game Rotation Vector streaming!\r\n");
  } else {
      printf("[FAIL] IMU1: Failed to enable report.\r\n");
  }

  /* 4. Enable 100 Hz Game Rotation Vector on IMU2 (if connected) */
  printf("[3/3] Enabling Game Rotation Vector on IMU2...\r\n");
  if (BNO085_EnableGameRotationVector(&bno2, 10) == BNO085_OK) {
      printf("[OK] IMU2: 100 Hz Game Rotation Vector streaming!\r\n");
  } else {
      printf("[INFO] IMU2: Sensor not ready or not yet wired.\r\n");
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* 1. Non-blocking IMU polling for each sensor */
    BNO085_Update(&bno1);
    BNO085_Update(&bno2);

    /* 2. Telemetry output at 20 Hz (every 50 ms) without mutual blocking */
    static uint32_t last_telemetry = 0;
    if ((HAL_GetTick() - last_telemetry) >= 50) {
        last_telemetry = HAL_GetTick();

        /* Print real-time Euler angles for each sensor */
        printf("IMU1: [Y:%+6.1f P:%+6.1f R:%+6.1f] (Acc:%u, %s) | IMU2: [Y:%+6.1f P:%+6.1f R:%+6.1f] (Acc:%u, %s)\r\n",
               bno1.data.yaw, bno1.data.pitch, bno1.data.roll, bno1.data.accuracy,
               bno1.data.has_new_data ? "ACTIVE" : "IDLE",
               bno2.data.yaw, bno2.data.pitch, bno2.data.roll, bno2.data.accuracy,
               bno2.data.has_new_data ? "ACTIVE" : "IDLE");

        bno1.data.has_new_data = false;
        bno2.data.has_new_data = false;
    }

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOMEDIUM;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
    printf("Hello STM32 - serial via BSP\\r\\n");
    HAL_Delay(1000);
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
