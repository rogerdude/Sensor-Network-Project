
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
    char temp[10], hum[10], gas[10], acc_x[10], acc_y[10], acc_z[10];
};

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
        0x0f, 0x18)  
};

static const char* mobile_mac =	"D8:7F:34:A5:D7:B4";

struct SensorValues values[NUM_OF_SENSORS];

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
			values[i].temp = (double) ad->data[5 + (12 * i)] + (double) ad->data[6 + (12 * i)] / 10;
			values[i].hum = (double) ad->data[7 + (12 * i)] + (double) ad->data[8 + (12 * i)] / 10;
			values[i].gas = (double) ((ad->data[9 + (12 * i)] << 8) | ad->data[10 + (12 * i)]);
			values[i].accel[0] = (double) ad->data[11 + (12 * i)] + (double) ad->data[12 + (12 * i)] / 10;
			values[i].accel[1] = (double) ad->data[13 + (12 * i)] + (double) ad->data[14 + (12 * i)] / 10;
			values[i].accel[2] = (double) ad->data[15 + (12 * i)] + (double) ad->data[16 + (12 * i)] / 10;
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
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, temp, JSON_TOK_STRING),
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, hum, JSON_TOK_STRING),
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, gas, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM(struct SensorJSON, acc_x, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM(struct SensorJSON, acc_y, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM(struct SensorJSON, acc_z, JSON_TOK_STRING),
	};

	while (1) {

		struct SensorJSON jsonData;

		sprintf(jsonData.temp, "%.1f", values[])

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
 