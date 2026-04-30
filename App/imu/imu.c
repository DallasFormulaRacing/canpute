#include "imu.h"
#include "app_globals.h"
#include "main.h"
#include <string.h>

static stmdev_ctx_t dev_ctx;
static uint8_t imu_dma_buf[IMU_FIFO_DMA_FRAME_SIZE];
static volatile uint8_t imu_dma_in_progress = 0;
static volatile uint8_t imu_dma_ready = 0;

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
    HAL_I2C_Mem_Read(handle, ASM330LHHX_I2C_ADD_L, reg,
                     I2C_MEMADD_SIZE_8BIT, bufp, len, HAL_MAX_DELAY);
    return 0;
}

static int32_t platform_write(void *handle, uint8_t reg,
                              const uint8_t *bufp, uint16_t len)
{
    HAL_I2C_Mem_Write(handle, ASM330LHHX_I2C_ADD_L, reg,
                      I2C_MEMADD_SIZE_8BIT, (uint8_t *)bufp, len,
                      HAL_MAX_DELAY);
    return 0;
}

static void platform_delay(uint32_t ms)
{
    HAL_Delay(ms);
}

void IMU_Init(void)
{
    uint8_t whoamI = 0;
    uint8_t rst = 1;
    asm330lhhx_pin_int2_route_t int2_route = {0};

    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg  = platform_read;
    dev_ctx.mdelay    = platform_delay;
    // IMU USES i2c2
    dev_ctx.handle    = &hi2c2;

    // Reset before use to ensure imu is in a known state and clear old data
    asm330lhhx_reset_set(&dev_ctx, PROPERTY_ENABLE);
    do {
        asm330lhhx_reset_get(&dev_ctx, &rst);
    } while (rst);

    // Verify WHO_AM_I
    asm330lhhx_device_id_get(&dev_ctx, &whoamI);
    if (whoamI != ASM330LHHX_ID) {
        return;
    }

    // Block data update - output regs not updated until read
    asm330lhhx_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);

    // Accelerometer: 417 Hz, +/-4g
    asm330lhhx_xl_data_rate_set(&dev_ctx, ASM330LHHX_XL_ODR_417Hz);
    asm330lhhx_xl_full_scale_set(&dev_ctx, ASM330LHHX_4g);

    // Gyroscope: 417 Hz, +/-2000 dps
    asm330lhhx_gy_data_rate_set(&dev_ctx, ASM330LHHX_GY_ODR_417Hz);
    asm330lhhx_gy_full_scale_set(&dev_ctx, ASM330LHHX_2000dps);

    // FIFO: batch both XL and GY at 417 Hz
    asm330lhhx_fifo_xl_batch_set(&dev_ctx, ASM330LHHX_XL_BATCHED_AT_417Hz);
    asm330lhhx_fifo_gy_batch_set(&dev_ctx, ASM330LHHX_GY_BATCHED_AT_417Hz);

    // Watermark at 10 words (5 gyro + 5 accel)
    asm330lhhx_fifo_watermark_set(&dev_ctx, IMU_FIFO_WATERMARK);

    // Route FIFO watermark interrupt to INT2, which is connected to PB2/I2C2_INT.
    int2_route.int2_ctrl.int2_fifo_th = PROPERTY_ENABLE;
    asm330lhhx_pin_int2_route_set(&dev_ctx, &int2_route);

    // Continuous (stream) mode - oldest data overwritten when full
    asm330lhhx_fifo_mode_set(&dev_ctx, ASM330LHHX_STREAM_MODE);
}

static asm330lhhx_fifo_tag_t IMU_TagFromRaw(uint8_t raw_tag)
{
    switch (raw_tag >> 3) {
        case 0x01U:
            return ASM330LHHX_GYRO_NC_TAG;
        case 0x02U:
            return ASM330LHHX_XL_NC_TAG;
        default:
            return (asm330lhhx_fifo_tag_t)0U;
    }
}

static void IMU_Start_FIFO_DMA(void)
{
    HAL_StatusTypeDef status;

    if (imu_dma_in_progress != 0U) {
        return;
    }

    imu_dma_in_progress = 1U;
    imu_dma_ready = 0U;

    status = HAL_I2C_Mem_Read_DMA(&hi2c2,
                                  ASM330LHHX_I2C_ADD_L,
                                  ASM330LHHX_FIFO_DATA_OUT_TAG,
                                  I2C_MEMADD_SIZE_8BIT,
                                  imu_dma_buf,
                                  IMU_FIFO_DMA_FRAME_SIZE);
    if (status != HAL_OK) {
        imu_dma_in_progress = 0U;
        (void)osThreadFlagsSet(imuHandle, IMU_THREAD_FLAG_DMA_ERROR);
    }
}

void IMU_FIFO_Read(uint8_t *out_buf, uint16_t *out_len)
{
    uint8_t gyro_idx = 0;
    uint8_t accel_idx = 0;

    *out_len = 0;

    if (imu_dma_ready == 0U) {
        return;
    }

    memset(out_buf, 0, IMU_FIFO_FRAME_SIZE);

    for (uint16_t i = 0; i < IMU_FIFO_WATERMARK; i++) {
        asm330lhhx_fifo_tag_t tag = IMU_TagFromRaw(imu_dma_buf[i * IMU_FIFO_WORD_SIZE]);
        uint8_t *data = &imu_dma_buf[(i * IMU_FIFO_WORD_SIZE) + 1U];

        if (tag == ASM330LHHX_GYRO_NC_TAG && gyro_idx < 5) {
            memcpy(&out_buf[gyro_idx * 6], data, 6);
            gyro_idx++;
        } else if (tag == ASM330LHHX_XL_NC_TAG && accel_idx < 5) {
            memcpy(&out_buf[30 + accel_idx * 6], data, 6);
            accel_idx++;
        }
    }

    if (gyro_idx == 5 && accel_idx == 5) {
        *out_len = IMU_FIFO_FRAME_SIZE;
    }

    imu_dma_ready = 0U;
    imu_dma_in_progress = 0U;
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == I2C2_INT_Pin) {
        IMU_Start_FIFO_DMA();
    }
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2) {
        imu_dma_ready = 1U;
        (void)osThreadFlagsSet(imuHandle, IMU_THREAD_FLAG_DMA_READY);
    }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c->Instance == I2C2) {
        imu_dma_ready = 0U;
        imu_dma_in_progress = 0U;
        (void)osThreadFlagsSet(imuHandle, IMU_THREAD_FLAG_DMA_ERROR);
    }
}
