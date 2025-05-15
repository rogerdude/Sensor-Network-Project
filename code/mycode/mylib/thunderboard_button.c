#include "thunderboard_button.h"
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback button0_cb_data;

/* User-provided callback to be invoked on button press */
static thunderboard_button_press_cb_t user_callback = NULL;

/* Internal button interrupt handler */
static void button0_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    printk("Button pressed\n");

    if (user_callback) {
        user_callback();
    }
}

int thunderboard_button_init(void) {
    int ret;

    if (!gpio_is_ready_dt(&button0)) {
        printk("Button device not ready.\n");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&button0, GPIO_INPUT | GPIO_PULL_UP);
    if (ret != 0) {
        printk("Failed to configure button: %d\n", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&button0, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        printk("Failed to configure button interrupt: %d\n", ret);
        return ret;
    }

    gpio_init_callback(&button0_cb_data, button0_pressed, BIT(button0.pin));
    gpio_add_callback(button0.port, &button0_cb_data);

    printk("Thunderboard button configured\n");
    return 0;
}

void thunderboard_button_set_callback(thunderboard_button_press_cb_t cb)
{
    user_callback = cb;
}
