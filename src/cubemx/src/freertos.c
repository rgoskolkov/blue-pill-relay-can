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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "led_driver.h"
#include "system_monitor.h"
#include "board_config.h"
#include <string.h>
#include "input_driver.h"
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
osMessageQueueId_t switchEventQueueHandle;

osThreadId_t inputTaskHandle;
const osThreadAttr_t inputTask_attributes = {
  .name = "inputTask",
  .stack_size = configMINIMAL_STACK_SIZE * 2,
  .priority = (osPriority_t) osPriorityNormal,
};
/* USER CODE END Variables */

const osThreadAttr_t ledTask_attributes = {
  .name = "ledTask",
  .stack_size = configMINIMAL_STACK_SIZE * 3,
  .priority = (osPriority_t) osPriorityLow,
};

osThreadId_t monitorTaskHandle;
const osThreadAttr_t monitorTask_attributes = {
  .name = "monitorTask",
  .stack_size = configMINIMAL_STACK_SIZE * 4,
  .priority = (osPriority_t) osPriorityLow1,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

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
  const osMessageQueueAttr_t switchEventQueue_attributes = {
    .name = "switchEventQueue"
  };
  switchEventQueueHandle = osMessageQueueNew(16, sizeof(uint8_t), &switchEventQueue_attributes);
  /* USER CODE END RTOS_QUEUES */

  /* USER CODE BEGIN RTOS_THREADS */
  osThreadNew(led_task, NULL, &ledTask_attributes);
  inputTaskHandle = osThreadNew(input_task, NULL, &inputTask_attributes);
  #if MONITOR_TASK == 1
    osThreadNew(system_monitor_task, NULL, &monitorTask_attributes);
  #endif
  /* can_adapter_init() вызывается из application_init() */
 /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("vApplicationStackOverflowHook: %s\n", pcTaskName);

    int blink_count = 5;

    if (strcmp(pcTaskName, "ledTask") == 0) {
        blink_count = 1;
    } else if (strcmp(pcTaskName, "inputTask") == 0) {
        blink_count = 2;
    }
    else if (strcmp(pcTaskName, "monitorTask") == 0) {
        blink_count = 3;
    }
    dead_hand(200, blink_count);
}

void vApplicationMallocFailedHook(void)
{
   printf("vApplicationMallocFailedHook\n");
   dead_hand(500, 4);
}
/* USER CODE END Application */
