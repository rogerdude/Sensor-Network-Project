/*
 * Viewer (M5stack Core2) driver main source file
 * Modified for larger grid and MQTT/WiFi severity overlay
 * - Grid covers ±100 m around center lat/lon
 * - Each cell represents ~10 m
 * - Incoming (lat, lon, severity) updates cell color
 * - Includes fake test coordinates on startup, including all four corners
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <math.h>
#include <stdlib.h>             /* atof, atoi */

/* === MQTT ADDITIONS === */
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/json/json.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>

LOG_MODULE_REGISTER(viewer, CONFIG_LOG_DEFAULT_LEVEL);

/* Wi-Fi credentials */
#define SSID            "jeremy phone"
#define PSK             "Habib1234"

/* MQTT broker */
#define MQTT_BROKER_ADDR    "10.0.0.3"
#define MQTT_BROKER_PORT    1883
#define MQTT_TOPIC          "m5core2/geo_severity"

/* Thread/sem for MQTT loop */
K_THREAD_STACK_DEFINE(mqtt_stack_area, 2048);
static struct k_thread   mqtt_thread_data;
static K_SEM_DEFINE(wifi_connected, 0, 1);

/* Network/MQTT objects */
static struct net_mgmt_event_callback wifi_cb;
static struct mqtt_client             client;
static struct sockaddr_storage        broker;
static uint8_t                        rx_buffer[256];
static uint8_t                        tx_buffer[256];

/* JSON descriptor for incoming geo_severity */
struct geo_severity {
    double longitude;
    double latitude;
    int    severity;
};

JSON_OBJ_DESCR_DEFINE(geo_descr,
    JSON_OBJ_DESCR_PRIM(struct geo_severity, longitude, JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct geo_severity, latitude,  JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct geo_severity, severity,  JSON_TOK_NUMBER)
);

/* === End MQTT ADDITIONS === */

/* Display grid constants */
#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  240
#define GRID_SPAN_M    100
#define CELL_SIZE_M    10
#define GRID_CELLS     ((GRID_SPAN_M * 2) / CELL_SIZE_M) /*200/10=20*/
#define ROWS           (GRID_CELLS + 1)
#define COLS           (GRID_CELLS + 1)
#define M_PI           3.14159265358979323846f

static const int CENTER_ROW = ROWS / 2;
static const int CENTER_COL = COLS / 2;
static const float METERS_PER_CELL_Y = (2.0f * GRID_SPAN_M) / ROWS;
static const float METERS_PER_CELL_X = (2.0f * GRID_SPAN_M) / COLS;

static const float CENTER_LAT = -27.499488f;
static const float CENTER_LON = 153.014495f;

static const float LAT_DEG_PER_M = 1.0f / 111000.0f;
static float       LON_DEG_PER_M;

static int cell_w, cell_h;
static lv_point_t vline_pts[COLS + 1][2];
static lv_point_t hline_pts[ROWS + 1][2];

/* Forward: update cell from lat/lon */
static void update_from_geo(float lat, float lon, int severity);

/* Draw the grid */
static void draw_grid(void) {
    cell_w = SCREEN_WIDTH / COLS;
    cell_h = SCREEN_HEIGHT / ROWS;
    for (int i = 0; i <= COLS; i++) {
        vline_pts[i][0].x = i * cell_w;
        vline_pts[i][0].y = 0;
        vline_pts[i][1].x = i * cell_w;
        vline_pts[i][1].y = SCREEN_HEIGHT;
        lv_obj_t *ln = lv_line_create(lv_scr_act());
        lv_line_set_points(ln, vline_pts[i], 2);
        lv_obj_set_style_line_color(ln, lv_color_black(), 0);
        lv_obj_set_style_line_width(ln, 1, 0);
    }
    for (int i = 0; i <= ROWS; i++) {
        hline_pts[i][0].x = 0;
        hline_pts[i][0].y = i * cell_h;
        hline_pts[i][1].x = SCREEN_WIDTH;
        hline_pts[i][1].y = i * cell_h;
        lv_obj_t *ln = lv_line_create(lv_scr_act());
        lv_line_set_points(ln, hline_pts[i], 2);
        lv_obj_set_style_line_color(ln, lv_color_black(), 0);
        lv_obj_set_style_line_width(ln, 1, 0);
    }
}

