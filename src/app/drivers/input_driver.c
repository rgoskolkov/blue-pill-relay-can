#include "input_driver.h"
#include "board_config.h"
#include <string.h>
#include "led_driver.h"
#include "stdbool.h"
#include "relay_driver.h"

#define LONG_PRESS_TIME_MS 2000

static uint8_t prev_states[NUM_SWITCHES];
static uint32_t last_event_ts[NUM_SWITCHES];
static uint32_t press_start_ts[NUM_SWITCHES];
static bool switch_state[NUM_SWITCHES];

static bool debounce_check(uint8_t idx)
{
    if (idx >= NUM_SWITCHES) return false;
    uint32_t now = Board_GetTick();
    return (now - last_event_ts[idx]) > DEBOUNCE_MS;
}

void Input_Init(void)
{
    memset(prev_states, 0xFF, sizeof(prev_states));
    memset(last_event_ts, 0, sizeof(last_event_ts));
    memset(press_start_ts, 0, sizeof(press_start_ts));
    memset(switch_state, 0, sizeof(switch_state));
}

void process_switch_event(uint8_t i)
{
    if (i >= NUM_SWITCHES) return;

    uint32_t now = Board_GetTick();

    if (!debounce_check(i)) {
        return;
    }
    
    uint8_t current_state = Board_Switch_Read(i);

    // printf("Switch %d interrupt, state: %d\n", i, current_state);

    last_event_ts[i] = now;

    if (current_state == 1) // Нажатие (переход в LOW)
    {
        press_start_ts[i] = now;
        // Мгновенное переключение
        switch_state[i] = !switch_state[i];
        led_signal_ack();
        switch_state[i] ? relay_on(i) : relay_off(i);
    }
    else // Отпускание (переход в HIGH)
    {
        uint32_t press_duration = now - press_start_ts[i];
        if (press_duration >= LONG_PRESS_TIME_MS)
        {
            // Долгое нажатие - отменяем действие
            switch_state[i] = !switch_state[i];
            led_signal_ack();
            switch_state[i] ? relay_on(i) : relay_off(i);
        }
    }
    prev_states[i] = current_state;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint8_t switch_index = 0;
    switch(GPIO_Pin)
    {
        case SWITCH1_PIN: switch_index = 0; break;
        case SWITCH2_PIN: switch_index = 1; break;
        case SWITCH3_PIN: switch_index = 2; break;
        case SWITCH4_PIN: switch_index = 3; break;
        case SWITCH5_PIN: switch_index = 4; break;
        case SWITCH6_PIN: switch_index = 5; break;
        case SWITCH7_PIN: switch_index = 6; break;
        case SWITCH8_PIN: switch_index = 7; break;
        default: return; // Not a switch pin
    }
    process_switch_event(switch_index);
}