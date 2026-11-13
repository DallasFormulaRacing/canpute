
#include "can_utils.h"
#include "app_globals.h"
#include "app_freertos.h"
#include "fdcan.h"
#include "command_handler.h"

/* ---------------------------------------------------------
 * CAN RECEIVE QUEUE
 * --------------------------------------------------------- */

osMessageQueueId_t canfd_rx_queueHandle;

static const osMessageQueueAttr_t canfd_rx_queue_attributes = {
    .name = "canfd_rx_queue"
};

/* Creates the queue for incoming CAN messages */
void CAN_RxQueue_Init(void)
{
    canfd_rx_queueHandle = osMessageQueueNew(
        16,
        sizeof(CAN_RXMsg_t),
        &canfd_rx_queue_attributes
    );
}

/* ---------------------------------------------------------
 * CAN TRANSMIT QUEUE
 * ---------------------------------------------------------
 * Allows IMU, tire temperature, and future sensors
 * to submit messages without directly accessing FDCAN.
 */

osMessageQueueId_t can_tx_queueHandle;

static const osMessageQueueAttr_t can_tx_queue_attributes = {
    .name = "can_tx_queue"
};

/* Creates the queue for outgoing CAN messages */
void CAN_TxQueue_Init(void)
{
    can_tx_queueHandle = osMessageQueueNew(
        16,
        sizeof(CAN_TxMessage_t),
        &can_tx_queue_attributes
    );
}

/* Adds a message to the CAN transmission queue */
bool CAN_QueueMessage(const CAN_TxMessage_t *msg)
{
    if (msg == NULL || can_tx_queueHandle == NULL) {
        return false;
    }

    return osMessageQueuePut(
        can_tx_queueHandle,
        msg,
        0,
        0
    ) == osOK;
}

/* ---------------------------------------------------------
 * CAN HEADER INITIALIZATION
 * ---------------------------------------------------------
 * Sets the default CAN FD message configuration.
 */

void CAN_InitHeader(FDCAN_TxHeaderTypeDef *tx_header)
{
    tx_header->IdType = FDCAN_EXTENDED_ID;
    tx_header->TxFrameType = FDCAN_DATA_FRAME;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch = FDCAN_BRS_OFF;
    tx_header->FDFormat = FDCAN_FD_CAN;
    tx_header->TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header->MessageMarker = 0;
}

/* ---------------------------------------------------------
 * CAN TRANSMISSION
 * ---------------------------------------------------------
 * Transmits data using the existing DFR CAN protocol.
 *
 * Identifier format:
 * [Priority][Target][Command][Source]
 *
 * Supports CAN FD payloads up to 64 bytes.
 */

HAL_StatusTypeDef CAN_Transmit(
    FDCAN_HandleTypeDef *hfdcan,
    uint8_t priority,
    uint8_t target,
    uint32_t cmd_type,
    uint8_t *pData,
    uint32_t dlc_bytes)
{
    FDCAN_TxHeaderTypeDef txHeader;

    CAN_InitHeader(&txHeader);

    txHeader.Identifier = BUILD_CAN_ID(
        priority,
        target,
        cmd_type,
        self_node_id
    );

    txHeader.DataLength = dlc_bytes;

    if (dlc_bytes == FDCAN_DLC_BYTES_0) {
        return HAL_FDCAN_AddMessageToTxFifoQ(
            hfdcan,
            &txHeader,
            NULL
        );
    }

    return HAL_FDCAN_AddMessageToTxFifoQ(
        hfdcan,
        &txHeader,
        pData
    );
}

/* ---------------------------------------------------------
 * CAN RECEIVE CALLBACK
 * ---------------------------------------------------------
 * Called when a new CAN message arrives.
 *
 * Reads the message from FDCAN RX FIFO0 and places it
 * into the FreeRTOS receive queue for processing.
 */

void HAL_FDCAN_RxFifo0Callback(
    FDCAN_HandleTypeDef *hfdcan,
    uint32_t RxFifo0ITs)
{
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
    {
        FDCAN_RxHeaderTypeDef rxHeader;
        CAN_RXMsg_t msg;

        if (HAL_FDCAN_GetRxMessage(
                hfdcan,
                FDCAN_RX_FIFO0,
                &rxHeader,
                msg.data) == HAL_OK)
        {
            msg.id = rxHeader.Identifier;

            if (canfd_rx_queueHandle != NULL)
            {
                (void)osMessageQueuePut(
                    canfd_rx_queueHandle,
                    &msg,
                    0,
                    0
                );
            }
        }
    }
}
