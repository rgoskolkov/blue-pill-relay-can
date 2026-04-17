#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include "stdio.h"

// Сигнал подтверждения (старт, выполнение команды)
void led_signal_ack(void);

// Сигнал о приёме CAN-фрейма (можно вызывать из прерывания)
void led_signal_can_rx_from_isr(void);

void dead_hand(uint32_t delay, uint32_t count);

// Единственная задача для управления светодиодом
void led_task(void *argument);

#endif // LED_DRIVER_H
