/*
 * Viewer (M5stack Core2) driver main source file
 * Optimized version without grid lines and with persistent sensor objects
 */
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <math.h>
#include <zephyr/data/json.h>
#include <myconfig.h>
#include <zephyr/sys/printk.h>

#define NUM_OF_SENSORS 2
#define BLOCK_SIZE    11

extern const lv_img_dsc_t bounds;

LOG_MODULE_REGISTER(viewer, CONFIG_LOG_DEFAULT_LEVEL);

#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  240
#define GRID_SPAN_M_Y   75     /* ±75 m vertically (latitude) */
#define GRID_SPAN_M_X   100    /* ±100 m horizontally (longitude) */
#define M_PI         3.14159265358979323846f

static const char* base_uuid = BASE_UUID;

/* Geographic center */
static const float CENTER_LAT = -27.499964f;
static const float CENTER_LON = 153.014639f;

/* Conversion factors */
static const float LAT_DEG_PER_M = 1.0f / 111000.0f;
static float LON_DEG_PER_M;

/* Sensor display objects */
static lv_obj_t *sensor_objs[NUM_OF_SENSORS] = {NULL};

/* Convert geographic coordinates to screen position */
static void geo_to_screen(float lat, float lon, lv_coord_t *x, lv_coord_t *y) {
    float dlat = lat - CENTER_LAT;
    float dlon = lon - CENTER_LON;
    float dy_m = dlat / LAT_DEG_PER_M;
    float dx_m = dlon / LON_DEG_PER_M;
    
    /* Calculate relative position within map area */
    float rel_x = (dx_m + GRID_SPAN_M_X) / (2 * GRID_SPAN_M_X);
    float rel_y = 1.0f - (dy_m + GRID_SPAN_M_Y) / (2 * GRID_SPAN_M_Y);
    
    /* Convert to screen coordinates */
    *x = (lv_coord_t)(rel_x * SCREEN_WIDTH);
    *y = (lv_coord_t)(rel_y * SCREEN_HEIGHT);
}

/* Initialize sensor display objects */
static void init_sensor_objects() {
    const lv_coord_t SIZE = 15;  /* Visible dot size */
    for(int i = 0; i < NUM_OF_SENSORS; i++) {
        sensor_objs[i] = lv_obj_create(lv_scr_act());
        lv_obj_set_size(sensor_objs[i], SIZE, SIZE);
        lv_obj_set_style_radius(sensor_objs[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(sensor_objs[i], 0, 0);
        lv_obj_set_style_bg_opa(sensor_objs[i], LV_OPA_COVER, 0);
        lv_obj_add_flag(sensor_objs[i], LV_OBJ_FLAG_HIDDEN);
    }
}

/* Update sensor position and color */
static void update_sensor(int idx, float lat, float lon, int severity) {
    if (idx < 0 || idx >= NUM_OF_SENSORS) return;
    
    lv_coord_t x, y;
    geo_to_screen(lat, lon, &x, &y);
    
    /* Center the object at calculated position */
    lv_coord_t size = lv_obj_get_width(sensor_objs[idx]);
    lv_obj_set_pos(sensor_objs[idx], 
                   x - size/2,
                   y - size/2);
    
    /* Set color based on severity */
    lv_color_t c = severity <= 0 ? lv_color_hex(0x00FF00) :  /* Green */
                    severity == 1 ? lv_color_hex(0xFFFF00) : /* Yellow */
                                    lv_color_hex(0xFF0000);  /* Red */
    lv_obj_set_style_bg_color(sensor_objs[idx], c, 0);
    lv_obj_clear_flag(sensor_objs[idx], LV_OBJ_FLAG_HIDDEN);
}

/* BLE device found callback */
static void device_found(const bt_addr_le_t *addr,
                         int8_t rssi,
                         uint8_t type,
                         struct net_buf_simple *ad) {

    if (ad->len < (2 + UUID_SIZE)) {
        return;
    }

    char name[7];
	for (int i = 2; i < (2 + UUID_SIZE); i++) {
        name[i - 2] = ad->data[i];
    }
    name[6] = '\0';

    if (strcmp(name, base_uuid) == 0) {
        for (int i = 8; i < (8 + (11 * 2)); i += 11) {
            uint8_t  id      = ad->data[i];
            uint8_t  stopped = ad->data[i + 1];
            int32_t  lat_i   = ad->data[i + 2] | (ad->data[i + 3] << 8) |
				                (ad->data[i + 4] << 16) | (ad->data[i + 5] << 24);
            int32_t  lon_i   = ad->data[i + 6] | (ad->data[i + 7] << 8) |
				                (ad->data[i + 8] << 16) | (ad->data[i + 9] << 24);
            uint8_t  sev     = ad->data[i + 10];

            float lat = lat_i / 1e6f;
            float lon = lon_i / 1e6f;

            printk("Sensor %u: id=%u stopped=%u lat=%f lon=%f sev=%u\n",
                i, id, stopped, lat, lon, sev);

            update_sensor(id, lat, lon, sev);
        }
    }
}

int main(void) {
    const struct device *disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(disp)) {
        LOG_ERR("Display not ready");
        return -ENODEV;
    }
    
    /* Calculate longitude conversion factor */
    LON_DEG_PER_M = 1.0f / (111000.0f * cosf(CENTER_LAT * M_PI / 180.0f));
    
    /* Initialize LVGL */
    lv_init();
    lv_obj_clean(lv_scr_act());
    display_blanking_off(disp);

    /* Create background image */
    lv_obj_t *bg_img = lv_img_create(lv_scr_act());
    lv_img_set_src(bg_img, &bounds);
    lv_obj_set_size(bg_img, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_pos(bg_img, 0, 0);
    lv_obj_move_background(bg_img);

    /* Create sensor objects */
    init_sensor_objects();

    /* Enable Bluetooth */
    int err = bt_enable(NULL);
    if (err) {
        LOG_ERR("BT init failed (%d)", err);
        return 0;
    }
    
    /* Configure BLE scanning */
    struct bt_le_scan_param sp = {
        .type     = BT_LE_SCAN_TYPE_PASSIVE,
        .options  = BT_LE_SCAN_OPT_NONE,
        .interval = BT_GAP_SCAN_FAST_INTERVAL,
        .window   = BT_GAP_SCAN_FAST_WINDOW,
    };
    
    err = bt_le_scan_start(&sp, device_found);
    if (err) {
        LOG_ERR("BT scan failed (%d)", err);
    } else {
        LOG_INF("BLE scanning started");
    }

    /* Main loop */
    while (1) {
        lv_timer_handler();
        k_msleep(10);
    }
    return 0;
}