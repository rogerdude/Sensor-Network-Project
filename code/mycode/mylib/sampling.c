#include <zephyr/kernel.h>
#include "sampling.h"

#define MSGQ_MAX_MSGS 10
#define MSGQ_MSG_SIZE sizeof(int)

// Define the message queue
K_MSGQ_DEFINE(sampling_rate_msgq, MSGQ_MSG_SIZE, MSGQ_MAX_MSGS, 4);

struct sampling_ctl sampling_settings = {
    .ctn_sampling_on = false,
    .ctn_temp_sampling_on = false,
    .ctn_hum_sampling_on = false,
    .ctn_pressure_sampling_on = false,
    .ctn_mag_sampling_on = false,
};
 
