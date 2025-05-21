
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>  
#include <zephyr/data/json.h>
#include <string.h>
#include <thingy52_sensors.h>

#define NUM_OF_SENSORS 2

struct SensorJSON {
    char temp[10], hum[10], gas[10], accel[10][3];
};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
        0x0f, 0x18)  
};

static const char* sensor_mac[NUM_OF_SENSORS] = {
	"C9:2B:FC:6A:D3:C0",
	"D1:31:A2:EB:63:2A"
};

uint8_t recv[NUM_OF_SENSORS][12];

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
	struct net_buf_simple *ad) {
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	if (ad->len != 30) {
		return;
	}

	char mac_addr[18];
	for (int i = 0; i < 17; i++) {
		mac_addr[i] = addr_str[i];
	}
	mac_addr[17] = '\0';

	for (int i = 0; i < NUM_OF_SENSORS; i++) {
		if (strcmp(mac_addr, sensor_mac[i]) == 0) {
			for (int j = 9; j < (9 + 12); j++) {
				recv[i][j - 9] = ad->data[j];
			}
			// values[i].temp = (double) ad->data[9] + (double) ad->data[10] / 10;
			// values[i].hum = (double) ad->data[11] + (double) ad->data[12] / 10;
			// values[i].gas = (double) ((ad->data[13] << 8) | ad->data[14]);
			// values[i].accel[0] = (double) ad->data[15] + (double) ad->data[16] / 10;
			// values[i].accel[1] = (double) ad->data[17] + (double) ad->data[18] / 10;
			// values[i].accel[2] = (double) ad->data[19] + (double) ad->data[20] / 10;
			break;
		}
	}
}

int main(void) {
	/* Initialize the Bluetooth Subsystem */
	int err = bt_enable(NULL);
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

	static const struct json_obj_descr sensor_descr[] = {
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, temp, JSON),
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, hum, JSON_TOK_NUMBER),
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, gas, JSON_TOK_NUMBER),
	};

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
 