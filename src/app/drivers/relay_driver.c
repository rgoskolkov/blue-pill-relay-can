#include "relay_driver.h"
#include "board_config.h"
#include <string.h>
#include "modbus_adapter.h"
#include "task.h"

static uint8_t relay_states_local[NUM_SWITCHES];

static void Relay_Init(void)
{
    memset(relay_states_local, 0, sizeof(relay_states_local));
    for (uint8_t i = 0; i < NUM_SWITCHES; ++i)
    {
        Board_Relay_Off(i);
        relay_states_local[i] = 0;
    }
}

static void Relay_SetState(uint8_t relay_number, uint8_t state)
{
    if (relay_number >= NUM_SWITCHES)
        return;
    relay_states_local[relay_number] = state ? 1 : 0;
    state ? Board_Relay_On(relay_number) : Board_Relay_Off(relay_number);
    if (syncTaskHandle != NULL)
    {
        // This function can be called from an ISR, so use the ISR-safe version.
        // A context switch is not forced here; the scheduler will run at the next tick.
        xTaskNotifyFromISR(syncTaskHandle, FLAG_SYNC_FROM_RELAY, eSetBits, NULL);
    }
}

uint8_t Relay_GetState(uint8_t relay_number)
{
    if (relay_number >= NUM_SWITCHES)
        return 0;
    return relay_states_local[relay_number];
}

void relay_init(void) { Relay_Init(); }
void relay_on(uint8_t relay_number) { Relay_SetState(relay_number, 1); }
void relay_off(uint8_t relay_number) { Relay_SetState(relay_number, 0); }