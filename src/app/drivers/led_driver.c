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
#define LED_EVENT_HEARTBEAT (1 << 1) // Событие: сигнал о работе Modbus

// --- Статические переменные ---
static TaskHandle_t led_task_handle = NULL;
static volatile TickType_t g_last_heartbeat_tick = 0;
const TickType_t HEARTBEAT_TIMEOUT_TICKS = pdMS_TO_TICKS(15000); // 15 секунд

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

void configASSERT_Handler(uint32_t pc) {
    printf("configASSERT_Handler at PC: %08lX\n", pc);
    for (;;) {
        LED_ON();
        for (volatile int i = 0; i < 200000; ++i) __asm volatile("nop");
        LED_OFF();
        for (volatile int i = 0; i < 200000; ++i) __asm volatile("nop");

        LED_ON();
        for (volatile int i = 0; i < 200000; ++i) __asm volatile("nop");
        LED_OFF();
        for (volatile int i = 0; i < 200000; ++i) __asm volatile("nop");

        LED_ON();
        for (volatile int i = 0; i < 200000; ++i) __asm volatile("nop");
        LED_OFF();

        for (volatile int i = 0; i < 800000; ++i) __asm volatile("nop");
    }
}

void led_signal_ack(void) {
    if (led_task_handle != NULL) {
        xTaskNotify(led_task_handle, LED_EVENT_ACK, eSetBits);
    }
}

void led_signal_heartbeat(void) {
    g_last_heartbeat_tick = xTaskGetTickCount();
    if (led_task_handle != NULL) {
        // Это уведомление просто разбудит задачу для проверки, если она спит
        xTaskNotify(led_task_handle, LED_EVENT_HEARTBEAT, eSetBits);
    }
}

// --- Основная задача ---

void led_task(void *argument) {
    led_task_handle = xTaskGetCurrentTaskHandle();
    uint32_t notified_value;
    TickType_t next_heartbeat_display_time = 0;

    // Сигнал о старте системы
    led_signal_ack();

    for (;;) {
        // Ожидаем уведомления. Выходим по таймауту, чтобы проверить heartbeat.
        // Таймаут равен времени до следующего отображения heartbeat.
        TickType_t now = xTaskGetTickCount();
        TickType_t timeout;

        if (next_heartbeat_display_time > now) {
            timeout = next_heartbeat_display_time - now;
        } else {
            // Если время уже прошло, ждем следующего события бесконечно.
            // Это не даст задаче зациклиться в состоянии Ready.
            // Проверка heartbeat ниже все равно выполнится.
            timeout = portMAX_DELAY;
        }
        
        BaseType_t result = xTaskNotifyWait(0x00,          // Не сбрасывать биты при входе
                                          ULONG_MAX,     // Сбрасывать все биты при выходе
                                          &notified_value, // Переменная для хранения уведомления
                                          timeout);      // Таймаут

        now = xTaskGetTickCount(); // Обновляем время после пробуждения

        if (result == pdPASS) { // Если проснулись по уведомлению
            // 1. Приоритет: Сигнал ACK
            if (notified_value & LED_EVENT_ACK) {
                for (uint8_t i = 0; i < 3; i++) {
                    LED_ON();
                    vTaskDelay(pdMS_TO_TICKS(100));
                    LED_OFF();
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                // После ACK лучше пересчитать время следующего heartbeat,
                // чтобы избежать слишком частого мигания
                next_heartbeat_display_time = now + HEARTBEAT_TIMEOUT_TICKS;
                continue; // Начинаем цикл ожидания заново
            }
        }
        
        // 2. Heartbeat (проверяется после таймаута или после любого события)
        if (now >= next_heartbeat_display_time) {
            // Если с момента последнего сигнала по Modbus прошло не много времени
            if ((now - g_last_heartbeat_tick) < HEARTBEAT_TIMEOUT_TICKS) {
                LED_ON();
                vTaskDelay(pdMS_TO_TICKS(500)); // Среднее мигание
                LED_OFF();
            }
            // Планируем следующий heartbeat
            next_heartbeat_display_time = now + HEARTBEAT_TIMEOUT_TICKS;
        }
    }
}