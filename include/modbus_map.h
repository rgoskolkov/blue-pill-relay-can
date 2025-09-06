#ifndef MODBUS_MAP_H
#define MODBUS_MAP_H

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