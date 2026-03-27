#include "app_freertos.h"
#include "app_globals.h"

void start_wheel_speed(void *argument)
{
    float rxFrequency = 0;
    for (;;)
    {
        osStatus_t status = osMessageQueueGet(wheelSpeedFrequencyHandle, &rxFrequency, NULL, 500);

        osMutexAcquire(nodeDataMutexHandle, osWaitForever);
        if (status == osOK) {
            nodeData.wheelSpeed = (uint32_t)rxFrequency;
        } else {
            nodeData.wheelSpeed = 0;
        }
        osMutexRelease(nodeDataMutexHandle);
    }
}
