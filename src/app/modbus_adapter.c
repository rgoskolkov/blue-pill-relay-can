#include "modbus_adapter.h"
#include "Modbus.h"
#include "main.h"
#include "usart.h" 
#include "relay_driver.h"
#include "led_driver.h"
#include "board_config.h"
#include "cmsis_os2.h" // Используем CMSIS-RTOS v2 API для работы с семафором, предоставляемым Modbus
#include "stm32f1xx.h"
#include "FreeRTOS.h"

// --- Глобальные переменные ---
modbusHandler_t mHandler;
// 1 регистр для хранения состояния 8 коилов
uint16_t usRegHolding[1] = {0}; 

// forward declaration for diagnostic task
void modbus_diag_task(void *argument);
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
    uint16_t previous_coils_state = 0;

    for(;;)
    {
        // Пытаемся захватить семафор, который защищает доступ к u16regs
        // mHandler.ModBusSphrHandle — CMSIS-RTOS семафор, используем osSemaphoreAcquire
        if (osSemaphoreAcquire(mHandler.ModBusSphrHandle, 10) == osOK)
         {
             uint16_t current_coils_state = usRegHolding[0];

            // 1. ПРОВЕРКА ЗАПИСИ (Master -> Slave)
            if (previous_coils_state != current_coils_state)
            {
                for (int i = 0; i < 8; i++)
                {
                    if ((previous_coils_state & (1 << i)) != (current_coils_state & (1 << i)))
                    {
                        if ((current_coils_state & (1 << i))) {
                            relay_on(i);
                        } else {
                            relay_off(i);
                        }
                    }
                }
                led_signal_ack();
            }

            // 2. ОБНОВЛЕНИЕ ДЛЯ ЧТЕНИЯ (Slave -> Master)
            uint16_t actual_state = 0;
            for (int i = 0; i < 8; i++) {
                if (Relay_GetState(i)) {
                    actual_state |= (1 << i);
                }
            }
            usRegHolding[0] = actual_state;
            previous_coils_state = actual_state;

            // Используем CMSIS-RTOS v2 API, библиотека создала семафор через osSemaphoreNew
            osSemaphoreRelease(mHandler.ModBusSphrHandle);
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}