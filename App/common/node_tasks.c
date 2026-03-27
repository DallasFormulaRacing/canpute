#include "app_freertos.h"
#include "app_globals.h"
#include "can_utils.h"

void start_canfd_tx(void *argument);
void start_canfd_rx(void *argument);
void start_wheel_speed(void *argument);
void start_tire_temp(void *argument);
void start_imu(void *argument);
void StandaloneTimer_Callback(void *argument);

/* Task handles */
osThreadId_t canfd_txHandle;
osThreadId_t canfd_rxHandle;
osThreadId_t wheel_speedHandle;
osThreadId_t tire_tempHandle;
osThreadId_t imuHandle;

/* Sync objects */
osMutexId_t nodeDataMutexHandle;
osTimerId_t standaloneTimerHandle;
osMessageQueueId_t wheelSpeedFrequencyHandle;
osEventFlagsId_t systemEventFlagsHandle;

void App_RTOS_Init(void)
{
    nodeDataMutexHandle = osMutexNew(NULL);
    if (nodeDataMutexHandle == NULL) Error_Handler();

    standaloneTimerHandle = osTimerNew(StandaloneTimer_Callback, osTimerPeriodic, NULL, NULL);

    wheelSpeedFrequencyHandle = osMessageQueueNew(16, sizeof(float), NULL);
    CAN_RxQueue_Init();

    canfd_txHandle    = osThreadNew(start_canfd_tx,    NULL, &(osThreadAttr_t){ .name = "canfd_tx",    .priority = osPriorityNormal,       .stack_size = 128 * 4 });
    canfd_rxHandle    = osThreadNew(start_canfd_rx,    NULL, &(osThreadAttr_t){ .name = "canfd_rx",    .priority = osPriorityHigh,         .stack_size = 128 * 4 });
    wheel_speedHandle = osThreadNew(start_wheel_speed, NULL, &(osThreadAttr_t){ .name = "wheel_speed", .priority = osPriorityLow,          .stack_size = 128 * 4 });
    tire_tempHandle   = osThreadNew(start_tire_temp,   NULL, &(osThreadAttr_t){ .name = "tire_temp",   .priority = osPriorityBelowNormal1, .stack_size = 1024 * 4 });
    imuHandle         = osThreadNew(start_imu,         NULL, &(osThreadAttr_t){ .name = "imu",         .priority = osPriorityAboveNormal2, .stack_size = 128 * 4 });

    if (canfd_txHandle == NULL || canfd_rxHandle == NULL || wheel_speedHandle == NULL ||
        tire_tempHandle == NULL || imuHandle == NULL)
    {
        Error_Handler();
    }

    systemEventFlagsHandle = osEventFlagsNew(NULL);
}

void StandaloneTimer_Callback(void *argument)
{
    osEventFlagsSet(systemEventFlagsHandle, FLAG_TIMER_TICK);
}
