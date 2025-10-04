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
/* USER CODE BEGIN Includes */
#include "Modbus.h"
#include "task.h"
#include <stdio.h>
#include "input_driver.h"
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

volatile uint32_t usart3_irq_count = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef htim4;
/* USER CODE BEGIN EV */
/* USER CODE BEGIN EV */
/* USER CODE END EV */

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
    uint32_t *stack_pointer;
    
    // Получаем указатель стека
    __asm volatile (
        "tst lr, #4\n"          // Проверяем какой стек использовался
        "ite eq\n"               // if-then-else
        "mrseq r0, msp\n"       // main stack pointer
        "mrsne r0, psp\n"       // process stack pointer  
        "mov %0, r0\n"          // сохраняем в stack_pointer
        : "=r" (stack_pointer)
        : 
        : "r0"
    );
    
    // Регистры из стека
    // uint32_t r0 = stack_pointer[0];
    // uint32_t r1 = stack_pointer[1];
    // uint32_t r2 = stack_pointer[2]; 
    // uint32_t r3 = stack_pointer[3];
    // uint32_t r12 = stack_pointer[4];
    uint32_t lr = stack_pointer[5];    // LR в момент сбоя
    uint32_t pc = stack_pointer[6];    // PC в момент сбоя! 
   // uint32_t psr = stack_pointer[7];
    
    // Регистры fault
    uint32_t cfsr = (*((volatile uint32_t *)(0xE000ED28))); // Configurable Fault Status
    uint32_t mmfar = (*((volatile uint32_t *)(0xE000ED34))); // MemManage Fault Address
//    uint32_t bfar = (*((volatile uint32_t *)(0xE000ED38))); // BusFault Address
    
    printf("\r\n=== HARD FAULT ===\r\n");
    
    printf("PC: 0x%08lX\r\n", pc);
    
    printf("LR: 0x%08lX\r\n", lr);
    
    printf("CFSR: 0x%08lX\r\n", cfsr);
    
    // Анализ причин
    if (cfsr & (1 << 0)) {
        printf("Reason: Instruction access violation\r\n");
    }
    if (cfsr & (1 << 1)) {
        printf("Reason: Data access violation\r\n");
    }
    if (cfsr & (1 << 16)) {
        printf("Reason: Invalid state (probably FPU)\r\n");
    }
    if (cfsr & (1 << 17)) {
        printf("Reason: Invalid PC load\r\n");
    }
    
    printf("Fault address: 0x%08lX\r\n", mmfar);
    
    // Вечный цикл с миганием
    while(1) {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
        HAL_Delay(200);
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
        HAL_Delay(200);
    }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
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
  usart3_irq_count++;
	 /* USER CODE END USART3_IRQn 0 */
	HAL_UART_IRQHandler(&huart3);
	 /* USER CODE BEGIN USART3_IRQn 1 */

	 /* USER CODE END USART3_IRQn 1 */
}

/* USER CODE BEGIN 1 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint8_t switch_index = 0;
    switch(GPIO_Pin)
    {
        case SWITCH1_Pin: switch_index = 0; break;
        case SWITCH2_Pin: switch_index = 1; break;
        case SWITCH3_Pin: switch_index = 2; break;
        case SWITCH4_Pin: switch_index = 3; break;
        case SWITCH5_Pin: switch_index = 4; break;
        case SWITCH6_Pin: switch_index = 5; break;
        case SWITCH7_Pin: switch_index = 6; break;
        case SWITCH8_Pin: switch_index = 7; break;
        default: return; // Not a switch pin
    }
    process_switch_event(switch_index);
}


/**
  * @brief  Rx Transfer completed callback. This is a strong implementation that overrides the weak one from HAL.
  * @param  huart: UART handle
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    extern modbusHandler_t *mHandlers[];
    extern uint8_t numberHandlers;
    //printf("rx callback\r\n");
    for (int i = 0; i < numberHandlers; i++) {
        if (mHandlers[i]->port == huart && mHandlers[i]->xTypeHW == USART_HW) {
            RingAdd(&mHandlers[i]->xBufferRX, mHandlers[i]->dataRX);
            HAL_UART_Receive_IT(mHandlers[i]->port, &mHandlers[i]->dataRX, 1);
            
            TimerHandle_t xTimer = mHandlers[i]->xTimerT35;
                BaseType_t timer_result = xTimerResetFromISR(xTimer, &xHigherPriorityTaskWoken);
                
                if (timer_result != pdPASS) {
                    // ДЕТАЛЬНАЯ ДИАГНОСТИКА ПРИ ОШИБКЕ
                    //char detail_msg[128];
                    // snprintf(detail_msg, sizeof(detail_msg),
                    //         "TIMER FAIL: TimerActive=%d\r\n", (int)xTimerIsTimerActive(xTimer));
                     printf("Timer reset FAIL");
                } else {
                   //printf("Timer reseted");
                }
            break;
        }
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    extern modbusHandler_t *mHandlers[];
    extern uint8_t numberHandlers;

	for (int i = 0; i < numberHandlers; i++ )
	{
	   	if (mHandlers[i]->port == huart)
	   	{
	   		// notify the end of TX
	   		xTaskNotifyFromISR(mHandlers[i]->myTaskModbusAHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
	   		break;
	   	}
	}
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/* USER CODE END 1 */
