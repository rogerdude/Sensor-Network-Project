#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <string.h>
#include <stdlib.h>
#include "sample.h"
#include "sltb_sensors.h"
#include "rtc.h"
#include <time.h>
#include <zephyr/data/json.h>
#include <zephyr/drivers/gpio.h>
#include "thunderboard_button.h" 
#include <zephyr/fs/fs.h>

LOG_MODULE_REGISTER(sample_module, LOG_LEVEL_DBG);

static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback button0_cb_data;


/* Global variables for continuous sampling configuration */
static volatile bool sampling_enabled = false;
static volatile uint8_t sample_did = 0;
static volatile uint32_t sample_rate = 1; // seconds

/* JSON structure for all sensors */
struct JSONAllSensors {
    int  DID;      /* sensor group ID */
    char *time;    /* formatted timestamp: "YYYY-MM-DD HH:MM:SS" */
    char *temp;    /* temperature as string */
    char *hum;     /* humidity as string */
    char *press;   /* pressure as string */
    char *light;   /* light as string */
};

struct JSONSensor {
    int  DID;      /* sensor group ID */
    char *time;    /* formatted timestamp: "YYYY-MM-DD HH:MM:SS" */
    char *value;
};

/* JSON descriptor for encoding all sensor data */
static const struct json_obj_descr allSensorDescr[] = {
    JSON_OBJ_DESCR_PRIM(struct JSONAllSensors, DID,   JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct JSONAllSensors, time,  JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct JSONAllSensors, temp,  JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct JSONAllSensors, hum,   JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct JSONAllSensors, press, JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct JSONAllSensors, light, JSON_TOK_STRING),
};

static const struct json_obj_descr singleSensorDescr[] = {
    JSON_OBJ_DESCR_PRIM(struct JSONSensor, DID,   JSON_TOK_NUMBER),
    JSON_OBJ_DESCR_PRIM(struct JSONSensor, time,  JSON_TOK_STRING),
    JSON_OBJ_DESCR_PRIM(struct JSONSensor, value, JSON_TOK_STRING),
};

struct sensor_event {
    struct mpsc_node node;
    uint8_t sensor_id;
    struct sensor_q31_data data;
};

/* Continuous sampling thread */
K_THREAD_STACK_DEFINE(sample_thread_stack, 1024);
static struct k_thread sample_thread_data;

static bool log_to_file = false;
static char log_filepath[64] = "/ram/sensors.log";

static void write_to_file(const char *data) {
    struct fs_file_t file;
    fs_file_t_init(&file);

    int rc = fs_open(&file, log_filepath, 
                   FS_O_CREATE | FS_O_WRITE | FS_O_READ);
    if (rc != 0) {
        printk("Failed to open file: %d\n", rc);
        return;
    }

    (void)fs_seek(&file, 0, FS_SEEK_END);
    
    rc = fs_write(&file, data, strlen(data));
    if (rc < 0) {
        printk("Write failed: %d\n", rc);
    }

    fs_close(&file);
}

