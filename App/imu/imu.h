#ifndef IMU_H
#define IMU_H

#include "stm32h5xx_hal.h"
#include "asm330lhhx_reg.h"
#include <stdbool.h>

#define IMU_FIFO_WATERMARK          10  // 5 samples of Accel (x,y,z) and Gyro (x,y,z). Each value is 2 bytes
// so 10 words * 6 data bytes = 60 bytes per FIFO frame, which will pack nicely into 64 byte CANFD frame
#define IMU_FIFO_FRAME_SIZE         60  // 10 words * 6 bytes
#define IMU_FIFO_WORD_SIZE          7   // FIFO tag byte + 6 data bytes
#define IMU_FIFO_DMA_FRAME_SIZE     (IMU_FIFO_WATERMARK * IMU_FIFO_WORD_SIZE)
#define IMU_THREAD_FLAG_DMA_READY   (1U << 0)
#define IMU_THREAD_FLAG_DMA_ERROR   (1U << 1)

/* Debug snapshot of the IMU pipeline. Read it from GDB / Live Watch:
 *   (gdb) p imu_dbg
 *   (gdb) p/x imu_dbg.whoami      -> expect 0x6B
 * If int_count stays 0, INT2 isn't reaching PB2 (I2C2_INT), so no DMA reads start. */
typedef struct {
    uint8_t  whoami;            /* WHO_AM_I register, 0x6B when the IMU answers */
    uint8_t  init_ok;           /* 1 once IMU_Init configured the sensor */
    uint8_t  i2c_addr;          /* 7-bit address tried last / answering (0x6A or 0x6B) */
    uint32_t i2c_error_count;   /* failed blocking I2C transactions (init/config) */
    uint32_t int_count;         /* FIFO watermark interrupts seen on PB2 */
    uint32_t dma_done_count;    /* FIFO DMA reads that completed */
    uint32_t dma_error_count;   /* FIFO DMA reads that failed to start or errored */
    uint32_t frame_count;       /* complete frames (5 gyro + 5 accel samples) */
    uint32_t bad_frame_count;   /* DMA reads that didn't contain a full frame */
    int16_t  gyro_raw[3];       /* latest gyro sample X, Y, Z (LSB) */
    int16_t  accel_raw[3];      /* latest accel sample X, Y, Z (LSB) */
    float    gyro_dps[3];       /* latest gyro X, Y, Z in dps, bias corrected */
    float    accel_g[3];        /* latest accel X, Y, Z in g */
    float    gyro_bias_dps[3];  /* MotionGC gyro bias estimate */
} IMU_Debug_t;

extern volatile IMU_Debug_t imu_dbg;

bool IMU_Init(void);
void IMU_FIFO_Read(uint8_t *out_buf, uint16_t *out_len);
bool IMU_GetLatestFrame(uint8_t *out_buf, uint16_t out_len);

#endif /* IMU_H */
