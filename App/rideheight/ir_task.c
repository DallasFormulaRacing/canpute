#include "app_freertos.h"
#include "app_globals.h"
#include "ride_height.h"
#include "VL53L4CD_api.h"
#include "app_config.h"

/* Non-static so they can be watched in the debugger during bring-up */
RideHeightData_t ride_height_latest;
RideHeightState_t ride_height_state = RIDE_HEIGHT_NOT_STARTED;
uint16_t ride_height_sensor_id;
static osMutexId_t ride_height_mutexHandle;
static bool ride_height_valid = false;

bool RideHeight_GetLatest(RideHeightData_t *out)
{
    if (out == NULL || ride_height_mutexHandle == NULL) {
        return false;
    }

    osMutexAcquire(ride_height_mutexHandle, osWaitForever);
    bool valid = ride_height_valid;
    if (valid) {
        *out = ride_height_latest;
    }
    osMutexRelease(ride_height_mutexHandle);

    return valid;
}

static int16_t RideHeight_LookupOffset(void)
{
    for (size_t i = 0; i < sizeof(RideHeight_Offset_Table) / sizeof(RideHeight_Offset_Table[0]); i++) {
        if (RideHeight_Offset_Table[i].nodeType == self_node_id) {
            return RideHeight_Offset_Table[i].offset_mm;
        }
    }

    return 0;
}

static RideHeightState_t RideHeight_Init(void)
{
    ride_height_sensor_id = 0;

    // Sensor boot time is 1.2 ms max after power up
    osDelay(2);

    if (VL53L4CD_GetSensorId(RIDE_HEIGHT_I2C_ADDR, &ride_height_sensor_id) != VL53L4CD_ERROR_NONE ||
        ride_height_sensor_id != RIDE_HEIGHT_SENSOR_ID) {
        return RIDE_HEIGHT_ERR_NO_SENSOR;
    }

    if (VL53L4CD_SensorInit(RIDE_HEIGHT_I2C_ADDR) != VL53L4CD_ERROR_NONE ||
        VL53L4CD_SetRangeTiming(RIDE_HEIGHT_I2C_ADDR, RIDE_HEIGHT_TIMING_BUDGET_MS,
                                RIDE_HEIGHT_INTER_MEAS_MS) != VL53L4CD_ERROR_NONE ||
        VL53L4CD_SetOffset(RIDE_HEIGHT_I2C_ADDR, RIDE_HEIGHT_OFFSET_MM) != VL53L4CD_ERROR_NONE) {
        return RIDE_HEIGHT_ERR_INIT;
    }

    if (VL53L4CD_StartRanging(RIDE_HEIGHT_I2C_ADDR) != VL53L4CD_ERROR_NONE) {
        return RIDE_HEIGHT_ERR_START;
    }

    return RIDE_HEIGHT_RUNNING;
}

void start_ride_height(void *argument)
{
    uint8_t seq = 0;

    ride_height_mutexHandle = osMutexNew(NULL);
    if (ride_height_mutexHandle == NULL) Error_Handler();

    // Keep retrying so an unplugged sensor doesn't take down the rest of the node
    while ((ride_height_state = RideHeight_Init()) != RIDE_HEIGHT_RUNNING) {
        osDelay(500);
    }

    for (;;)
    {
        uint8_t data_ready = 0;
        VL53L4CD_ResultsData_t result;

        // Polling mode, GPIO1 data ready interrupt is not wired to the MCU
        if (VL53L4CD_CheckForDataReady(RIDE_HEIGHT_I2C_ADDR, &data_ready) != VL53L4CD_ERROR_NONE ||
            data_ready == 0U) {
            osDelay(2);
            continue;
        }

        VL53L4CD_GetResult(RIDE_HEIGHT_I2C_ADDR, &result);
        // Sensor holds the next measurement until the interrupt is cleared
        VL53L4CD_ClearInterrupt(RIDE_HEIGHT_I2C_ADDR);

        RideHeightData_t sample = {
            .timestamp_ms = osKernelGetTickCount(),
            .distance_mm  = result.distance_mm,
            .sigma_mm     = result.sigma_mm,
            .signal_kcps  = result.signal_rate_kcps,
            .range_status = result.range_status,
            .seq          = seq++,
        };

        osMutexAcquire(ride_height_mutexHandle, osWaitForever);
        ride_height_latest = sample;
        ride_height_valid = true;
        osMutexRelease(ride_height_mutexHandle);
    }
}
