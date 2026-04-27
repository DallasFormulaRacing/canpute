#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>


#define FLAG_PI_SYNC      0x01
#define FLAG_TIMER_TICK   0x02
#define FLAG_RPM_PULSE    0x04

#define CAN_EXT_ID_MASK 0x1FFFFFFF

#define BUILD_CAN_ID(prio, target, cmd, source) \
    (((uint32_t)(prio) & 0x07) << 26) |         \
    (((uint32_t)(target) & 0x1F) << 21) |       \
    (((uint32_t)(cmd) & 0xFFFF) << 5) |         \
    ((uint32_t)(source) & 0x1F)

typedef struct __attribute__((packed)){
    uint32_t linPotData;
    uint32_t wheelSpeed;
    uint32_t fillerData4bytes;
    uint16_t fillerData2bytes;
    uint8_t brakeTemperature;
    uint8_t tireTemperature;
} NodeDataTypeDef;
typedef enum {
    NODE_ID_ALL_NODES   = 0x01, // 00001 (Broadcast)
    NODE_ID_FRONT_LEFT  = 0x02, // 00010
    NODE_ID_FRONT_RIGHT = 0x03, // 00011
    NODE_ID_REAR_LEFT   = 0x04, // 00100
    NODE_ID_REAR_RIGHT  = 0x05, // 00101

    NODE_ID_NUCLEO_1    = 0x06, // 00110
    NODE_ID_NUCLEO_2    = 0x07, // 00111
    NODE_ID_RASPI       = 0x1E, // 11110 (Node 30, reserved for Raspberry Pi)
    NODE_ID_DASH        = 0x1D, //  11101 (Node 29, reserved for Dashboard)
    NODE_ID_BMS         = 0x1F, // 11111 (Node 31, reserved for Battery Management System)
    NODE_ID_UNKNOWN     = 0x00
} NodeHardwareID_t;

typedef enum {
    CMD_PING                = 0xA001,
    CMD_PONG                = 0xA002,

    CMD_REQ_IMU_DATA        = 0xD101,
    CMD_REQ_TEMP_DATA       = 0xD102,
    CMD_REQ_SPEED_DATA      = 0xD103,
    CMD_REQ_RIDE_HEIGHT_DATA = 0xD104,
    CMD_IMU_DATA            = 0xD201,
    CMD_TEMP_DATA           = 0xD202,
    CMD_SPEED_DATA          = 0xD203,
    CMD_RIDE_HEIGHT_DATA    = 0xD204,

    CMD_SET_LED             = 0xD301,
    CMD_RESET_NODE          = 0xDF01,
    CMD_REQ_UUID            = 0xDF02,
    CMD_REQ_FW_VER          = 0xDF03,

    BL_CMD_ERASE            = 0xF001,
    BL_CMD_ERASE_OK         = 0xF002,
    BL_CMD_WRITE            = 0xF003,
    BL_CMD_WRITE_OK         = 0xF004,
    BL_CMD_ADDR_SIZE        = 0xF005,
    BL_CMD_FW_QUERY         = 0xF006,
    BL_CMD_FW_RESP          = 0xF007,
    BL_CMD_REBOOT           = 0xF008,
    BL_CMD_JUMP             = 0xFAAA
} CommandID_t;

typedef struct {
    uint32_t uid[3];            // STM32 96-bit UID
    NodeHardwareID_t nodeType;  // Assigned CAN ID
} UID_Mapping_t;

typedef enum {
    MODE_STARTUP,
    MODE_STANDALONE,
    MODE_PI_LINKED
} CAN_SystemMode_t;
typedef struct {
      uint32_t id;
      uint8_t  data[64];
} CAN_RXMsg_t;

#endif
