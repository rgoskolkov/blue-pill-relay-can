/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define RELAY_VCC_Pin GPIO_PIN_0
#define RELAY_VCC_GPIO_Port GPIOA
#define RELAY8_Pin GPIO_PIN_1
#define RELAY8_GPIO_Port GPIOA
#define RELAY7_Pin GPIO_PIN_2
#define RELAY7_GPIO_Port GPIOA
#define RELAY6_Pin GPIO_PIN_3
#define RELAY6_GPIO_Port GPIOA
#define RELAY5_Pin GPIO_PIN_4
#define RELAY5_GPIO_Port GPIOA
#define RELAY4_Pin GPIO_PIN_5
#define RELAY4_GPIO_Port GPIOA
#define RELAY3_Pin GPIO_PIN_6
#define RELAY3_GPIO_Port GPIOA
#define RELAY2_Pin GPIO_PIN_7
#define RELAY2_GPIO_Port GPIOA
#define RELAY1_Pin GPIO_PIN_0
#define RELAY1_GPIO_Port GPIOB
#define RELAY_GND_Pin GPIO_PIN_1
#define RELAY_GND_GPIO_Port GPIOB
#define USART_TX_B10_Pin GPIO_PIN_10
#define USART_TX_B10_GPIO_Port GPIOB
#define USART_RX_B11_Pin GPIO_PIN_11
#define USART_RX_B11_GPIO_Port GPIOB
#define USART_TX_PA9_Pin GPIO_PIN_9
#define USART_TX_PA9_GPIO_Port GPIOA
#define USART_RX_PA10_Pin GPIO_PIN_10
#define USART_RX_PA10_GPIO_Port GPIOA
#define SWITCH1_Pin GPIO_PIN_11
#define SWITCH1_GPIO_Port GPIOA
#define SWITCH1_EXTI_IRQn EXTI15_10_IRQn
#define SWITCH2_Pin GPIO_PIN_12
#define SWITCH2_GPIO_Port GPIOA
#define SWITCH2_EXTI_IRQn EXTI15_10_IRQn
#define SWITCH3_Pin GPIO_PIN_15
#define SWITCH3_GPIO_Port GPIOA
#define SWITCH3_EXTI_IRQn EXTI15_10_IRQn
#define SWITCH4_Pin GPIO_PIN_3
#define SWITCH4_GPIO_Port GPIOB
#define SWITCH4_EXTI_IRQn EXTI3_IRQn
#define SWITCH5_Pin GPIO_PIN_4
#define SWITCH5_GPIO_Port GPIOB
#define SWITCH5_EXTI_IRQn EXTI4_IRQn
#define SWITCH6_Pin GPIO_PIN_5
#define SWITCH6_GPIO_Port GPIOB
#define SWITCH6_EXTI_IRQn EXTI9_5_IRQn
#define SWITCH7_Pin GPIO_PIN_6
#define SWITCH7_GPIO_Port GPIOB
#define SWITCH7_EXTI_IRQn EXTI9_5_IRQn
#define SWITCH8_Pin GPIO_PIN_7
#define SWITCH8_GPIO_Port GPIOB
#define SWITCH8_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
