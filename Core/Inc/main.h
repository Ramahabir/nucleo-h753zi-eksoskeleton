/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define FSR_ADC0_Pin GPIO_PIN_3
#define FSR_ADC0_GPIO_Port GPIOA
#define WAKE_BNO3_Pin GPIO_PIN_8
#define WAKE_BNO3_GPIO_Port GPIOE
#define INT_BNO1_Pin GPIO_PIN_9
#define INT_BNO1_GPIO_Port GPIOE
#define WAKE_BNO1_Pin GPIO_PIN_11
#define WAKE_BNO1_GPIO_Port GPIOE
#define INT_BNO2_Pin GPIO_PIN_13
#define INT_BNO2_GPIO_Port GPIOE
#define CS_BNO2_Pin GPIO_PIN_14
#define CS_BNO2_GPIO_Port GPIOE
#define BNO_RST_Pin GPIO_PIN_15
#define BNO_RST_GPIO_Port GPIOD
#define CS_BNO1_Pin GPIO_PIN_12
#define CS_BNO1_GPIO_Port GPIOG
#define WAKE_BNO2_Pin GPIO_PIN_14
#define WAKE_BNO2_GPIO_Port GPIOG
#define CS_BNO3_Pin GPIO_PIN_6
#define CS_BNO3_GPIO_Port GPIOB
#define INT_BNO3_Pin GPIO_PIN_7
#define INT_BNO3_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
