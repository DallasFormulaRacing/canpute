#include "can_utils.h"
#include "fdcan.h"
#include "command_handler.h"

extern FDCAN_HandleTypeDef hfdcan2;
extern NodeHardwareID_t self_node_id;

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
 * @brief Logic-heavy transmission wrapper for the new 29-bit ID structure
 * @param priority 3-bit priority (0-7, 0 is highest)
 * @param target   5-bit Target Device ID (e.g., NODE_ID_ALL_NODES or a specific node)
 * @param cmd_type 16-bit Command/Data Type identifier
 * @param pData    Pointer to the data payload (max 64 bytes for CAN FD)
 * @param dlc_bytes The FDCAN_DLC_BYTES_XX macro representing the payload size
 */
HAL_StatusTypeDef CAN_Transmit(uint8_t priority, uint8_t target, uint32_t cmd_type, uint8_t* pData, uint32_t dlc_bytes) {
    FDCAN_TxHeaderTypeDef txHeader;
    
    // Use the internal helper to set up fixed FD/Extended settings
    CAN_InitHeader(&txHeader); 
    
    // Build the 29-bit ID: [Priority][Target][Command][Source]
    // self_node_id is the 'Source' established during App_Hardware_Init
    txHeader.Identifier = BUILD_CAN_ID(priority, target, cmd_type, self_node_id);
    
    // Set the data length (must be an FDCAN_DLC_BYTES_x macro)
    txHeader.DataLength = dlc_bytes;

    return HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &txHeader, pData);
}

/**
 * @brief Move the RX Callback here as well to keep it away from generated code
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET) {
        FDCAN_RxHeaderTypeDef rxHeader;
        uint8_t rxData[64];

        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK) {
            Process_CAN_Command(rxHeader.Identifier, rxData);
        }
    }
}