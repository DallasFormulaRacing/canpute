/**
 * @copyright (C) 2017 Melexis N.V.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include "MLX90641_I2C_Driver.h"
#include "app_globals.h"

#define MLX90641_I2C_TIMEOUT 100

void MLX90641_I2CInit(void)
{
    // CubeMX MX_I2C1_Init() handles peripheral setup
}

int MLX90641_I2CGeneralReset(void)
{
    uint8_t cmd = 0x06;

    if (HAL_I2C_Master_Transmit(&hi2c1, 0x00, &cmd, 1, MLX90641_I2C_TIMEOUT) != HAL_OK)
    {
        return -1;
    }

    HAL_Delay(1);

    return 0;
}

int MLX90641_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{
    uint16_t sa = (uint16_t)slaveAddr << 1;
    uint16_t bytesToRead = nMemAddressRead * 2;
    static uint8_t i2cData[1664];

    if (HAL_I2C_Mem_Read(&hi2c1, sa, startAddress, I2C_MEMADD_SIZE_16BIT,
                         i2cData, bytesToRead, MLX90641_I2C_TIMEOUT) != HAL_OK)
    {
        return -1;
    }

    // Reassemble 16-bit words from MSByte-first I2C transfer
    for (uint16_t cnt = 0; cnt < nMemAddressRead; cnt++)
    {
        uint16_t i = cnt << 1;
        data[cnt] = ((uint16_t)i2cData[i] << 8) | (uint16_t)i2cData[i + 1];
    }

    return 0;
}

void MLX90641_I2CFreqSet(int freq)
{
    (void)freq;
    // I2C clock is configured by CubeMX timing register
}

int MLX90641_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{
    uint16_t sa = (uint16_t)slaveAddr << 1;
    uint8_t cmd[2];
    uint16_t dataCheck;

    cmd[0] = data >> 8;
    cmd[1] = data & 0x00FF;

    if (HAL_I2C_Mem_Write(&hi2c1, sa, writeAddress, I2C_MEMADD_SIZE_16BIT,
                          cmd, 2, MLX90641_I2C_TIMEOUT) != HAL_OK)
    {
        return -1;
    }

    HAL_Delay(5);

    // Read back and verify
    if (MLX90641_I2CRead(slaveAddr, writeAddress, 1, &dataCheck) != 0)
    {
        return -1;
    }

    if (dataCheck != data)
    {
        return -2;
    }

    return 0;
}
