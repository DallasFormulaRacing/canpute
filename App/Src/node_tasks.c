#include "node_tasks.h"
#include "commands.h"
#include "cmsis_os2.h"
#include <string.h>


FDCAN_TxHeaderTypeDef FDCAN2_TxHeader;
FDCAN_RxHeaderTypeDef FDCAN2_RxHeader;
uint8_t FDCAN2_txMessageData[16];
uint8_t FDCAN2_rxMessageData[64];
NodeDataTypeDef nodeData;
float wheelSpeedQueueMsg = 0;
/*
volatile float rpm_current = 0.0f;         // EMA RPM
volatile float rpm_instantaneous = 0.0f;
volatile float rpm_dt = 0.0f;
*/


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

}


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
	      }
	    }

	}
}


