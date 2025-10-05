#include "main.h"
#include "input_driver.h"
#include "modbus_map.h"
#include "board_config.h"
#include "stm32f1xx_hal.h"
#include <string.h>
#include "led_driver.h"
#include <stdio.h>

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

// Array to map switch index to GPIO Port
static GPIO_TypeDef* const switch_ports[NUM_SWITCHES] = {
    SWITCH1_GPIO_Port, SWITCH2_GPIO_Port, SWITCH3_GPIO_Port, SWITCH4_GPIO_Port,
    SWITCH5_GPIO_Port, SWITCH6_GPIO_Port, SWITCH7_GPIO_Port, SWITCH8_GPIO_Port
};

// Array to map switch index to GPIO Pin
static const uint16_t switch_pins[NUM_SWITCHES] = {
    SWITCH1_Pin, SWITCH2_Pin, SWITCH3_Pin, SWITCH4_Pin,
    SWITCH5_Pin, SWITCH6_Pin, SWITCH7_Pin, SWITCH8_Pin
};

void process_switch_event(uint8_t i)
{
    if (i >= NUM_SWITCHES) return;

    uint32_t now = HAL_GetTick();

    if (!debounce_check(i)) {
        return;
    }
    
    GPIO_TypeDef* port = switch_ports[i];
    uint16_t pin = switch_pins[i];
    uint8_t current_state = (HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_SET) ? 0u : 1u;

    printf("Switch %d interrupt, state: %d\n", i, current_state);

    last_event_ts[i] = now;

    if (current_state == 1) // Нажатие (переход в LOW)
    {
        press_start_ts[i] = now;
        // Мгновенное переключение
        switch_state[i] = !switch_state[i];
        led_signal_ack();
        modbus_map_update_switch(i, switch_state[i]);
    }
    else // Отпускание (переход в HIGH)
    {
        uint32_t press_duration = now - press_start_ts[i];
        if (press_duration >= LONG_PRESS_TIME_MS)
        {
            // Долгое нажатие - отменяем действие
            switch_state[i] = !switch_state[i];
            led_signal_ack();
            modbus_map_update_switch(i, switch_state[i]);
        }
    }
    prev_states[i] = current_state;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint8_t switch_index = 0;
    switch(GPIO_Pin)
    {
        case SWITCH1_Pin: switch_index = 0; break;
        case SWITCH2_Pin: switch_index = 1; break;
        case SWITCH3_Pin: switch_index = 2; break;
        case SWITCH4_Pin: switch_index = 3; break;
        case SWITCH5_Pin: switch_index = 4; break;
        case SWITCH6_Pin: switch_index = 5; break;
        case SWITCH7_Pin: switch_index = 6; break;
        case SWITCH8_Pin: switch_index = 7; break;
        default: return; // Not a switch pin
    }
    process_switch_event(switch_index);
}