#include "node_tasks.h"
#include "command_handler.h"
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

extern osEventFlagsId_t systemEventFlagsHandle;

extern osTimerId_t standaloneTimerHandle;

/*
volatile float rpm_current = 0.0f;         // EMA RPM
volatile float rpm_instantaneous = 0.0f;
volatile float rpm_dt = 0.0f;
*/



void Start_canfdTXTask(void *argument)
{
    uint32_t flags;
    uint32_t timeout = 5000; // Start with a 5s wait for Pi boot
    
    // Initial Mode
    CAN_SystemMode_t current_mode = MODE_STARTUP;

    for(;;) {
        // Wait for Pi Sync or the 60Hz Standalone Timer
        flags = osEventFlagsWait(systemEventFlagsHandle, 
                                 FLAG_PI_SYNC | FLAG_TIMER_TICK, 
                                 osFlagsWaitAny, timeout);

        //State Transitions
        if (flags & FLAG_PI_SYNC) {
            current_mode = MODE_PI_LINKED;
            timeout = 500; // Falling back to Standalone if Pi is silent for 500ms
            osTimerStop(standaloneTimerHandle); // Stop standalone pulses while Pi is active
        } 
        else if ((flags & FLAG_TIMER_TICK) || (flags == (uint32_t)osErrorTimeout)) {
            current_mode = MODE_STANDALONE;
            timeout = osWaitForever; // Controlled by the standaloneTimerHandle callback
            osTimerStart(standaloneTimerHandle, 16U); // Ensure 60Hz heartbeat
        }

        //Execution
        Update_Simulated_Sensors();

        osMutexAcquire(nodeDataMutexHandle, osWaitForever);
        // Transmit to Pi using our Priority 2, Target Pi ID, and DAQ Command 
        CAN_Transmit(2, NODE_ID_RASPI, CMD_ID_SENDING_DATA, (uint8_t*)&nodeData, FDCAN_DLC_BYTES_16);
        osMutexRelease(nodeDataMutexHandle);
    }
}


void Start_rpmEvalTask(void *argument)
{
  float rxFrequency = 0;
  for (;;)
  {
    // Wait for 500ms. Car is stopped if no pulses received.
    osStatus_t status = osMessageQueueGet(wheelSpeedFrequencyHandle, &rxFrequency, NULL, 500);

    osMutexAcquire(nodeDataMutexHandle, osWaitForever);
    if (status == osOK) {
        nodeData.wheelSpeed = (uint32_t)rxFrequency; // Convert freq to RPM/Speed as needed
    } else {
        nodeData.wheelSpeed = 0; // Timeout reached, car is stopped
    }
    osMutexRelease(nodeDataMutexHandle);
  }
}
void StandaloneTimer_Callback(void *argument)
{
  osEventFlagsSet(systemEventFlagsHandle, FLAG_TIMER_TICK);
}
void Update_Simulated_Sensors(void) {
    uint32_t tick = HAL_GetTick();

    if (osMutexAcquire(nodeDataMutexHandle, osWaitForever) == osOK) {
        // 4-byte fields: Use a counter or sine-wave simulation
        nodeData.linPotData = (tick / 10) % 4096;      // Simulates 12-bit ADC travel
        //nodeData.wheelSpeed = (tick / 50) % 200;       // Simulates 0-200 units

        // 2-byte and 1-byte fields: Use bitwise shifts of the tick
        nodeData.fillerData4bytes = tick;
        nodeData.fillerData2bytes = (uint16_t)(tick & 0xFFFF);
        
        // Simulates temperatures in Celsius
        nodeData.brakeTemperature = (uint8_t)(40 + (tick % 60)); // 40-100C
        nodeData.tireTemperature = (uint8_t)(30 + (tick % 40));  // 30-70C

        osMutexRelease(nodeDataMutexHandle);
    }
}


