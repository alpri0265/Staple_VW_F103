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
#include "stm32f1xx_hal.h"

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
#define STOP_Pin GPIO_PIN_0
#define STOP_GPIO_Port GPIOA
#define STOP_EXTI_IRQn EXTI0_IRQn
#define CLK_Pin GPIO_PIN_1
#define CLK_GPIO_Port GPIOA
#define CLK_EXTI_IRQn EXTI1_IRQn
#define DT_Pin GPIO_PIN_2
#define DT_GPIO_Port GPIOA
#define SW_Pin GPIO_PIN_3
#define SW_GPIO_Port GPIOA
#define DAT_Pin GPIO_PIN_0
#define DAT_GPIO_Port GPIOB
#define HX_CLK_Pin GPIO_PIN_1
#define HX_CLK_GPIO_Port GPIOB
#define TOP_Pin GPIO_PIN_10
#define TOP_GPIO_Port GPIOB
#define BOT_Pin GPIO_PIN_11
#define BOT_GPIO_Port GPIOB
#define UP_Pin GPIO_PIN_12
#define UP_GPIO_Port GPIOB
#define DOWN_Pin GPIO_PIN_13
#define DOWN_GPIO_Port GPIOB
#define STEP_Pin GPIO_PIN_8
#define STEP_GPIO_Port GPIOA
#define DIR_Pin GPIO_PIN_9
#define DIR_GPIO_Port GPIOA
#define ENABLE_Pin GPIO_PIN_10
#define ENABLE_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

#define STOP_BTN_Pin STOP_Pin
#define STOP_BTN_GPIO_Port STOP_GPIO_Port

#define ENC_CLK_Pin CLK_Pin
#define ENC_CLK_GPIO_Port CLK_GPIO_Port

#define ENC_DT_Pin DT_Pin
#define ENC_DT_GPIO_Port DT_GPIO_Port

#define ENC_SW_Pin SW_Pin
#define ENC_SW_GPIO_Port SW_GPIO_Port

#define HX_DAT_Pin DAT_Pin
#define HX_DAT_GPIO_Port DAT_GPIO_Port

#define LIMIT_TOP_Pin TOP_Pin
#define LIMIT_TOP_GPIO_Port TOP_GPIO_Port

#define LIMIT_BOT_Pin BOT_Pin
#define LIMIT_BOT_GPIO_Port BOT_GPIO_Port

#define JOY_UP_Pin UP_Pin
#define JOY_UP_GPIO_Port UP_GPIO_Port

#define JOY_DOWN_Pin DOWN_Pin
#define JOY_DOWN_GPIO_Port DOWN_GPIO_Port



#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
