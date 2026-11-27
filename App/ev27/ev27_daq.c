/*
 * canpute on the ev27-can main bus. See ev27_daq.h.
 *
 * The ev27_port_* functions ev27_node needs are here too (started from
 * ev27-can template/ev27_port_stm32.c). canpute keeps its own RX interrupt in
 * can_utils.c, which stamps every frame with ev27_daq_time_us() first thing,
 * and its own CAN RX/TX tasks in can_task.c; standard-ID frames go to
 * ev27_daq_rx(), 29-bit frames (ezfd bootloader, old DFR commands) to
 * Process_CAN_Command().
 */
#include "ev27_daq.h"

#include <string.h>

#include "cmsis_os2.h"
#include "main.h"

#include "app_globals.h"
#include "can_utils.h"
#include "ev27_daq_config.h"
#include "ev27_node.h"

static uint8_t s_addr;
static uint8_t s_slot;
static uint8_t s_role;

static volatile uint32_t s_identify_until_ms;
static bool s_identify_was_on;

static volatile uint32_t s_imu_last_ms;
static volatile uint32_t s_tire_last_ms;
static volatile bool s_imu_seen;
static volatile bool s_tire_seen;
static uint8_t s_imu_counter;
static uint32_t s_ticks;

/* ---- setup ------------------------------------------------------------- */

/* TIM2 (32-bit) free-running at 1 MHz: the local clock behind car time. Set
 * up here rather than in CubeMX so the .ioc doesn't need a new timer. */
static void timer_init(void)
{
    uint32_t clk = HAL_RCC_GetPCLK1Freq();
    if ((RCC->CFGR2 & RCC_CFGR2_PPRE1) != 0u) {
        clk *= 2u;   /* timers run at 2x PCLK1 when APB1 is divided */
    }
    __HAL_RCC_TIM2_CLK_ENABLE();
    TIM2->CR1 = 0;
    TIM2->PSC = (clk / 1000000u) - 1u;
    TIM2->ARR = 0xFFFFFFFFu;
    TIM2->CNT = 0;
    TIM2->EGR = TIM_EGR_UG;      /* load PSC */
    TIM2->SR = 0;
    TIM2->CR1 = TIM_CR1_CEN;
}

/* Map canpute's Fleet_Table identity onto ev27-can devices.yaml. Corners get
 * their ev27 address, slot and role. Anything else (the bench board at 0x1F,
 * the Nucleos) keeps its own address and sends data as EV27_BENCH_SLOT. */
static void identify_self(void)
{
    switch (self_node_id) {
    case NODE_ID_FRONT_LEFT:
        s_addr = EV27_ADDR_DAQ_FL; s_slot = EV27_SLOT_DAQ_FL; s_role = EV27_MAIN_STATUS_ROLE_FL_CHOICE; break;
    case NODE_ID_FRONT_RIGHT:
        s_addr = EV27_ADDR_DAQ_FR; s_slot = EV27_SLOT_DAQ_FR; s_role = EV27_MAIN_STATUS_ROLE_FR_CHOICE; break;
    case NODE_ID_REAR_LEFT:
        s_addr = EV27_ADDR_DAQ_RL; s_slot = EV27_SLOT_DAQ_RL; s_role = EV27_MAIN_STATUS_ROLE_RL_CHOICE; break;
    case NODE_ID_REAR_RIGHT:
        s_addr = EV27_ADDR_DAQ_RR; s_slot = EV27_SLOT_DAQ_RR; s_role = EV27_MAIN_STATUS_ROLE_RR_CHOICE; break;
    default:
        s_addr = (uint8_t)self_node_id; s_slot = EV27_BENCH_SLOT; s_role = EV27_BENCH_ROLE; break;
    }
}

void ev27_daq_hw_init(void)
{
    identify_self();
    timer_init();
}

void ev27_daq_config_filters(FDCAN_HandleTypeDef *hfdcan, uint32_t first_index)
{
    static const uint32_t ids[] = EV27_RX_IDS;
    const uint32_t n = sizeof(ids) / sizeof(ids[0]);
    FDCAN_FilterTypeDef f;

    for (uint32_t i = 0; i < n; i += 2u) {
        memset(&f, 0, sizeof(f));
        f.IdType = FDCAN_STANDARD_ID;
        f.FilterIndex = first_index + i / 2u;
        f.FilterType = FDCAN_FILTER_DUAL;
        f.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
        f.FilterID1 = ids[i];
        f.FilterID2 = (i + 1u < n) ? ids[i + 1u] : ids[i];
        if (HAL_FDCAN_ConfigFilter(hfdcan, &f) != HAL_OK) {
            Error_Handler();
        }
    }
}

