#include "input_driver.h"
#include "can_adapter.h"
#include "relay_driver.h"

void application_init(void)
{
    can_adapter_init();
    Input_Init();
    relay_init();
}