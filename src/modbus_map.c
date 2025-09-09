#include "modbus_map.h"
#include "relay_driver.h"
#include "input_driver.h"
#include <string.h>

/* Local caches */
static uint8_t switch_states[NUM_SWITCHES];
static uint8_t coil_cache[NUM_SWITCHES];

// Function to initialize the Modbus register map
void modbus_map_init(void)
{
    memset(switch_states, 0, sizeof(switch_states));
    memset(coil_cache, 0, sizeof(coil_cache));
    Relay_Init();
    /* ensure relays off on startup and update cache */
    for (uint8_t i = 0; i < NUM_SWITCHES; ++i)
    {
        Relay_SetState(i, 0);
        coil_cache[i] = 0;
    }
}

/* update switch state and mirror to relay */
void modbus_map_update_switch(uint8_t idx, uint8_t state)
{
    if (idx >= NUM_SWITCHES)
        return;
    switch_states[idx] = state ? 1 : 0;
    /* mirror switch -> relay and update cache */
    Relay_SetState(idx, switch_states[idx]);
    coil_cache[idx] = Relay_GetState(idx);
}

/* refresh coil_cache from hardware (call periodically or before responding) */
void modbus_map_update_registers(void)
{
    for (uint8_t i = 0; i < NUM_SWITCHES; ++i)
    {
        coil_cache[i] = Relay_GetState(i);
    }
}

/* used by Modbus responder */
uint8_t modbus_map_get_coil(uint16_t coil_addr)
{
    if (coil_addr >= NUM_SWITCHES)
        return 0;
    return coil_cache[coil_addr];
}