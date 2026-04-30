#include "app_freertos.h"
#include "app_globals.h"

void start_wheel_speed(void *argument)
{
    float rxFrequency = 0;
    for (;;)
    {
        (void)osMessageQueueGet(wheelSpeedFrequencyHandle, &rxFrequency, NULL, 500);
    }
}
