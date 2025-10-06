#include "input_driver.h"
#include "modbus_adapter.h"
#include "relay_driver.h"

void application_init(UART_HandleTypeDef *port)
{
    Input_Init();
    relay_init();
    modbus_adapter_init(port);
}