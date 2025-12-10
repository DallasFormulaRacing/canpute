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
typedef struct __attribute__((packed)){
	uint32_t linPotData; //4 bytes
	uint32_t wheelSpeed; //4 bytes
	uint32_t fillerData4bytes; //4 bytes
	uint16_t fillerData2bytes; //2 bytes
	uint8_t brakeTemperature; //1 byte
	uint8_t tireTemperature; //1 byte


	// total = 16 bytes i guess!

}NodeDataTypeDef;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
FDCAN_TxHeaderTypeDef FDCAN2_TxHeader;
FDCAN_RxHeaderTypeDef FDCAN2_RxHeader;
uint8_t FDCAN2_txMessageData[16]; //set to 16 bytes for now
uint8_t FDCAN2_rxMessageData[64];
NodeDataTypeDef nodeData;
float wheelSpeedQueueMsg = 0;
/*
volatile float rpm_current = 0.0f;         // EMA RPM
volatile float rpm_instantaneous = 0.0f;
volatile float rpm_dt = 0.0f;
*/

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
/* Definitions for wheelSpeedFrequency */
osMessageQueueId_t wheelSpeedFrequencyHandle;
const osMessageQueueAttr_t wheelSpeedFrequency_attributes = {
  .name = "wheelSpeedFrequency"
};
/* Definitions for rpmEvent */
osEventFlagsId_t rpmEventHandle;
const osEventFlagsAttr_t rpmEvent_attributes = {
  .name = "rpmEvent"
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

  /* creation of rpmEvent */
  rpmEventHandle = osEventFlagsNew(&rpmEvent_attributes);

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
void Start_canfdTXTask(void *argument)
{
  /* USER CODE BEGIN canfdTXTask */
	initFDCANTransmissionHeader(&FDCAN2_TxHeader);
	uint32_t canfd_message_extended_id = 0x000;

  /* Infinite loop */
  for(;;)
  {
	  configureFDCANTransmissionHeader(&FDCAN2_TxHeader,canfd_message_extended_id,FDCAN_DLC_BYTES_16); //16 byte data size
	  osMutexAcquire(nodeDataMutexHandle,osWaitForever);
    nodeData.linPotData += 1; //just for testing, increment the value each cycle
	  memcpy(FDCAN2_txMessageData, &nodeData, sizeof(NodeDataTypeDef));
	  osMutexRelease(nodeDataMutexHandle);
	if(HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2,&FDCAN2_TxHeader,FDCAN2_txMessageData) != HAL_OK){
		Error_Handler();
	}

    osDelay(100);
  }
  /* USER CODE END canfdTXTask */
}

/* USER CODE BEGIN Header_Start_rpmEvalTask */
/**
* @brief Function implementing the rpmEvalTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Start_rpmEvalTask */
void Start_rpmEvalTask(void *argument)
{
  /* USER CODE BEGIN rpmEvalTask */

  /* Infinite loop */
	for (;;)
	{
	    if (osMessageQueueGet(wheelSpeedFrequencyHandle, &wheelSpeedQueueMsg, NULL, osWaitForever) != osOK)
	    {
	      Error_Handler();
	    }
	    else
	    {
	      /* Check if it is the correct message */
	      if(wheelSpeedQueueMsg > 0.0f)
	      {
	        /* Toggle LED1 (LED_GREEN) */
	    	  osMutexAcquire(nodeDataMutexHandle,osWaitForever);
	    	  nodeData.wheelSpeed = wheelSpeedQueueMsg;
	    	  osMutexRelease(nodeDataMutexHandle);

	        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	      }
	    }

	}
  /* USER CODE END rpmEvalTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

