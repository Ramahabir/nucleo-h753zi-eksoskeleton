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
// #include "bno085.h"  /* TEMP: commented out for FSR standalone test */
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
// BNO085_t imu0;  /* TEMP: commented out for FSR standalone test */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint16_t FSR_ReadRaw(void){
  uint16_t raw_val = 0;

  // 1. Start ADC conversion
  HAL_ADC_Start(&hadc1);

  // 2. Wait for conversion to complete
  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK){
    // 3. Read the 16 bit
    raw_val = (uint16_t)HAL_ADC_GetValue(&hadc1);
  }

  // 4. Stop ADC
  HAL_ADC_Stop(&hadc1);

  return raw_val;


}
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
  MX_USART6_UART_Init();
  MX_USART3_UART_Init();  /* USB-TTL on COM17: TX=PD8, RX=PD9 */
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  // Calibrate ADC1
  if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET_LINEARITY, ADC_SINGLE_ENDED) != HAL_OK){
    printf("[ADC] Calibration failed!\r\n");
  } else {
    printf("[ADC] Calibration OK");
  };

  printf("\r\n========================================\r\n");
  printf("   FSR Standalone Test (PA3 / ADC1_INP15)\r\n");
  printf("========================================\r\n");

  /* TEMP: BNO085 init commented out for FSR standalone test */
  // printf("   BNO085 Stage 2: 200Hz Quaternion Stream\r\n");
  // BNO085_Init(&imu0, &hspi1,
  //             BNO_CS0_GPIO_Port,   BNO_CS0_Pin,
  //             BNO_RST_GPIO_Port,   BNO_RST_Pin,
  //             BNO_HINTN0_GPIO_Port, BNO_HINTN0_Pin,
  //             BNO_WAKE_GPIO_Port,  BNO_WAKE_Pin);
  // printf("[1/3] Hardware Resetting BNO085...\r\n");
  // BNO085_HardwareReset(&imu0);
  // HAL_Delay(300);
  // printf("[2/3] Draining boot packets until SH-2 is initialized...\r\n");
  // uint8_t boot_buf[300];
  // uint32_t drain_start = HAL_GetTick();
  // bool sh2_initialized = false;
  // while ((HAL_GetTick() - drain_start) < 2000) {
  //     if (HAL_GPIO_ReadPin(imu0.hintn_port, imu0.hintn_pin) == GPIO_PIN_RESET) {
  //         uint16_t plen = BNO085_ReadPacket(&imu0, boot_buf, sizeof(boot_buf));
  //         if (plen > 0) {
  //             printf("  -> Boot packet: %u bytes on CH%u (ID: 0x%02X)\r\n",
  //                    plen, boot_buf[2], (plen > 4) ? boot_buf[4] : 0);
  //             if (boot_buf[2] == 2 && plen >= 5 && boot_buf[4] == 0xF1) {
  //                 printf("  [OK] SH-2 System Initialized (0xF1 received)!\r\n");
  //                 sh2_initialized = true;
  //             }
  //             drain_start = HAL_GetTick();
  //         }
  //     } else {
  //         if (sh2_initialized && ((HAL_GetTick() - drain_start) > 100)) {
  //             break;
  //         }
  //     }
  // }
  // printf("[3/3] Sending Set Feature Command (200Hz Game Rotation Vector)...\r\n");
  // BNO085_EnableGameRotationVector(&imu0);
  // printf("[READY] Polling for sensor data...\r\n");

  printf("[READY] Starting FSR polling...\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* TEMP: BNO085 poll commented out for FSR standalone test */
    // BNO085_Stage2_PollData(&imu0);

    /* --- FSR Test --- */
    static uint32_t last_fsr_print = 0;

    if ((HAL_GetTick() - last_fsr_print) >= 100) // 10 Hz
    {
      last_fsr_print = HAL_GetTick();

      uint16_t raw = FSR_ReadRaw();
      float voltage = (raw / 65535.0f) * 3.3f;

      if (raw > 500) // Contact threshold (~0.025 V)
      {
        float r_fsr = 10000.0f * ((65535.0f - (float)raw) / (float)raw);
        float conductance_uS = (1.0f / r_fsr) * 1000000.0f;

        printf("[FSR] Raw: %5u | Volt: %.2fV | R: %.1fkOhm | Cond: %.1fuS (PRESSED)\r\n",
               raw, voltage, r_fsr / 1000.0f, conductance_uS);
      }
      else
      {
        printf("[FSR] Raw: %5u | Volt: %.2fV | (NO FORCE)\r\n", raw, voltage);
      }
    }
    /* USER CODE END WHILE */

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
