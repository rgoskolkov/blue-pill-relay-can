#ifndef MODBUS_MAP_H
#define MODBUS_MAP_H
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

#include <stdint.h>
#include "board_config.h"

/* Initialize map and hardware */
void modbus_map_init(void);

/* Called when input (switch) changes to update map and mirror to relay */
void modbus_map_update_switch(uint8_t idx, uint8_t state);

/* Update internal Modbus coil/register cache from hardware states */
void modbus_map_update_registers(void);

/* Read coil state for Modbus responder */
uint8_t modbus_map_get_coil(uint16_t coil_addr);

#endif // MODBUS_MAP_H