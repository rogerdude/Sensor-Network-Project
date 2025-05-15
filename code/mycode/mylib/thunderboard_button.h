#ifndef THUNDERBOARD_BUTTON_H
#define THUNDERBOARD_BUTTON_H

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

typedef void (*thunderboard_button_press_cb_t)(void);


int thunderboard_button_init(void);
void thunderboard_button_set_callback(thunderboard_button_press_cb_t cb);


#endif /* THUNDERBOARD_BUTTON_H */
