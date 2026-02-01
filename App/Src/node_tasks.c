#include "node_tasks.h"
#include "commands.h"
#include "can_utils.h"
#include "types.h"
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

// Registry of all physical nodes on the car
static const UID_Mapping_t Fleet_Table[] = {
    {{0x12345678, 0xABCDEF01, 0x55667788}, NODE_ID_FRONT_LEFT},
    {{0x87654321, 0x10FEDCBA, 0x99887766}, NODE_ID_FRONT_RIGHT},
    {{0x11223344, 0x55667788, 0x99AABBCC}, NODE_ID_REAR_LEFT},
    {{0x44332211, 0x88776655, 0xCCBBAA99}, NODE_ID_REAR_RIGHT},
    {{0x001E005F, 0x33335101, 0x32313831}, NODE_ID_NUCLEO_1},
    {{0x4D3C2B1A, 0x80706050, 0x40302010}, NODE_ID_NUCLEO_2},
    {{0xDEADBEEF, 0xCAFEBABE, 0xFEEDFACE}, NODE_ID_DASH}
};

NodeHardwareID_t self_node_id = NODE_ID_UNKNOWN;

void Identify_Self(void) {
    uint32_t current_uid[3];
    
    // Using HAL functions for H5 series
    current_uid[0] = HAL_GetUIDw0();
    current_uid[1] = HAL_GetUIDw1();
    current_uid[2] = HAL_GetUIDw2();

    for (int i = 0; i < sizeof(Fleet_Table)/sizeof(UID_Mapping_t); i++) {
        if (current_uid[0] == Fleet_Table[i].uid[0] &&
            current_uid[1] == Fleet_Table[i].uid[1] &&
            current_uid[2] == Fleet_Table[i].uid[2]) {
            
            self_node_id = Fleet_Table[i].nodeType;
            return;
        }
    }
    
    self_node_id = NODE_ID_UNKNOWN; // Node not recognized
}


void Start_canfdTXTask(void *argument)
{
  /* USER CODE BEGIN canfdTXTask */
    Identify_Self();
    

  /* Infinite loop */
  for(;;) {
    Update_Simulated_Sensors();
    
    osMutexAcquire(nodeDataMutexHandle, osWaitForever);
    
    // Send 16 bytes of DAQ data with Priority 2 and Command Type 
    CAN_Transmit(2, 0x000ABC, (uint8_t*)&nodeData, FDCAN_DLC_BYTES_16);
    
    osMutexRelease(nodeDataMutexHandle);
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
void Update_Simulated_Sensors(void) {
    uint32_t tick = HAL_GetTick();

    if (osMutexAcquire(nodeDataMutexHandle, osWaitForever) == osOK) {
        // 4-byte fields: Use a counter or sine-wave simulation
        nodeData.linPotData = (tick / 10) % 4096;      // Simulates 12-bit ADC travel
        nodeData.wheelSpeed = (tick / 50) % 200;       // Simulates 0-200 units

        // 2-byte and 1-byte fields: Use bitwise shifts of the tick
        nodeData.fillerData4bytes = tick;
        nodeData.fillerData2bytes = (uint16_t)(tick & 0xFFFF);
        
        // Simulates temperatures in Celsius
        nodeData.brakeTemperature = (uint8_t)(40 + (tick % 60)); // 40-100C
        nodeData.tireTemperature = (uint8_t)(30 + (tick % 40));  // 30-70C

        osMutexRelease(nodeDataMutexHandle);
    }
}


