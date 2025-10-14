#ifndef INPUT_DRIVER_H
#define INPUT_DRIVER_H
#include <stdint.h>
#include "cmsis_os.h"

extern osMessageQueueId_t switchEventQueueHandle;

void Input_Init(void);
void input_task(void *argument);
void process_switch_event(uint8_t i);

#endif