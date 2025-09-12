#include <stdint.h>
#include <main.h>

#define LED_BLINK_COUNT 3
#define LED_BLINK_INTERVAL_MS 70

/* Инвертированные определения для светодиода с обратной логикой */
#define LED_ON()  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)    // Горит
#define LED_OFF() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)      // Не горит
#define LED_STATE() (HAL_GPIO_ReadPin(LED_GPIO_Port, LED_Pin) == GPIO_PIN_RESET) // true = горит

static struct {
    uint8_t is_blinking;
    uint8_t blink_step;
    uint32_t next_step_time;
} led_state = {0};

/* Принудительное выключение светодиода */
void Force_LED_Off(void)
{
    LED_OFF(); // Устанавливаем высокий уровень
}

/* Запуск мигания */
void Start_LED_Blink(void)
{
    Force_LED_Off(); // Гарантированно выключаем перед началом
    
    led_state.is_blinking = 1;
    led_state.blink_step = 0;
    led_state.next_step_time = HAL_GetTick();
    
    // Включаем первый раз (устанавливаем низкий уровень)
    LED_ON();
}

/* Обработка мигания */
void Process_LED_Blink(void)
{
    if (!led_state.is_blinking) {
        // Когда не мигаем - гарантированно выключаем
        Force_LED_Off();
        return;
    }
    
    uint32_t now = HAL_GetTick();
    if (now < led_state.next_step_time) {
        return;
    }
    
    led_state.next_step_time = now + LED_BLINK_INTERVAL_MS;
    led_state.blink_step++;
    
    if (led_state.blink_step % 2 == 1) {
        // Нечетные шаги - включаем (низкий уровень)
        LED_ON();
    } else {
        // Четные шаги - выключаем (высокий уровень)
        LED_OFF();
    }
    
    // Завершили все мигания (6 шагов = 3 включения + 3 выключения)
    if (led_state.blink_step >= 6) {
        led_state.is_blinking = 0;
        Force_LED_Off(); // Гарантированно выключаем
    }
}