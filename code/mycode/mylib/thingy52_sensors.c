#include <thingy52_sensors.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ccs811.h>
#include <zephyr/kernel.h>

static bool app_fw_2;

const struct device *const temp_hum = DEVICE_DT_GET_ONE(st_hts221);
const struct device *const gas = DEVICE_DT_GET_ONE(ams_ccs811);
const struct device *const acc = DEVICE_DT_GET_ANY(st_lis2dh);

int init_thingy52_sensors(void) {

    struct ccs811_configver_type cfgver;

	if (!device_is_ready(temp_hum) || !device_is_ready(gas) 
        || !device_is_ready(acc)) {
		printk("sensor: device not ready.\n");
		return -1;
	}

    int rc = ccs811_configver_fetch(gas, &cfgver);
	if (rc == 0) {
		app_fw_2 = (cfgver.fw_app_version >> 8) > 0x11;
	} else {
        return -2;
    }

    return 0;
}

void thingy52_process_sample(struct SensorData* data) {

    if (sensor_sample_fetch(temp_hum) < 0) {
		printk("Sensor temp hum update error\n");
		return;
	}

	if (sensor_channel_get(temp_hum, SENSOR_CHAN_AMBIENT_TEMP, &(data->temp)) < 0) {
		printk("Cannot read HTS221 temperature channel\n");
		return;
	}

	if (sensor_channel_get(temp_hum, SENSOR_CHAN_HUMIDITY, &(data->hum)) < 0) {
		printk("Cannot read HTS221 humidity channel\n");
		return;
	}

    int rc = sensor_sample_fetch(gas);

    if (rc == 0) {
        if (sensor_channel_get(gas, SENSOR_CHAN_VOC, &(data->tvoc)) < 0) {
            printk("Cannot read CCS811 gas channel\n");
            return;
        }
    }

    rc = sensor_sample_fetch(acc);
    if (rc == 0) {
		rc = sensor_channel_get(acc, SENSOR_CHAN_ACCEL_XYZ, data->accel);
	}
	if (rc < 0) {
		printf("ERROR: Accelerometer failed: %d\n", rc);
        return;
    }
}

void thingy52_convert_data(struct SensorData* data, struct SensorValues* values) {
    values->temp = sensor_value_to_double(&(data->temp));
    values->hum = sensor_value_to_double(&(data->hum));
    values->tvoc = sensor_value_to_double(&(data->tvoc));
    values->accel[0] = sensor_value_to_double(&(data->accel[0]));
    values->accel[1] = sensor_value_to_double(&(data->accel[1]));
    values->accel[2] = sensor_value_to_double(&(data->accel[2]));
}