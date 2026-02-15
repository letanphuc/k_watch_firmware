#ifndef LED_H
#define LED_H

#include <stdint.h>
int led_init(void);
int led_set_color(uint8_t r, uint8_t g, uint8_t b);

#endif /* LED_H */
