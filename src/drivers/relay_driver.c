#include "main.h"
#include "relay_driver.h"
#include "board_config.h"
#include "stm32f1xx_hal.h"
#include <string.h>

static uint8_t relay_states_local[NUM_SWITCHES];

static GPIO_TypeDef *relay_ports[NUM_SWITCHES] = {
    RELAY1_GPIO_Port, RELAY2_GPIO_Port, RELAY3_GPIO_Port, RELAY4_GPIO_Port,
    RELAY5_GPIO_Port, RELAY6_GPIO_Port, RELAY7_GPIO_Port, RELAY8_GPIO_Port};
static uint16_t relay_pins[NUM_SWITCHES] = {
    RELAY1_Pin, RELAY2_Pin, RELAY3_Pin, RELAY4_Pin,
    RELAY5_Pin, RELAY6_Pin, RELAY7_Pin, RELAY8_Pin};

static void Relay_Init(void)
{
    memset(relay_states_local, 0, sizeof(relay_states_local));
    for (uint8_t i = 0; i < NUM_SWITCHES; ++i)
    {
        HAL_GPIO_WritePin(relay_ports[i], relay_pins[i], GPIO_PIN_SET);
        relay_states_local[i] = 0;
    }
}

static void Relay_SetState(uint8_t relay_number, uint8_t state)
{
    if (relay_number >= NUM_SWITCHES)
        return;
    relay_states_local[relay_number] = state ? 1 : 0;
    HAL_GPIO_WritePin(relay_ports[relay_number], relay_pins[relay_number],
                      state ? GPIO_PIN_RESET : GPIO_PIN_SET);
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