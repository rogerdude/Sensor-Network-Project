#include "sltb_sensors.h"
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/dsp/print_format.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

LOG_MODULE_REGISTER(sensor_module, LOG_LEVEL_DBG);

static struct mpsc sensor_queue = MPSC_INIT(sensor_queue);
struct sensor_event {
    struct mpsc_node node;
    uint8_t sensor_id;
    struct sensor_q31_data data;
};


const struct device *const dev  = DEVICE_DT_GET_ANY(bosch_bme280);
const struct device *const dev2 = DEVICE_DT_GET_ANY(silabs_si7006);
const struct device *const dev3 = DEVICE_DT_GET_ANY(silabs_si1133);

SENSOR_DT_READ_IODEV(iodev, DT_COMPAT_GET_ANY_STATUS_OKAY(bosch_bme280),
                     {SENSOR_CHAN_PRESS, 0});

SENSOR_DT_READ_IODEV(iodev2, DT_COMPAT_GET_ANY_STATUS_OKAY(silabs_si7006),
                     {SENSOR_CHAN_AMBIENT_TEMP, 0},
                     {SENSOR_CHAN_HUMIDITY, 0});

SENSOR_DT_READ_IODEV(iodev3, DT_COMPAT_GET_ANY_STATUS_OKAY(silabs_si1133),
                     {SENSOR_CHAN_LIGHT, 0});

RTIO_DEFINE(ctx, 1, 1);
RTIO_DEFINE(ctx2, 1, 1);
RTIO_DEFINE(ctx3, 1, 1);


int read_sensor_data(uint8_t did, struct sensor_q31_data *data) {
    uint8_t buf[128];
    int rc = 0;
    uint32_t fit = 0;
    struct sensor_chan_spec chan;
    const struct sensor_decoder_api *decoder;

    switch (did) {
    case SENSOR_DID_TEMP:
        rc = sensor_read(&iodev2, &ctx2, buf, sizeof(buf));
        if (rc != 0) {
            return rc;
        }
        rc = sensor_get_decoder(dev2, &decoder);
        if (rc != 0) {
            return rc;
        }
        chan = (struct sensor_chan_spec){SENSOR_CHAN_AMBIENT_TEMP, 0};
        break;
    case SENSOR_DID_HUMID:
        rc = sensor_read(&iodev2, &ctx2, buf, sizeof(buf));
        if (rc != 0) {
            return rc;
        }
        rc = sensor_get_decoder(dev2, &decoder);
        if (rc != 0) {
            return rc;
        }
        chan = (struct sensor_chan_spec){SENSOR_CHAN_HUMIDITY, 0};
        break;
    case SENSOR_DID_PRESS:
        rc = sensor_read(&iodev, &ctx, buf, sizeof(buf));
        if (rc != 0) {
            return rc;
        }
        rc = sensor_get_decoder(dev, &decoder);
        if (rc != 0) {
            return rc;
        }
        chan = (struct sensor_chan_spec){SENSOR_CHAN_PRESS, 0};
        break;
    case SENSOR_DID_LIGHT:
        rc = sensor_read(&iodev3, &ctx3, buf, sizeof(buf));
        if (rc != 0) {
            return rc;
        }
        rc = sensor_get_decoder(dev3, &decoder);
        if (rc != 0) {
            return rc;
        }
        chan = (struct sensor_chan_spec){SENSOR_CHAN_LIGHT, 0};
        break; 
    default:
        return -EINVAL;
    }

    decoder->decode(buf, chan, &fit, 1, data);
    return 0;
}

static inline float q31_to_float(q31_t q_val, uint8_t shift) {
    return (float)q_val / (float)(1LL << (31 - shift));
}

int get_sensor_values(sensor_values_t *values) {
    int rc;
    struct sensor_q31_data data;

    if (values == NULL) {
        return -EINVAL;
    }

    rc = read_sensor_data(SENSOR_DID_TEMP, &data);
    if (rc != 0) {
        LOG_ERR("Failed to read temperature: %d", rc);
        return rc;
    }
    values->temperature = q31_to_float(data.readings[0].temperature, data.shift);

    rc = read_sensor_data(SENSOR_DID_HUMID, &data);
    if (rc != 0) {
        LOG_ERR("Failed to read humidity: %d", rc);
        return rc;
    }
    values->humidity = q31_to_float(data.readings[0].humidity, data.shift);

    rc = read_sensor_data(SENSOR_DID_PRESS, &data);
    if (rc != 0) {
        LOG_ERR("Failed to read pressure: %d", rc);
        return rc;
    }
    values->pressure = q31_to_float(data.readings[0].pressure, data.shift) * 10.0f;

    rc = read_sensor_data(SENSOR_DID_LIGHT, &data);
    if (rc != 0) {
        LOG_ERR("Failed to read light: %d", rc);
        return rc;
    }
    values->light = q31_to_float(data.readings[0].light, data.shift);

    return 0;
}

