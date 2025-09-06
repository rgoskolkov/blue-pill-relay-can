#include "modbus_rtu.h"
#include "relay_driver.h"
#include "input_driver.h"
#include "stm32f1xx_hal.h"
#include <string.h>
#include <stdbool.h>

#ifndef MODBUS_SLAVE_ID
#define MODBUS_SLAVE_ID 1U
#endif

#define FRAME_TIMEOUT_MS 50U
#define RX_BUF_SIZE 256

extern UART_HandleTypeDef huart1; // должен быть определён в main.c / MX_USART1_UART_Init()

static uint8_t rx_byte;
static uint8_t rx_buf[RX_BUF_SIZE];
static uint16_t rx_len = 0;
static uint32_t last_rx_tick = 0;
static volatile bool rx_active = false;

/* CRC16 (Modbus) */
static uint16_t modbus_crc16(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < len; pos++)
    {
        crc ^= (uint16_t)buf[pos];
        for (uint8_t i = 0; i < 8; i++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* low-level send */
static void modbus_send_response(const uint8_t *data, uint16_t len)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 200);
}

/* frame processor */
static void process_modbus_frame(const uint8_t *frame, uint16_t len)
{
    if (len < 4)
        return; // too short
    uint16_t crc_calculated = modbus_crc16(frame, len - 2);
    uint16_t crc_received = frame[len - 2] | (frame[len - 1] << 8);
    if (crc_calculated != crc_received)
        return;

    uint8_t addr = frame[0];
    uint8_t func = frame[1];

    if (addr != MODBUS_SLAVE_ID && addr != 0)
    {
        return; // not for us (0 is broadcast)
    }

    if (func == 0x01)
    { // Read Coils
        if (len < 8)
            return;
        uint16_t start = (frame[2] << 8) | frame[3];
        uint16_t qty = (frame[4] << 8) | frame[5];
        if (qty == 0 || qty > 2000)
            return;

        uint16_t byte_count = (qty + 7) / 8;
        uint8_t resp[5 + byte_count + 2]; // addr func byte_count data... CRC(2)
        resp[0] = MODBUS_SLAVE_ID;
        resp[1] = 0x01;
        resp[2] = (uint8_t)byte_count;
        memset(&resp[3], 0, byte_count);

        for (uint16_t i = 0; i < qty; i++)
        {
            uint16_t coil_idx = start + i;
            uint8_t state = Relay_GetState(coil_idx);
            if (state)
            {
                resp[3 + (i >> 3)] |= (1 << (i & 7));
            }
        }
        uint16_t resp_len = 3 + byte_count;
        uint16_t crc = modbus_crc16(resp, resp_len);
        resp[resp_len++] = crc & 0xFF;
        resp[resp_len++] = (crc >> 8) & 0xFF;
        // For broadcast (addr==0) do not respond
        if (addr != 0)
            modbus_send_response(resp, resp_len);
    }
    else if (func == 0x05)
    { // Write Single Coil
        if (len < 8)
            return;
        uint16_t coil = (frame[2] << 8) | frame[3];
        uint16_t val = (frame[4] << 8) | frame[5];
        uint8_t on = (val == 0xFF00) ? 1 : 0;
        Relay_SetState(coil, on);
        // Echo request as normal response (unless broadcast)
        if (addr != 0)
        {
            modbus_send_response(frame, len);
        }
    }
    // Other function codes can be added here.
}

/* Called from HAL UART RX complete callback to collect bytes */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1)
        return;
    if (rx_len < RX_BUF_SIZE)
    {
        rx_buf[rx_len++] = rx_byte;
        last_rx_tick = HAL_GetTick();
        rx_active = true;
    }
    else
    {
        // buffer overflow -> reset
        rx_len = 0;
        rx_active = false;
    }
    // restart receiving next byte
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

void Modbus_Init(void)
{
    rx_len = 0;
    rx_active = false;
    last_rx_tick = 0;
    // start UART byte receive
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

void Modbus_Task(void)
{
    if (!rx_active)
        return;
    uint32_t now = HAL_GetTick();
    if (rx_len > 0 && (now - last_rx_tick) >= FRAME_TIMEOUT_MS)
    {
        // consider frame finished
        process_modbus_frame(rx_buf, rx_len);
        rx_len = 0;
        rx_active = false;
    }
}