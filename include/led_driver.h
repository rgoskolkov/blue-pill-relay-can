#ifndef LED_DRIVER_H
#define LED_DRIVER_H

// Сигнал подтверждения (старт, выполнение команды)
void led_signal_ack(void);

// Сообщить, что связь по Modbus жива
void led_signal_heartbeat(void);

// Единственная задача для управления светодиодом
void led_task(void *argument);

#endif // LED_DRIVER_H