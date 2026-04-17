#include "led_driver.h"
#include "FreeRTOS.h"
#include "task.h"
#include <limits.h> // Для ULONG_MAX
#include "board_config.h"

// --- Определения для управления светодиодом ---
#define LED_ON() Board_LED_On()
#define LED_OFF() Board_LED_Off()

// --- Определения событий для уведомлений задачи ---
#define LED_EVENT_ACK       (1 << 0) // Событие: подтверждение команды
#define LED_EVENT_CAN_RX    (1 << 2) // Событие: приём CAN-фрейма

// --- Статические переменные ---
static TaskHandle_t led_task_handle = NULL;

// --- Публичные функции ---
void dead_hand(uint32_t delay, uint32_t count){
    // Вечный цикл с миганием
    for(;;)
    {
        for (int i=0; i < count; i++) {
            LED_ON();
            Board_Delay(delay);
            LED_OFF();
            Board_Delay(delay);
        }
        Board_Delay(2000);
    }
}

void led_signal_ack(void) {
    if (led_task_handle != NULL) {
        xTaskNotify(led_task_handle, LED_EVENT_ACK, eSetBits);
    }
}

// --- Функция для вызова из прерывания (ISR) ---
void led_signal_can_rx_from_isr(void) {
    if (led_task_handle != NULL) {
        xTaskNotifyFromISR(led_task_handle, LED_EVENT_CAN_RX, eSetBits, NULL);
    }
}

// --- Основная задача ---

void led_task(void *argument) {
    led_task_handle = xTaskGetCurrentTaskHandle();
    uint32_t notified_value;

    // Сигнал о старте системы
    led_signal_ack();
    printf("led_task started\r\n");

    for (;;) {
        BaseType_t result = xTaskNotifyWait(0x00,          // Не сбрасывать биты при входе
                                          ULONG_MAX,     // Сбрасывать все биты при выходе
                                          &notified_value, // Переменная для хранения уведомления
                                          portMAX_DELAY);      // Таймаут


        if (result == pdPASS) { // Если проснулись по уведомлению
            // 1. Приоритет: Сигнал ACK
            if (notified_value & LED_EVENT_ACK) {
                for (uint8_t i = 0; i < 3; i++) {
                    LED_ON();
                    vTaskDelay(pdMS_TO_TICKS(100));
                    LED_OFF();
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                continue; // Начинаем цикл ожидания заново
            }
            
            // 2. Приём CAN-фрейма (мигаем 1 раз коротко)
            if (notified_value & LED_EVENT_CAN_RX) {
                LED_ON();
                vTaskDelay(pdMS_TO_TICKS(50));
                LED_OFF();
            }
        }
    }
}
