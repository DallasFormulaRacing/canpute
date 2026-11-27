/*
 * canpute on the ev27-can main bus: ev27_node (Status, TimeSync, Commands)
 * plus the DAQ data messages (Imu, TireTemp).
 *
 *   ev27_daq_hw_init()     App_Hardware_Init(), before HAL_FDCAN_Start (1 MHz timer, address, filters)
 *   ev27_daq_task_init()   once at the start of the CAN TX task
 *   ev27_daq_tick_10ms()   every 10 ms from the CAN TX task (Status, Identify LED)
 *   ev27_daq_rx()          every received standard-ID frame, from the CAN RX task
 *   ev27_daq_imu()         from the IMU task after each FIFO batch
 *   ev27_daq_tire_temp()   from the tire temp task after each frame
 */
#ifndef EV27_DAQ_H
#define EV27_DAQ_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32h5xx_hal.h"

void ev27_daq_hw_init(void);
void ev27_daq_config_filters(FDCAN_HandleTypeDef *hfdcan, uint32_t first_index);
void ev27_daq_task_init(void);
void ev27_daq_tick_10ms(void);
void ev27_daq_rx(uint32_t id, const uint8_t *data, uint8_t len, uint32_t rx_time_us);

/* Physical units, already in the vehicle frame when a mount rotation exists. */
void ev27_daq_imu(const float accel_g[3], const float gyro_dps[3]);
/* 16 zones, inner to outer, in 0.1 degC. */
void ev27_daq_tire_temp(const int16_t zones_dC[16]);

uint8_t ev27_daq_addr(void);
uint32_t ev27_daq_time_us(void);

#endif /* EV27_DAQ_H */
