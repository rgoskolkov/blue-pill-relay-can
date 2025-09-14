#ifndef LED_DRIVER_H
#define LED_DRIVER_H

/* Короткое мигание (быстрое) */
void start_Short_LED_Blink(uint8_t count);
/* Длинное мигание (медленное) */
void start_Long_LED_Blink(uint8_t count);
void start_Heartbeat_Blink(void);

void process_LED_Blink(void);
void led_Blink_Init(void);

#endif