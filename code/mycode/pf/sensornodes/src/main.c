#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
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

static uint8_t id = 1;
static uint8_t stopped = 0;

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
	struct net_buf_simple *ad) {
    
    char name[7];
	for (int i = 2; i < (2 + UUID_SIZE); i++) {
        name[i - 2] = ad->data[i];
    }
    name[6] = '\0';

    if (strcmp(name, mobile_uuid) == 0) {
        for (int i = 8; i < ad->len; i += BYTES_PER_SENSOR) {
            // check if enabled
            if (id == ad->data[i]) {
                stopped = ad->data[i + 1];
                break;
            }
        }
    }
}

int main(void) {

    if (thingy52_rgb_init() != 0) {
        printk("RGB LED couldn't be initialised\n");
        return -1;
    } else {
        printk("RGB LED initialised\n");
        thingy52_rgb_colour_set(BLUE);
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

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found);
	if (err) {
		printk("Start scanning failed (err %d)\n", err);
		return 0;
	}
	printk("Started scanning...\n");

	/* Start advertising */
    err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
		NULL, 0);

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

    struct SensorData data;
    struct SensorValues values;

    uint8_t allData[29] = {0};

    while (1) {

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
        allData[UUID_SIZE] = id;
        allData[UUID_SIZE + 1] = data.temp.val1;
        allData[UUID_SIZE + 2] = data.hum.val1;
        allData[UUID_SIZE + 3] = data.gas.val1 >> 8;
        allData[UUID_SIZE + 4] = data.gas.val1 & 0xFF;
        allData[UUID_SIZE + 5] = acc_int;
        allData[UUID_SIZE + 6] = acc_dec;

        struct bt_data adv[] = {
            BT_DATA(BT_DATA_MANUFACTURER_DATA, allData, 29)
        };

        err = bt_le_adv_update_data(adv, ARRAY_SIZE(adv), NULL, 0);

        if (err) {
            printk("adv_update failed: %d\n", err);
        }

        k_msleep(1000);
    }

    return 0;
}