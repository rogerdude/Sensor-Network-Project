#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/shell/shell.h>  
#include <zephyr/data/json.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <zephyr/sys/byteorder.h>
#include <myconfig.h>

#define NUM_OF_SENSORS 2
#define BYTES_PER_SENSOR 17

struct SensorVal {
	int id, stopped;
	int32_t lat, lon;
    int temp, hum, gas;
	float acc;
};

struct SensorJSON {
	char *lat, *lon, *acc;
    int id, temp, hum, gas, severity;
};

static const char* mobile_uuid = MOBILE_UUID;

struct SensorVal values[NUM_OF_SENSORS];
uint8_t stopped[NUM_OF_SENSORS] = {0};
K_MUTEX_DEFINE(stopped_mutex);

static const struct bt_data adv_init[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x0f, 0x18)  
};

static int cmd_stop(const struct shell *sh, size_t argc, char **argv) {

    k_mutex_lock(&stopped_mutex, K_FOREVER);
    int id = atoi(argv[1]);
	if (id > NUM_OF_SENSORS) {
		shell_print(sh, "Invalid ID\n");
		return -1;
	}
	if (argv[2]) {
		stopped[id] = 1;
	} else {
		stopped[id] = 0;
	}
    k_mutex_unlock(&stopped_mutex);

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
			int id = buf->data[i];
			values[id].id = id;
            values[id].lat = buf->data[i + 2] | (buf->data[i + 3] << 8) |
				(buf->data[i + 4] << 16) | (buf->data[i + 5] << 24);
			
			values[id].lon = buf->data[i + 6] | (buf->data[i + 7] << 8) |
				(buf->data[i + 8] << 16) | (buf->data[i + 9] << 24);
			
			values[id].temp = buf->data[i + 10];
			values[id].hum = buf->data[i + 11];
			values[id].gas = (buf->data[i + 12] << 8) | buf->data[i + 13];
			values[id].acc = (double) buf->data[i + 14] + (double) buf->data[i + 15] / 100;
        }
    }
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

int get_severity(int temp, int hum, int gas, float acc) {
	if (temp > 30 || hum > 70 || gas > 500 || (double) acc > 2.0) {
		return 2; // High severity
	} else if (temp > 25 || hum > 50 || gas > 500 || (double) acc > 1.0) {
		return 1; // Medium severity
	}
	return 0; // Low severity
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

    bt_le_scan_cb_register(&scan_callbacks);

	err = bt_le_scan_start(&scan_param, NULL);
	if (err) {
		printk("Start scanning failed (err %d)\n", err);
		return 0;
	}
	printk("Started scanning...\n");

	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, adv_init, 0, NULL, 0);
    if (err) {
        printk("Advertising start failed (err %d)\n", err);
        return -1;
    }
    printk("Advertising started\n");

	static const struct json_obj_descr sensor_descr[] = {
		JSON_OBJ_DESCR_PRIM(struct SensorJSON, id, JSON_TOK_INT),
		JSON_OBJ_DESCR_PRIM(struct SensorJSON, lat, JSON_TOK_STRING),
		JSON_OBJ_DESCR_PRIM(struct SensorJSON, lon, JSON_TOK_STRING),
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, temp, JSON_TOK_INT),
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, hum, JSON_TOK_INT),
    	JSON_OBJ_DESCR_PRIM(struct SensorJSON, gas, JSON_TOK_INT),
		JSON_OBJ_DESCR_PRIM(struct SensorJSON, acc, JSON_TOK_STRING)
	};

	while (1) {

		uint8_t mfg_data[11 * NUM_OF_SENSORS];

		for (int i = 0; i < NUM_OF_SENSORS; i++) {
			struct SensorJSON jsonData;
			jsonData.id = i;
			char latBuf[20];
			sprintf(latBuf, "%.6f", (double) (values[i].lat / 1000000.0));
			char lonBuf[20];
			sprintf(lonBuf, "%.6f", (double) (values[i].lon / 1000000.0));
			char accBuf[20];
			sprintf(accBuf, "%f", (double) values[i].acc);
			jsonData.lat = latBuf;
			jsonData.lon = lonBuf;
			jsonData.temp = values[i].temp;
			jsonData.hum = values[i].hum;
			jsonData.gas = values[i].gas;
			jsonData.acc = accBuf;
			jsonData.severity = get_severity(values[i].temp, values[i].hum, values[i].gas, values[i].acc);

			// UART
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

			// BLE
			int32_t lat_enc = (int32_t)lroundf(values[i].lat * 1e6f);
			int32_t lon_enc = (int32_t)lroundf(values[i].lon * 1e6f);

			// 11 byte [id, stop, lat(4), lon(4), sev(1)]
			mfg_data[11 * i] = i;
			mfg_data[1 + (11 * i)] = stopped[i];
			sys_put_le32(lat_enc, &mfg_data[2 + (11 * i)]);
			sys_put_le32(lon_enc, &mfg_data[6 + (11 * i)]);
			mfg_data[10 + (11 * i)] = (uint8_t)jsonData.severity;
		}

		struct bt_data adv_data[] = {
				BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
				BT_DATA(BT_DATA_MANUFACTURER_DATA,
						mfg_data, 11 * NUM_OF_SENSORS),
		};

		err = bt_le_adv_update_data(adv_data,
									ARRAY_SIZE(adv_data),
									NULL, 0);
		if (err) {
			printk("adv_update failed: %d\n", err);
		}
		
		k_msleep(500);
	} 
}

SHELL_CMD_ARG_REGISTER(stop, NULL, "Start or stop sensor node\n Usage: stop <ID> <1 for stop/ 0 for start>", cmd_stop, 3, 0);