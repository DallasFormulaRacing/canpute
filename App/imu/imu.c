#include "imu.h"
#include "app_globals.h"
#include <string.h>

static stmdev_ctx_t dev_ctx;

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

    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg  = platform_read;
    dev_ctx.mdelay    = platform_delay;
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

    // Continuous (stream) mode - oldest data overwritten when full
    asm330lhhx_fifo_mode_set(&dev_ctx, ASM330LHHX_STREAM_MODE);
}

void IMU_FIFO_Read(uint8_t *out_buf, uint16_t *out_len)
{
    uint8_t wtm_flag = 0;
    uint16_t fifo_level = 0;
    uint8_t gyro_idx = 0;
    uint8_t accel_idx = 0;

    *out_len = 0;

    asm330lhhx_fifo_wtm_flag_get(&dev_ctx, &wtm_flag);
    if (!wtm_flag) {
        return;
    }

    asm330lhhx_fifo_data_level_get(&dev_ctx, &fifo_level);
    if (fifo_level < IMU_FIFO_WATERMARK) {
        return;
    }

    memset(out_buf, 0, IMU_FIFO_FRAME_SIZE);

    for (uint16_t i = 0; i < IMU_FIFO_WATERMARK; i++) {
        asm330lhhx_fifo_tag_t tag;
        uint8_t data[6];

        asm330lhhx_fifo_sensor_tag_get(&dev_ctx, &tag);
        asm330lhhx_fifo_out_raw_get(&dev_ctx, data);

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
}
