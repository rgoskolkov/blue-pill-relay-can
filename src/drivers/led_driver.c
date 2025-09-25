#include "led_driver.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define SHORT_BLINK_INTERVAL_MS 70
#define LONG_BLINK_INTERVAL_MS 200
#define ERROR_BLINK_INTERVAL_MS 100
#define HEARTBEAT_INTERVAL_MS 7000
#define HEARTBEAT_DURATION_MS 1000
#define HEARTBEAT_TIMEOUT_MS 1500

#define LED_ON() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)
#define LED_OFF() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)

QueueHandle_t short_blink_queue;
QueueHandle_t long_blink_queue;
QueueHandle_t heartbeat_queue;
QueueHandle_t error_signal_queue;



void start_Short_LED_Blink(uint8_t count)
{
    xQueueSend(short_blink_queue, &count, (TickType_t)10);
}

void start_Long_LED_Blink(uint8_t count)
{
    xQueueSend(long_blink_queue, &count, (TickType_t)10);
}

void reset_Heartbeat_Timer(void)
{
    uint8_t cmd = 1;
    xQueueSend(heartbeat_queue, &cmd, (TickType_t)10);
}

void signal_error(void)
{
    uint8_t cmd = 1;
    xQueueSend(error_signal_queue, &cmd, (TickType_t)10);
}

void short_blink_task(void *argument)
{
    uint8_t count;
    for (;;)
    {
        if (xQueueReceive(short_blink_queue, &count, portMAX_DELAY) == pdPASS)
        {
            for (uint8_t i = 0; i < count; i++)
            {
                LED_ON();
                vTaskDelay(pdMS_TO_TICKS(SHORT_BLINK_INTERVAL_MS));
                LED_OFF();
                vTaskDelay(pdMS_TO_TICKS(SHORT_BLINK_INTERVAL_MS));
            }
        }
    }
}

void long_blink_task(void *argument)
{
    uint8_t count;
    for (;;)
    {
        if (xQueueReceive(long_blink_queue, &count, portMAX_DELAY) == pdPASS)
        {
            for (uint8_t i = 0; i < count; i++)
            {
                LED_ON();
                vTaskDelay(pdMS_TO_TICKS(LONG_BLINK_INTERVAL_MS));
                LED_OFF();
                vTaskDelay(pdMS_TO_TICKS(LONG_BLINK_INTERVAL_MS));
            }
        }
    }
}

void heartbeat_task(void *argument)
{
    uint8_t cmd;
    TickType_t last_reset = xTaskGetTickCount();

    for (;;)
    {
        if (xQueueReceive(heartbeat_queue, &cmd, pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS)) == pdPASS)
        {
            last_reset = xTaskGetTickCount();
        }

        if ((xTaskGetTickCount() - last_reset) < pdMS_TO_TICKS(HEARTBEAT_TIMEOUT_MS))
        {
            LED_ON();
            vTaskDelay(pdMS_TO_TICKS(HEARTBEAT_DURATION_MS));
            LED_OFF();
        }
        else
        {
            vTaskDelay(pdMS_TO_TICKS(100)); // Prevent busy-looping
        }
    }
}

void error_signal_task(void *argument)
{
    uint8_t cmd;
    for (;;)
    {
        if (xQueueReceive(error_signal_queue, &cmd, portMAX_DELAY) == pdPASS)
        {
            for (uint8_t i = 0; i < 5; i++)
            {
                LED_ON();
                vTaskDelay(pdMS_TO_TICKS(ERROR_BLINK_INTERVAL_MS));
                LED_OFF();
                vTaskDelay(pdMS_TO_TICKS(ERROR_BLINK_INTERVAL_MS));
            }
        }
    }
}