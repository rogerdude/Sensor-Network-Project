#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <time.h>
#include <stdlib.h>
#include "rtc.h"

/* Define the RTC node using the node label from your device tree overlay */
#define RTC_NODE DT_NODELABEL(rtcc0)
static const struct device *rtc_dev = DEVICE_DT_GET(RTC_NODE);

/* Baseline values for calendar conversion */
static time_t baseline_epoch = 0;
static uint32_t baseline_ticks = 0;

void rtc_init(void) {
    counter_start(rtc_dev);
    SHELL_STATIC_SUBCMD_SET_CREATE(sub_rtc,
        SHELL_CMD_ARG(r, NULL, "Read RTC time", cmd_rtc_read, 1, 0),
        SHELL_CMD_ARG(w, NULL, "Write RTC time. Usage: rtc w <year> <month> <day> <hour> <min>", cmd_rtc_write, 6, 0),
        SHELL_SUBCMD_SET_END
    );
    
    SHELL_CMD_REGISTER(rtc, &sub_rtc, "RTC commands", NULL);
}

void rtc_set_calendar_time(const struct tm *tm) {
    /* Convert provided time to epoch seconds */
    time_t epoch = mktime((struct tm *)tm);

    /* current tick count from the counter */
    uint32_t ticks;
    counter_get_value(rtc_dev, &ticks);

    baseline_epoch = epoch;
    baseline_ticks = ticks;

}

void rtc_get_calendar_time(struct tm *tm) {
    uint32_t ticks;
    counter_get_value(rtc_dev, &ticks);

    uint32_t delta_ticks = ticks - baseline_ticks;

    uint64_t elapsed_us = counter_ticks_to_us(rtc_dev, delta_ticks);

    uint32_t elapsed_seconds = elapsed_us / 1000000U;

    time_t current_epoch = baseline_epoch + elapsed_seconds;

    struct tm *result = gmtime(&current_epoch);

    *tm = *result;
}


static int cmd_rtc_read(const struct shell *shell, size_t argc, char **argv) {
    struct tm current_tm;
    rtc_get_calendar_time(&current_tm);
  
    shell_print(shell, "Current time: %02d:%02d:%02d %02d-%02d-%04d",
                current_tm.tm_hour,
                current_tm.tm_min,
                current_tm.tm_sec,
                current_tm.tm_mday,
                current_tm.tm_mon + 1,
                current_tm.tm_year + 1900
                );

    return 0;
}

/* Shell command: rtc w <year> <month> <day> <hour> <min>*/
static int cmd_rtc_write(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 6) {
        shell_print(shell, "Usage: rtc w <year> <month> <day> <hour> <min>");
        return -EINVAL;
    }
    
    struct tm set_tm = {0};
    set_tm.tm_year = atoi(argv[1]) - 1900;  /* tm_year is years since 1900 */
    set_tm.tm_mon  = atoi(argv[2]) - 1;      /* tm_mon is 0-11 */
    set_tm.tm_mday = atoi(argv[3]);
    set_tm.tm_hour = atoi(argv[4]);
    set_tm.tm_min  = atoi(argv[5]);
    set_tm.tm_isdst = 0;                    
    
    rtc_set_calendar_time(&set_tm);

    shell_print(shell, "RTC calendar time set successfully");
    return 0;
}