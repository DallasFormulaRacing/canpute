#ifndef RIDE_HEIGHT_H
#define RIDE_HEIGHT_H

#include <stdbool.h>
#include "types.h"

#define RIDE_HEIGHT_I2C_ADDR          0x52  // 8-bit address (7-bit 0x29)
#define RIDE_HEIGHT_SENSOR_ID         0xEBAA
#define RIDE_HEIGHT_TIMING_BUDGET_MS  20    // 10..200 ms, longer = less noise, 20 ms = 50 Hz
#define RIDE_HEIGHT_INTER_MEAS_MS     0     // 0 = continuous ranging
#define RIDE_HEIGHT_OFFSET_MM         0     // TODO: per-sensor value from VL53L4CD_CalibrateOffset()

typedef enum {
    RIDE_HEIGHT_NOT_STARTED = 0,
    RIDE_HEIGHT_ERR_NO_SENSOR,      // no I2C response / wrong sensor ID
    RIDE_HEIGHT_ERR_INIT,           // SensorInit/SetRangeTiming failed
    RIDE_HEIGHT_ERR_START,          // StartRanging failed
    RIDE_HEIGHT_RUNNING
} RideHeightState_t;

bool RideHeight_GetLatest(RideHeightData_t *out);

#endif /* RIDE_HEIGHT_H */
