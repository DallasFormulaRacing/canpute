#ifndef CAN_UTILS_H
#define CAN_UTILS_H

#include "stm32h5xx_hal.h"
#include "types.h"

// ID Construction Macro
#define CAN_EXT_ID_MASK 0x1FFFFFFF
#define BUILD_CAN_ID(prio, target, cmd, source) \
    (((uint32_t)(prio) & 0x07) << 26) |         \
    (((uint32_t)(target) & 0x1F) << 21) |       \
    (((uint32_t)(cmd) & 0xFFFF) << 5) |         \
    ((uint32_t)(source) & 0x1F)
// Prototypes
void CAN_InitHeader(FDCAN_TxHeaderTypeDef *tx_header);
HAL_StatusTypeDef CAN_Transmit(uint8_t priority, uint8_t target, uint32_t cmd_type, uint8_t* pData, uint32_t dlc_bytes);
#endif