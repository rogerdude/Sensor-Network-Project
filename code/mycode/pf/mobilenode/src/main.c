
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <string.h>

#define NUM_OF_SENSORS 2

#ifndef IBEACON_RSSI
#define IBEACON_RSSI 0xc8
#endif

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
        0x0f, 0x18)  
};

static const char* sensor_mac[NUM_OF_SENSORS] = {
	"C9:2B:FC:6A:D3:C0",
	"D1:31:A2:EB:63:2A"
};

uint8_t recv[NUM_OF_SENSORS * 12] = {0};

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
			for (int j = 5; j < (5 + 12); j++) {
				recv[(i * 12) + j - 5] = ad->data[j];
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

	while (1) {
		struct bt_data adv[] = {
            BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
            BT_DATA(BT_DATA_MANUFACTURER_DATA,
                    recv, NUM_OF_SENSORS * 12),
        };

        err = bt_le_adv_update_data(adv, ARRAY_SIZE(adv), NULL, 0);
        // err = bt_le_adv_update_data(adv, ARRAY_SIZE(adv), sd, ARRAY_SIZE(sd));

        if (err) {
            printk("adv_update failed: %d\n", err);
        }

        k_msleep(1000);
	}

}
 