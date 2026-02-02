/* App/Inc/command_handler.h */
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "stm32h5xx_hal.h"
#include "types.h"


typedef enum {
    CMD_ID_PING             = 0x001,
    CMD_ID_REQ_DATA         = 0x050, // Pi requests the 16-byte NodeDataTypeDef
    CMD_ID_SENDING_DATA     = 0x051, // Node sending the 16-byte NodeDataTypeDef
    CMD_ID_RESET_NODE       = 0x099
} CommandID_t;

/**
 * @brief Dispatches incoming CAN messages to specific handling functions.
 * @param cmd_id The Identifier of the received CAN frame.
 * @param data   Pointer to the CAN frame data payload.
 */
void Process_CAN_Command(uint32_t ext_id, uint8_t* data);



#endif /* COMMAND_HANDLER_H */