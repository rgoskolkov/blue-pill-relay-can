/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
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

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led_driver.h"
#include "system_monitor.h"
#include <stdio.h>
/* USER CODE END Includes */
/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void configASSERT_Handler(uint32_t assert_address) {
    //printf("configASSERT_Handler at PC: %08lX\n", assert_address);
    // Save the assert location to a backup register
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
    BKP->DR4 = 0xAAAA; // Magic number for configASSERT
    BKP->DR5 = (assert_address & 0xFFFF0000) >> 16;
    BKP->DR6 = (assert_address & 0x0000FFFF);
    HAL_PWR_DisableBkUpAccess();

    // Infinite loop with a different blink pattern
    while(1) {
        // Blink fast
        for (int i = 0; i < 5; i++) {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            for (volatile int d = 0; d < 100000; d++);
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            for (volatile int d = 0; d < 100000; d++);
        }
        for (volatile int d = 0; d < 1000000; d++);
    }
}

void HardFault_c_handler(uint32_t *stacked_frame) {
    uint32_t fault_address = stacked_frame[6];

    // Enable PWR and BKP clocks
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    // Allow access to Backup domain
    HAL_PWR_EnableBkUpAccess();

    // Use BKP registers to store crash information.
    // On STM32F1, these are separate from RTC.
    BKP->DR1 = 0xDEAD; // Magic number
    BKP->DR2 = (fault_address & 0xFFFF0000) >> 16; // High 16 bits
    BKP->DR3 = (fault_address & 0x0000FFFF);      // Low 16 bits

    // Disable access to Backup domain
    HAL_PWR_DisableBkUpAccess();

    // Infinite loop with SOS blink
    while (1) {
        // S
        for (int i = 0; i < 3; i++) {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            for (volatile int d = 0; d < 100000; d++);
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            for (volatile int d = 0; d < 100000; d++);
        }
        for (volatile int d = 0; d < 300000; d++);
        // O
        for (int i = 0; i < 3; i++) {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            for (volatile int d = 0; d < 400000; d++);
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            for (volatile int d = 0; d < 100000; d++);
        }
        for (volatile int d = 0; d < 300000; d++);
        // S
        for (int i = 0; i < 3; i++) {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            for (volatile int d = 0; d < 100000; d++);
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            for (volatile int d = 0; d < 100000; d++);
        }
        for (volatile int d = 0; d < 10000000; d++);
    }
}
/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;
extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef htim4;

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
    __asm volatile (
        " tst lr, #4                                                \n"
        " ite eq                                                    \n"
        " mrseq r0, msp                                             \n"
        " mrsne r0, psp                                             \n"
        " b HardFault_c_handler                                     \n"
    );
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_c_handler(uint32_t *stacked_frame); // Forward declaration

void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  __asm volatile (
      " tst lr, #4                                                \n"
      " ite eq                                                    \n"
      " mrseq r0, msp                                             \n"
      " mrsne r0, psp                                             \n"
      " b MemManage_c_handler                                     \n"
  );
}

void MemManage_c_handler(uint32_t *stacked_frame) {
    uint32_t fault_address = stacked_frame[6];

    // Enable PWR and BKP clocks
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_RCC_BKP_CLK_ENABLE();
    // Allow access to Backup domain
    HAL_PWR_EnableBkUpAccess();

    // Use BKP registers to store crash information.
    BKP->DR7 = 0xBEEF; // Magic number for MemManage
    BKP->DR8 = (fault_address & 0xFFFF0000) >> 16;
    BKP->DR9 = (fault_address & 0x0000FFFF);

    // Disable access to Backup domain
    HAL_PWR_DisableBkUpAccess();

    while(1) {
        // Fast blinks for MemManage fault
        for (int i = 0; i < 10; i++) {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            for (volatile int d = 0; d < 50000; d++);
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            for (volatile int d = 0; d < 50000; d++);
        }
        for (volatile int d = 0; d < 1000000; d++);
    }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI line0 interrupt.
  */
void EXTI0_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI0_IRQn 0 */

  /* USER CODE END EXTI0_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(USER_KEY_Pin);
  /* USER CODE BEGIN EXTI0_IRQn 1 */

  /* USER CODE END EXTI0_IRQn 1 */
}

