/* App/Inc/command_handler.h */
#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "stm32h5xx_hal.h"
#include "types.h"
#include "app_globals.h"
#include "can_utils.h"
#include "main.h"
#include <string.h>



/**
 * @brief Dispatches incoming CAN messages to specific handling functions.
 * @param ext_id The Identifier of the received CAN frame.
 * @param data   Pointer to the CAN frame data payload.
 */
void Process_CAN_Command(uint32_t ext_id, uint8_t* data);



#endif /* COMMAND_HANDLER_H */
