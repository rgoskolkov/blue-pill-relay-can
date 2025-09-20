#include <stdint.h>
#include <main.h>

#define SHORT_BLINK_INTERVAL_MS 70
#define LONG_BLINK_INTERVAL_MS 200
#define HEARTBEAT_INTERVAL_MS 7000
#define HEARTBEAT_DURATION_MS 1000

#define LED_ON() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)
#define LED_OFF() HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)
#define HEARTBEAT_TIMEOUT_MS 1500 // например, 8 секунд

static uint8_t fast_blink_count = 0;
static uint8_t slow_blink_count = 0;
static uint8_t heartbeat_active = 0;

static uint32_t next_blink_time = 0;
static uint32_t next_heartbeat_time = 0;
static uint32_t heartbeat_end_time = 0;
static uint8_t led_state = 0;
static uint8_t heartbeat_blinking = 0;
static uint32_t last_heartbeat_signal = 0;

void led_Blink_Init(void)
{
    fast_blink_count = 0;
    slow_blink_count = 0;
    heartbeat_active = 0;
    next_blink_time = 0;
    next_heartbeat_time = 0;
    heartbeat_end_time = 0;
    led_state = 0;
    heartbeat_blinking = 0;
    LED_OFF();
}

void start_Short_LED_Blink(uint8_t count)
{
    fast_blink_count += count;
    if (!led_state && fast_blink_count > 0 && !heartbeat_blinking)
    {
        LED_ON();
        led_state = 1;
        next_blink_time = HAL_GetTick() + SHORT_BLINK_INTERVAL_MS;
    }
}

void start_Long_LED_Blink(uint8_t count)
{
    if (fast_blink_count == 0)
    {
        slow_blink_count += count;
        if (!led_state && slow_blink_count > 0 && !heartbeat_blinking)
        {
            LED_ON();
            led_state = 1;
            next_blink_time = HAL_GetTick() + LONG_BLINK_INTERVAL_MS;
        }
    }
}

void start_Heartbeat_Blink(void)
{
    heartbeat_active = 1;
    last_heartbeat_signal = HAL_GetTick(); // <<< ЗАПОМИНАЕМ ВРЕМЯ
    if (next_heartbeat_time == 0)
    {
        next_heartbeat_time = HAL_GetTick() + HEARTBEAT_INTERVAL_MS;
    }
}

void process_LED_Blink(void)
{
    uint32_t now = HAL_GetTick();

    // 1. Обрабатываем heartbeat (высший приоритет)
    if (heartbeat_active)
    {
        if (heartbeat_active && (now - last_heartbeat_signal > HEARTBEAT_TIMEOUT_MS))
        {
            heartbeat_active = 0;
            heartbeat_blinking = 0;
            next_heartbeat_time = 0;
            LED_OFF();
            led_state = 0;
        }
        else
        {
            if (!heartbeat_blinking && now >= next_heartbeat_time)
            {
                // Начинаем heartbeat - горим 1 секунду
                if (fast_blink_count == 0 && slow_blink_count == 0)
                {
                    LED_ON();
                    led_state = 1;
                    heartbeat_blinking = 1;
                    heartbeat_end_time = now + HEARTBEAT_DURATION_MS;
                }
                else
                {
                    // Откладываем heartbeat если есть активные мигания
                    next_heartbeat_time = now + 1000;
                }
            }

            if (heartbeat_blinking && now >= heartbeat_end_time)
            {
                // Завершаем heartbeat
                if (fast_blink_count == 0 && slow_blink_count == 0)
                {
                    LED_OFF();
                    led_state = 0;
                }
                heartbeat_blinking = 0;
                next_heartbeat_time = now + HEARTBEAT_INTERVAL_MS;
            }
        }
    }

    // 2. Обрабатываем обычные мигания (только если не активен heartbeat)
    if (!heartbeat_blinking && now >= next_blink_time)
    {
        if (led_state)
        {
            // Выключаем светодиод
            LED_OFF();
            led_state = 0;

            if (fast_blink_count > 0)
            {
                fast_blink_count--;
                if (fast_blink_count > 0)
                {
                    next_blink_time = now + SHORT_BLINK_INTERVAL_MS;
                }
            }
            else if (slow_blink_count > 0)
            {
                slow_blink_count--;
                if (slow_blink_count > 0)
                {
                    next_blink_time = now + LONG_BLINK_INTERVAL_MS;
                }
            }
        }
        else if (fast_blink_count > 0 || slow_blink_count > 0)
        {
            // Включаем светодиод если есть мигания
            LED_ON();
            led_state = 1;
            next_blink_time = now + fast_blink_count > 0 ? SHORT_BLINK_INTERVAL_MS : LONG_BLINK_INTERVAL_MS;
        }
    }

    // 3. Выключаем светодиод если все задачи завершены
    if (fast_blink_count == 0 && slow_blink_count == 0 &&
        !heartbeat_blinking && led_state)
    {
        LED_OFF();
        led_state = 0;
    }
}