static void sample_thread(void *p1, void *p2, void *p3) {
    while (1) {
        if (sampling_enabled) {
            struct tm current_tm;
            rtc_get_calendar_time(&current_tm);

            sensor_values_t sensor_values = {0};
            int rc = get_sensor_values(&sensor_values);
            char json_buf[256];

            if (rc == 0) {
                if (sample_did == SENSOR_DID_ALL) {
                    /* Create and populate JSONAllSensors structure */
                    struct JSONAllSensors sensorJSON;
                    char time_str[32], temp_str[16], hum_str[16], press_str[16], light_str[16];

                    /* Format the current time */
                    snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d",
                             current_tm.tm_year + 1900,
                             current_tm.tm_mon + 1,
                             current_tm.tm_mday,
                             current_tm.tm_hour,
                             current_tm.tm_min,
                             current_tm.tm_sec);

                    /* Convert sensor values to formatted strings */
                    int32_t temp = (int32_t)(sensor_values.temperature * 100.0f);
                    int32_t humid = (int32_t)(sensor_values.humidity * 100.0f);
                    int32_t press = (int32_t)sensor_values.pressure;
                    int32_t light = (int32_t)sensor_values.light;
                    snprintf(temp_str, sizeof(temp_str), "%d.%02d", temp / 100, abs(temp % 100));
                    snprintf(hum_str, sizeof(hum_str), "%d.%02d", humid / 100, abs(humid % 100));
                    snprintf(press_str, sizeof(press_str), "%d", press);
                    snprintf(light_str, sizeof(light_str), "%d", light);


                    /* Populate the JSON structure */
                    sensorJSON.DID   = sample_did;
                    sensorJSON.time  = time_str;
                    sensorJSON.temp  = temp_str;
                    sensorJSON.hum   = hum_str;
                    sensorJSON.press = press_str;
                    sensorJSON.light = light_str;

                    /* Encode to JSON */
                    int ret = json_obj_encode_buf(
                        allSensorDescr,
                        ARRAY_SIZE(allSensorDescr),
                        &sensorJSON,
                        json_buf,
                        sizeof(json_buf)
                    );

                    if (ret < 0) {
                        printk("JSON encoding failed: %d\n", ret);
                    } else {
                        if (log_to_file) {
                            write_to_file(json_buf);
                        } else {
                            printk("%s\n", json_buf);
                        }
                    }
                } else if (sample_did == SENSOR_DID_TEMP ||
                           sample_did == SENSOR_DID_HUMID ||
                           sample_did == SENSOR_DID_PRESS ||
                           sample_did == SENSOR_DID_LIGHT) {
                    /* For individual sensor readings, use manual formatting */
                    struct JSONSensor sensorJSON;
                    char time_str[32], value_str[16];

                    snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d:%02d",
                             current_tm.tm_year + 1900,
                             current_tm.tm_mon + 1,
                             current_tm.tm_mday,
                             current_tm.tm_hour,
                             current_tm.tm_min,
                             current_tm.tm_sec);

                    uint32_t value = 0;
                    if (sample_did == SENSOR_DID_TEMP) {
                        value = (int32_t)(sensor_values.temperature * 100.0f);
                        snprintf(value_str, sizeof(value_str), "%d.%02d", value / 100, abs(value % 100));
                    } else if (sample_did == SENSOR_DID_HUMID) {
                        value = (int32_t)(sensor_values.humidity * 100.0f);
                        snprintf(value_str, sizeof(value_str), "%d.%02d", value / 100, abs(value % 100));
                    } else if (sample_did == SENSOR_DID_PRESS) {
                        value = (int32_t)sensor_values.pressure;
                        snprintf(value_str, sizeof(value_str), "%d", value);
                    } else if (sample_did == SENSOR_DID_LIGHT) {
                        value = (int32_t)sensor_values.light;
                        snprintf(value_str, sizeof(value_str), "%d", value);
                    } 

                    sensorJSON.DID   = sample_did;
                    sensorJSON.time  = time_str;
                    sensorJSON.value = value_str;

                    int ret = json_obj_encode_buf(
                        singleSensorDescr,
                        ARRAY_SIZE(singleSensorDescr),
                        &sensorJSON,
                        json_buf,
                        sizeof(json_buf)
                    );
                    if (log_to_file) {
                        write_to_file(json_buf);
                    } else {
                        printk("%s\n", json_buf);
                    }
                }
            } else {
                printk("Sensor read error: %d\n", rc);
            }
            k_sleep(K_SECONDS(sample_rate));
        } else {
            k_sleep(K_MSEC(100));
        }
    }
}

int sample_start(uint8_t did) {
    sample_did = did;
    sampling_enabled = true;
    return 0;
}

int sample_stop(void) {
    sampling_enabled = false;
    return 0;
}

int sample_set_rate(uint32_t rate) {
    sample_rate = rate;
    return 0;
}

static void sample_button_callback(void)
{
    sampling_enabled = !sampling_enabled;

    if (sampling_enabled) {
        printk("Button pressed: Starting sampling\n");
        sample_start(sample_did);
    } else {
        printk("Button pressed: Stopping sampling\n");
        sample_stop();
    }
}

int sample_init(void) {
    thunderboard_button_set_callback(sample_button_callback);
    
    /* Ensure the RTC is initialized prior to sampling (if needed, call rtc_init() here) */
    k_thread_create(&sample_thread_data, sample_thread_stack,
                    K_THREAD_STACK_SIZEOF(sample_thread_stack),
                    sample_thread, NULL, NULL, NULL,
                    K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
    return 0;
}

/* Shell command to control continuous sampling:
 * Usage:
 *   sample s <DID>   -> start sampling for sensor DID
 *   sample p         -> stop sampling
 *   sample w <rate>  -> set sampling rate in seconds
 */
static int cmd_sample(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 2) {
        shell_error(shell, "Usage: sample <s/p/w> <argument>");
        return -EINVAL;
    }
    if (strcmp(argv[1], "s") == 0) {
        if (argc < 3) {
            shell_error(shell, "Usage: sample s <DID>");
            return -EINVAL;
        }
        uint8_t did = (uint8_t)strtol(argv[2], NULL, 10);
        if (did == SENSOR_DID_ALL || 
            did == SENSOR_DID_TEMP ||
            did == SENSOR_DID_HUMID ||
            did == SENSOR_DID_PRESS ||
            did == SENSOR_DID_LIGHT) {
            sample_did = did;
        } else {
            shell_error(shell, "Invalid DID");
            return -EINVAL;
        }
        sample_start(did);
        shell_print(shell, "Continuous sampling started for DID %d", did);
    } else if (strcmp(argv[1], "p") == 0) {
        sample_stop();
        shell_print(shell, "Continuous sampling stopped");
    } else if (strcmp(argv[1], "w") == 0) {
        if (argc < 3) {
            shell_error(shell, "Usage: sample w <rate>");
            return -EINVAL;
        }
        uint32_t rate = (uint32_t)strtol(argv[2], NULL, 10);
        sample_set_rate(rate);
        shell_print(shell, "Sampling rate set to %d seconds", rate);
    } else {
        shell_error(shell, "Invalid command. Use s, p, or w.");
        return -EINVAL;
    }
    return 0;
}

