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
osMutexId_t temperatureSpeedDataMutexHandle;
osTimerId_t standaloneTimerHandle;
osMessageQueueId_t wheelSpeedFrequencyHandle;
osEventFlagsId_t systemEventFlagsHandle;

#define CANFD_TX_STACK_SIZE       (512U * 4U)
#define CANFD_RX_STACK_SIZE       (512U * 4U)
#define WHEEL_SPEED_STACK_SIZE    (256U * 4U)
#define TIRE_TEMP_STACK_SIZE      (1024U * 4U)
#define IMU_STACK_SIZE            (2048U * 4U)

void App_RTOS_Init(void)
{
    temperatureSpeedDataMutexHandle = osMutexNew(NULL);
    if (temperatureSpeedDataMutexHandle == NULL) Error_Handler();

    standaloneTimerHandle = osTimerNew(StandaloneTimer_Callback, osTimerPeriodic, NULL, NULL);
    if (standaloneTimerHandle == NULL) Error_Handler();

    wheelSpeedFrequencyHandle = osMessageQueueNew(16, sizeof(float), NULL);
    if (wheelSpeedFrequencyHandle == NULL) Error_Handler();

    systemEventFlagsHandle = osEventFlagsNew(NULL);
    if (systemEventFlagsHandle == NULL) Error_Handler();

    CAN_RxQueue_Init();
    if (canfd_rx_queueHandle == NULL) Error_Handler();

    canfd_txHandle    = osThreadNew(start_canfd_tx,    NULL, &(osThreadAttr_t){ .name = "canfd_tx",    .priority = osPriorityNormal,       .stack_size = CANFD_TX_STACK_SIZE });
    canfd_rxHandle    = osThreadNew(start_canfd_rx,    NULL, &(osThreadAttr_t){ .name = "canfd_rx",    .priority = osPriorityHigh,         .stack_size = CANFD_RX_STACK_SIZE });
    wheel_speedHandle = osThreadNew(start_wheel_speed, NULL, &(osThreadAttr_t){ .name = "wheel_speed", .priority = osPriorityLow,          .stack_size = WHEEL_SPEED_STACK_SIZE });
    tire_tempHandle   = osThreadNew(start_tire_temp,   NULL, &(osThreadAttr_t){ .name = "tire_temp",   .priority = osPriorityBelowNormal1, .stack_size = TIRE_TEMP_STACK_SIZE });
    imuHandle         = osThreadNew(start_imu,         NULL, &(osThreadAttr_t){ .name = "imu",         .priority = osPriorityAboveNormal2, .stack_size = IMU_STACK_SIZE });

    if (canfd_txHandle == NULL || canfd_rxHandle == NULL || wheel_speedHandle == NULL ||
        tire_tempHandle == NULL || imuHandle == NULL)
    {
        Error_Handler();
    }

}

void StandaloneTimer_Callback(void *argument)
{
    osEventFlagsSet(systemEventFlagsHandle, FLAG_TIMER_TICK);
}
