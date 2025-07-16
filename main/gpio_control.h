#ifndef GPIO_CONTROL_H
#define GPIO_CONTROL_H

#include <stdbool.h>

// Function prototypes
void gpio_control_init(void);
void gpio_control_set_relay(bool state);
void gpio_control_set_blue_led(bool state);
bool gpio_control_get_button_state(void);

#endif // GPIO_CONTROL_H
