#ifndef MODBUS_ADAPTER_H
#define MODBUS_ADAPTER_H

#include "cmsis_os.h"

extern osThreadId_t syncTaskHandle;

#define FLAG_SYNC_FROM_RELAY  (0x0001U)
#define FLAG_SYNC_FROM_MODBUS (0x0002U)

void modbus_adapter_init(void);
void sync_task(void *argument);
void modbus_diag_task(void *argument);

#endif // MODBUS_ADAPTER_H