#include "input_driver.h"
#include "board_config.h"
#include <string.h>
#include "led_driver.h"
#include "relay_driver.h"
#include "can_adapter.h"

/* Параметры дебаунса */
#define DEBOUNCE_CHECK_INTERVAL_MS 20 
#define DEBOUNCE_CONFIRM_COUNT 5
typedef struct {
    uint8_t integrator;    
    uint8_t debounced_state;
} debounce_info_t;

static debounce_info_t debounce_info[NUM_SWITCHES];

void Input_Init(void)
{
    for (uint8_t i = 0; i < NUM_SWITCHES; i++)
    {
        if (Board_Switch_Read(i)) { 
            debounce_info[i].integrator = DEBOUNCE_CONFIRM_COUNT;
            debounce_info[i].debounced_state = 1;
        } else { 
            debounce_info[i].integrator = 0;
            debounce_info[i].debounced_state = 0;
        }
    }
}

void process_switch_event(uint8_t i)
{
    if (i >= NUM_SWITCHES)
        return;
    led_signal_ack();
    uint8_t relay_idx = switch_to_relay_map[i];
    if (relay_idx >= NUM_RELAYS) return; // Ignore invalid mappings
    uint8_t state = relay_toggle(relay_idx);
    uint8_t mask = 0;
    for (int j = 0; j < NUM_RELAYS; j++)
    {
        if (relay_getState(j))
            mask |= (1 << j);
    }
    can_publish_relay_changed(mask, relay_idx, state);
}

void input_task(void *argument)
{
    printf("input_task started (polling debouncer)");
    
    for (;;)
    {
        osDelay(pdMS_TO_TICKS(DEBOUNCE_CHECK_INTERVAL_MS));
        
        for (uint8_t i = 0; i < NUM_SWITCHES; i++)
        {
            debounce_info_t *info = &debounce_info[i];
            uint8_t previous_debounced_state = info->debounced_state;
            
            uint8_t current_physical_state = Board_Switch_Read(i);
            if (current_physical_state) {
                if (info->integrator < DEBOUNCE_CONFIRM_COUNT) {
                    info->integrator++;
                }
            } else {
                if (info->integrator > 0) {
                    info->integrator--;
                }
            }

            if (info->integrator >= DEBOUNCE_CONFIRM_COUNT) {
                info->debounced_state = 1;
            } 
            else if (info->integrator == 0) {
                info->debounced_state = 0;
            }
            
            if (info->debounced_state != previous_debounced_state) {
                printf("Switch %d debounced state changed to %d", i, info->debounced_state);
                process_switch_event(i);
            }
        }
    }
}
