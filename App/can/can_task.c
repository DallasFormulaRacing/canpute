#include "app_freertos.h"
#include "app_globals.h"
#include "can_utils.h"
#include "command_handler.h"

void start_canfd_tx(void *argument)
{
    for(;;) {
        osDelay(1000);
    }
}

void start_canfd_rx(void *argument)
{
    CAN_RXMsg_t msg;

    for(;;) {
        if (osMessageQueueGet(canfd_rx_queueHandle, &msg, NULL, osWaitForever) == osOK) {
            Process_CAN_Command(msg.id, msg.data);
        }
    }
}
