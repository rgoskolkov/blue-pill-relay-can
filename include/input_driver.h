#ifndef INPUT_DRIVER_H
#define INPUT_DRIVER_H
#include "board_config.h"
#include <stdint.h>
#include <stdbool.h>

void Input_Init(void);
void process_switch_event(uint8_t i);

#endif