
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <myconfig.h>

#define NUM_OF_SENSORS 		2
#define NUM_OF_SENSOR_BYTES 6
#define NUM_OF_LOC_BYTES 	9
#define NUM_OF_ID 			2
#define BYTES_PER_SENSOR 	(NUM_OF_SENSOR_BYTES + NUM_OF_LOC_BYTES + NUM_OF_ID)
#define TOTAL_BYTES 		(BYTES_PER_SENSOR * 2 + UUID_SIZE)

#define LAT 0
#define LON 1

// #define GNSS_MODEM DEVICE_DT_GET(DT_ALIAS(gnss))
// LOG_MODULE_REGISTER(gnss_sample, CONFIG_GNSS_LOG_LEVEL);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
        0x0f, 0x18)  
};

static const char* sensor_uuid = SENSOR_UUID;
static const char* mobile_uuid = MOBILE_UUID;
static const char* base_uuid = BASE_UUID;

uint8_t recv[NUM_OF_SENSORS][NUM_OF_SENSOR_BYTES + 1] = {0};
int32_t locations[NUM_OF_SENSORS][2] = {0};
int32_t currentlocation[2] = {0};
uint8_t stopped[NUM_OF_SENSORS] = {0};

struct k_poll_signal signal;
K_MUTEX_DEFINE(loc_signal);

static bool parse_gps(struct bt_data *data, void *user_data) {
    if (data->type == BT_DATA_MANUFACTURER_DATA && data->data_len >= 10) {
        const uint8_t *mdata = data->data;
        uint16_t comp_id = sys_get_le16(mdata);
        if (comp_id == 0xFFFF) {
            int32_t lat_i = sys_get_le32(mdata + 2);
            int32_t lon_i = sys_get_le32(mdata + 6);
            float lat = lat_i / 1e6f;
            float lon = lon_i / 1e6f;
            printk("[GPS] lat=%.6f, lon=%.6f\n", lat, lon);
            /* store latest GPS into locations[0] if desired */
            currentlocation[LAT] = lat_i;
            currentlocation[LON] = lon_i;
        }
    }
    return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
	struct net_buf_simple *ad) {

	if (ad->len < (2 + UUID_SIZE)) {
        return;
    }

	char name[7];
	for (int i = 2; i < (2 + UUID_SIZE); i++) {
        name[i - 2] = ad->data[i];
    }
    name[6] = '\0';

	bt_data_parse(ad, parse_gps, NULL);

    if (strcmp(name, sensor_uuid) == 0) {
		// Get id
		int id = ad->data[2];
		if (id < NUM_OF_SENSORS) {
			// Get data
			for (int j = 8; j < (8 + NUM_OF_SENSOR_BYTES); j++) {
				recv[id][j - 8] = ad->data[j];
			}
			// Update location
			if (rssi > (-50)) {
				// k_poll_signal_raise(&signal, id);
				locations[id][LAT] = currentlocation[LAT];
				locations[id][LON] = currentlocation[LON];
			}
		}
	} else if (strcmp(name, base_uuid) == 0) {
		// get stopped status from base
		for (int i = 8; i < ad->len; i += 11) {
			int id = ad->data[i];
			stopped[id] = ad->data[i + 1];
		}
	}
}

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

// static void gnss_data_cb(const struct device *dev, const struct gnss_data *data) {
// 	uint64_t timepulse_ns;
// 	k_ticks_t timepulse;

// 	if (data->info.fix_status != GNSS_FIX_STATUS_NO_FIX) {
// 		printk("lon: %lld, lat: %lld\n", data->nav_data.longitude, data->nav_data.latitude);

// 		if (gnss_get_latest_timepulse(dev, &timepulse) == 0) {
// 			timepulse_ns = k_ticks_to_ns_near64(timepulse);
// 			printf("Got a fix @ %lld ns\n", timepulse_ns);
// 		} else {
// 			printf("Got a fix!\n");
// 		}
// 	}
// }
// GNSS_DATA_CALLBACK_DEFINE(GNSS_MODEM, gnss_data_cb);

