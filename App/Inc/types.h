/* App/Inc/commands.h */
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>


#define FLAG_PI_SYNC      0x01  
#define FLAG_TIMER_TICK   0x02  
#define FLAG_RPM_PULSE    0x04  

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
    NODE_ID_DASH        = 0x1F, // 11111 (Node 31, reserved for Dashboard)
    NODE_ID_UNKNOWN     = 0x00
} NodeHardwareID_t;


typedef struct {
    uint32_t uid[3];            // STM32 96-bit UID
    NodeHardwareID_t nodeType;  // Assigned CAN ID
} UID_Mapping_t;

typedef enum {
    MODE_STARTUP,
    MODE_STANDALONE,
    MODE_PI_LINKED
} CAN_SystemMode_t;



extern NodeHardwareID_t self_node_id;
extern NodeDataTypeDef nodeData;

#endif