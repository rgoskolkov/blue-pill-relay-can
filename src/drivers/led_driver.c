#include "led_driver.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED_ON() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)
#define LED_OFF() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)

#define HEARTBEAT_PERIOD_MS 10000 
#define TASK_LOOP_DELAY_MS 100    

// --- Глобальные флаги/счетчики ---
static volatile uint8_t g_ack_signal_flag = 0;
static volatile TickType_t g_last_heartbeat_tick = 0;


// --- Публичные функции ---

void led_signal_ack(void) {
    g_ack_signal_flag = 1;
}

void led_signal_heartbeat(void) {
    g_last_heartbeat_tick = xTaskGetTickCount();
}


// --- Основная задача ---

void led_task(void *argument) {
    TickType_t next_heartbeat_time = 0;

    // Сигнал о старте
    led_signal_ack();

    for (;;) {
        // 1. Приоритет: Сигнал ACK (старт или выполнение команды)
        if (g_ack_signal_flag) {
            g_ack_signal_flag = 0; // Сбрасываем флаг
            for (uint8_t i = 0; i < 3; i++) {
                LED_ON();
                vTaskDelay(pdMS_TO_TICKS(100));
                LED_OFF();
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            continue; // Пропускаем остаток цикла, чтобы не было ложного хартбита
        }
        
        // 2. Низкий приоритет: Heartbeat
        TickType_t now = xTaskGetTickCount();
        if (now >= next_heartbeat_time) {
            // Если пора показать heartbeat и есть свежий сигнал с Modbus
            if ((now - g_last_heartbeat_tick) < pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS)) {
                LED_ON();
                vTaskDelay(pdMS_TO_TICKS(1000)); // Длинное мигание
                LED_OFF();
            }
            // Планируем следующий heartbeat не раньше, чем через 5 секунд
            next_heartbeat_time = now + pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS);
        }

        // Короткая пауза, чтобы задача не блокировала CPU и была отзывчивой
        vTaskDelay(pdMS_TO_TICKS(TASK_LOOP_DELAY_MS));
    }
}