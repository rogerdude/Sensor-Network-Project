// const struct device *magnetometer_dev = DEVICE_DT_GET(DT_ALIAS(lis3mdl));
#include "magnetometer_sensor.h"
#include "sampling.h"
LOG_MODULE_REGISTER(magnetometer_sensor, LOG_LEVEL_DBG);
extern struct k_mutex sampling_rate_mutex;
extern int global_sampling_rate;
extern struct sampling_ctl sampling_settings;

static struct sensor_value mag_x, mag_y, mag_z;

#define MAGNETOMETER_NODE DT_ALIAS(st_lis3mdl_magn)

#define RING_BUFFER_SIZE 1000
 
RING_BUF_DECLARE(mag_ring_buf, RING_BUFFER_SIZE);   

const struct device *magnetometer_dev = DEVICE_DT_GET_ONE(st_lis3mdl_magn);



void read_mag_continous() {
    if (!device_is_ready(magnetometer_dev)) {
        LOG_ERR("Magnetometer sensor not ready\n");
        return;
    }

   // int sampling_rate = 2000;  

    while (sampling_settings.ctn_mag_sampling_on) {
      //  int new_rate; 
        int sampling_rate;
        k_mutex_lock(&sampling_rate_mutex, K_FOREVER);
        sampling_rate = global_sampling_rate;
        k_mutex_unlock(&sampling_rate_mutex);

        // if (k_msgq_get(&sampling_rate_msgq, &new_rate, K_NO_WAIT) == 0) {
        //     sampling_rate = new_rate;
        //     LOG_INF("New sampling rate: %d ms", sampling_rate);
        // }
 
        if (sensor_sample_fetch(magnetometer_dev) < 0) {
            LOG_ERR("Failed to fetch sensor sample");
            k_sleep(K_MSEC(sampling_rate));
            continue;
        } 
        if (sensor_channel_get(magnetometer_dev, SENSOR_CHAN_MAGN_X, &mag_x) < 0) {
            LOG_ERR("Failed to get magnetometer X channel");
            k_sleep(K_MSEC(sampling_rate));
            continue;
        }
        if (sensor_channel_get(magnetometer_dev, SENSOR_CHAN_MAGN_Y, &mag_y) < 0) {
            LOG_ERR("Failed to get magnetometer Y channel");
            k_sleep(K_MSEC(sampling_rate));
            continue;
        }
        if (sensor_channel_get(magnetometer_dev, SENSOR_CHAN_MAGN_Z, &mag_z) < 0) {
            LOG_ERR("Failed to get magnetometer Z channel");
            k_sleep(K_MSEC(sampling_rate));
            continue;
        }

        // if (sensor_channel_get(magnetometer_dev, SENSOR_CHAN_MAGN_XYZ, magn_xyz) < 0) {
        //     LOG_ERR("Failed to get magnetometer Z channel");
        //     k_sleep(K_MSEC(sampling_rate));
        //     continue;
        // }
       

        
        
  
         struct mag_data mag_val = {
            .x = mag_x,
            .y = mag_y,
            .z = mag_z,
        };
        // if (ring_buf_put(&mag_ring_buf, (uint8_t *)&mag_val, sizeof(mag_val)) < sizeof(mag_val)) {
        //   }
        if (ring_buf_put(&mag_ring_buf, (uint8_t *)&mag_val, sizeof(mag_val)) < sizeof(mag_val)) {
            LOG_ERR("Failed to put magnetometer data in ring buffer");
        // } else {
        //     LOG_INF("Magnetometer data written to ring buffer: X=%d.%06d, Y=%d.%06d, Z=%d.%06d",
        //             mag_val.x.val1, mag_val.x.val2,
        //             mag_val.y.val1, mag_val.y.val2,
        //             mag_val.z.val1, mag_val.z.val2);
                  
        }

 
        // if (ring_buf_put(&mag_ring_buf, (uint8_t *)&mag_val, sizeof(mag_val)) < sizeof(mag_val)) {
 
        // }

        // LOG_INF("Magnetometer - X: %d.%06d, Y: %d.%06d, Z: %d.%06d",
        //         mag_x.val1, mag_x.val2,
        //         mag_y.val1, mag_y.val2,
        //         mag_z.val1, mag_z.val2);
 
        k_sleep(K_MSEC(sampling_rate));
    }
}

// int get_latest_mag_val() {
//     struct mag_data mag_val;
//     if (ring_buf_get(&mag_ring_buf, (uint8_t *)&mag_val, sizeof(mag_val)) == sizeof(mag_val)) {
//         return 0; 
//     } else {
//        return -1;  
//     }
// }
int get_latest_mag_val(struct mag_data *mag_val) {
    if (ring_buf_get(&mag_ring_buf, (int *)mag_val, sizeof(*mag_val)) == sizeof(*mag_val)) {
        // LOG_INF("Magnetometer data retrieved from ring buffer: X=%d.%06d, Y=%d.%06d, Z=%d.%06d",
        //         mag_val->x.val1, mag_val->x.val2,
        //         mag_val->y.val1, mag_val->y.val2,
        //         mag_val->z.val1, mag_val->z.val2);
        return 0;
    } else {
      //  LOG_ERR("No magnetometer data available in ring buffer");
        return -1;
    }
}
void read_mag() { 
 //   magnetometer_dev = DEVICE_DT_GET_ONE(st_lis3mdl_magn);
    if (!device_is_ready(magnetometer_dev)) {
        LOG_ERR("magnetometer sensor not ready\n");
        return;
    }




    
   // while (1) {
        if (sensor_sample_fetch(magnetometer_dev) < 0) {
            LOG_ERR("sample update error\n");
      //      k_sleep(K_MSEC(2000));
            return;
        } else if (sensor_channel_get(magnetometer_dev, SENSOR_CHAN_MAGN_X, &mag_x)) {
            LOG_ERR("Cannot read magnetometer X channel\n");
        //    k_sleep(K_MSEC(2000));
            return;
        } else if (sensor_channel_get(magnetometer_dev, SENSOR_CHAN_MAGN_Y, &mag_y)) {
            LOG_ERR("Cannot read magnetometer Y channel\n");
          //  k_sleep(K_MSEC(2000));
            return;
        } else if (sensor_channel_get(magnetometer_dev, SENSOR_CHAN_MAGN_Z, &mag_z)) {
            LOG_ERR("Cannot read magnetometer Z channel\n");
            //k_sleep(K_MSEC(2000));
            return;
        }
    //    LOG_INF("Raw X: %f, Y: %f, Z: %f", mag_x.val1, mag_y.val1, mag_z.val1);
      //  LOG_INF("test 2 Raw X: %d, Y: %d, Z: %d", SENSOR_CHAN_MAGN_X, SENSOR_CHAN_MAGN_Y, SENSOR_CHAN_MAGN_Z);
        LOG_INF("X: %d.%06d, Y: %d.%06d, Z: %d.%06d", 
            mag_x.val1, mag_x.val2, 
            mag_y.val1, mag_y.val2, 
            mag_z.val1, mag_z.val2);

        //  if (sensor_channel_get(magnetometer_dev, SENSOR_CHAN_MAGN_XYZ, magn_xyz) < 0) {
        //     LOG_ERR("Failed to get magnetometer Z channel");
        //     //k_sleep(K_MSEC(2000)); 
        // }
       
        

        
            // LOG_INF("x: %f, y: %f, z: %f",
            //     sensor_value_to_double(&mag_x),
            //     sensor_value_to_double(&mag_y),
            //     sensor_value_to_double(&mag_z));
    
        
   //     k_sleep(K_MSEC(2000));
  //  }
}