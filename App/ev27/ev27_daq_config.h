/*
 * canpute's settings for the ev27-can main bus (Status, TimeSync, Commands and
 * the DAQ data messages). Started from ev27-can template/ev27_device_config.h.
 */
#ifndef EV27_DAQ_CONFIG_H
#define EV27_DAQ_CONFIG_H

#include "ev27_devices.h"
#include "ev27_main.h"
#include "can_utils.h"

/* Firmware version reported in Status (FW_VERSION_* live in can_utils.h). */
#define EV27_FW_MAJOR     FW_VERSION_MAJOR
#define EV27_FW_MINOR     FW_VERSION_MINOR
#define EV27_FW_PATCH     FW_VERSION_PATCH
#ifndef EV27_GIT_HASH
#define EV27_GIT_HASH     0x00000000u   /* CMakeLists.txt sets it from `git rev-parse --short=8 HEAD` */
#endif

/* A board that isn't a known corner (not in Fleet_Table, so address 0x1F, the
 * ezfd default) still sends DAQ data, using this corner slot and role. Bench
 * only: two such boards on one bus would collide. */
#define EV27_BENCH_SLOT   EV27_SLOT_DAQ_FL
#define EV27_BENCH_ROLE   EV27_MAIN_STATUS_ROLE_NONE_CHOICE

/* Main-bus standard IDs this node receives (two per hardware filter). The
 * ezfd / legacy 29-bit commands are filtered separately in app_init.c.
 * Keep hfdcan2.Init.StdFiltersNbr >= (number of IDs + 1) / 2. */
#define EV27_RX_IDS { \
    EV27_MAIN_CM5_TIME_SYNC_FRAME_ID, \
    EV27_MAIN_CM5_COMMAND_FRAME_ID, \
}

/* Status.SensorsEnabled bits (can/main/status.yaml). */
#define EV27_SENSOR_IMU        (1u << 0)
#define EV27_SENSOR_TIRE_TEMP  (1u << 1)
#define EV27_SENSOR_BRAKE_TEMP (1u << 2)
#define EV27_SENSOR_WHEEL      (1u << 3)
#define EV27_SENSOR_LINPOT     (1u << 4)
#define EV27_SENSOR_RIDE       (1u << 5)

/* What this firmware drives today. */
#define EV27_SENSORS_ENABLED   (EV27_SENSOR_IMU | EV27_SENSOR_TIRE_TEMP)

/* A sensor counts as healthy if it delivered data this recently. */
#define EV27_IMU_STALE_MS      200u
#define EV27_TIRE_STALE_MS     1000u

#endif /* EV27_DAQ_CONFIG_H */
