#include "app_freertos.h"
#include "app_globals.h"
#include "imu.h"
#include "asm330lhhx_reg.h"
#include "motion_gc.h"
#include "ev27_daq.h"
#include <string.h>

uint8_t imu_frame[IMU_FIFO_FRAME_SIZE];
float imu_gyro_dps[3];   /* latest gyro sample: X, Y, Z in degrees/sec */
float imu_accel_g[3];    /* latest accel sample: X, Y, Z in g */
static osMutexId_t imu_frame_mutexHandle;
static bool imu_frame_valid = false;

bool IMU_GetLatestFrame(uint8_t *out_buf, uint16_t out_len)
{
    if (out_buf == NULL || out_len < IMU_FIFO_FRAME_SIZE || imu_frame_mutexHandle == NULL) {
        return false;
    }

    osMutexAcquire(imu_frame_mutexHandle, osWaitForever);
    bool valid = imu_frame_valid;
    if (valid) {
        memcpy(out_buf, imu_frame, IMU_FIFO_FRAME_SIZE);
    }
    osMutexRelease(imu_frame_mutexHandle);

    return valid;
}

/* No FIFO batch for this long (one batch is ~12 ms): the IMU is missing, was
 * unplugged, or the bus hung. Set it up again. */
#define IMU_NO_DATA_TIMEOUT_MS  500U

void start_imu(void *argument)
{
    /* Keep trying: a board can boot without the IMU plugged in, or with the bus
     * still held by an IMU that was mid-transfer when the MCU reset. */
    while (!IMU_Init()) {
        osDelay(IMU_NO_DATA_TIMEOUT_MS);
    }
    imu_frame_mutexHandle = osMutexNew(NULL);
    if (imu_frame_mutexHandle == NULL) Error_Handler();

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
        uint8_t current_frame[IMU_FIFO_FRAME_SIZE];
        uint32_t flags = osThreadFlagsWait(IMU_THREAD_FLAG_DMA_READY | IMU_THREAD_FLAG_DMA_ERROR,
                                           osFlagsWaitAny,
                                           IMU_NO_DATA_TIMEOUT_MS);

        if ((flags & osFlagsError) != 0U || (flags & IMU_THREAD_FLAG_DMA_ERROR) != 0U) {
            /* Timeout or I2C error: no data is coming. Recover the bus and set
             * the IMU up again (the ev27 Status shows it unhealthy meanwhile). */
            (void)IMU_Init();
            continue;
        }

        if ((flags & IMU_THREAD_FLAG_DMA_READY) != 0U) {
            IMU_FIFO_Read(current_frame, &len);
        }

        if (len == IMU_FIFO_FRAME_SIZE) {
            osMutexAcquire(imu_frame_mutexHandle, osWaitForever);
            memcpy(imu_frame, current_frame, IMU_FIFO_FRAME_SIZE);
            imu_frame_valid = true;
            osMutexRelease(imu_frame_mutexHandle);

            /* Convert first gyro sample (bytes 0-5) to degrees/sec */
            int16_t gx = (int16_t)(current_frame[1] << 8 | current_frame[0]);
            int16_t gy = (int16_t)(current_frame[3] << 8 | current_frame[2]);
            int16_t gz = (int16_t)(current_frame[5] << 8 | current_frame[4]);
            imu_gyro_dps[0] = asm330lhhx_from_fs2000dps_to_mdps(gx) / 1000.0f;
            imu_gyro_dps[1] = asm330lhhx_from_fs2000dps_to_mdps(gy) / 1000.0f;
            imu_gyro_dps[2] = asm330lhhx_from_fs2000dps_to_mdps(gz) / 1000.0f;

            /* Convert first accel sample (bytes 30-35) to g */
            int16_t ax = (int16_t)(current_frame[31] << 8 | current_frame[30]);
            int16_t ay = (int16_t)(current_frame[33] << 8 | current_frame[32]);
            int16_t az = (int16_t)(current_frame[35] << 8 | current_frame[34]);
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

            /* ev27 Imu: average of the 5 samples in this batch (~83 Hz), gyro
             * bias removed. Sensor axes as mounted: there is no mount rotation
             * (config record) yet, so this is not the vehicle frame. */
            float acc_avg[3] = {0}, gyr_avg[3] = {0};
            for (uint8_t k = 0; k < 5U; k++) {
                const uint8_t *g = &current_frame[k * 6U];
                const uint8_t *a = &current_frame[30U + k * 6U];
                for (uint8_t ax_i = 0; ax_i < 3U; ax_i++) {
                    int16_t graw = (int16_t)(g[ax_i * 2U + 1U] << 8 | g[ax_i * 2U]);
                    int16_t araw = (int16_t)(a[ax_i * 2U + 1U] << 8 | a[ax_i * 2U]);
                    gyr_avg[ax_i] += asm330lhhx_from_fs2000dps_to_mdps(graw) / 5000.0f;
                    acc_avg[ax_i] += asm330lhhx_from_fs4g_to_mg(araw) / 5000.0f;
                }
            }
            gyr_avg[0] -= gyro_bias.GyroBiasX;
            gyr_avg[1] -= gyro_bias.GyroBiasY;
            gyr_avg[2] -= gyro_bias.GyroBiasZ;
            ev27_daq_imu(acc_avg, gyr_avg);
        }
    }
}
