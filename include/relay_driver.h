#ifndef RELAY_DRIVER_H
#define RELAY_DRIVER_H

#include <stdint.h>

/* Initialize relay hardware */
void relay_init(void); /* legacy alias */

/* Direct on/off helpers (optional) */
void relay_on(uint8_t relay_number);
void relay_off(uint8_t relay_number);
uint8_t relay_toggle(uint8_t relay_number);
uint8_t relay_getState(uint8_t relay_number);

#endif // RELAY_DRIVER_H