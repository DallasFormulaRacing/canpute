#include "app_freertos.h"
#include "app_globals.h"
#include "can_utils.h"
#include "command_handler.h"
#include <string.h>
#define CMD_IMU_DATA /* existing assigned ID*/
#define CMD_TIRE_TEMP_DATA /* existing assigned ID*/

/*global variables provided by imu_task.c */
extern float imu_gyro_dps[3];   /* latest Angular Gyro sample: X, Y, Z in degrees/sec */
extern float imu_accel_g[3];    /* latest Accelerometer sample: X, Y, Z in g */


volatile uint32_t can_tx_count = 0; // Count of CAN messages transmitted
volatile uint32_t can_rx_count = 0; // Count of CAN messages received



void start_canfd_tx(void *argument){
    CAN_TxMessage_t msg;

    for (;;)
    {
        /* Wait until a sensor adds a CAN message */
        if (osMessageQueueGet(
                can_tx_queueHandle,
                &msg,
                NULL,
                osWaitForever) == osOK)
        {
            /* Transmit through FDCAN2 */
            HAL_StatusTypeDef status = CAN_Transmit(
                &hfdcan2,
                msg.priority,
                msg.target,
                msg.command,
                msg.data,
                msg.dlc
            );

            if (status == HAL_OK) {
                can_tx_count++;
            }
        }
    }
}


void start_canfd_rx(void *argument)
{
    CAN_RXMsg_t msg;

    for(;;) {
        if (osMessageQueueGet(canfd_rx_queueHandle, &msg, NULL, osWaitForever) == osOK) {
            Process_CAN_Command(msg.id, msg.data);
        }
    }
}
