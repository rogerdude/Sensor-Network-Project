#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <thingy52_sensors.h>
#include <thingy52_gas_colour.h>

#ifndef IBEACON_RSSI
#define IBEACON_RSSI 0xc8
#endif

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0f, 0x18)  
};

// static const struct bt_data sd[] = {
// 	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME)),
// };

int main(void) {

    if (thingy52_rgb_init() != 0) {
        printk("RGB LED couldn't be initialised\n");
        return -1;
    } else {
        printk("RGB LED initialised\n");
        thingy52_rgb_colour_set(MAGENTA);
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

	/* Start advertising */
	// err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
	// 	sd, ARRAY_SIZE(sd));
    err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
		NULL, 0);

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

    struct SensorData data;
    struct SensorValues values;

    while (1) {

        thingy52_process_sample(&data);
        thingy52_convert_data(&data, &values);

        printk("temp: %.1f C, hum: %.1f %%, gas: %.1f eTVOC, accel x: %.1f, y: %.1f, z: %.1f\n",
            values.temp, values.hum, values.tvoc,
            values.accel[0], values.accel[1], values.accel[2]);

        struct bt_data adv[] = {
            BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
            BT_DATA_BYTES(BT_DATA_MANUFACTURER_DATA,
                    data.temp.val1,
                    data.temp.val2,
                    data.hum.val1,
                    data.hum.val2,
                    data.tvoc.val1,
                    data.tvoc.val2,
                    data.accel[0].val1,
                    data.accel[0].val2,
                    data.accel[1].val1,
                    data.accel[1].val2,
                    data.accel[2].val1,
                    data.accel[2].val2,
                    0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00,
                    IBEACON_RSSI),
        };

        err = bt_le_adv_update_data(adv, ARRAY_SIZE(adv), NULL, 0);
        // err = bt_le_adv_update_data(adv, ARRAY_SIZE(adv), sd, ARRAY_SIZE(sd));

        if (err) {
            printk("adv_update failed: %d\n", err);
        }

        k_msleep(1000);
    }

    return 0;
}