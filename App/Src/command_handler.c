/* App/Src/command_handler.c */
#include "command_handler.h"
#include "can_utils.h"
#include "cmsis_os2.h"
#include "types.h"

extern osEventFlagsId_t systemEventFlagsHandle;

void Process_CAN_Command(uint32_t ext_id, uint8_t* data) {
    uint8_t  source_id = ext_id & 0x1F;
    CommandID_t command  = (CommandID_t)((ext_id >> 5) & 0xFFFF);
    uint8_t  target_id = (ext_id >> 21) & 0x1F;

    // To clear the "unused variable" warnings, use them or cast to void
    (void)source_id; 
    (void)target_id;

    switch(command) {
        case CMD_ID_REQ_DATA: 
            osEventFlagsSet(systemEventFlagsHandle, FLAG_PI_SYNC);
            break;
        case CMD_ID_PING:
            // Optionally implement a PING response here
            break;
        case CMD_ID_RESET_NODE:
            NVIC_SystemReset();
            break;  
        default:
            break;
    }
}