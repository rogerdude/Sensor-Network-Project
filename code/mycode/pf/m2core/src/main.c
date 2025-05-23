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

LOG_MODULE_REGISTER(viewer, CONFIG_LOG_DEFAULT_LEVEL);

#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  240
#define GRID_SPAN_M    100    /* ±100 m */
#define CELL_SIZE_M    10     /* 10 m per cell */
#define GRID_CELLS     ((GRID_SPAN_M * 2) / CELL_SIZE_M) /* 200/10=20 */
#define ROWS           (GRID_CELLS + 1)  /* 21 */
#define COLS           (GRID_CELLS + 1)  /* 21 */
#define M_PI         3.14159265358979323846f

/* Center cell indices */
static const int CENTER_ROW = ROWS / 2;
static const int CENTER_COL = COLS / 2;
/* Meters per cell */
static const float METERS_PER_CELL_Y = (2.0f * GRID_SPAN_M) / ROWS;
static const float METERS_PER_CELL_X = (2.0f * GRID_SPAN_M) / COLS;

/* Geographic center */
static const float CENTER_LAT = -27.499488f;
static const float CENTER_LON = 153.014495f;

/* Conversion factors */
static const float LAT_DEG_PER_M = 1.0f / 111000.0f;
static float LON_DEG_PER_M;

/* Pre-calc cell pixel size */
static int cell_w;
static int cell_h;

/* Persistent arrays for line endpoints */
static lv_point_t vline_pts[COLS + 1][2];
static lv_point_t hline_pts[ROWS + 1][2];

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

static int cmd_geo_update(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 4) {
        shell_print(shell, "Usage: geo_update <lat> <lon> <severity>");
        return -EINVAL;
    }
    float lat = atof(argv[1]);
    float lon = atof(argv[2]);
    int sev = atoi(argv[3]);
    update_from_geo(lat, lon, sev);
    shell_print(shell, "Updated cell for (%.6f,%.6f) sev %d", lat, lon, sev);
    return 0;
}
SHELL_CMD_REGISTER(geo_update, NULL, "Update grid from geo coords", cmd_geo_update);

int main(void) {
    const struct device *disp = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    if (!device_is_ready(disp)) {
        LOG_ERR("Display not ready");
        return -ENODEV;
    }
    LON_DEG_PER_M = 1.0f / (111000.0f * cosf(CENTER_LAT * M_PI / 180.0f));
    lv_init();
    lv_obj_clean(lv_scr_act());
    draw_grid();

    /* Center test */
    update_from_geo(CENTER_LAT, CENTER_LON, 0);
    /* Four corners: NW, NE, SW, SE */
    update_from_geo(CENTER_LAT + GRID_SPAN_M * LAT_DEG_PER_M,
                    CENTER_LON - GRID_SPAN_M * LON_DEG_PER_M, 1);
    update_from_geo(CENTER_LAT + GRID_SPAN_M * LAT_DEG_PER_M,
                    CENTER_LON + GRID_SPAN_M * LON_DEG_PER_M, 2);
    update_from_geo(CENTER_LAT - GRID_SPAN_M * LAT_DEG_PER_M,
                    CENTER_LON - GRID_SPAN_M * LON_DEG_PER_M, 2);
    update_from_geo(CENTER_LAT - GRID_SPAN_M * LAT_DEG_PER_M,
                    CENTER_LON + GRID_SPAN_M * LON_DEG_PER_M, 1);

    display_blanking_off(disp);
    while (1) {
        lv_timer_handler();
        k_msleep(10);
    }
    return 0;
}
