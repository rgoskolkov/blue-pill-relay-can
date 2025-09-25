#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

extern QueueHandle_t short_blink_queue;
extern QueueHandle_t long_blink_queue;
extern QueueHandle_t heartbeat_queue;
extern QueueHandle_t error_signal_queue;



/* Короткое мигание (быстрое) */
void start_Short_LED_Blink(uint8_t count);
/* Длинное мигание (медленное) */
void start_Long_LED_Blink(uint8_t count);
/* Сброс таймера Heartbeat */
void reset_Heartbeat_Timer(void);

/* Сигнал об ошибке */
void signal_error(void);

void short_blink_task(void *argument);
void long_blink_task(void *argument);
void heartbeat_task(void *argument);
void error_signal_task(void *argument);


#endif