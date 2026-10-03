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

typedef enum {
    IMU_FAIL_NONE = 0,
    IMU_FAIL_NO_ACK,          /* nothing ACKed at 0xD5: wiring, power, pull-ups, SA0, CS pin */
    IMU_FAIL_I2C_ERROR,       /* a transfer failed: see last_hal_status / last_error_code */
    IMU_FAIL_RESET_TIMEOUT,   /* SW_RESET bit never cleared */
    IMU_FAIL_BAD_WHOAMI,      /* device ACKs but WHO_AM_I != 0x6B */
} imu_fail_reason_t;

typedef struct {
    uint32_t attempts;
    uint32_t i2c_errors;
    uint8_t  ack_0xD5;
    uint8_t  ack_0xD7;
    uint8_t  whoami;
    uint8_t  fail_reason;      /* imu_fail_reason_t */
    uint8_t  last_hal_status;  /* HAL_StatusTypeDef */
    uint32_t last_error_code;  /* hi2c2.ErrorCode: 0x04 = AF (NACK) */
    uint32_t i2c_state;        /* hi2c2.State */
    uint8_t  sda_idle;         /* PB3 level before probing: must be 1 */
    uint8_t  scl_idle;         /* PB10 level before probing: must be 1 */
    uint8_t  trace_ioen;       /* DBGMCU TRACE_IOEN: if 1, PB3 is hijacked as SWO */
    uint8_t  ack_on_i2c1;      /* sensor found on PB6/PB7 (I2C1) instead */
    uint32_t pb3_moder_af;
    /* Raw address-frame probe: I2C2->ISR snapshot after sending START+addr.
     * bit4 NACKF=1          -> real NACK, bus is clocking, device not answering
     * bit5 STOPF=1, NACKF=0 -> device ACKed
     * neither set           -> bus never completed: SCL/SDA not toggling
     * bit15 BUSY            -> bus busy flag */
    uint32_t raw_isr_0xD5;
    uint32_t raw_isr_0xD7;
    uint32_t raw_probe_ms;     /* ms the two raw probes took: NACK is ~0, stuck bus ~40 */     /* MODER bits 7:6 (expect 2=AF) | AFRL nibble for PB3 (expect 4) */
} imu_debug_t;

extern volatile imu_debug_t imu_debug;

/* Returns true when the sensor answered WHO_AM_I and was configured. */
bool IMU_Init(void);
void IMU_FIFO_Read(uint8_t *out_buf, uint16_t *out_len);
bool IMU_GetLatestFrame(uint8_t *out_buf, uint16_t out_len);

#endif /* IMU_H */
