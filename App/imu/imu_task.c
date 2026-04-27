#include "app_freertos.h"
#include "app_globals.h"
#include "imu.h"
#include "asm330lhhx_reg.h"
#include "motion_gc.h"

uint8_t imu_frame[IMU_FIFO_FRAME_SIZE];
float imu_gyro_dps[3];   /* latest gyro sample: X, Y, Z in degrees/sec */
float imu_accel_g[3];    /* latest accel sample: X, Y, Z in g */

void start_imu(void *argument)
{
    IMU_Init();

    /* MotionGC: runtime gyro bias calibration */
    float gc_freq = 83.0f; /* ~1000/12ms per batch */
    MotionGC_Initialize(MGC_MCU_STM32, &gc_freq);

    MGC_knobs_t gc_knobs;
    MotionGC_GetKnobs(&gc_knobs);
    gc_knobs.AccThr  = 0.01f;  /* still-detection accel threshold [g] */
    gc_knobs.GyroThr = 0.2f;   /* still-detection gyro threshold [dps] */
    MotionGC_SetKnobs(&gc_knobs);

    MGC_output_t gyro_bias = {0};

    for (;;)
    {
        uint16_t len = 0;
        uint32_t flags = osThreadFlagsWait(IMU_THREAD_FLAG_DMA_READY | IMU_THREAD_FLAG_DMA_ERROR,
                                           osFlagsWaitAny,
                                           osWaitForever);

        if ((flags & IMU_THREAD_FLAG_DMA_ERROR) != 0U) {
            continue;
        }

        if ((flags & IMU_THREAD_FLAG_DMA_READY) != 0U) {
            IMU_FIFO_Read(imu_frame, &len);
        }

        if (len == IMU_FIFO_FRAME_SIZE) {
            /* Convert first gyro sample (bytes 0-5) to degrees/sec */
            int16_t gx = (int16_t)(imu_frame[1] << 8 | imu_frame[0]);
            int16_t gy = (int16_t)(imu_frame[3] << 8 | imu_frame[2]);
            int16_t gz = (int16_t)(imu_frame[5] << 8 | imu_frame[4]);
            imu_gyro_dps[0] = asm330lhhx_from_fs2000dps_to_mdps(gx) / 1000.0f;
            imu_gyro_dps[1] = asm330lhhx_from_fs2000dps_to_mdps(gy) / 1000.0f;
            imu_gyro_dps[2] = asm330lhhx_from_fs2000dps_to_mdps(gz) / 1000.0f;

            /* Convert first accel sample (bytes 30-35) to g */
            int16_t ax = (int16_t)(imu_frame[31] << 8 | imu_frame[30]);
            int16_t ay = (int16_t)(imu_frame[33] << 8 | imu_frame[32]);
            int16_t az = (int16_t)(imu_frame[35] << 8 | imu_frame[34]);
            imu_accel_g[0] = asm330lhhx_from_fs4g_to_mg(ax) / 1000.0f;
            imu_accel_g[1] = asm330lhhx_from_fs4g_to_mg(ay) / 1000.0f;
            imu_accel_g[2] = asm330lhhx_from_fs4g_to_mg(az) / 1000.0f;

            /* Feed MotionGC and apply gyro bias correction */
            MGC_input_t gc_in = {
                .Acc  = { imu_accel_g[0], imu_accel_g[1], imu_accel_g[2] },
                .Gyro = { imu_gyro_dps[0], imu_gyro_dps[1], imu_gyro_dps[2] }
            };
            int bias_updated;
            MotionGC_Update(&gc_in, &gyro_bias, &bias_updated);

            imu_gyro_dps[0] -= gyro_bias.GyroBiasX;
            imu_gyro_dps[1] -= gyro_bias.GyroBiasY;
            imu_gyro_dps[2] -= gyro_bias.GyroBiasZ;
        }
    }
}