static void color_cell(int row, int col, int severity) {
    if (row < 0) row = 0;
    if (row >= ROWS) row = ROWS - 1;
    if (col < 0) col = 0;
    if (col >= COLS) col = COLS - 1;
    lv_obj_t *cell = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cell, cell_w - 2, cell_h - 2);
    lv_color_t c = (severity <= 0 ? lv_color_hex(0x00FF00)
                      : (severity == 1 ? lv_color_hex(0xFFFF00)
                                       : lv_color_hex(0xFF0000)));
    lv_obj_set_style_bg_color(cell, c, 0);
    lv_obj_set_style_border_width(cell, 0, 0);
    lv_obj_set_pos(cell, col * cell_w + 1, row * cell_h + 1);
}

static void update_from_geo(float lat, float lon, int severity) {
    float dlat = lat - CENTER_LAT;
    float dlon = lon - CENTER_LON;
    float dy_m = dlat / LAT_DEG_PER_M;
    float dx_m = dlon / LON_DEG_PER_M;
    int row = CENTER_ROW - (int)roundf(dy_m / METERS_PER_CELL_Y);
    int col = CENTER_COL + (int)roundf(dx_m / METERS_PER_CELL_X);
    color_cell(row, col, severity);
}

/* Shell command for manual testing */
static int cmd_geo_update(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 4) {
        shell_print(shell, "Usage: geo_update <lat> <lon> <severity>");
        return -EINVAL;
    }
    float lat = atof(argv[1]);
    float lon = atof(argv[2]);
    int sev   = atoi(argv[3]);
    update_from_geo(lat, lon, sev);
    shell_print(shell, "Updated cell for (%.6f,%.6f) sev %d", lat, lon, sev);
    return 0;
}
SHELL_CMD_REGISTER(geo_update, NULL, "Update grid from geo coords", cmd_geo_update);

/* === MQTT ADDITIONS === */

/* Handle Wi-Fi connect result */
static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb) {
    const struct wifi_status *status = cb->info;
    if (status->status == 0) {
        printk("Wi-Fi connected.\n");
        k_sem_give(&wifi_connected);
    } else {
        printk("Wi-Fi connection failed: %d\n", status->status);
    }
}

static void wifi_event_handler(struct net_mgmt_event_callback *cb,
                               uint32_t mgmt_event, struct net_if *iface) {
    if (mgmt_event == NET_EVENT_WIFI_CONNECT_RESULT) {
        handle_wifi_connect_result(cb);
    }
}

/* Kick off Wi-Fi */
static void wifi_connect(void) {
    struct net_if *iface = net_if_get_default();
    struct wifi_connect_req_params params = {
        .ssid         = SSID,
        .ssid_length  = strlen(SSID),
        .psk          = PSK,
        .psk_length   = strlen(PSK),
        .channel      = WIFI_CHANNEL_ANY,
        .security     = WIFI_SECURITY_TYPE_PSK,
        .band         = WIFI_FREQ_BAND_2_4_GHZ,
        .mfp          = WIFI_MFP_OPTIONAL
    };
    printk("Connecting to WiFi: %s\n", SSID);
    net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
}

/* MQTT event callback */
static void mqtt_evt_handler(struct mqtt_client *c, const struct mqtt_evt *evt) {
    switch (evt->type) {
    case MQTT_EVT_CONNACK:
        if (evt->result == 0) {
            printk("MQTT connected, subscribing...\n");
            /* Subscribe once connected */
            static struct mqtt_topic topic = {
                .topic = {.utf8 = MQTT_TOPIC, .size = sizeof(MQTT_TOPIC)-1},
                .qos   = MQTT_QOS_1_AT_LEAST_ONCE
            };
            mqtt_subscribe(&client, &topic, 1);
        } else {
            printk("MQTT CONNACK failed: %d\n", evt->result);
        }
        break;

    case MQTT_EVT_PUBLISH: {
        const struct mqtt_publish_param *p = &evt->param.publish;
        size_t len = p->message.payload.len;
        char   buf[len+1];
        mqtt_read_publish_payload(&client, buf, len);
        buf[len] = '\0';
        printk("→ %.*s: %s\n",
               p->message.topic.topic.size,
               p->message.topic.topic.utf8,
               buf);
        /* Parse JSON */
        struct geo_severity data;
        int ret = json_obj_parse(buf, len, true,
                                 geo_descr, ARRAY_SIZE(geo_descr),
                                 &data);
        if (ret == 0) {
            update_from_geo(data.latitude, data.longitude, data.severity);
        } else {
            printk("JSON parse error: %d\n", ret);
        }
        break;
    }

    default:
        break;
    }
}

