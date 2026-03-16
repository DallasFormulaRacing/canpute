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
/* Definitions for canfdTXTask */
osThreadId_t canfdTXTaskHandle;
const osThreadAttr_t canfdTXTask_attributes = {
  .name = "canfdTXTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};
/* Definitions for rpmEvalTask */
osThreadId_t rpmEvalTaskHandle;
const osThreadAttr_t rpmEvalTask_attributes = {
  .name = "rpmEvalTask",
  .priority = (osPriority_t) osPriorityLow,
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
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
  /* creation of canfdTXTask */
  canfdTXTaskHandle = osThreadNew(Start_canfdTXTask, NULL, &canfdTXTask_attributes);

  /* creation of rpmEvalTask */
  rpmEvalTaskHandle = osThreadNew(Start_rpmEvalTask, NULL, &rpmEvalTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  if((canfdTXTaskHandle == NULL)||(rpmEvalTaskHandle == NULL))
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
/* USER CODE BEGIN Header_Start_canfdTXTask */
/**
* @brief Function implementing the canfdTXTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_canfdTXTask */
__weak void Start_canfdTXTask(void *argument)
{
  /* USER CODE BEGIN canfdTXTask */
	
  /* USER CODE END canfdTXTask */
}

/* USER CODE BEGIN Header_Start_rpmEvalTask */
/**
* @brief Function implementing the rpmEvalTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_rpmEvalTask */
__weak void Start_rpmEvalTask(void *argument)
{
  /* USER CODE BEGIN rpmEvalTask */

  
  /* USER CODE END rpmEvalTask */
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

