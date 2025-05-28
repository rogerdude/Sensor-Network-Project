#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/shell/shell.h>
#include <thingy52_sensors.h>
#include <thingy52_gas_colour.h>
#include <math.h>
#include <myconfig.h>

#define BYTES_PER_SENSOR 17

static const char* sensor_uuid = SENSOR_UUID;
static const char* mobile_uuid = MOBILE_UUID;

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0f, 0x18)  
};

static int8_t id = -1;
static uint8_t stopped = 1;
static bool once = true;

K_MUTEX_DEFINE(id_mutex);

static int cmd_set_id(const struct shell *sh, size_t argc, char **argv) {

    k_mutex_lock(&id_mutex, K_FOREVER);
    id = atoi(argv[1]);
    stopped = 0;
    k_mutex_unlock(&id_mutex);

    return 0;
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf) {

    if (buf->len < (2 + UUID_SIZE)) {
        return;
    }

    char name[7];
	for (int i = 2; i < (2 + UUID_SIZE); i++) {
        name[i - 2] = buf->data[i];
    }
    name[6] = '\0';

    if (strcmp(name, mobile_uuid) == 0) {
        for (int i = 8; i < buf->len; i += BYTES_PER_SENSOR) {
            // check if enabled
            if (id == buf->data[i]) {
                if (stopped != buf->data[i + 1]) {
                    stopped = buf->data[i + 1];
                    once = false;
                }
                break;
            }
        }
    }
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

int start_adv(struct bt_le_ext_adv *adv) {
	int err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
	} else {
		printk("Starting Extended Advertising\n");
	}
	return err;
}

int stop_adv(struct bt_le_ext_adv *adv) {
	int err = bt_le_ext_adv_stop(adv);
	if (err) {
		printk("Failed to stop extended advertising\n");
	} else {
		printk("Stopped Extended Advertising\n");
	}
	return err;
}

int main(void) {

    if (thingy52_rgb_init() != 0) {
        printk("RGB LED couldn't be initialised\n");
        return -1;
    } else {
        printk("RGB LED initialised\n");
        thingy52_rgb_colour_set(RED);
    }

    if (init_thingy52_sensors() != 0) {
        printk("Sensors couldn't be initialised\n");
        return -1;
    } else {
        printk("Sensors initialised\n");
    }

    int err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialised\n");

    struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_PASSIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};

    bt_le_scan_cb_register(&scan_callbacks);

	err = bt_le_scan_start(&scan_param, NULL);
	if (err) {
		printk("Start scanning failed (err %d)\n", err);
		return 0;
	}
	printk("Started scanning...\n");

	/* Start advertising */
    struct bt_le_ext_adv *adv;

    /* Create a connectable advertising set */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
	if (err) {
		printk("Failed to create advertising set (err %d)\n", err);
		return err;
	}

	/* Set advertising data to have complete local name set */
	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Failed to set advertising data (err %d)\n", err);
		return 0;
	}

	start_adv(adv);

    struct SensorData data;
    struct SensorValues values;

    uint8_t allData[29] = {0};

    while (1) {
        stop_adv(adv);

        if (stopped) {
            if (!once) {
                stop_adv(adv);
                if (err) {
                    printk("Advertising failed to stop (err %d)\n", err);
                } else {
                    printk("Advertising stopped.\n");
                }
                once = true;
            }
            k_msleep(200);
            continue;
        } else {
            if (!once) {
                start_adv(adv);
                once = true;
            }
        }

        thingy52_process_sample(&data);
        thingy52_convert_data(&data, &values);
        
        float acc_combined = sqrtf(powf(values.accel[0], 2.0) + powf(values.accel[1], 2.0) + powf(values.accel[1], 2.0));
        int acc_int = (int) acc_combined;
        int acc_dec = (int) ((acc_combined - (float) acc_int) * 100);

        printk("temp: %.1f C, hum: %.1f %%, gas: %.1f eTVOC, accel total: %.2f, x: %.1f, y: %.1f, z: %.1f\n",
            values.temp, values.hum, values.gas, (double) acc_combined,
            values.accel[0], values.accel[1], values.accel[2]);

        for (int i = 0; i < UUID_SIZE; i++) {
            allData[i] = sensor_uuid[i];
        }
        k_mutex_lock(&id_mutex, K_FOREVER);
        allData[UUID_SIZE] = id;
        k_mutex_unlock(&id_mutex);

        allData[UUID_SIZE + 1] = data.temp.val1;
        allData[UUID_SIZE + 2] = data.hum.val1;
        allData[UUID_SIZE + 3] = data.gas.val1 >> 8;
        allData[UUID_SIZE + 4] = data.gas.val1 & 0xFF;
        allData[UUID_SIZE + 5] = acc_int;
        allData[UUID_SIZE + 6] = acc_dec;

        struct bt_data ad1[] = {
            BT_DATA(BT_DATA_MANUFACTURER_DATA, allData, 29)
        };

        for (int i = 0; i < 29; i++) {
            printk("%d ", allData[i]);
        }
        printk("\n");

        err = bt_le_ext_adv_set_data(adv, ad1, ARRAY_SIZE(ad1), NULL, 0);
		if (err) {
			printk("Failed to set advertising data (err %d)\n", err);
			return 0;
		}

		start_adv(adv);

        k_msleep(1000);
    }

    return 0;
}

SHELL_CMD_ARG_REGISTER(set_id, NULL, "Set sensor node's ID", cmd_set_id, 2, 0);