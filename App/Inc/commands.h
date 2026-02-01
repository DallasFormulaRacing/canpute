#ifndef COMMANDS_H
#define COMMANDS_H

#include "types.h"

// Unified Command IDs for the FSAE CAN Bus
typedef enum {
    CMD_ID_PING             = 0x001,
    CMD_ID_REQ_DATA         = 0x050, // Pi requests the 16-byte NodeDataTypeDef
    CMD_ID_SET_INTERVAL     = 0x051, // Pi tells node how fast to broadcast
    CMD_ID_RESET_NODE       = 0x099
} CommandID_t;

// Prototype for the handler
void Process_CAN_Command(uint32_t cmd_id, uint8_t* data);

#endif