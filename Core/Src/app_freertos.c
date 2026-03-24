/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : FreeRTOS applicative file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "types.h"
#include "can_utils.h"
#include "app_globals.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern osEventFlagsId_t systemEventFlagsHandle;

/* USER CODE END Variables */
/* Definitions for canfd_tx */
osThreadId_t canfd_txHandle;
const osThreadAttr_t canfd_tx_attributes = {
  .name = "canfd_tx",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for wheel_speed */
osThreadId_t wheel_speedHandle;
const osThreadAttr_t wheel_speed_attributes = {
  .name = "wheel_speed",
  .priority = (osPriority_t) osPriorityLow,
  .stack_size = 128 * 4
};
/* Definitions for tire_temp */
osThreadId_t tire_tempHandle;
const osThreadAttr_t tire_temp_attributes = {
  .name = "tire_temp",
  .priority = (osPriority_t) osPriorityBelowNormal1,
  .stack_size = 1024 * 4
};
/* Definitions for canfd_rx */
osThreadId_t canfd_rxHandle;
const osThreadAttr_t canfd_rx_attributes = {
  .name = "canfd_rx",
  .priority = (osPriority_t) osPriorityHigh,
  .stack_size = 128 * 4
};
/* Definitions for nodeDataMutex */
osMutexId_t nodeDataMutexHandle;
const osMutexAttr_t nodeDataMutex_attributes = {
  .name = "nodeDataMutex"
};
/* Definitions for standaloneTimer */
osTimerId_t standaloneTimerHandle;
const osTimerAttr_t standaloneTimer_attributes = {
  .name = "standaloneTimer"
};
/* Definitions for wheelSpeedFrequency */
osMessageQueueId_t wheelSpeedFrequencyHandle;
const osMessageQueueAttr_t wheelSpeedFrequency_attributes = {
  .name = "wheelSpeedFrequency"
};
/* Definitions for systemEventFlags */
osEventFlagsId_t systemEventFlagsHandle;
const osEventFlagsAttr_t systemEventFlags_attributes = {
  .name = "systemEventFlags"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{

}

__weak unsigned long getRunTimeCounterValue(void)
{
  return 0;
}
/* USER CODE END 1 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* creation of nodeDataMutex */
  nodeDataMutexHandle = osMutexNew(&nodeDataMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  if(nodeDataMutexHandle == NULL)
  {
    Error_Handler();
  }
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */
  /* creation of standaloneTimer */
  standaloneTimerHandle = osTimerNew(StandaloneTimer_Callback, osTimerPeriodic, NULL, &standaloneTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */
  /* creation of wheelSpeedFrequency */
  wheelSpeedFrequencyHandle = osMessageQueueNew (16, sizeof(float), &wheelSpeedFrequency_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  CAN_RxQueue_Init();
  /* USER CODE END RTOS_QUEUES */
  /* creation of canfd_tx */
  canfd_txHandle = osThreadNew(start_canfd_tx, NULL, &canfd_tx_attributes);

  /* creation of wheel_speed */
  wheel_speedHandle = osThreadNew(start_wheel_speed, NULL, &wheel_speed_attributes);

  /* creation of tire_temp */
  tire_tempHandle = osThreadNew(start_tire_temp, NULL, &tire_temp_attributes);

  /* creation of canfd_rx */
  canfd_rxHandle = osThreadNew(start_canfd_rx, NULL, &canfd_rx_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  if((canfd_txHandle == NULL)||(wheel_speedHandle == NULL) || (tire_tempHandle == NULL) || (canfd_rxHandle == NULL))
  {
    Error_Handler();
  }
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* creation of systemEventFlags */
  systemEventFlagsHandle = osEventFlagsNew(&systemEventFlags_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}
/* USER CODE BEGIN Header_start_canfd_tx */
/**
* @brief Function implementing the canfd_tx thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_start_canfd_tx */
__weak void start_canfd_tx(void *argument)
{
  /* USER CODE BEGIN canfd_tx */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END canfd_tx */
}

/* USER CODE BEGIN Header_start_wheel_speed */
/**
* @brief Function implementing the wheel_speed thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_start_wheel_speed */
__weak void start_wheel_speed(void *argument)
{
  /* USER CODE BEGIN wheel_speed */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END wheel_speed */
}

/* USER CODE BEGIN Header_start_tire_temp */
/**
* @brief Function implementing the tire_temp thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_start_tire_temp */
__weak void start_tire_temp(void *argument)
{
  /* USER CODE BEGIN tire_temp */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END tire_temp */
}

/* USER CODE BEGIN Header_start_canfd_rx */
/**
* @brief Function implementing the canfd_rx thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_start_canfd_rx */
__weak void start_canfd_rx(void *argument)
{
  /* USER CODE BEGIN canfd_rx */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END canfd_rx */
}

/* StandaloneTimer_Callback function */
__weak void StandaloneTimer_Callback(void *argument)
{
  /* USER CODE BEGIN StandaloneTimer_Callback */
  /* USER CODE END StandaloneTimer_Callback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

