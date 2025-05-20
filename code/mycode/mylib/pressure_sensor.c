#include "pressure_sensor.h"
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h> 
#include <stdio.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include "sampling.h"

LOG_MODULE_REGISTER(pressure_sensor, LOG_LEVEL_DBG);

#define PRESS_NODE DT_ALIAS(st_lps22hb_press)

#define RING_BUFFER_SIZE 10  
  
RING_BUF_DECLARE(press_ring_buf, RING_BUFFER_SIZE);   

//
// const struct device *pressure_dev = DEVICE_DT_GET_ANY(PRESS_NODE);
const struct device *pressure_dev = DEVICE_DT_GET_ONE(st_lps22hb_press);
struct sensor_value pressure, temp;
extern struct k_mutex sampling_rate_mutex;
extern int global_sampling_rate;
extern struct sampling_ctl sampling_settings;

//= DEVICE_DT_GET(DT_ALIAS(lps22hb));

void read_pressure_continous() {
    if (!device_is_ready(pressure_dev)) {
        LOG_ERR("Pressure sensor not ready\n");
        return;
    }

    int sampling_rate = 2000;  

    while (sampling_settings.ctn_pressure_sampling_on) {
        int sampling_rate; 
        k_mutex_lock(&sampling_rate_mutex, K_FOREVER);
        sampling_rate = global_sampling_rate;
        k_mutex_unlock(&sampling_rate_mutex);

        // if (k_msgq_get(&sampling_rate_msgq, &new_rate, K_NO_WAIT) == 0) {
        //     sampling_rate = new_rate;
        //     LOG_INF("New sampling rate: %d ms", sampling_rate);
        // }
 
        if (sensor_sample_fetch(pressure_dev) < 0) {
            LOG_ERR("Failed to fetch sensor sample");
            k_sleep(K_MSEC(sampling_rate));
            continue;
        }
 
        if (sensor_channel_get(pressure_dev, SENSOR_CHAN_PRESS, &pressure) < 0) {
            LOG_ERR("Failed to get pressure channel");
            k_sleep(K_MSEC(sampling_rate));
            continue;
        }
        int press_val = pressure.val1;
        if (ring_buf_put(&press_ring_buf, (uint8_t *)&press_val, sizeof(press_val)) < 0) {
            LOG_ERR("Failed to put press value in ring buffer");
        }
 //       LOG_INF("Pressure: %d.%06d kPa", pressure.val1, pressure.val2);
 
        k_sleep(K_MSEC(sampling_rate));
    }
}

int get_latest_press_val(int *press_value) {
     
    if (ring_buf_get(&press_ring_buf, (uint8_t *)press_value, sizeof(*press_value)) == sizeof(*press_value)) {
        return 0; 
    } else {
    //    LOG_ERR("No temperature value available in ring buffer");
        return -1;  
    }
}


void read_pressure() {
    if (!device_is_ready(pressure_dev)) {
        LOG_ERR("Pressure sensor not ready\n");
        return;
    }

 //   while (1) {
        if (sensor_sample_fetch(pressure_dev) < 0) {
            printf("Sensor sample update error\n");
          //  k_sleep(K_MSEC(2000));
            return;
        }
    
        else if (sensor_channel_get(pressure_dev, SENSOR_CHAN_PRESS, &pressure) < 0) {
            printf("Cannot read LPS22HB pressure channel\n");
        //    k_sleep(K_MSEC(2000));
            return;
        }
        LOG_INF("Pressure: %d.%06d kPa", pressure.val1);
      //  LOG_INF("Pressure: %.1f kPa", sensor_value_to_double(&pressure));
        
 //   k_sleep(K_MSEC(2000));
      

//    }
}