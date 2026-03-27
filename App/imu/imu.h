#ifndef IMU_H
#define IMU_H

#include "stm32h5xx_hal.h"
#include "asm330lhhx_reg.h"

#define IMU_FIFO_WATERMARK   10  // 5 samples of Accel (x,y,z) and Gyro (x,y,z). Each value is 2 bytes
// so 10 words * 6 data bytes = 60 bytes per FIFO frame, which will pack nicely into 64 byte CANFD frame
#define IMU_FIFO_FRAME_SIZE  60  // 10 words * 6 bytes

void IMU_Init(void);
void IMU_FIFO_Read(uint8_t *out_buf, uint16_t *out_len);

#endif /* IMU_H */
