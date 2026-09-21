#ifndef VL53L4CD_PLATFORM_H_
#define VL53L4CD_PLATFORM_H_

#include <stdint.h>

typedef uint16_t Dev_t;

uint8_t VL53L4CD_RdDWord(Dev_t dev, uint16_t RegisterAdress, uint32_t *value);
uint8_t VL53L4CD_RdWord(Dev_t dev, uint16_t RegisterAdress, uint16_t *value);
uint8_t VL53L4CD_RdByte(Dev_t dev, uint16_t RegisterAdress, uint8_t *value);
uint8_t VL53L4CD_WrByte(Dev_t dev, uint16_t RegisterAdress, uint8_t value);
uint8_t VL53L4CD_WrWord(Dev_t dev, uint16_t RegisterAdress, uint16_t value);
uint8_t VL53L4CD_WrDWord(Dev_t dev, uint16_t RegisterAdress, uint32_t value);
uint8_t VL53L4CD_WaitMs(Dev_t dev, uint32_t TimeMs);

#endif /* VL53L4CD_PLATFORM_H_ */
