#ifndef APP_GLOBALS_H
#define APP_GLOBALS_H

#include "stm32h5xx_hal.h"
#include "cmsis_os2.h"
#include "types.h"

extern NodeHardwareID_t self_node_id;
extern NodeDataTypeDef nodeData;

extern FDCAN_HandleTypeDef hfdcan2;
extern I2C_HandleTypeDef hi2c1;

extern osMutexId_t nodeDataMutexHandle;
extern osEventFlagsId_t systemEventFlagsHandle;
extern osTimerId_t standaloneTimerHandle;
extern osMessageQueueId_t wheelSpeedFrequencyHandle;

#endif