// void thread_gps(void) {

// 	struct k_poll_event events[1] = {
//         K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
//                                  K_POLL_MODE_NOTIFY_ONLY,
//                                  &signal),
//     };

// 	gnss_systems_t supported, enabled;
// 	uint32_t fix_interval;
// 	int rc;

// 	rc = gnss_get_supported_systems(GNSS_MODEM, &supported);
// 	if (rc < 0) {
// 		printf("Failed to query supported systems (%d)\n", rc);
// 	}
// 	rc = gnss_get_enabled_systems(GNSS_MODEM, &enabled);
// 	if (rc < 0) {
// 		printf("Failed to query enabled systems (%d)\n", rc);
// 	}
// 	rc = gnss_get_fix_rate(GNSS_MODEM, &fix_interval);
// 	if (rc < 0) {
// 		printf("Failed to query fix rate (%d)\n", rc);
// 	}
// 	printf("Fix rate = %d ms\n", fix_interval);

// 	while (1) {
// 		k_poll(events, 1, K_FOREVER);

//         int signaled, result;

//         k_poll_signal_check(&signal, &signaled, &result);

//         if (signaled && (result < NUM_OF_SENSORS)) {
//             int id = result;
// 			// Update location
// 			k_mutex_lock(&loc_signal, K_FOREVER);
// 			// code here

// 			k_mutex_unlock(&loc_signal);
//         } else {
//             printk("Signal error\n");
//         }

//         k_poll_signal_reset(&signal);
//         events[0].state = K_POLL_STATE_NOT_READY;

// 		k_msleep(500);
// 	}
// }

int main(void) {
	// k_poll_signal_init(&signal);

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

	uint8_t allData[TOTAL_BYTES] = {0};

	for (int i = 0; i < UUID_SIZE; i++) {
        allData[i] = mobile_uuid[i];
    }

	while (1) {
		stop_adv(adv);

		for (int i = 0; i < NUM_OF_SENSORS; i++) {
			int index = (i * BYTES_PER_SENSOR) + UUID_SIZE;
			// Get id and state
			allData[index] = i;
			allData[index + 1] = stopped[i];

			k_mutex_lock(&loc_signal, K_FOREVER);
			// Get lat
			allData[index + 2] = locations[i][LAT] & 0xFF;
			allData[index + 3] = (locations[i][LAT] >> 8) & 0xFF;
			allData[index + 4] = (locations[i][LAT] >> 16) & 0xFF;
			allData[index + 5] = (locations[i][LAT] >> 24) & 0xFF;

			// Get lon
			allData[index + 6] = locations[i][LON] & 0xFF;
			allData[index + 7] = (locations[i][LON] >> 8) & 0xFF;
			allData[index + 8] = (locations[i][LON] >> 16) & 0xFF;
			allData[index + 9] = (locations[i][LON] >> 24) & 0xFF;
			k_mutex_unlock(&loc_signal);

			// Get sensor data
			for (int j = 0; j < NUM_OF_SENSOR_BYTES; j++) {
				allData[j + (index + 10)] = recv[i][j];
			}
		}

		struct bt_data ad1[] = {
            BT_DATA(BT_DATA_MANUFACTURER_DATA, allData, TOTAL_BYTES),
        };

		/* Set advertising data to have complete local name set */
		err = bt_le_ext_adv_set_data(adv, ad1, ARRAY_SIZE(ad1), NULL, 0);
		if (err) {
			printk("Failed to set advertising data (err %d)\n", err);
			return 0;
		}

		start_adv(adv);

        k_msleep(1000);
	}

}

// K_THREAD_DEFINE(gps_id, 1024, thread_gps, NULL, NULL, NULL,
// 	5, 0, 0);
 