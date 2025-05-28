
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

#define NUM_OF_SENSORS 		2
#define NUM_OF_SENSOR_BYTES 6
#define BYTES_PER_SENSOR 	NUM_OF_SENSOR_BYTES + 8
#define TOTAL_BYTES 		BYTES_PER_SENSOR * 2

#define LAT 0
#define LON 1

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
        0x0f, 0x18)  
};

static const char* sensor_mac[NUM_OF_SENSORS] = {
	"C9:2B:FC:6A:D3:C0",
	"D1:31:A2:EB:63:2A"
};

uint8_t recv[NUM_OF_SENSORS][NUM_OF_SENSOR_BYTES] = {0};
float locations[NUM_OF_SENSORS][2] = {0};

static bool parse_gps(struct bt_data *data, void *user_data)
{
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
            locations[0][LAT] = lat;
            locations[0][LON] = lon;
        }
    }
    return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
	struct net_buf_simple *ad) {

	char addr_str[BT_ADDR_LE_STR_LEN];
	char mac_addr[18];

	bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    strncpy(mac_addr, addr_str, 17);
    mac_addr[17] = '\0';

	bt_data_parse(ad, parse_gps, NULL);

	for (int i = 0; i < NUM_OF_SENSORS; i++) {
		if (strcmp(mac_addr, sensor_mac[i]) == 0) {
			if (ad->len >= 5 + NUM_OF_SENSOR_BYTES) {
                memcpy(recv[i], &ad->data[5], NUM_OF_SENSOR_BYTES);
            }
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

	/* Start advertising */
	// err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
	// 	sd, ARRAY_SIZE(sd));
    err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
		NULL, 0);

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return 0;
	}
	printk("Started advertising\n");

	uint8_t allData[TOTAL_BYTES];

	while (1) {
		memset(allData, 0, sizeof(allData));
        for (int i = 0; i < NUM_OF_SENSORS; i++) {
            /* Insert GPS or sensor data into allData array */
            int base = i * BYTES_PER_SENSOR;
            /* pack latitude (2 bytes int16 + 2 bytes frac16) */
            int16_t lat_whole = (int16_t)locations[i][LAT];
            int32_t lat_frac = (int32_t)((locations[i][LAT] - lat_whole) * 1e6);
            sys_put_le16(lat_whole, &allData[base]);
            sys_put_le16((int16_t)(lat_frac & 0xFFFF), &allData[base + 2]);
            /* pack longitude */
            int16_t lon_whole = (int16_t)locations[i][LON];
            int32_t lon_frac = (int32_t)((locations[i][LON] - lon_whole) * 1e6);
            sys_put_le16(lon_whole, &allData[base + 4]);
            sys_put_le16((int16_t)(lon_frac & 0xFFFF), &allData[base + 6]);
            /* copy sensor raw data */
            memcpy(&allData[base + 8], recv[i], NUM_OF_SENSOR_BYTES);
        }

		struct bt_data adv[] = {
            BT_DATA(BT_DATA_MANUFACTURER_DATA, allData, TOTAL_BYTES),
        };

        err = bt_le_adv_update_data(adv, ARRAY_SIZE(adv), NULL, 0);
        // err = bt_le_adv_update_data(adv, ARRAY_SIZE(adv), sd, ARRAY_SIZE(sd));

        if (err) {
            printk("adv_update failed: %d\n", err);
        }

        k_msleep(1000);
	}
}


 