#include "main.h"
#include "input_driver.h"
#include "modbus_map.h"
#include "board_config.h"
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdbool.h>
#include "led_driver.h"

#define LONG_PRESS_TIME_MS 2000

static uint8_t prev_states[NUM_SWITCHES];
static uint32_t last_event_ts[NUM_SWITCHES];
static uint32_t press_start_ts[NUM_SWITCHES];
static bool switch_state[NUM_SWITCHES];

static bool debounce_check(uint8_t idx)
{
    if (idx >= NUM_SWITCHES) return false;
    uint32_t now = HAL_GetTick();
    return (now - last_event_ts[idx]) > DEBOUNCE_MS;
}

void Input_Init(void)
{
    memset(prev_states, 0xFF, sizeof(prev_states));
    memset(last_event_ts, 0, sizeof(last_event_ts));
    memset(press_start_ts, 0, sizeof(press_start_ts));
    memset(switch_state, 0, sizeof(switch_state));
}

void Input_Update(void)
{
    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < NUM_SWITCHES; ++i)
    {
        GPIO_TypeDef *port;
        uint16_t pin;

        switch (i)
        {
            case 0: port = SWITCH1_GPIO_Port; pin = SWITCH1_Pin; break;
            case 1: port = SWITCH2_GPIO_Port; pin = SWITCH2_Pin; break;
            case 2: port = SWITCH3_GPIO_Port; pin = SWITCH3_Pin; break;
            case 3: port = SWITCH4_GPIO_Port; pin = SWITCH4_Pin; break;
            case 4: port = SWITCH5_GPIO_Port; pin = SWITCH5_Pin; break;
            case 5: port = SWITCH6_GPIO_Port; pin = SWITCH6_Pin; break;
            case 6: port = SWITCH7_GPIO_Port; pin = SWITCH7_Pin; break;
            case 7: port = SWITCH8_GPIO_Port; pin = SWITCH8_Pin; break;
            default: continue;
        }

        uint8_t current_state = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 0u : 1u;

        if (current_state != prev_states[i])
        {
            if (!debounce_check(i)) continue;
            
            last_event_ts[i] = now;
            prev_states[i] = current_state;

            if (current_state == 1) // Переход в LOW (нажатие)
            {
                press_start_ts[i] = now;
                
                // Мгновенно переключаем состояние
                switch_state[i] = !switch_state[i];
                Start_LED_Blink();
                modbus_map_update_switch(i, switch_state[i]);
            }
            else // Переход в HIGH (отпускание)
            {
                // Проверяем длительность нажатия
                uint32_t press_duration = now - press_start_ts[i];
                
                if (press_duration >= LONG_PRESS_TIME_MS)
                {
                    // Долгое нажатие - отменяем действие
                    switch_state[i] = !switch_state[i];
                    Start_LED_Blink();
                    modbus_map_update_switch(i, switch_state[i]);
                }
            }
        }
    }
}