/* App/Inc/command_handler.h */
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "stm32h5xx_hal.h"
#include "types.h"


typedef enum {
    CMD_ID_PING             = 0x001,
    CMD_ID_REQ_DATA         = 0x050, 
    CMD_ID_SENDING_DATA     = 0x051, 
    CMD_ID_RESET_NODE       = 0x099,
    
    // New Test Commands
    CMD_ID_SET_LED          = 0x100, // Data[0]: 0=Off, 1=On
    CMD_ID_SET_FREQ         = 0x101, // Data[0]: New Standalone frequency in Hz
    CMD_ID_RESET_SIM        = 0x102, // No data: Resets the simulated sensor counters
    CMD_ID_SET_OFFSET       = 0x103  // Data[0-3]: uint32_t offset for LinPot data
} CommandID_t;

/**
 * @brief Dispatches incoming CAN messages to specific handling functions.
 * @param cmd_id The Identifier of the received CAN frame.
 * @param data   Pointer to the CAN frame data payload.
 */
void Process_CAN_Command(uint32_t ext_id, uint8_t* data);



#endif /* COMMAND_HANDLER_H */