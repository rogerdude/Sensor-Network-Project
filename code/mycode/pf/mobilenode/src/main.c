
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/gnss.h>
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

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
        0x0f, 0x18)  
};

static const char* sensor_uuid = SENSOR_UUID;
static const char* mobile_uuid = MOBILE_UUID;
static const char* base_uuid = BASE_UUID;

uint8_t recv[NUM_OF_SENSORS][NUM_OF_SENSOR_BYTES + 1] = {0};
float locations[NUM_OF_SENSORS][2] = {0};
uint8_t stopped[NUM_OF_SENSORS] = {0};

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

			// Get lat
			int lat_int = (int) locations[i][LAT];
			allData[index + 2] = lat_int;
			allData[index + 3] = (int) ((locations[i][LAT] - lat_int) * 100);
			allData[index + 4] = (int) ((locations[i][LAT] - lat_int) * 10000);
			allData[index + 5] = (int) ((locations[i][LAT] - lat_int) * 1000000);

			// Get lon
			int lon_int = (int) locations[i][LON];
			allData[index + 6] = lon_int;
			allData[index + 7] = (int) ((locations[i][LON] - lon_int) * 100);
			allData[index + 8] = (int) ((locations[i][LON] - lon_int) * 10000);
			allData[index + 9] = (int) ((locations[i][LON] - lon_int) * 1000000);

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


 