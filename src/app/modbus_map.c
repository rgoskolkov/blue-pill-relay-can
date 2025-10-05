#include "modbus_map.h"
#include "relay_driver.h"
#include "input_driver.h"
#include <string.h>
#include "board_config.h"

/* Local caches */
static uint8_t switch_states[NUM_SWITCHES];

// Function to initialize the Modbus register map
void modbus_map_init(void)
{
    memset(switch_states, 0, sizeof(switch_states));
    relay_init();
    /* ensure relays off on startup and update cache */
    for (uint8_t i = 0; i < NUM_SWITCHES; ++i)
    {
        relay_off(i);
    }
}

/* update switch state and mirror to relay */
void modbus_map_update_switch(uint8_t idx, uint8_t state)
{
    if (idx >= NUM_SWITCHES)
        return;
    switch_states[idx] = state ? 1 : 0;
    /* mirror switch -> relay and update cache */
    state ? relay_on(idx) : relay_off(idx);
}