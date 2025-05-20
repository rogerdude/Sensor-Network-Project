#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/json/json.h>
#include <string.h>

#define TEMP_HUMIDITY_NODE DT_ALIAS(st_hts221)
static const struct device *temp_humidity_dev = DEVICE_DT_GET_ONE(TEMP_HUMIDITY_NODE);
static struct sensor_value temp, hum;

struct SensorJSON {
    int t;
    int h;
    int g;
};

static const struct json_obj_descr sensor_descr[] = {
    JSON_OBJ_DESCR_PRIM(struct SensorJSON, t, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct SensorJSON, h, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct SensorJSON, g, JSON_TOK_NUMBER),
};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0f, 0x18)  
};

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
	struct net_buf_simple *ad) {
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
	
	char mac_addr[18];
	for (int i = 0; i < 17; i++) {
		mac_addr[i] = addr_str[i];
	}
	mac_addr[17] = '\0';

}

static void read_temp(void) {
    if (!device_is_ready(temp_humidity_dev)) {
        printk("Temp/Hum sensor not ready\n");
        return;
    }
    if (sensor_sample_fetch(temp_humidity_dev) < 0) {
        printk("Failed to fetch temp sample\n");
        return;
    }
    if (sensor_channel_get(temp_humidity_dev,
                           SENSOR_CHAN_AMBIENT_TEMP,
                           &temp) < 0) {
        printk("Failed to get temp channel\n");
    }
}

static void read_hum(void) {
    if (!device_is_ready(temp_humidity_dev)) {
        printk("Temp/Hum sensor not ready\n");
        return;
    }
    if (sensor_sample_fetch(temp_humidity_dev) < 0) {
        printk("Failed to fetch hum sample\n");
        return;
    }
    if (sensor_channel_get(temp_humidity_dev,
                           SENSOR_CHAN_HUMIDITY,
                           &hum) < 0) {
        printk("Failed to get hum channel\n");
    }
}

static int process_gas(void) {
	//hamza code
    return 0;
}

int main(void) {
	int err; 
	
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialised\n");

	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_PASSIVE,
		.options    = BT_LE_SCAN_OPT_FILTER_DUPLICATE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
		NULL, 0);

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}

	while (1) {
        read_temp();  
        read_hum();    

        int tvoc = process_gas();

		struct SensorJSON data = {
            .t = temp.val1,
            .h = hum.val1,
            .g = tvoc
        };

		char jsonBuf[128];
        int ret = json_obj_encode_buf(sensor_descr,
                                          ARRAY_SIZE(sensor_descr),
                                          &data,
                                          jsonBuf,
                                          sizeof(jsonBuf));
        if (ret != 0) {
			printk("JSON error\n");
		} else {
			printk("%s\n", jsonBuf);
		}

        struct bt_data adv[] = {
            BT_DATA_BYTES(BT_DATA_FLAGS,
                          (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
            BT_DATA(BT_DATA_MANUFACTURER_DATA,
                    (const uint8_t *)jsonBuf, (size_t)ret),
        };

        err = bt_le_adv_update_data(adv, ARRAY_SIZE(adv), NULL, 0);

        if (err) {
            printk("adv_update failed: %d\n", err);
        } else {
            printk("ADV -> %s\n", json);
        }

        // wait 0.1 second 
        k_msleep(100);
    }
}