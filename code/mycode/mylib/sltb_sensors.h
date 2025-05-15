#ifndef SLTB_SENSOR_H
#define SLTB_SENSOR_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>

/* Initialize the sensor subsystem */
int sltb_sensor_init(void);

typedef struct {
    float temperature; // in Celsius
    float humidity;    // in percentage
    float pressure;    // in hPa
    float light;       // in lux
} sensor_values_t;

int get_sensor_values(sensor_values_t *values);
int sltb_sensor_init(void);
int sltb_sensors_read(struct sensor_data_t *data);

/* Sensor DIDs */
#define SENSOR_DID_TEMP    0
#define SENSOR_DID_HUMID   1
#define SENSOR_DID_PRESS   2
#define SENSOR_DID_LIGHT   5
#define SENSOR_DID_ALL     15

#define SENSOR_THREAD_STACK_SIZE 1024
#define SENSOR_THREAD_PRIORITY 7

#endif /* SLTB_SENSOR_H */