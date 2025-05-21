#ifndef _THINGY52_SENSORS_H_
#define _THINGY52_SENSORS_H_

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

struct SensorData {
    struct sensor_value temp, hum, tvoc, accel[3];
};

struct SensorValues {
    double temp, hum, tvoc, accel[3];
};

int init_thingy52_sensors(void);
void thingy52_process_sample(struct SensorData* data);
void thingy52_convert_data(struct SensorData* data, struct SensorValues* values);

#endif