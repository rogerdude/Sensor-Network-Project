#include <thingy52_gas_colour.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

int thingy52_rgb_init(void) {
    int ret1, ret2, ret3;

	if (!gpio_is_ready_dt(&red) || !gpio_is_ready_dt(&green) ||
            !gpio_is_ready_dt(&blue)) {
		return -1;
	}

	ret1 = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	ret2 = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
    ret3 = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
	if (ret1 < 0 || ret2 < 0 || ret3 < 0) {
		return -1;
	}

    return 0;
}

void thingy52_rgb_set(int r, int g, int b) {
    gpio_pin_set_dt(&red, r);
    gpio_pin_set_dt(&green, g);
    gpio_pin_set_dt(&blue, b);
}

void thingy52_rgb_colour_set(int colour) {
    switch (colour) {
        case BLUE:
            thingy52_rgb_set(OFF, OFF, ON);
            break;
        case GREEN:
            thingy52_rgb_set(OFF, ON, OFF);
            break;
        case CYAN:
            thingy52_rgb_set(OFF, ON, ON);
            break;
        case RED:
            thingy52_rgb_set(ON, OFF, OFF);
            break;
        case MAGENTA:
            thingy52_rgb_set(ON, OFF, ON);
            break;
        case YELLOW:
            thingy52_rgb_set(ON, ON, OFF);
            break;
        case WHITE:
            thingy52_rgb_set(ON, ON, ON);
            break;
        default:
            thingy52_rgb_set(OFF, OFF, OFF);
            break;
    }
}