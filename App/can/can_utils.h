#ifndef CAN_UTILS_H
#define CAN_UTILS_H

#include "stm32h5xx_hal.h"
#include "types.h"

// Set to 1 to run FDCAN2 in internal loopback and send a broadcast PING every second (debug only)
#define CAN_LOOPBACK_TEST 0
#define CAN_HEARTBEAT 1   // send a 1 Hz APP_CMD_HEARTBEAT to the Pi ("APP", version, counter)

// Firmware version. Bump on every release you flash: the dashboard shows it
// (ev27 Status FwMajor.FwMinor.FwPatch, and the minor in the ezfd heartbeat).
#define FW_VERSION_MAJOR 0
#define FW_VERSION_MINOR 4
#define FW_VERSION_PATCH 0
#define APP_FW_VERSION   FW_VERSION_MINOR


// Prototypes
void CAN_RxQueue_Init(void);
void CAN_InitHeader(FDCAN_TxHeaderTypeDef *tx_header);
HAL_StatusTypeDef CAN_Transmit(FDCAN_HandleTypeDef *hfdcan,uint8_t priority, uint8_t target, uint32_t cmd_type, uint8_t* pData, uint32_t dlc_bytes);
// Serialises access to the FDCAN2 TX FIFO between tasks (CAN_Transmit and ev27_port_send).
void CAN_TxLock(void);
void CAN_TxUnlock(void);
#endif