/* App/Inc/commands.h */
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef struct __attribute__((packed)){
    uint32_t linPotData;
    uint32_t wheelSpeed;
    uint32_t fillerData4bytes;
    uint16_t fillerData2bytes;
    uint8_t brakeTemperature;
    uint8_t tireTemperature;
} NodeDataTypeDef;


extern NodeDataTypeDef nodeData;

#endif