void ev27_daq_task_init(void)
{
    const ev27_node_info_t info = {
        .addr = s_addr,
        .role = s_role,
        .fw_major = EV27_FW_MAJOR, .fw_minor = EV27_FW_MINOR, .fw_patch = EV27_FW_PATCH,
        .git_hash = EV27_GIT_HASH,
        .uid = {HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2()},
        .config_valid = false,   /* canpute has no config record yet (mount rotation, role) */
    };
    ev27_node_init(&info);
}

uint8_t ev27_daq_addr(void) { return s_addr; }

uint32_t ev27_daq_time_us(void) { return TIM2->CNT; }

/* ---- periodic ---------------------------------------------------------- */

static uint16_t sensors_healthy(uint32_t now_ms)
{
    uint16_t h = 0;
    if (s_imu_seen && (now_ms - s_imu_last_ms) <= EV27_IMU_STALE_MS) {
        h |= EV27_SENSOR_IMU;
    }
    if (s_tire_seen && (now_ms - s_tire_last_ms) <= EV27_TIRE_STALE_MS) {
        h |= EV27_SENSOR_TIRE_TEMP;
    }
    return h;
}

void ev27_daq_tick_10ms(void)
{
    const uint32_t now_ms = ev27_port_uptime_ms();
    s_ticks++;

    if (s_ticks % (EV27_MAIN_STATUS_CYCLE_TIME_MS / 10u) == 0u) {
        uint16_t present = (s_imu_seen ? EV27_SENSOR_IMU : 0u) | (s_tire_seen ? EV27_SENSOR_TIRE_TEMP : 0u);
        ev27_node_set_sensors(EV27_SENSORS_ENABLED, present, sensors_healthy(now_ms));
        ev27_node_set_error_flags(0);
        ev27_node_send_status();
    }

    /* Identify: blink the green LED at 5 Hz for a few seconds. */
    if ((int32_t)(s_identify_until_ms - now_ms) > 0) {
        if (s_ticks % 10u == 0u) {
            HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);
        }
        s_identify_was_on = true;
    } else if (s_identify_was_on) {
        HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
        s_identify_was_on = false;
    }
}

void ev27_daq_rx(uint32_t id, const uint8_t *data, uint8_t len, uint32_t rx_time_us)
{
    if (ev27_node_rx(id, data, len, rx_time_us)) {
        return;
    }
    /* Nothing else on the main bus is for canpute yet. */
}

/* ---- data messages ----------------------------------------------------- */

static float clampf(float v, float lim)
{
    if (v > lim) return lim;
    if (v < -lim) return -lim;
    return v;
}

void ev27_daq_imu(const float accel_g[3], const float gyro_dps[3])
{
    /* i16 limits of the message: +-7.99 g and +-286 deg/s. The generated
     * encoders don't clamp, and an out-of-range float->int cast is undefined. */
    const float a_lim = 32767.0f * 0.000244f;
    const float g_lim = 32767.0f * 0.00875f;
    struct ev27_main_imu_t m;
    uint8_t buf[EV27_MAIN_IMU_LENGTH];

    memset(&m, 0, sizeof(m));
    m.accel_x = ev27_main_imu_accel_x_encode(clampf(accel_g[0], a_lim));
    m.accel_y = ev27_main_imu_accel_y_encode(clampf(accel_g[1], a_lim));
    m.accel_z = ev27_main_imu_accel_z_encode(clampf(accel_g[2], a_lim));
    m.gyro_x = ev27_main_imu_gyro_x_encode(clampf(gyro_dps[0], g_lim));
    m.gyro_y = ev27_main_imu_gyro_y_encode(clampf(gyro_dps[1], g_lim));
    m.gyro_z = ev27_main_imu_gyro_z_encode(clampf(gyro_dps[2], g_lim));
    m.counter = (uint8_t)(s_imu_counter++ & 0x0Fu);
    s_imu_last_ms = ev27_port_uptime_ms();
    s_imu_seen = true;
    if (ev27_main_imu_pack(buf, &m, sizeof(buf)) > 0) {
        (void)ev27_node_send(EV27_MAIN_IMU_FRAME_ID + s_slot, buf, sizeof(buf));
    }
}

