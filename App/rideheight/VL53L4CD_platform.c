#include "VL53L4CD_platform.h"
#include "app_globals.h"

// timeout for mutex handle
#define VL53L4CD_I2C_TIMEOUT 100

static uint8_t VL53L4CD_I2CRead(Dev_t dev, uint16_t reg, uint8_t *buf,
                                uint16_t len)
{
    osMutexAcquire(i2c1MutexHandle, osWaitForever);
    HAL_StatusTypeDef status =
        HAL_I2C_Mem_Read(&hi2c1, dev, reg, I2C_MEMADD_SIZE_16BIT, buf, len,
                         VL53L4CD_I2C_TIMEOUT);
    osMutexRelease(i2c1MutexHandle);

    return (status == HAL_OK) ? 0U : 255U;
}

static uint8_t VL53L4CD_I2CWrite(Dev_t dev, uint16_t reg, uint8_t *buf,
                                 uint16_t len)
{
    osMutexAcquire(i2c1MutexHandle, osWaitForever);
    HAL_StatusTypeDef status =
        HAL_I2C_Mem_Write(&hi2c1, dev, reg, I2C_MEMADD_SIZE_16BIT, buf, len,
                          VL53L4CD_I2C_TIMEOUT);
    osMutexRelease(i2c1MutexHandle);

    return (status == HAL_OK) ? 0U : 255U;
}

// Multi-byte registers are big-endian
uint8_t VL53L4CD_RdDWord(Dev_t dev, uint16_t RegisterAdress, uint32_t *value)
{
    uint8_t buf[4] = {0};
    uint8_t status = VL53L4CD_I2CRead(dev, RegisterAdress, buf, 4);

    *value = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
             ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
    return status;
}

uint8_t VL53L4CD_RdWord(Dev_t dev, uint16_t RegisterAdress, uint16_t *value)
{
    uint8_t buf[2] = {0};
    uint8_t status = VL53L4CD_I2CRead(dev, RegisterAdress, buf, 2);

    *value = (uint16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return status;
}

uint8_t VL53L4CD_RdByte(Dev_t dev, uint16_t RegisterAdress, uint8_t *value)
{
    return VL53L4CD_I2CRead(dev, RegisterAdress, value, 1);
}

uint8_t VL53L4CD_WrByte(Dev_t dev, uint16_t RegisterAdress, uint8_t value)
{
    return VL53L4CD_I2CWrite(dev, RegisterAdress, &value, 1);
}

uint8_t VL53L4CD_WrWord(Dev_t dev, uint16_t RegisterAdress, uint16_t value)
{
    uint8_t buf[2] = {(uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};

    return VL53L4CD_I2CWrite(dev, RegisterAdress, buf, 2);
}

uint8_t VL53L4CD_WrDWord(Dev_t dev, uint16_t RegisterAdress, uint32_t value)
{
    uint8_t buf[4] = {(uint8_t)(value >> 24), (uint8_t)(value >> 16),
                      (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};

    return VL53L4CD_I2CWrite(dev, RegisterAdress, buf, 4);
}

uint8_t VL53L4CD_WaitMs(Dev_t dev, uint32_t TimeMs)
{
    (void)dev;
    osDelay(TimeMs); // configTICK_RATE_HZ = 1000
    return 0;
}
