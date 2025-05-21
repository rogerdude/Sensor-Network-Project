
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>  

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
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
        0x0f, 0x18)  
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

int main(void) {
	int err;

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_PASSIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_GAP_SCAN_FAST_INTERVAL,
		.window     = BT_GAP_SCAN_FAST_WINDOW,
	};

	err = bt_le_scan_start(&scan_param, device_found);
	if (err) {
		printk("Start scanning failed (err %d)\n", err);
		return 0;
	}
	printk("Started scanning...\n");

	while (1) {

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
		
		k_msleep(500);
	}

 
}
 