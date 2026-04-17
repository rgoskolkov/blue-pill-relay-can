#ifndef INPUT_DRIVER_H
#define INPUT_DRIVER_H
#include <stdint.h>
#include "cmsis_os.h"

void Input_Init(void);
void input_task(void *argument);

#endif