/* Enqueue sensor data into the MPSC queue */
static int enqueue_sensor_data(uint8_t did) {
    struct sensor_event *event = k_malloc(sizeof(struct sensor_event));
    if (!event) {
        printk("Error: Failed to allocate sensor event\n");
        return -ENOMEM;
    }
    event->sensor_id = did;
    int rc = read_sensor_data(did, &event->data);
    if (rc != 0) {
        k_free(event);
        return rc;
    }
    /* Push the event into the queue */
    mpsc_push(&sensor_queue, &event->node);
    return 0;
}

int sltb_sensor_init(void)
{
    if (dev == NULL) {
        printk("\nError: no BME280 device found.\n");
        return -ENODEV;
    }
    if (!device_is_ready(dev)) {
        printk("\nError: Device \"%s\" is not ready\n", dev->name);
        return -ENODEV;
    }
    printk("Found device \"%s\"\n", dev->name);

    if (dev2 == NULL) {
        printk("\nError: no SI7006 device found.\n");
        return -ENODEV;
    }
    if (!device_is_ready(dev2)) {
        printk("\nError: Device \"%s\" is not ready\n", dev2->name);
        return -ENODEV;
    }
    printk("Found device \"%s\"\n", dev2->name);

    if (dev3 == NULL) {
        printk("\nError: no SI1133 device found.\n");
        return -ENODEV;
    }
    if (!device_is_ready(dev3)) {
        printk("\nError: Device \"%s\" is not ready\n", dev3->name);
        return -ENODEV;
    }
    printk("Found device \"%s\"\n", dev3->name);

    return 0;
}

/* Shell command: sensor <DID>
 * Enqueues a sensor event based on the provided device ID.
 */
int cmd_sensor(const struct shell *shell, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(shell, "Usage: sensor <DID>");
        shell_print(shell, "Available DIDs:");
        shell_print(shell, "  0 - Temperature");
        shell_print(shell, "  1 - Humidity");
        shell_print(shell, "  2 - Pressure");
        shell_print(shell, "  5 - Light");
        shell_print(shell, "  15 - All readings");
        return -EINVAL;
    }

    uint8_t did;
    int rc = 0;
    did = (uint8_t)strtol(argv[1], NULL, 10);
    if (did != SENSOR_DID_ALL &&
        did != SENSOR_DID_TEMP &&
        did != SENSOR_DID_HUMID &&
        did != SENSOR_DID_PRESS &&
        did != SENSOR_DID_LIGHT) {
        shell_error(shell, "Invalid DID: %d", did);
        return -EINVAL;
    }
    if (did == SENSOR_DID_ALL) {
        rc |= enqueue_sensor_data(SENSOR_DID_TEMP);
        rc |= enqueue_sensor_data(SENSOR_DID_HUMID);
        rc |= enqueue_sensor_data(SENSOR_DID_PRESS);
        rc |= enqueue_sensor_data(SENSOR_DID_LIGHT);
    } else {
        rc = enqueue_sensor_data(did);
    }

    if (rc != 0) {
        shell_error(shell, "Failed to enqueue sensor data: %d", rc);
        return rc;
    }

    return 0;
}


void sensor_print_thread(void *p1, void *p2, void *p3) {
    struct mpsc_node *node;
    while (1) {
        node = mpsc_pop(&sensor_queue);
        if (node == NULL) {
            k_msleep(100);
            continue;
        }
        /* Retrieve the sensor_event pointer from the mpsc node */
        struct sensor_event *event = CONTAINER_OF(node, struct sensor_event, node);

        switch (event->sensor_id) {
        case SENSOR_DID_TEMP:
            printk("Temperature: %s%d.%d C\n",
                   PRIq_arg(event->data.readings[0].temperature, 2, event->data.shift));
            break;
        case SENSOR_DID_HUMID:
            printk("Humidity: %s%d.%d %%\n",
                   PRIq_arg(event->data.readings[0].humidity, 2, event->data.shift));
            break;
        case SENSOR_DID_PRESS:
            printk("Pressure: %s%d.%d hPa\n",
                   PRIq_arg(event->data.readings[0].pressure * 10, 2, event->data.shift));
            break;
        case SENSOR_DID_LIGHT:
            printk("Light: %s%d.%d lux\n",
                   PRIq_arg(event->data.readings[0].light, 2, event->data.shift));
            break;
        default:
            printk("Invalid sensor id: %d\n", event->sensor_id);
        }
        k_free(event);
    }
}

/* Define the sensor print thread */
#define SENSOR_THREAD_STACK_SIZE 1024
#define SENSOR_THREAD_PRIORITY 7
K_THREAD_DEFINE(sensor_print_tid, SENSOR_THREAD_STACK_SIZE,
                sensor_print_thread, NULL, NULL, NULL,
                SENSOR_THREAD_PRIORITY, 0, 0);

SHELL_CMD_REGISTER(sensor, NULL, "Sensor commands\n"
                    "Usage:\n"
                    "  sensor 0    - Read temperature\n"
                    "  sensor 1    - Read humidity\n"
                    "  sensor 2    - Read pressure\n"
                    "  sensor 5    - Read Light\n"
                    "  sensor 15  - Read all values", cmd_sensor);