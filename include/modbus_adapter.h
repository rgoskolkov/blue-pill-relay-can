#ifndef MODBUS_ADAPTER_H
#define MODBUS_ADAPTER_H

#include <stdint.h>

void modbusAdapter_Init(void);
void modbusTask(void * pvParameters);

#endif // MODBUS_ADAPTER_H