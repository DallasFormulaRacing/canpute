#include "imu.h"
#include "stm32h533xx.h"
#include "asm330lhhx_reg.h"

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
    HAL_I2C_Mem_Read(handle, ASM330LHHX_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT, bufp, len, HAL_MAX_DELAY);
    return 0;
}
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp, uint16_t len)
{
    HAL_I2C_Mem_Write(handle, ASM330LHHX_I2C_ADD_L, reg, I2C_MEMADD_SIZE_8BIT, (uint8_t *)bufp, len, HAL_MAX_DELAY);
    return 0;
}
static void platform_delay(uint32_t ms){
    HAL_Delay(ms);
}
