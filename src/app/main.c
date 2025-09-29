#include "main.h"
#include "gpio.h"
#include "cmsis_os.h"
#include "usart.h"
#include "modbus_map.h"
#include "input_driver.h"
#include "modbus_adapter.h"
#include "relay_driver.h"
#include "tim.h"
#include "led_driver.h"
#include "stm32f1xx.h"
#include <stdio.h>

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
extern TaskHandle_t led_task_handle;
extern TaskHandle_t input_task_handle;
extern TaskHandle_t modbus_task_handle;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  // CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  // ITM->LAR = 0xC5ACCE55;
  // ITM->TCR |= ITM_TCR_ITMENA_Msk;
  // ITM->TER |= 1; // ITM Port 0 enabled
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  SystemClock_Config();  // сгенерирован CubeMX
  MX_GPIO_Init();        // CubeMX
  MX_TIM1_Init();        // CubeMX
  MX_USART3_UART_Init(); // CubeMX
  
  printf("ITM output test\n");
  modbus_map_init();
  Input_Init();
  relay_init();
  
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  
  MX_FREERTOS_Init();
  modbusAdapter_Init();

  // Сигнал об успешном старте
  led_signal_ack();
  

   /* Init scheduler */
  osKernelStart();
  /* Start scheduler */
 
  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

  }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
#include <string.h>

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    int blink_count = 5; // По умолчанию 5 миганий для неизвестной задачи

    if (xTask == led_task_handle) {
        blink_count = 1;
    } else if (xTask == input_task_handle) {
        blink_count = 2;
    } else if (xTask == modbus_task_handle) {
        blink_count = 3;
    }
    // Задачу таймеров (Tmr Svc) нельзя получить так просто,
    // но если это не одна из трех наших, скорее всего, это она.

    for(;;)
    {
        for (int i=0; i < blink_count; i++) {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            HAL_Delay(200);
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            HAL_Delay(200);
        }
        HAL_Delay(1000);
    }
}

void vApplicationMallocFailedHook(void)
{
   /* This function will be called if a call to pvPortMalloc() fails. */
   /* Blink fast pattern for malloc failed */
   for(;;)
   {
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      HAL_Delay(50);
   }
}
/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
