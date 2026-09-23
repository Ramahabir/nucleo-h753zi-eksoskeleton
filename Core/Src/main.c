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
  printf("\r\n=== BNO085 IMU Initializing ===\r\n");
  BNO085_Init(&bno1, &hspi1,
              BNO_CS0_GPIO_Port, BNO_CS0_Pin,
              BNO_RST_GPIO_Port, BNO_RST_Pin,
              BNO_HINTN0_GPIO_Port, BNO_HINTN0_Pin,
              BNO_WAKE_GPIO_Port, BNO_WAKE_Pin);

  if (BNO085_HardwareReset(&bno1, 1000) != BNO085_OK) {
      printf("[ERROR] BNO085 reset timeout! Check SPI/Power/HINTN connections.\r\n");
  } else {
      printf("[OK] BNO085 online!\r\n");
  }

  /* Request Game Rotation Vector at 100 Hz (10 ms interval) */
  if (BNO085_EnableGameRotationVector(&bno1, 10) == BNO085_OK) {
      printf("[OK] Game Rotation Vector configured at 100 Hz.\r\n");
  } else {
      printf("[FAIL] Failed to enable rotation vector report.\r\n");
  }

  /* Initialize FSR on ADC1 (contact threshold: 15000, release: 10000, alpha: 0.3) */
  printf("\r\n=== FSR Initializing ===\r\n");
  FSR_Init(&fsr1, &hadc1, 15000, 10000, 0.3f);
  printf("[OK] FSR online (Thresholds: contact=15000, release=10000)\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* 1. Non-blocking IMU update (takes 0 CPU time if no packet is pending) */
    BNO085_Update(&bno1);

    /* 2. Periodic FSR update sampled at 100 Hz (every 10 ms) */
    static uint32_t last_fsr_sample = 0;
    if ((HAL_GetTick() - last_fsr_sample) >= 10) {
        last_fsr_sample = HAL_GetTick();
        FSR_Update(&fsr1);
    }

    /* 3. Application telemetry rate-limited to 20 Hz (every 50 ms) */
    static uint32_t last_telemetry = 0;
    if ((HAL_GetTick() - last_telemetry) >= 50) {
        last_telemetry = HAL_GetTick();

        if (bno1.data.has_new_data) {
            bno1.data.has_new_data = false;
            printf("IMU YPR: [%+6.1f, %+6.1f, %+6.1f] deg | FSR: raw=%5u filt=%5u (%4.2fV) [%s]\r\n",
                   bno1.data.yaw, bno1.data.pitch, bno1.data.roll,
                   fsr1.raw_adc, (uint16_t)fsr1.filtered_adc, fsr1.voltage,
                   FSR_IsContact(&fsr1) ? "STANCE" : "SWING");
        } else {
            printf("IMU: waiting... | FSR: raw=%5u filt=%5u (%4.2fV) [%s]\r\n",
                   fsr1.raw_adc, (uint16_t)fsr1.filtered_adc, fsr1.voltage,
                   FSR_IsContact(&fsr1) ? "STANCE" : "SWING");
        }
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
