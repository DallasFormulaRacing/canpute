#ifndef APP_GLOBALS_H
#define APP_GLOBALS_H

#include "stm32h5xx_hal.h"
#include "cmsis_os2.h"
#include "types.h"

extern NodeHardwareID_t self_node_id;
extern TemperatureSpeedData_t temperatureSpeedData;

extern FDCAN_HandleTypeDef hfdcan2;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;

extern osMutexId_t temperatureSpeedDataMutexHandle;
extern osEventFlagsId_t systemEventFlagsHandle;
extern osTimerId_t standaloneTimerHandle;
extern osMessageQueueId_t wheelSpeedFrequencyHandle;
extern osMessageQueueId_t canfd_rx_queueHandle;
extern osThreadId_t imuHandle;
extern volatile uint32_t can_tx_status;
extern volatile uint32_t can_tx_error;
extern volatile uint32_t debug_rx_level;
extern volatile uint32_t can_tx_free;
extern volatile uint32_t debug_start_status;
extern volatile uint32_t debug_notify_status;
#endif