/* MQTT input loop */
static void mqtt_loop(void) {
    while (1) {
        mqtt_input(&client);
        mqtt_live(&client);
        k_msleep(100);
    }
}

/* Connect to broker & start loop thread */
static void mqtt_connect_and_subscribe(void) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *)&broker;
    memset(addr4, 0, sizeof(*addr4));
    addr4->sin_family = AF_INET;
    addr4->sin_port   = htons(MQTT_BROKER_PORT);
    net_addr_pton(AF_INET, MQTT_BROKER_ADDR, &addr4->sin_addr);

    mqtt_client_init(&client);
    client.broker            = &broker;
    client.evt_cb            = mqtt_evt_handler;
    client.client_id.utf8    = "m5core2";
    client.client_id.size    = strlen(client.client_id.utf8);
    client.protocol_version  = MQTT_VERSION_3_1_1;
    client.rx_buf            = rx_buffer;
    client.rx_buf_size       = sizeof(rx_buffer);
    client.tx_buf            = tx_buffer;
    client.tx_buf_size       = sizeof(tx_buffer);
    client.transport.type    = MQTT_TRANSPORT_NON_SECURE;

    int rc = mqtt_connect(&client);
    if (rc) {
        printk("mqtt_connect failed: %d\n", rc);
        return;
    }
    /* spawn the loop */
    k_thread_create(&mqtt_thread_data, mqtt_stack_area,
                    K_THREAD_STACK_SIZEOF(mqtt_stack_area),
                    (k_thread_entry_t)mqtt_loop,
                    NULL, NULL, NULL,
                    7, 0, K_NO_WAIT);
}

/* === End MQTT ADDITIONS === */

int main(void) {
    /* Calculate lon-degree per meter */
    LON_DEG_PER_M = 1.0f / (111000.0f * cosf(CENTER_LAT * M_PI / 180.0f));

    /* Initialize LVGL display */
    const struct device *disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(disp)) {
        LOG_ERR("Display not ready");
        return -ENODEV;
    }
    lv_init();
    lv_obj_clean(lv_scr_act());
    draw_grid();

    /* Initial test points */
    update_from_geo(CENTER_LAT, CENTER_LON, 0);
    update_from_geo(CENTER_LAT + GRID_SPAN_M * LAT_DEG_PER_M,
                    CENTER_LON - GRID_SPAN_M * LON_DEG_PER_M, 1);
    update_from_geo(CENTER_LAT + GRID_SPAN_M * LAT_DEG_PER_M,
                    CENTER_LON + GRID_SPAN_M * LON_DEG_PER_M, 2);
    update_from_geo(CENTER_LAT - GRID_SPAN_M * LAT_DEG_PER_M,
                    CENTER_LON - GRID_SPAN_M * LON_DEG_PER_M, 2);
    update_from_geo(CENTER_LAT - GRID_SPAN_M * LAT_DEG_PER_M,
                    CENTER_LON + GRID_SPAN_M * LON_DEG_PER_M, 1);

    /* === MQTT startup === */
    net_mgmt_init_event_callback(&wifi_cb, wifi_event_handler,
                                 NET_EVENT_WIFI_CONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_cb);
    wifi_connect();

    /* wait up to 10s */
    if (k_sem_take(&wifi_connected, K_SECONDS(10)) == 0) {
        printk("Wi-Fi ready, starting MQTT...\n");
        mqtt_connect_and_subscribe();
    } else {
        printk("Wi-Fi timed out\n");
    }
    /* === End MQTT startup === */

    display_blanking_off(disp);

    /* Main LVGL loop (MQTT runs in background thread) */
    while (1) {
        lv_timer_handler();
        k_msleep(10);
    }
    return 0;
}
