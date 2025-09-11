#include <stdint.h>
#include <main.h>

/* Добавьте в начало файла */
#define LED_BLINK_COUNT 3
#define LED_BLINK_INTERVAL_MS 500

/* Состояние мигания светодиода */
typedef struct
{
    uint8_t is_active;
    uint8_t blink_count;
    uint32_t last_blink_time;
} led_blink_state_t;

static led_blink_state_t led_blink_state = {0};

/* Функция для запуска мигания светодиодом */
void Start_LED_Blink(void)
{
    led_blink_state.is_active = 1;
    led_blink_state.blink_count = 0;
    led_blink_state.last_blink_time = HAL_GetTick();

    /* Включим светодиод сразу при старте */
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

/* Функция обработки мигания светодиодом (вызывать в основном цикле) */
void Process_LED_Blink(void)
{
    if (!led_blink_state.is_active)
    {
        return;
    }

    uint32_t current_time = HAL_GetTick();

    if ((current_time - led_blink_state.last_blink_time) >= LED_BLINK_INTERVAL_MS)
    {
        led_blink_state.last_blink_time = current_time;
        led_blink_state.blink_count++;

        if (led_blink_state.blink_count >= LED_BLINK_COUNT * 2)
        {
            /* Завершили мигание - выключаем светодиод и сбрасываем состояние */
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            led_blink_state.is_active = 0;
        }
        else
        {
            /* Переключаем состояние светодиода */
            GPIO_PinState current_state = HAL_GPIO_ReadPin(LED_GPIO_Port, LED_Pin);
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,
                              (current_state == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET);
        }
    }
}