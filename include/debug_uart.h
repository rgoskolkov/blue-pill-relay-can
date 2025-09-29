#ifndef DEBUG_UART_H
#define DEBUG_UART_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

void debug_uart_init(void);
void debug_uart_send_byte(uint8_t byte);
void debug_uart_send_string(const char* str);
void debug_uart_send_hex(uint8_t byte);

#endif /* DEBUG_UART_H */