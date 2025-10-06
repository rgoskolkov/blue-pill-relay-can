#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include "stdio.h"

// Сигнал подтверждения (старт, выполнение команды)
void led_signal_ack(void);

// Сообщить, что связь по Modbus жива
void led_signal_heartbeat(void);

void dead_hand(uint32_t delay, uint32_t count);

// Единственная задача для управления светодиодом
void led_task(void *argument);

#endif // LED_DRIVER_H