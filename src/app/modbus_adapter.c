#include "modbus_adapter.h"
#include "Modbus.h"
#include "board_config.h"
#include "relay_driver.h"
#include "led_driver.h"
#include "FreeRTOS.h"
#include "task.h"

// --- Глобальные переменные ---
modbusHandler_t mHandler;
// 1 регистр для хранения состояния 8 коилов
uint16_t usRegHolding[1] = {0};

// Callback function to notify the sync task
void ModbusDataReceiveCallback(void) {
    if (syncTaskHandle != NULL) {
        osThreadFlagsSet(syncTaskHandle, FLAG_SYNC_FROM_MODBUS);
    }
}

void modbus_adapter_init(void)
{
    mHandler.uModbusType = MB_SLAVE;
    mHandler.port = &MODBUS_UART_HANDLE;
    mHandler.u8id = MODBUS_SLAVE_ID;
    mHandler.u16timeOut = 1000;
    mHandler.EN_Port = NULL;
    mHandler.xTypeHW = USART_HW;
    mHandler.u16regs = usRegHolding;
    mHandler.u16regsize = 8;

    // Assign callbacks
    mHandler.OnHeartbeat = led_signal_heartbeat;
    mHandler.OnDataReceive = ModbusDataReceiveCallback;

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
            previous_coils_state = current_coils_state;
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
        }
    }
}

/**
  * @brief  Rx Transfer completed callback. This is a strong implementation that overrides the weak one from HAL.
  * @param  huart: UART handle
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    extern modbusHandler_t *mHandlers[];
    extern uint8_t numberHandlers;
    for (int i = 0; i < numberHandlers; i++) {
        if (mHandlers[i]->port == huart && mHandlers[i]->xTypeHW == USART_HW) {
            RingAdd(&mHandlers[i]->xBufferRX, mHandlers[i]->dataRX);
            HAL_UART_Receive_IT(mHandlers[i]->port, &mHandlers[i]->dataRX, 1);
            if (xTimerResetFromISR(mHandlers[i]->xTimerT35, &xHigherPriorityTaskWoken) != pdPASS) {
                printf("Timer reset FAIL");
            }
            break;
        }
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    extern modbusHandler_t *mHandlers[];
    extern uint8_t numberHandlers;

	for (int i = 0; i < numberHandlers; i++ )
	{
	   	if (mHandlers[i]->port == huart)
	   	{
	   		// notify the end of TX
	   		xTaskNotifyFromISR(mHandlers[i]->myTaskModbusAHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
	   		break;
	   	}
	}
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}
