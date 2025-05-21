#include "temp_humidity_sensor.h"
#include "sampling.h" 

LOG_MODULE_REGISTER(temp_humidity_sensor, LOG_LEVEL_DBG);

static struct sensor_value temp, hum;

#define TEMP_HUMIDITY_NODE DT_ALIAS(st_hts221)
extern struct k_mutex sampling_rate_mutex;
extern int global_sampling_rate;
static const struct device *temp_humidity_dev = DEVICE_DT_GET_ONE(st_hts221);
extern struct sampling_ctl sampling_settings;

#define RING_BUFFER_SIZE 10  

RING_BUF_DECLARE(temp_ring_buf, RING_BUFFER_SIZE);  
RING_BUF_DECLARE(hum_ring_buf, RING_BUFFER_SIZE);   

void read_temp() { 
   // temp_humidity_dev = DEVICE_DT_GET_ONE(st_hts221);
    if (!device_is_ready(temp_humidity_dev)) {
        LOG_ERR("temp/humdity sensor not ready\n");
        return;
    }
 
//while (1) {
    if (sensor_sample_fetch(temp_humidity_dev) < 0) {
        LOG_ERR("Failed to fetch sensor sample");
        k_sleep(K_MSEC(2000));
  //      continue;
    }

    if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
        LOG_ERR("Failed to get temperature channel");
        k_sleep(K_MSEC(2000));
    //    continue;
    }
 
    LOG_INF("Sensor Temp: %dC", temp.val1); //PROBABLY USE PIPE FOR THIS
  
}
 

void read_temp_continous() { 
    if (!device_is_ready(temp_humidity_dev)) {
        LOG_ERR("Temperature sensor not ready");
        return;
    }

    while (sampling_settings.ctn_temp_sampling_on) { 
        // Get the curr sampling rate
        int sampling_rate;
        k_mutex_lock(&sampling_rate_mutex, K_FOREVER);
        sampling_rate = global_sampling_rate;
        k_mutex_unlock(&sampling_rate_mutex);

        if (sensor_sample_fetch(temp_humidity_dev) < 0) {
            LOG_ERR("Failed to fetch sensor sample");
            k_sleep(K_MSEC(sampling_rate));
            continue;
        }

        if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
            LOG_ERR("Failed to get temperature channel");
            k_sleep(K_MSEC(sampling_rate));
            continue;
        }

        int temp_val = temp.val1;
        if (ring_buf_put(&temp_ring_buf, (uint8_t *)&temp_val, sizeof(temp_val)) < 0) {
            LOG_ERR("Failed to put temperature value in ring buffer");
        }

   //     LOG_INF("Raw Temp: %dC", temp.val1);

        k_sleep(K_MSEC(sampling_rate));
    }
}

 int get_latest_temp_val(int *temp_value) {
     
    if (ring_buf_get(&temp_ring_buf, (uint8_t *)temp_value, sizeof(*temp_value)) == sizeof(*temp_value)) {
        return 0; 
    } else {
    //    LOG_ERR("No temperature value available in ring buffer");
        return -1;  
    }
}

int get_latest_hum_val(int *hum_val) {
     
    if (ring_buf_get(&hum_ring_buf, (uint8_t *)hum_val, sizeof(*hum_val)) == sizeof(*hum_val)) {
        return 0; 
    } else {
   //     LOG_ERR("No temperature value available in ring buffer");
        return -1;  
    }
}
 
void read_hum_continous() { 
    if (!device_is_ready(temp_humidity_dev)) {
        LOG_ERR("Humidity sensor not ready");
        return;
    }

    while (sampling_settings.ctn_hum_sampling_on) { 
        // Get the current sampling rate
        int sampling_rate;
        k_mutex_lock(&sampling_rate_mutex, K_FOREVER);
        sampling_rate = global_sampling_rate;
        k_mutex_unlock(&sampling_rate_mutex);

        if (sensor_sample_fetch(temp_humidity_dev) < 0) {
            LOG_ERR("Failed to fetch sensor sample");
            k_sleep(K_MSEC(sampling_rate));
            continue;
        }

        if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
            LOG_ERR("Failed to get humidity channel");
            k_sleep(K_MSEC(sampling_rate));
            continue;
        }

        int hum_val = hum.val1;
        if (ring_buf_put(&hum_ring_buf, (uint8_t *)&hum_val, sizeof(hum_val)) < 0) {
            LOG_ERR("Failed to put humidity value in ring buffer");
        }

      //  LOG_INF("Raw Humidity: %d%%", hum.val1);

        k_sleep(K_MSEC(sampling_rate));
    }
}
  
 
void read_hum() { 
    // temp_humidity_dev = DEVICE_DT_GET_ONE(st_hts221);
     if (!device_is_ready(temp_humidity_dev)) {
         LOG_ERR("temp/humdity sensor not ready\n");
         return;
     }
   
 //while (1) {
     if (sensor_sample_fetch(temp_humidity_dev) < 0) {
         LOG_ERR("Failed to fetch sensor sample");
         k_sleep(K_MSEC(2000));
    //     continue;
     }
 
    //  if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) < 0) {
    //      LOG_ERR("Failed to get temperature channel");
    //      k_sleep(K_MSEC(2000));
    //      continue;
    //  }
 
     if (sensor_channel_get(temp_humidity_dev, SENSOR_CHAN_HUMIDITY, &hum) < 0) {
         LOG_ERR("Failed to get humidity channel");
         k_sleep(K_MSEC(2000));
    //     continue;
     }
 
    // LOG_INF("Temperature: %.1f C", sensor_value_to_double(&temp));
    // LOG_INF("Relative Humidity: %.1f%%", sensor_value_to_double(&hum));
   
   
     //LOG_INF("Raw Humidity: %d.%06d%%", hum.val1, hum.val2);
     LOG_INF("Raw Humidity: %d%%", hum.val1);
    
    // k_sleep(K_MSEC(2000)); // Sleep for 2 seconds before the next reading
 ///}
 }
 
 
 
 
 
 
 
     
  