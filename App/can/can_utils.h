#ifndef CAN_UTILS_H
#define CAN_UTILS_H

#include "stm32h5xx_hal.h"
#include "cmsis_os2.h"
#include "types.h"
#include <stdbool.h>
#include <stdint.h>

#define CAN_LOOPBACK_TEST 1

typedef struct
{
    uint8_t priority;
    uint8_t target;
    uint32_t command;
    uint32_t dlc;
    uint8_t data[64];
} CAN_TxMessage_t;

extern osMessageQueueId_t can_tx_queueHandle;

void CAN_RxQueue_Init(void);
void CAN_TxQueue_Init(void);
bool CAN_QueueMessage(const CAN_TxMessage_t *msg);

void CAN_InitHeader(FDCAN_TxHeaderTypeDef *tx_header);

HAL_StatusTypeDef CAN_Transmit(
    FDCAN_HandleTypeDef *hfdcan,
    uint8_t priority,
    uint8_t target,
    uint32_t cmd_type,
    uint8_t *pData,
    uint32_t dlc_bytes
);

#endif