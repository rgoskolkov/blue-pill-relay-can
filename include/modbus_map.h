#ifndef MODBUS_MAP_H
#define MODBUS_MAP_H

#include "stdio.h"

/* Initialize map and hardware */
void modbus_map_init(void);

/* Called when input (switch) changes to update map and mirror to relay */
void modbus_map_update_switch(uint8_t idx, uint8_t state);

#endif // MODBUS_MAP_H