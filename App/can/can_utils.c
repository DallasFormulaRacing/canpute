#include "can_utils.h"
#include "app_globals.h"
#include "app_freertos.h"
#include "fdcan.h"
#include "command_handler.h"

osMessageQueueId_t canfd_rx_queueHandle;

/* Counts frames pulled out of RX FIFO 0 by the callback. Watch this. */
volatile uint32_t debug_rx_count;

static const osMessageQueueAttr_t canfd_rx_queue_attributes = {
    .name = "canfd_rx_queue"
};

// Called from app_freertos.c during RTOS initialization.
void CAN_RxQueue_Init(void)
{
    canfd_rx_queueHandle =
        osMessageQueueNew(16U, sizeof(CAN_RXMsg_t), &canfd_rx_queue_attributes);
}

/**
 * @brief Set default values for the FDCAN header.
 */
void CAN_InitHeader(FDCAN_TxHeaderTypeDef *tx_header)
{
    tx_header->IdType              = FDCAN_EXTENDED_ID;
    tx_header->TxFrameType         = FDCAN_DATA_FRAME;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch       = FDCAN_BRS_ON;
    tx_header->FDFormat            = FDCAN_FD_CAN;
    tx_header->TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
    tx_header->MessageMarker       = 0U;
}

/**
 * @brief Transmit a CAN FD message.
 * @note  Never halts the CPU. Check the return value (and can_tx_status).
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

    if (hfdcan == NULL) {
        return HAL_ERROR;
    }

    if ((dlc_bytes != FDCAN_DLC_BYTES_0) && (pData == NULL)) {
        return HAL_ERROR;
    }

    CAN_InitHeader(&txHeader);

    // Match the header to how the peripheral was actually initialized.
    if (hfdcan->Init.FrameFormat == FDCAN_FRAME_CLASSIC) {
        txHeader.FDFormat      = FDCAN_CLASSIC_CAN;
        txHeader.BitRateSwitch = FDCAN_BRS_OFF;
    } else if (hfdcan->Init.FrameFormat == FDCAN_FRAME_FD_NO_BRS) {
        txHeader.BitRateSwitch = FDCAN_BRS_OFF;
    }

    // Identifier format: [Priority][Target][Command][Source]
    txHeader.Identifier = BUILD_CAN_ID(priority, target, cmd_type, self_node_id);
    txHeader.DataLength = dlc_bytes;

    can_tx_free = HAL_FDCAN_GetTxFifoFreeLevel(hfdcan);

    if (can_tx_free == 0U) {
        can_tx_status = (uint32_t)HAL_BUSY;
        return HAL_BUSY;
    }

    can_tx_status = (uint32_t)HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &txHeader, pData);
    can_tx_error  = HAL_FDCAN_GetError(hfdcan);

    return (HAL_StatusTypeDef)can_tx_status;
}

/**
 * @brief Handle new messages in FDCAN RX FIFO 0.
 */
void HAL_FDCAN_RxFifo0Callback(
    FDCAN_HandleTypeDef *hfdcan,
    uint32_t RxFifo0ITs)
{
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != 0U) {
        FDCAN_RxHeaderTypeDef rxHeader;
        CAN_RXMsg_t msg;

        // Fill level before reading (shows how many were waiting).
        debug_rx_level = HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0);

        // Drain everything, since one interrupt can cover several frames.
        while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0U) {
            if (HAL_FDCAN_GetRxMessage(
                    hfdcan, FDCAN_RX_FIFO0, &rxHeader, msg.data) != HAL_OK) {
                break;
            }

            debug_rx_count++;
            msg.id = rxHeader.Identifier;

            if (canfd_rx_queueHandle != NULL) {
                (void)osMessageQueuePut(canfd_rx_queueHandle, &msg, 0U, 0U);
            }
        }
    }
}