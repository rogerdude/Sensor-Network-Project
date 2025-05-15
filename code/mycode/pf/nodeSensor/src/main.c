#include <zephyr/kernel.h>
#include <stdlib.h>
#include "sltb_sensors.h"
#include "rtc.h"
#include "sample.h"
#include "thunderboard_button.h"

int main(void) {
    thunderboard_button_init();
    
    printk("Button initialized successfully\n");

    rtc_init();
    printk("RTC initialized successfully\n");
    
    sltb_sensor_init();
    printk("Sensors initialized successfully\n");

    sample_init();
    printk("Sample module initialized successfully\n");

    return 0;
}