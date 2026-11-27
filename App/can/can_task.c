#include "app_freertos.h"
#include "app_globals.h"
#include "can_utils.h"
#include "command_handler.h"
#include "ev27_daq.h"

void start_canfd_tx(void *argument)
{
    uint8_t counter = 0;
    uint32_t n = 0;
    uint32_t tick = osKernelGetTickCount();

    ev27_daq_task_init();

    for(;;) {
        tick += 10U;                 // 10 ms base period
        (void)osDelayUntil(tick);

        ev27_daq_tick_10ms();        // ev27 Status every 500 ms, Identify LED

        if (n++ % 100U != 0U) {
            continue;
        }
        // Once a second:
#if CAN_LOOPBACK_TEST
        uint8_t ping = 0;
        CAN_Transmit(&hfdcan2, 1, NODE_ID_ALL_NODES, CMD_PING, &ping, FDCAN_DLC_BYTES_1);
#endif
#if CAN_HEARTBEAT
        // Same layout as ezfd-test-app's heartbeat: "APP", version, rolling counter.
        // NODE_ID_ALL_NODES (0x01) is also the Pi's address in the ezfd protocol.
        uint8_t hb[5] = { 'A', 'P', 'P', APP_FW_VERSION, counter++ };
        CAN_Transmit(&hfdcan2, 1, NODE_ID_ALL_NODES, APP_CMD_HEARTBEAT, hb, FDCAN_DLC_BYTES_5);
#endif
    }
}

void start_canfd_rx(void *argument)
{
    CAN_RXMsg_t msg;

    for(;;) {
        if (osMessageQueueGet(canfd_rx_queueHandle, &msg, NULL, osWaitForever) == osOK) {
            if (msg.extended) {
                Process_CAN_Command(msg.id, msg.data);    // ezfd bootloader + DFR 29-bit commands
            } else {
                ev27_daq_rx(msg.id, msg.data, msg.len, msg.time_us);   // ev27-can main bus
            }
        }
    }
}
