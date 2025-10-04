#include "modbus_adapter.h"
#include "Modbus.h"
#include "main.h"
#include "usart.h" 
#include "relay_driver.h"
#include "led_driver.h"
#include "board_config.h"
#include "cmsis_os2.h"
#include "stm32f1xx.h"
#include "FreeRTOS.h"
#include <stdio.h>
#include "task.h"

// --- Глобальные переменные ---
modbusHandler_t mHandler;
// 1 регистр для хранения состояния 8 коилов
uint16_t usRegHolding[1] = {0}; 

/* Diagnostic IRQ counter exported from ISR file */
extern volatile uint32_t usart3_irq_count;

void modbus_adapter_init(void)
{
    mHandler.uModbusType = MB_SLAVE;
    mHandler.port = &huart3;
    mHandler.u8id = MODBUS_SLAVE_ID;
    mHandler.u16timeOut = 1000;
    mHandler.EN_Port = NULL;
    mHandler.xTypeHW = USART_HW;
    mHandler.u16regs = usRegHolding;
    mHandler.u16regsize = 8; // Увеличим, чтобы покрыть все запросы от HA

    ModbusInit(&mHandler);
    ModbusStart(&mHandler);
}

void sync_task(void *argument)
{
    uint32_t flags;
    uint16_t previous_coils_state = 0;

    for(;;)
    {
        flags = osThreadFlagsWait(FLAG_SYNC_FROM_RELAY | FLAG_SYNC_FROM_MODBUS, osFlagsWaitAny, osWaitForever);

        if (flags & FLAG_SYNC_FROM_MODBUS) {
            uint16_t current_coils_state = usRegHolding[0];
            if (previous_coils_state != current_coils_state) {
                //printf("Sync: master write %04X -> %04X\r\n", previous_coils_state, current_coils_state);
                for (int i = 0; i < 8; i++) {
                    if ((previous_coils_state & (1 << i)) != (current_coils_state & (1 << i))) {
                        if ((current_coils_state & (1 << i))) {
                            relay_on(i);
                        } else {
                            relay_off(i);
                        }
                    }
                }
                led_signal_ack();
            }
        }

        if (flags & FLAG_SYNC_FROM_RELAY) {
             uint16_t actual_state = 0;
            for (int i = 0; i < 8; i++) {
                if (Relay_GetState(i)) {
                    actual_state |= (1 << i);
                }
            }
            usRegHolding[0] = actual_state;
            previous_coils_state = actual_state;
        } else {
            // Если флаг от реле не установлен, значит, мы обновили состояние реле из Modbus.
            // В этом случае нам нужно обновить previous_coils_state, чтобы избежать
            // ложного срабатывания при следующем уведомлении от Modbus.
            previous_coils_state = usRegHolding[0];
        }
    }
}
