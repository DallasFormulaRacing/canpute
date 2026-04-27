#include "command_handler.h"
#include "imu.h"


void Process_CAN_Command(uint32_t ext_id, uint8_t* data) {
    uint8_t  source_id = ext_id & 0x1F;
    CommandID_t command  = (CommandID_t)((ext_id >> 5) & 0xFFFF);
    uint8_t  target_id = (ext_id >> 21) & 0x1F;

    (void)target_id;

    switch(command) {
        case CMD_REQ_IMU_DATA:
        {
            uint8_t tx_buf[64] = {0};
            if (IMU_GetLatestFrame(tx_buf, sizeof(tx_buf))) {
                CAN_Transmit(&hfdcan2, 1, source_id, CMD_IMU_DATA, tx_buf, FDCAN_DLC_BYTES_64);
            }
            break;
        }
        case CMD_REQ_TEMP_DATA:
        case CMD_REQ_SPEED_DATA:
        case CMD_REQ_RIDE_HEIGHT_DATA:
            osEventFlagsSet(systemEventFlagsHandle, FLAG_PI_SYNC);
            break;

        case CMD_SET_LED:
            if (data[0] == 1) HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_SET);
            else HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
            break;

        case CMD_RESET_NODE:
            HAL_NVIC_SystemReset();
            break;
        case CMD_PING: {
            uint8_t ping_resp = 1;
            CAN_Transmit(&hfdcan2, 1, source_id, CMD_PONG, &ping_resp, FDCAN_DLC_BYTES_1);
            break;
        }
        case BL_CMD_REBOOT:
            // Reboot into bootloader
            HAL_NVIC_SystemReset();
            break;
        default:
            break;
    }
}
