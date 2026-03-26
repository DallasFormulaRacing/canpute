#ifndef IMU_H
#define IMU_H

#include "stm32h5xx_hal.h"
#include "gpio.h"
#include "i2c.h"
#include <string.h>
#include <stdio.h>
#include <asm330lhhx_reg.h>

static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void platform_delay(uint32_t ms);


#endif /* IMU_H */
