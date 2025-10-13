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
    mHandler.xTypeHW = USART_HW_DMA;
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
// void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
//     BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//     extern modbusHandler_t *mHandlers[];
//     extern uint8_t numberHandlers;
//     for (int i = 0; i < numberHandlers; i++) {
//         if (mHandlers[i]->port == huart && mHandlers[i]->xTypeHW == USART_HW) {
//             RingAdd(&mHandlers[i]->xBufferRX, mHandlers[i]->dataRX);
//             HAL_UART_Receive_IT(mHandlers[i]->port, &mHandlers[i]->dataRX, 1);
//             portDISABLE_INTERRUPTS();
//             if (xTimerChangePeriodFromISR(mHandlers[i]->xTimerT35, T35, &xHigherPriorityTaskWoken) != pdPASS) {
//                 printf("Timer reset FAIL");                // Failed to change the timer period
//             }
//             portENABLE_INTERRUPTS();
//             break;
//         }
//     }
//     portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
// }

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	/* Modbus RTU TX callback BEGIN */
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	int i;
	for (i = 0; i < numberHandlers; i++ )
	{
	   	if (mHandlers[i]->port == huart  )
	   	{
	   		// notify the end of TX
	   		xTaskNotifyFromISR(mHandlers[i]->myTaskModbusAHandle, 0, eNoAction, &xHigherPriorityTaskWoken);
	   		break;
	   	}

	}
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );

	/* Modbus RTU TX callback END */

	/*
	 * Here you should implement the callback code for other UARTs not used by Modbus
	 *
	 * */

}


#if  ENABLE_USART_DMA ==  1
/*
 * DMA requires to handle callbacks for special communication modes of the HAL
 * It also has to handle eventual errors including extra steps that are not automatically
 * handled by the HAL
 * */


void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{

 int i;

 for (i = 0; i < numberHandlers; i++ )
 {
    	if (mHandlers[i]->port == huart  )
    	{

    		if(mHandlers[i]->xTypeHW == USART_HW_DMA)
    		{
    			while(HAL_UARTEx_ReceiveToIdle_DMA(mHandlers[i]->port, mHandlers[i]->xBufferRX.uxBuffer, MAX_BUFFER) != HAL_OK)
    		    {
    					HAL_UART_DMAStop(mHandlers[i]->port);
   				}
				__HAL_DMA_DISABLE_IT(mHandlers[i]->port->hdmarx, DMA_IT_HT); // we don't need half-transfer interrupt

    		}

    		break;
    	}
   }
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		/* Modbus RTU RX callback BEGIN */
	    int i;
	    for (i = 0; i < numberHandlers; i++ )
	    {
	    	if (mHandlers[i]->port == huart  )
	    	{


	    		if(mHandlers[i]->xTypeHW == USART_HW_DMA)
	    		{
	    			if(Size) //check if we have received any byte
	    			{
		    				mHandlers[i]->xBufferRX.u8available = Size;
		    				mHandlers[i]->xBufferRX.overflow = false;

		    				while(HAL_UARTEx_ReceiveToIdle_DMA(mHandlers[i]->port, mHandlers[i]->xBufferRX.uxBuffer, MAX_BUFFER) != HAL_OK)
		    				{
		    					HAL_UART_DMAStop(mHandlers[i]->port);

		    				}
		    				__HAL_DMA_DISABLE_IT(mHandlers[i]->port->hdmarx, DMA_IT_HT); // we don't need half-transfer interrupt

		    				xTaskNotifyFromISR(mHandlers[i]->myTaskModbusAHandle, 0 , eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
	    			}
	    		}

	    		break;
	    	}
	    }
	    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

#endif
