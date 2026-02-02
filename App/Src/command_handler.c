/* App/Src/command_handler.c */
#include "command_handler.h"
#include "can_utils.h"
#include "cmsis_os2.h"
#include "types.h"
#include "main.h"

#include <string.h>

extern osEventFlagsId_t systemEventFlagsHandle;
extern osTimerId_t standaloneTimerHandle;
extern osMutexId_t nodeDataMutexHandle;
extern NodeDataTypeDef nodeData;

void Process_CAN_Command(uint32_t ext_id, uint8_t* data) {
    uint8_t  source_id = ext_id & 0x1F;
    CommandID_t command  = (CommandID_t)((ext_id >> 5) & 0xFFFF);
    uint8_t  target_id = (ext_id >> 21) & 0x1F;

    (void)source_id; 
    (void)target_id;

    switch(command) {
        case CMD_ID_REQ_DATA: 
            osEventFlagsSet(systemEventFlagsHandle, FLAG_PI_SYNC);
            break;

        case CMD_ID_SET_LED:
            // Assuming LED_GREEN is on a GPIO
            if (data[0] == 1) HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
            else HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
            break;

        case CMD_ID_SET_FREQ:
            if (data[0] > 0 && data[0] <= 100) { // Limit 1-100Hz for safety
                uint32_t period_ms = 1000 / data[0];
                osTimerStop(standaloneTimerHandle);
                osTimerStart(standaloneTimerHandle, period_ms);
            }
            break;

        case CMD_ID_RESET_SIM:
            osMutexAcquire(nodeDataMutexHandle, osWaitForever);
            memset(&nodeData, 0, sizeof(NodeDataTypeDef));
            osMutexRelease(nodeDataMutexHandle);
            break;

        case CMD_ID_SET_OFFSET:
            osMutexAcquire(nodeDataMutexHandle, osWaitForever);
            // Reconstruct uint32 from bytes (Little Endian)
            uint32_t offset = (uint32_t)data[0] | ((uint32_t)data[1] << 8) | 
                              ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
            nodeData.fillerData4bytes = offset; // Use a filler field for testing
            osMutexRelease(nodeDataMutexHandle);
            break;

        case CMD_ID_RESET_NODE:
            HAL_NVIC_SystemReset();
            break;  

        default:
            break;
    }
}