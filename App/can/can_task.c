#include "app_freertos.h"
#include "app_globals.h"
#include "can_utils.h"
#include "command_handler.h"

/* Default to loopback test on if nothing else defines it.
 * (An undefined macro in "#if" silently evaluates to 0 and compiles the
 * test code out, so define it explicitly.) */
#ifndef CAN_LOOPBACK_TEST
#define CAN_LOOPBACK_TEST 1
#endif

/* Watch these in the debugger. */
volatile uint32_t debug_tx_ret;         // return value of CAN_Transmit (0 = HAL_OK)
volatile uint32_t debug_tx_free_after;  // TX FIFO free level after sending
volatile uint32_t debug_rx_poll;        // RX FIFO0 fill level, polled (not from ISR)
volatile uint32_t debug_ir;             // FDCAN interrupt register
volatile uint32_t debug_psr;            // FDCAN protocol status register

void start_canfd_tx(void *argument)
{
    (void)argument;

    for (;;) {
#if CAN_LOOPBACK_TEST
        uint8_t ping = 0;

        debug_tx_ret = (uint32_t)CAN_Transmit(
            &hfdcan2, 1, NODE_ID_ALL_NODES, CMD_PING, &ping, FDCAN_DLC_BYTES_1);

        osDelay(5);  // give the frame time to go out and loop back

        debug_tx_free_after = HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan2);
        debug_rx_poll       = HAL_FDCAN_GetRxFifoFillLevel(&hfdcan2, FDCAN_RX_FIFO0);
        debug_ir            = hfdcan2.Instance->IR;
        debug_psr           = hfdcan2.Instance->PSR;
#endif
        osDelay(1000);
    }
}