#include "relay_driver.h"
#include "board_config.h"
#include "can_adapter.h"
#include <string.h>
#include <stdbool.h>

static uint8_t relay_states_local[NUM_RELAYS];

static void Relay_Init(void)
{
    memset(relay_states_local, 0, sizeof(relay_states_local));
    for (uint8_t i = 0; i < NUM_RELAYS; ++i)
    {
        Board_Relay_Off(i);
        relay_states_local[i] = 0;
    }
}

static void Relay_SetState(uint8_t relay_number, uint8_t state)
{
    if (relay_number >= NUM_RELAYS)
        return;

    uint8_t old_state = relay_states_local[relay_number];
    if (old_state == state)
        return;

    relay_states_local[relay_number] = state;
    state ? Board_Relay_On(relay_number) : Board_Relay_Off(relay_number);

    uint8_t relay_mask = 0;
    for(int i=0; i<NUM_RELAYS; ++i) {
        if(relay_states_local[i]) {
            relay_mask |= (1 << i);
        }
    }
    can_publish_relay_changed(relay_mask, relay_number, state);
}

uint8_t relay_getState(uint8_t relay_number)
{
    if (relay_number >= NUM_RELAYS)
        return 0;
    return relay_states_local[relay_number];
}

void relay_init(void) { Relay_Init(); }
void relay_on(uint8_t n) { Relay_SetState(n, 1); }
void relay_off(uint8_t n) { Relay_SetState(n, 0); }
uint8_t relay_toggle(uint8_t relay_number)
{
    uint8_t newState = !relay_getState(relay_number);
    Relay_SetState(relay_number, newState);
    return newState;
}
