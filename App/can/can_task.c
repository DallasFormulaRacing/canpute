#include "app_freertos.h"
#include "app_globals.h"
#include "can_utils.h"
#include "command_handler.h"

void start_canfd_tx(void *argument)
{
    for(;;) {
#if CAN_LOOPBACK_TEST
        uint8_t ping = 0;
        CAN_Transmit(&hfdcan2, 1, NODE_ID_ALL_NODES, CMD_PING, &ping, FDCAN_DLC_BYTES_1);
#endif
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
