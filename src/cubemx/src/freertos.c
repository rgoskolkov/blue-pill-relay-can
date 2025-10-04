/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led_driver.h"
#include "input_driver.h"
#include "modbus_adapter.h"
#include "system_monitor.h"
#include "board_config.h"
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
/* USER CODE BEGIN Variables */
/* USER CODE END Variables */

const osThreadAttr_t ledTask_attributes = {
  .name = "ledTask",
  .stack_size = configMINIMAL_STACK_SIZE * 6,
  .priority = (osPriority_t) osPriorityLow,
};


osThreadId_t syncTaskHandle;
const osThreadAttr_t syncTask_attributes = {
  .name = "syncTask",
  .stack_size = configMINIMAL_STACK_SIZE * 6,
  .priority = (osPriority_t) osPriorityLow5,
};

osThreadId_t monitorTaskHandle;
const osThreadAttr_t monitorTask_attributes = {
  .name = "monitorTask",
  .stack_size = configMINIMAL_STACK_SIZE * 8,
  .priority = (osPriority_t) osPriorityLow1,
};


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* USER CODE END RTOS_QUEUES */

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadNew(led_task, NULL, &ledTask_attributes);
  syncTaskHandle = osThreadNew(sync_task, NULL, &syncTask_attributes);
  #if MONITOR_TASK == 1
    osThreadNew(system_monitor_task, NULL, &monitorTask_attributes);
  #endif  
  // xTaskCreate(, "led_task", configMINIMAL_STACK_SIZE, NULL, 30, NULL);
  // xTaskCreate(, "input_task", 128, NULL, 23, NULL);
  // /* StartTaskModbusSlave is created by the Modbus library in ModbusInit().
  //   Avoid double-creating it here; call modbus_adapter_init() before scheduler start
  //   so the library can create its own task. */
  // xTaskCreate(sync_task, "sync_task", 256, NULL, 26, NULL);
  // xTaskCreate(modbus_diag_task, "diag_task", 512, NULL, 40, NULL);
 /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */
