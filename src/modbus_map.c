#include "modbus_map.h"
#include "relay_driver.h"
#include "input_driver.h"
#include <string.h>

/* Local caches */
static uint8_t switch_states[NUM_SWITCHES];
static uint8_t coil_cache[NUM_SWITCHES];

// Define Modbus register addresses
#define RELAY_1_ADDRESS 0x0001
#define RELAY_2_ADDRESS 0x0002
#define RELAY_3_ADDRESS 0x0003
#define RELAY_4_ADDRESS 0x0004
#define RELAY_5_ADDRESS 0x0005
#define RELAY_6_ADDRESS 0x0006
#define RELAY_7_ADDRESS 0x0007
#define RELAY_8_ADDRESS 0x0008
#define SWITCH_1_ADDRESS 0x0101
#define SWITCH_2_ADDRESS 0x0102
#define SWITCH_3_ADDRESS 0x0103
#define SWITCH_4_ADDRESS 0x0104
#define SWITCH_5_ADDRESS 0x0105
#define SWITCH_6_ADDRESS 0x0106
#define SWITCH_7_ADDRESS 0x0107
#define SWITCH_8_ADDRESS 0x0108

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