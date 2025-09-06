#include "main.h"
#include "input_driver.h"
#include "modbus_map.h"
#include "board_config.h"
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdbool.h>

static uint8_t prev_states[NUM_SWITCHES];
static uint32_t last_event_ts[NUM_SWITCHES];

/* Простая inline-функция дебаунса, использует HAL_GetTick() */
static bool debounce_check(uint8_t idx)
{
    if (idx >= NUM_SWITCHES) return false;
    uint32_t now = HAL_GetTick();
    if ((now - last_event_ts[idx]) > DEBOUNCE_MS) {
        last_event_ts[idx] = now;
        return true;
    }
    return false;
}

void Input_Init(void) {
    /* CubeMX MX_GPIO_Init() уже настроил входы (pull-up) */
    memset(prev_states, 0xFF, sizeof(prev_states)); // форсируем первое событие
    memset(last_event_ts, 0, sizeof(last_event_ts));
}

void Input_Update(void) {
    for (uint8_t i = 0; i < NUM_SWITCHES; ++i) {
        GPIO_TypeDef* port;
        uint16_t pin;
        switch (i) {
            case 0: port = SWITCH1_GPIO_Port; pin = SWITCH1_Pin; break;
            case 1: port = SWITCH2_GPIO_Port; pin = SWITCH2_Pin; break;
            case 2: port = SWITCH3_GPIO_Port; pin = SWITCH3_Pin; break;
            case 3: port = SWITCH4_GPIO_Port; pin = SWITCH4_Pin; break;
            case 4: port = SWITCH5_GPIO_Port; pin = SWITCH5_Pin; break;
            case 5: port = SWITCH6_GPIO_Port; pin = SWITCH6_Pin; break;
            case 6: port = SWITCH7_GPIO_Port; pin = SWITCH7_Pin; break;
            case 7: port = SWITCH8_GPIO_Port; pin = SWITCH8_Pin; break;
            default: port = NULL; pin = 0; break;
        }

        uint8_t cur = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 1u : 0u;
        if (cur != prev_states[i]) {
            if (!debounce_check(i)) {
                continue; // отбросить дребезг
            }
            prev_states[i] = cur;
            modbus_map_update_switch(i, cur);
        }
    }
}