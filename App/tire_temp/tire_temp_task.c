#include "app_freertos.h"
#include "app_globals.h"
#include "MLX90641_API.h"
#include "MLX90641_I2C_Driver.h"

#define MLX90641_ADDR    0x33
#define MLX_REFRESH_RATE 0x04  // 8 Hz
#define EMISSIVITY       0.98f // human skin
//#define EMISSIVITY       0.95f // rubber tire

static int16_t Temperature_ToDeciC(float temperature_c)
{
    float temperature_dC = (temperature_c * 10.0f);

    if (temperature_dC >= (float)INT16_MAX) {
        return INT16_MAX;
    }

    if (temperature_dC <= (float)INT16_MIN) {
        return INT16_MIN;
    }

    if (temperature_dC >= 0.0f) {
        return (int16_t)(temperature_dC + 0.5f);
    }

    return (int16_t)(temperature_dC - 0.5f);
}

void start_tire_temp(void *argument)
{
    static uint16_t eeData[832];
    static paramsMLX90641 mlxParams;
    static uint16_t frameData[242];
    static float tempGrid[192]; // 16 cols x 12 rows

    MLX90641_I2CInit();
    MLX90641_SetRefreshRate(MLX90641_ADDR, MLX_REFRESH_RATE);
    MLX90641_DumpEE(MLX90641_ADDR, eeData);
    MLX90641_ExtractParameters(eeData, &mlxParams);

    for (;;)
    {
        int status = MLX90641_GetFrameData(MLX90641_ADDR, frameData);
        if (status < 0) {
            osDelay(100);
            continue;
        }

        float ta = MLX90641_GetTa(frameData, &mlxParams);
        float tr = ta - 8.0f;

        MLX90641_CalculateTo(frameData, &mlxParams, EMISSIVITY, tr, tempGrid);
        MLX90641_BadPixelsCorrection(mlxParams.brokenPixel, tempGrid);

        int16_t tireZones[15];
        uint8_t zoneIdx = 0;

        for (uint8_t rowBand = 0; rowBand < 3; rowBand++) {
            uint8_t rowStart = rowBand * 4;
            uint8_t rowEnd = rowStart + 4;

            for (uint8_t colBand = 0; colBand < 5; colBand++) {
                uint8_t colStart;
                uint8_t colEnd;

                if (colBand == 0) {
                    colStart = 0;
                    colEnd = 3;
                } else if (colBand == 1) {
                    colStart = 3;
                    colEnd = 6;
                } else if (colBand == 2) {
                    colStart = 6;
                    colEnd = 10;
                } else if (colBand == 3) {
                    colStart = 10;
                    colEnd = 13;
                } else {
                    colStart = 13;
                    colEnd = 16;
                }

                float sum = 0.0f;
                uint8_t sampleCount = 0;

                for (uint8_t row = rowStart; row < rowEnd; row++) {
                    for (uint8_t col = colStart; col < colEnd; col++) {
                        sum += tempGrid[(row * 16) + col];
                        sampleCount++;
                    }
                }

                tireZones[zoneIdx] = Temperature_ToDeciC(sum / (float)sampleCount);
                zoneIdx++;
            }
        }

        osMutexAcquire(temperatureSpeedDataMutexHandle, osWaitForever);
        for (uint8_t i = 0; i < 15; i++) {
            temperatureSpeedData.tireTemperature_dC[i] = tireZones[i];
        }
        osMutexRelease(temperatureSpeedDataMutexHandle);
    }
}
