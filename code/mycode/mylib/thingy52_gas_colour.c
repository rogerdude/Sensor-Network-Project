#include "thingy52_gas_colour.h"
#include <thingy52_sensors.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

struct k_poll_signal gasSignal;

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

    k_poll_signal_init(&gasSignal);

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

void thread_gas_col_control(void) {

    while (1) {
        // Receive sensor values from thread
        //k_poll_signal_raise(&sensorSignal, 0);
        
        struct SensorValues recv;
        uint16_t my_type;
        uint8_t  my_value;
        uint8_t  my_size = DATA_SIZE_32;
        int ret = -EAGAIN;

        k_mutex_lock(&sensorMutex, K_FOREVER);
        while (ret == -EAGAIN) {
            k_poll_signal_raise(&sensorSignal, 0);
            ret = ring_buf_item_get(&buffer.rb, &my_type, &my_value, (uint32_t*) &recv, &my_size);
            k_msleep(20);
        }
        k_mutex_unlock(&sensorMutex);

        if (ret == -EMSGSIZE) {
            printk("Buffer is too small, need %d uint32_t", my_size);
            continue;
        }

        k_poll_signal_raise(&gasSignal, recv.tvoc);

        ring_buf_reset(&buffer.rb);

        // int tvoc = process_gas();

        // k_poll_signal_raise(&gasSignal, tvoc);

        k_msleep(SAMPLING_PERIOD);
    }
}

void thread_gas_col_set(void) {
    struct k_poll_event events[1] = {
        K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                K_POLL_MODE_NOTIFY_ONLY,
                                &gasSignal)
    };

    while (1) {
        k_poll(events, 1, K_FOREVER);

        int signaled, value;
        k_poll_signal_check(&gasSignal, &signaled, &value);

        int base = 250 / 6;

        if (signaled) {
            if (value < (base * 1)) {
                thingy52_rgb_colour_set(MAGENTA);
            } else if (value < (base * 2)) {
                thingy52_rgb_colour_set(BLUE);
            } else if (value < (base * 3)) {
                thingy52_rgb_colour_set(GREEN);
            } else if (value < (base * 4)) {
                thingy52_rgb_colour_set(CYAN);
            } else if (value < (base * 5)) {
                thingy52_rgb_colour_set(WHITE);
            } else if (value < (base * 6)) {
                thingy52_rgb_colour_set(YELLOW);
            } else {
                thingy52_rgb_colour_set(RED);
            }
        }

        k_poll_signal_reset(&gasSignal);
        k_msleep(50);
    }
}