void ev27_daq_tire_temp(const int16_t z[16])
{
    struct ev27_main_tire_temp_t m;
    uint8_t buf[EV27_MAIN_TIRE_TEMP_LENGTH];

    /* Signal scale is 0.1 degC, the same unit as the input, so raw = value. */
    memset(&m, 0, sizeof(m));
    m.zone01 = z[0];  m.zone02 = z[1];  m.zone03 = z[2];  m.zone04 = z[3];
    m.zone05 = z[4];  m.zone06 = z[5];  m.zone07 = z[6];  m.zone08 = z[7];
    m.zone09 = z[8];  m.zone10 = z[9];  m.zone11 = z[10]; m.zone12 = z[11];
    m.zone13 = z[12]; m.zone14 = z[13]; m.zone15 = z[14]; m.zone16 = z[15];
    s_tire_last_ms = ev27_port_uptime_ms();
    s_tire_seen = true;
    if (ev27_main_tire_temp_pack(buf, &m, sizeof(buf)) > 0) {
        (void)ev27_node_send(EV27_MAIN_TIRE_TEMP_FRAME_ID + s_slot, buf, sizeof(buf));
    }
}

/* ---- ev27_port_* for ev27_node ----------------------------------------- */

static uint32_t len_to_dlc(uint8_t len)
{
    static const uint8_t sizes[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};
    uint32_t dlc;
    for (dlc = 0; dlc < 15u; dlc++) {
        if (sizes[dlc] >= len) {
            break;
        }
    }
    return dlc;   /* FDCAN_DLC_BYTES_x on the H5 HAL are these same codes */
}

int ev27_port_send(uint32_t id, const uint8_t *data, uint8_t len)
{
    FDCAN_TxHeaderTypeDef h;
    uint8_t buf[64];
    HAL_StatusTypeDef st;

    if (len > 64u) {
        return -1;
    }
    memset(&h, 0, sizeof(h));
    h.Identifier = id;
    h.IdType = FDCAN_STANDARD_ID;
    h.TxFrameType = FDCAN_DATA_FRAME;
    h.DataLength = len_to_dlc(len);
    h.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    /* canpute runs the data phase at 5 Mbit/s (fdcan.c), matching the ezfd
     * bootloader, so switch bit rate like CAN_Transmit does. ev27-can lists
     * the main bus data rate as 1 Mbit/s "TBD"; BRS on is fine either way. */
    h.BitRateSwitch = FDCAN_BRS_ON;
    h.FDFormat = FDCAN_FD_CAN;
    h.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    memset(buf, 0, sizeof(buf));
    memcpy(buf, data, len);

    CAN_TxLock();
    st = HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &h, buf);
    CAN_TxUnlock();
    return (st == HAL_OK) ? 0 : -1;
}

uint32_t ev27_port_time_us(void) { return TIM2->CNT; }

uint32_t ev27_port_uptime_ms(void) { return osKernelGetTickCount(); }   /* 1 kHz tick */

void ev27_port_can_errors(uint8_t *tec, uint8_t *rec)
{
    FDCAN_ErrorCountersTypeDef c;
    if (HAL_FDCAN_GetErrorCounters(&hfdcan2, &c) == HAL_OK) {
        *tec = (uint8_t)(c.TxErrorCnt > 255u ? 255u : c.TxErrorCnt);
        *rec = (uint8_t)(c.RxErrorCnt > 255u ? 255u : c.RxErrorCnt);
    }
}

void ev27_port_reboot_to_bootloader(void)
{
    /* The ezfd bootloader has no entry flag: on every reset it sends
     * FirmwareUpdateQuery and stays if the Pi answers within 100 ms. So a plain
     * reset is enough when the Pi is waiting (the dashboard's flash job is). */
    NVIC_SystemReset();
}

void ev27_port_identify(void)
{
    s_identify_until_ms = ev27_port_uptime_ms() + 5000u;
}
