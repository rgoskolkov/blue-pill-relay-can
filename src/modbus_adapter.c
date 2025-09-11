#include "main.h"
#include "board_config.h"
#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <string.h>

#include "microtbx.h"
#include "microtbxmodbus.h"

#include "modbus_map.h"
#include "relay_driver.h"

/* helper: map human config -> microtbx constants */
static inline int tbx_map_port(int p) { return (p == 2) ? TBX_MB_UART_PORT2 : TBX_MB_UART_PORT1; }
static inline int tbx_map_stopbits(int s) { return (s == 2) ? TBX_MB_UART_2_STOPBITS : TBX_MB_UART_1_STOPBITS; }
static inline int tbx_map_parity(int p)
{
    if (p == 0)
        return TBX_MB_NO_PARITY;
    if (p == 1)
        return TBX_MB_ODD_PARITY;
    return TBX_MB_EVEN_PARITY;
}
static inline int tbx_map_baudrate(int baud)
{
    switch (baud)
    {
    case 9600:
        return TBX_MB_UART_9600BPS;
    case 19200:
        return TBX_MB_UART_19200BPS;
    case 38400:
        return TBX_MB_UART_38400BPS;
    case 57600:
        return TBX_MB_UART_57600BPS;
    case 115200:
        return TBX_MB_UART_115200BPS;
    default:
        return TBX_MB_UART_19200BPS;
    }
}

/* --------------------------------------------------------------------------
   Callbacks: интеграция с modbus_map.c
   - Чтение coil'ов (релеи) => modbus_map_get_coil / Relay_GetState
   - Запись coil'ов => Relay_SetState + обновление кеша (modbus_map_update_registers)
   - Чтение input-registers (пример: 30000/30001) — возвращаем константы, можно заменить на API modbus_map
   --------------------------------------------------------------------------*/

/* Read single coil (coil address interpreted relative to RELAY_1_ADDRESS constants in modbus_map.c) */
tTbxMbServerResult ModbusReadCoil(tTbxMbServer channel, uint16_t addr, uint8_t *value)
{
    for (uint8_t i = 0; i < NUM_SWITCHES; ++i)
    {
        if (addr == (RELAY_1_ADDRESS + i))
        {
            *value = modbus_map_get_coil(i); /* returns 0/1 */
            return TBX_MB_SERVER_OK;
        }
    }
    return TBX_MB_SERVER_ERR_ILLEGAL_DATA_ADDR;
}

/* Write single coil */
tTbxMbServerResult ModbusWriteCoil(tTbxMbServer channel, uint16_t addr, uint8_t value)
{
    for (uint8_t i = 0; i < NUM_SWITCHES; ++i)
    {
        if (addr == (RELAY_1_ADDRESS + i))
        {
            /* value expected 0 or 1 */
            Relay_SetState(i, value ? 1 : 0);
            /* обновим внутренний кэш перед отдачей следующего запроса */
            modbus_map_update_registers();
            return TBX_MB_SERVER_OK;
        }
    }
    return TBX_MB_SERVER_ERR_ILLEGAL_DATA_ADDR;
}

/* Read input register (example from tutorial). Возвращаем две тестовые величины.
   При желании замените реализацией, читающей значения из modbus_map (если у вас есть input registers). */
tTbxMbServerResult ModbusReadInputReg(tTbxMbServer channel, uint16_t addr, uint16_t *value)
{
    switch (addr)
    {
    case 30000U:
        *value = 1234U;
        return TBX_MB_SERVER_OK;
    case 30001U:
        *value = 5678U;
        return TBX_MB_SERVER_OK;
    default:
        return TBX_MB_SERVER_ERR_ILLEGAL_DATA_ADDR;
    }
}

/* --------------------------------------------------------------------------*/
/* Modbus adapter init / HAL callbacks                                        */
/* --------------------------------------------------------------------------*/

tTbxMbTp modbusTp;
tTbxMbServer modbusServer;

void ModbusAdapter_Init(void)
{
    /* Не стартуем HAL_UART_Receive_IT() здесь — это делает порт (TbxMbPortUartInit). */
    int tbx_port = tbx_map_port(MODBUS_UART_PORT);
    int tbx_stop = tbx_map_stopbits(MODBUS_UART_STOPBITS);
    int tbx_par = tbx_map_parity(MODBUS_UART_PARITY);
    int tbx_baud = tbx_map_baudrate(MODBUS_BAUDRATE);

    modbusTp = TbxMbRtuCreate(MODBUS_SLAVE_ID, tbx_port, tbx_baud, tbx_stop, tbx_par);
    modbusServer = TbxMbServerCreate(modbusTp);

    /* регистрация application callbacks */
    TbxMbServerSetCallbackReadCoil(modbusServer, ModbusReadCoil);
    TbxMbServerSetCallbackWriteCoil(modbusServer, ModbusWriteCoil);
    TbxMbServerSetCallbackReadInputReg(modbusServer, ModbusReadInputReg);
}

/* Poll/task вызов для стека: вызывать в main loop */
void ModbusAdapter_Poll(void)
{
    TbxMbEventTask();
}