/**
  * @brief This function handles EXTI line3 interrupt.
  */
void EXTI3_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI3_IRQn 0 */

  /* USER CODE END EXTI3_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(SWITCH4_Pin);
  /* USER CODE BEGIN EXTI3_IRQn 1 */

  /* USER CODE END EXTI3_IRQn 1 */
}

/**
  * @brief This function handles EXTI line4 interrupt.
  */
void EXTI4_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI4_IRQn 0 */

  /* USER CODE END EXTI4_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(SWITCH5_Pin);
  /* USER CODE BEGIN EXTI4_IRQn 1 */

  /* USER CODE END EXTI4_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel2 global interrupt.
  */
void DMA1_Channel2_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel2_IRQn 0 */

  /* USER CODE END DMA1_Channel2_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart3_tx);
  /* USER CODE BEGIN DMA1_Channel2_IRQn 1 */

  /* USER CODE END DMA1_Channel2_IRQn 1 */
}

/**
  * @brief This function handles DMA1 channel3 global interrupt.
  */
void DMA1_Channel3_IRQHandler(void)
{
  /* USER CODE BEGIN DMA1_Channel3_IRQn 0 */

  /* USER CODE END DMA1_Channel3_IRQn 0 */
  HAL_DMA_IRQHandler(&hdma_usart3_rx);
  /* USER CODE BEGIN DMA1_Channel3_IRQn 1 */

  /* USER CODE END DMA1_Channel3_IRQn 1 */
}

/**
  * @brief This function handles EXTI line[9:5] interrupts.
  */
void EXTI9_5_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI9_5_IRQn 0 */

  /* USER CODE END EXTI9_5_IRQn 0 */
  if(__HAL_GPIO_EXTI_GET_IT(SWITCH6_Pin) != RESET)
  {
    HAL_GPIO_EXTI_IRQHandler(SWITCH6_Pin);
  }
  if(__HAL_GPIO_EXTI_GET_IT(SWITCH7_Pin) != RESET)
  {
    HAL_GPIO_EXTI_IRQHandler(SWITCH7_Pin);
  }
  if(__HAL_GPIO_EXTI_GET_IT(SWITCH8_Pin) != RESET)
  {
    HAL_GPIO_EXTI_IRQHandler(SWITCH8_Pin);
  }
  /* USER CODE BEGIN EXTI9_5_IRQn 1 */

  /* USER CODE END EXTI9_5_IRQn 1 */
}

/**
  * @brief This function handles EXTI line[15:10] interrupts.
  */
void EXTI15_10_IRQHandler(void)
{
  /* USER CODE BEGIN EXTI15_10_IRQn 0 */

  /* USER CODE END EXTI15_10_IRQn 0 */
  if(__HAL_GPIO_EXTI_GET_IT(SWITCH1_Pin) != RESET)
  {
    HAL_GPIO_EXTI_IRQHandler(SWITCH1_Pin);
  }
  if(__HAL_GPIO_EXTI_GET_IT(SWITCH2_Pin) != RESET)
  {
    HAL_GPIO_EXTI_IRQHandler(SWITCH2_Pin);
  }
  if(__HAL_GPIO_EXTI_GET_IT(SWITCH3_Pin) != RESET)
  {
    HAL_GPIO_EXTI_IRQHandler(SWITCH3_Pin);
  }
  /* USER CODE BEGIN EXTI15_10_IRQn 1 */

  /* USER CODE END EXTI15_10_IRQn 1 */
}

/**
  * @brief This function handles TIM4 global interrupt.
  */
void TIM4_IRQHandler(void)
{
  /* USER CODE BEGIN TIM4_IRQn 0 */

  /* USER CODE END TIM4_IRQn 0 */
  HAL_TIM_IRQHandler(&htim4);
  /* USER CODE BEGIN TIM4_IRQn 1 */

  /* USER CODE END TIM4_IRQn 1 */
}

/**
  * @brief This function handles USART3 global interrupt.
  */
void USART3_IRQHandler(void)
{
  /* USER CODE BEGIN USART3_IRQn 0 */
	 /* USER CODE END USART3_IRQn 0 */
	HAL_UART_IRQHandler(&huart3);
	 /* USER CODE BEGIN USART3_IRQn 1 */

	 /* USER CODE END USART3_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
