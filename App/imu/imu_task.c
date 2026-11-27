#include "app_freertos.h"
#include "app_globals.h"
#include "imu.h"
#include "asm330lhhx_reg.h"
#include "motion_gc.h"
#include "ev27_daq.h"
#include <string.h>
#include <math.h>

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

/* ---------------------------------------------------------------------------
 * Level calibration (mount tilt).
 *
 * At rest the accelerometer reads +1 g on whatever axis points up. If the board
 * isn't mounted flat, part of that lands in X/Y and the G meter sits off centre.
 * After boot, once the IMU has been still for LEVEL_BATCHES batches (~1.2 s), the
 * average accel vector is taken as "up" and a rotation R that turns it onto +Z
 * is applied to every sample after that (accel and gyro). Gravity then stays in
 * Z, X/Y read 0 g at rest, and real motion is still measured (this is a rotation,
 * not an offset, so braking on a tilted mount isn't mixed into the wrong axis).
 *
 * Limits: gravity can't tell which way is forward, so X/Y keep the board's own
 * heading (the yaw mount config record is still TBD). If the car is on a slope
 * at boot, that slope is calibrated out as "level".
 * ------------------------------------------------------------------------- */
#define LEVEL_BATCHES        100U    /* ~1.2 s of still batches */
#define LEVEL_GYRO_STILL_DPS 3.0f    /* |gyro| below this counts as still */
#define LEVEL_ACC_TOL_G      0.05f   /* | |accel| - 1 g | below this counts as still */

static float level_R[3][3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
static bool level_done = false;
static float level_sum[3];
static uint32_t level_n;

static void level_compute(const float up[3])
{
    float n = sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);
    if (n < 0.5f) {
        return;
    }
    const float u[3] = { up[0] / n, up[1] / n, up[2] / n };
    /* Rotation taking u onto z = (0,0,1), Rodrigues: v = u x z, c = u . z,
     * R = I + [v]x + [v]x^2 / (1 + c). */
    const float c = u[2];
    if (c < -0.999f) {
        /* Mounted upside down: 180 deg about X. */
        const float flip[3][3] = { {1, 0, 0}, {0, -1, 0}, {0, 0, -1} };
        memcpy(level_R, flip, sizeof(level_R));
    } else {
        const float vx = u[1], vy = -u[0];   /* u x z, vz = 0 */
        const float k = 1.0f / (1.0f + c);
        level_R[0][0] = 1.0f - k * vy * vy;
        level_R[0][1] = k * vx * vy;
        level_R[0][2] = vy;
        level_R[1][0] = k * vx * vy;
        level_R[1][1] = 1.0f - k * vx * vx;
        level_R[1][2] = -vx;
        level_R[2][0] = -vy;
        level_R[2][1] = vx;
        level_R[2][2] = 1.0f - k * (vx * vx + vy * vy);
    }
    level_done = true;
}

/* Feed one batch average; collects still batches until the level is known. */
static void level_update(const float acc[3], const float gyr[3])
{
    if (level_done) {
        return;
    }
    const float g = sqrtf(gyr[0] * gyr[0] + gyr[1] * gyr[1] + gyr[2] * gyr[2]);
    const float a = sqrtf(acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2]);
    if (g > LEVEL_GYRO_STILL_DPS || fabsf(a - 1.0f) > LEVEL_ACC_TOL_G) {
        level_sum[0] = level_sum[1] = level_sum[2] = 0.0f;   /* moved: start over */
        level_n = 0;
        return;
    }
    for (int i = 0; i < 3; i++) {
        level_sum[i] += acc[i];
    }
    if (++level_n >= LEVEL_BATCHES) {
        const float up[3] = { level_sum[0] / level_n, level_sum[1] / level_n, level_sum[2] / level_n };
        level_compute(up);
    }
}

static void level_apply(float v[3])
{
    const float x = v[0], y = v[1], z = v[2];
    for (int i = 0; i < 3; i++) {
        v[i] = level_R[i][0] * x + level_R[i][1] * y + level_R[i][2] * z;
    }
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

            /* Mount tilt: level once still after boot, then rotate every
             * sample so gravity is on +Z and X/Y read 0 g at rest. Until then
             * the raw (tilted) axes are sent. */
            level_update(acc_avg, gyr_avg);
            level_apply(acc_avg);
            level_apply(gyr_avg);
            ev27_daq_imu(acc_avg, gyr_avg);
        }
    }
}
