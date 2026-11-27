#include "can_utils.h"
#include "app_globals.h"
#include "app_freertos.h"
#include "fdcan.h"
#include "command_handler.h"
#include "ev27_daq.h"

osMessageQueueId_t canfd_rx_queueHandle;
static const osMessageQueueAttr_t canfd_rx_queue_attributes = {
    .name = "canfd_rx_queue"
};

static osMutexId_t can_tx_mutexHandle;

// Called in app_freertos.c during RTOS init to set up the CAN RX message queue
void CAN_RxQueue_Init(void) {
    canfd_rx_queueHandle = osMessageQueueNew(32, sizeof(CAN_RXMsg_t), &canfd_rx_queue_attributes);
    can_tx_mutexHandle = osMutexNew(NULL);
}

void CAN_TxLock(void) {
    if (can_tx_mutexHandle != NULL) {
        (void)osMutexAcquire(can_tx_mutexHandle, osWaitForever);
    }
}

void CAN_TxUnlock(void) {
    if (can_tx_mutexHandle != NULL) {
        (void)osMutexRelease(can_tx_mutexHandle);
    }
}

/**
 * @brief Sets default values for the FDCAN header
 */
void CAN_InitHeader(FDCAN_TxHeaderTypeDef *tx_header) {
    tx_header->IdType = FDCAN_EXTENDED_ID;
    tx_header->TxFrameType = FDCAN_DATA_FRAME;
    tx_header->ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header->BitRateSwitch = FDCAN_BRS_ON;
    tx_header->FDFormat = FDCAN_FD_CAN;
    tx_header->TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header->MessageMarker = 0;
}


/**
 * @brief CAN transmit wrapper for DFR CAN standardized header format
 * @param priority 3-bit priority (0-7, 0 is highest)
 * @param target   5-bit Target Device ID (e.g., NODE_ID_ALL_NODES or a specific node)
 * @param cmd_type 16-bit Command/Data Type identifier
 * @param pData    Pointer to the data payload (max 64 bytes for CAN FD)
 * @param dlc_bytes The FDCAN_DLC_BYTES_XX macro representing the payload size
 */
HAL_StatusTypeDef CAN_Transmit(FDCAN_HandleTypeDef *hfdcan,uint8_t priority, uint8_t target, uint32_t cmd_type, uint8_t* pData, uint32_t dlc_bytes) {
    FDCAN_TxHeaderTypeDef txHeader;
    
    CAN_InitHeader(&txHeader); 
    
    // Identifier built as: [Priority][Target][Command][Source]
    txHeader.Identifier = BUILD_CAN_ID(priority, target, cmd_type, self_node_id);
    txHeader.DataLength = dlc_bytes;

    // Safety: HAL_FDCAN_AddMessageToTxFifoQ will fail if pData is NULL 
    // unless the DLC is 0. 
    HAL_StatusTypeDef st;
    CAN_TxLock();
    if (dlc_bytes == FDCAN_DLC_BYTES_0) {
        st = HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &txHeader, NULL);
    } else {
        st = HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &txHeader, pData);
    }
    CAN_TxUnlock();
    return st;
}

/**
 * @brief Move the RX Callback here as well to keep it away from generated code and getting overwritten.
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    // Read the clock first: every node does this at the same instant for the
    // same frame, which is what makes ev27 TimeSync (car time) work.
    const uint32_t now_us = ev27_daq_time_us();
    static const uint8_t dlc_len[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) == RESET) {
        return;
    }
    while (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) > 0U) {
        FDCAN_RxHeaderTypeDef rxHeader;
        CAN_RXMsg_t msg;

        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, msg.data) != HAL_OK) {
            break;
        }
        msg.id = rxHeader.Identifier;
        msg.time_us = now_us;
        msg.extended = (rxHeader.IdType == FDCAN_EXTENDED_ID) ? 1U : 0U;
        msg.len = dlc_len[rxHeader.DataLength & 0xFU];
        if (canfd_rx_queueHandle != NULL) {
            (void)osMessageQueuePut(canfd_rx_queueHandle, &msg, 0, 0);   // full queue: frame dropped
        }
    }
}
