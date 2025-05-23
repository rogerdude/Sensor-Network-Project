#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>  
#include <zephyr/data/json.h>
#include <string.h>

#define NUM_OF_SENSORS 2

struct SensorVal {
	float lat, lon;
    int temp, hum, gas;
	float acc;
};

struct SensorJSON {
	float lat, lon, acc;
    int temp, hum, gas;
};

static const char* mobile_mac =	"D8:7F:34:A5:D7:B4";

struct SensorVal values[NUM_OF_SENSORS];

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
	struct net_buf_simple *ad) {
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));

	if (ad->len < 28) {
		return;
	}

	char mac_addr[18];
	for (int i = 0; i < 17; i++) {
		mac_addr[i] = addr_str[i];
	}
	mac_addr[17] = '\0';

	if (strcmp(mac_addr, mobile_mac) == 0) {
		for (int i = 0; i < NUM_OF_SENSORS; i++) {
			values[i].lat = (double) ad->data[0 + (6 * i)] + (double) ad->data[1 + (6 * i)] / 100 +
				(double) ad->data[2 + (6 * i)] / 10000 + (double) ad->data[3 + (6 * i)] / 1000000;
				
			values[i].lon = (double) ad->data[4 + (6 * i)] + (double) ad->data[5 + (6 * i)] / 100 +
				(double) ad->data[6 + (6 * i)] / 10000 + (double) ad->data[7 + (6 * i)] / 1000000;
			
			values[i].temp = ad->data[8 + (6 * i)];
			values[i].hum = ad->data[9 + (6 * i)];
			values[i].gas = (ad->data[10 + (6 * i)] << 8) | ad->data[11 + (6 * i)];
			values[i].acc = (double) ad->data[12 + (6 * i)] + (double) ad->data[13 + (6 * i)] / 100;
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
		JSON_OBJ_DESCR_PRIM(struct SensorJSON, lat, JSON_TOK_FLOAT_FP),
		JSON_OBJ_DESCR_PRIM(struct SensorJSON, lon, JSON_TOK_FLOAT_FP),
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, temp, JSON_TOK_INT),
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, hum, JSON_TOK_INT),
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, gas, JSON_TOK_INT),
		JSON_OBJ_DESCR_PRIM(struct SensorJSON, acc, JSON_TOK_FLOAT_FP)
	};

	while (1) {

		for (int i = 0; i < NUM_OF_SENSORS; i++) {
			struct SensorJSON jsonData;
			jsonData.lat = values[i].lat;
			jsonData.lon = values[i].lon;
			jsonData.temp = values[i].temp;
			jsonData.hum = values[i].hum;
			jsonData.gas = values[i].gas;
			jsonData.acc = values[i].acc;

			char jsonBuf[128];
			int ret = json_obj_encode_buf(sensor_descr,
											ARRAY_SIZE(sensor_descr),
											&jsonData,
											jsonBuf,
											sizeof(jsonBuf));
			if (ret != 0) {
				printk("JSON error\n");
			} else {
				printk("%s\n", jsonBuf);
			}
		}
		
		k_msleep(500);
	} 
}
 