/* List directory contents */
static int cmd_ls(const struct shell *shell, size_t argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "/lfs";
    struct fs_dir_t dir;
    struct fs_dirent entry;
    int rc;

    fs_dir_t_init(&dir);
    rc = fs_opendir(&dir, path);
    if (rc != 0) {
        shell_error(shell, "Failed to open dir %s: %d", path, rc);
        return rc;
    }

    shell_print(shell, "Contents of %s:", path);
    while (1) {
        rc = fs_readdir(&dir, &entry);
        if (rc != 0 || entry.name[0] == '\0') break;

        shell_print(shell, "[%s] %s (size: %zu)",
                    (entry.type == FS_DIR_ENTRY_DIR) ? "DIR " : "FILE",
                    entry.name, entry.size);
    }

    fs_closedir(&dir);
    return 0;
}

/* Create directory */
static int cmd_mkdir(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 2) {
        shell_error(shell, "Usage: mkdir <path>");
        return -EINVAL;
    }
    int rc = fs_mkdir(argv[1]);
    if (rc < 0) shell_error(shell, "Error creating dir: %d", rc);
    else shell_print(shell, "Created %s", argv[1]);
    return rc;
}

/* Remove file or directory */
static int cmd_rm(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 2) {
        shell_error(shell, "Usage: rm <path>");
        return -EINVAL;
    }
    int rc = fs_unlink(argv[1]);
    if (rc < 0) shell_error(shell, "Error removing: %d", rc);
    else shell_print(shell, "Removed %s", argv[1]);
    return rc;
}

/* Read file content */
static int cmd_cat(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 2) {
        shell_error(shell, "Usage: cat <file>");
        return -EINVAL;
    }

    struct fs_file_t file;
    char buffer[128];
    int rc, bytes_read;

    fs_file_t_init(&file);
    rc = fs_open(&file, argv[1], FS_O_READ);
    if (rc != 0) {
        shell_error(shell, "Failed to open file: %d", rc);
        return rc;
    }

    while ((bytes_read = fs_read(&file, buffer, sizeof(buffer))) > 0) {
        shell_print(shell, "%.*s", bytes_read, buffer);
    }

    fs_close(&file);
    return (bytes_read < 0) ? bytes_read : 0;
}

static int cmd_log_start(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 3) {
        shell_error(shell, "Usage: log start <DID> [filename]");
        return -EINVAL;
    }

    uint8_t did = (uint8_t)strtol(argv[2], NULL, 10);
    if (argc >= 4) {
        snprintf(log_filepath, sizeof(log_filepath), "/lfs/%s", argv[3]);
    }

    log_to_file = true;
    sample_start(did);
    shell_print(shell, "Logging DID %d to %s", did, log_filepath);
    return 0;
}

static int cmd_log_stop(const struct shell *shell, size_t argc, char **argv) {
    log_to_file = false;
    sample_stop();
    shell_print(shell, "Logging stopped");
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_log,
    SHELL_CMD_ARG(start, NULL, "Start logging", cmd_log_start, 3, 1),
    SHELL_CMD(stop, NULL, "Stop logging", cmd_log_stop),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(log, &sub_log, "Sensor logging commands", NULL);
SHELL_CMD_ARG_REGISTER(ls, NULL, "List directory", cmd_ls, 1, 1);
SHELL_CMD_ARG_REGISTER(mkdir, NULL, "Create directory", cmd_mkdir, 2, 0);
SHELL_CMD_ARG_REGISTER(rm, NULL, "Remove file/dir", cmd_rm, 2, 0);
SHELL_CMD_ARG_REGISTER(cat, NULL, "Read file", cmd_cat, 2, 0);
SHELL_CMD_REGISTER(sample, NULL, "Continuous sampling commands:\n  sample s <DID>  - Start sampling for sensor DID\n  sample p        - Stop sampling\n  sample w <rate> - Set sampling rate in seconds", cmd_sample);
 