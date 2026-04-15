#include "input_driver.h"
#include "can_adapter.h"
#include "relay_driver.h"
// #include "usb_cdc_monitor.h"  // ВРЕМЕННО ОТКЛЮЧЕНО

void application_init(void)
{
    Input_Init();
    relay_init();
    can_adapter_init();
    
    /* Initialize USB CDC logging - ВРЕМЕННО ОТКЛЮЧЕНО */
    //USB_CDC_Log_Init();
}