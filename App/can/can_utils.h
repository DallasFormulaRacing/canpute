#ifndef CAN_UTILS_H
#define CAN_UTILS_H

#include "stm32h5xx_hal.h"
#include "types.h"

// Set to 1 to run FDCAN2 in internal loopback and send a broadcast PING every second (debug only)
#define CAN_LOOPBACK_TEST 1


// Prototypes
void CAN_RxQueue_Init(void);
void CAN_InitHeader(FDCAN_TxHeaderTypeDef *tx_header);
HAL_StatusTypeDef CAN_Transmit(FDCAN_HandleTypeDef *hfdcan,uint8_t priority, uint8_t target, uint32_t cmd_type, uint8_t* pData, uint32_t dlc_bytes);
#endif