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
    CMD_ID_PING             = 0x001,
    CMD_ID_PONG             = 0x060,
    CMD_ID_GET_RANDOM       = 0x049, // For testing: Node responds with random data,
    CMD_ID_REQ_DATA         = 0x050, 
    CMD_ID_SENDING_DATA     = 0x051, 
    CMD_ID_RESET_NODE       = 0x099,
    
    CMD_ID_SET_LED          = 0x100, // Data[0]: 0=Off, 1=On
    CMD_ID_SET_FREQ         = 0x101, // Data[0]: New Standalone frequency in Hz
    CMD_ID_RESET_SIM        = 0x102, // No data: Resets the simulated sensor counters
    CMD_ID_SET_OFFSET       = 0x103, // Data[0-3]: uint32_t offset for LinPot data

    // Bootloader commands (shared with ezfdbootloader protocol)
    BL_CMD_PING             = 0x040, // Ping: reply data[0]=0 (bootloader) or 1 (app)
    BL_CMD_REBOOT           = 0x04D  // Reboot into bootloader
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

#endif