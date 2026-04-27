#ifndef IMU_H
#define IMU_H

#include "stm32h5xx_hal.h"
#include "asm330lhhx_reg.h"

#define IMU_FIFO_WATERMARK          10  // 5 samples of Accel (x,y,z) and Gyro (x,y,z). Each value is 2 bytes
// so 10 words * 6 data bytes = 60 bytes per FIFO frame, which will pack nicely into 64 byte CANFD frame
#define IMU_FIFO_FRAME_SIZE         60  // 10 words * 6 bytes
#define IMU_FIFO_WORD_SIZE          7   // FIFO tag byte + 6 data bytes
#define IMU_FIFO_DMA_FRAME_SIZE     (IMU_FIFO_WATERMARK * IMU_FIFO_WORD_SIZE)
#define IMU_THREAD_FLAG_DMA_READY   (1U << 0)
#define IMU_THREAD_FLAG_DMA_ERROR   (1U << 1)

void IMU_Init(void);
void IMU_FIFO_Read(uint8_t *out_buf, uint16_t *out_len);

#endif /* IMU_H */
