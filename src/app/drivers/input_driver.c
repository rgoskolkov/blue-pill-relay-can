#include "input_driver.h"
#include "board_config.h"
#include <string.h>
#include "led_driver.h"
#include "stdbool.h"
#include "relay_driver.h"
#define DEBOUNCE_POLL_PERIOD_MS 20
#define DEBOUNCE_TICKS (DEBOUNCE_MS / DEBOUNCE_POLL_PERIOD_MS)

static uint8_t debounce_counter[NUM_SWITCHES];
static uint8_t stable_state[NUM_SWITCHES];

void Input_Init(void)
{
    memset(debounce_counter, 0, sizeof(debounce_counter));
    for(int i=0; i<NUM_SWITCHES; i++)
    {
        stable_state[i] = Board_Switch_Read(i);
        if (stable_state[i]) {
            debounce_counter[i] = DEBOUNCE_TICKS;
        }
    }
}

void process_switch_event(uint8_t i)
{
    if (i >= NUM_SWITCHES) return;
    led_signal_ack();
    relay_toggle(i);
}

void input_task(void *argument)
{
    uint8_t switch_index;
    osStatus_t status;

    for(;;)
    {
        status = osMessageQueueGet(switchEventQueueHandle, &switch_index, NULL, osWaitForever);
        if (status == osOK)
        {
            process_switch_event(switch_index);
        }
    }
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
        case USER_KEY_Pin: switch_index = 8; break;
        default: return; // Not a switch pin
    }
    led_signal_ack();

    if (switch_index == 8)
    {
        led_signal_ack();
    }
    
    
    if (switchEventQueueHandle != NULL)
    {
        osMessageQueuePut(switchEventQueueHandle, &switch_index, 0U, 0U);
    }
}