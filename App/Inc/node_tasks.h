#ifndef NODE_TASKS_H
#define NODE_TASKS_H

#include "stm32h5xx_hal.h"
#include "cmsis_os2.h"
#include "fdcan.h"
#include "types.h"

// Task Prototypes
void Start_canfdTXTask(void *argument);
void Start_rpmEvalTask(void *argument);

// External handles provided by CubeMX
extern osMutexId_t nodeDataMutexHandle;
extern osMessageQueueId_t wheelSpeedFrequencyHandle;
extern FDCAN_HandleTypeDef hfdcan2;

#endif