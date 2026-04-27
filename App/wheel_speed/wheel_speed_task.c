#include "app_freertos.h"
#include "app_globals.h"

static uint32_t WheelSpeed_ToMilliHz(float frequency_hz)
{
    if (frequency_hz <= 0.0f) {
        return 0;
    }

    float frequency_mHz = (frequency_hz * 1000.0f) + 0.5f;
    if (frequency_mHz >= (float)UINT32_MAX) {
        return UINT32_MAX;
    }

    return (uint32_t)frequency_mHz;
}

void start_wheel_speed(void *argument)
{
    float rxFrequency = 0;
    for (;;)
    {
        osStatus_t status = osMessageQueueGet(wheelSpeedFrequencyHandle, &rxFrequency, NULL, 500);

        osMutexAcquire(temperatureSpeedDataMutexHandle, osWaitForever);
        if (status == osOK) {
            temperatureSpeedData.wheelSpeed_mHz = WheelSpeed_ToMilliHz(rxFrequency);
        } else {
            temperatureSpeedData.wheelSpeed_mHz = 0;
        }
        osMutexRelease(temperatureSpeedDataMutexHandle);
    }
}
