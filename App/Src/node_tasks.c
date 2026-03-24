#include "node_tasks.h"
#include "app_freertos.h"
#include "app_globals.h"
#include "can_utils.h"
#include "command_handler.h"
#include "MLX90641_API.h"
#include "MLX90641_I2C_Driver.h"
#include <string.h>

#define MLX90641_ADDR    0x33
#define MLX_REFRESH_RATE 0x04  // 8 Hz
#define EMISSIVITY       0.95f // rubber tire


void start_canfd_tx(void *argument)
{
    for(;;) {
        osDelay(1000);
    }
}
void start_canfd_rx(void *argument){
    CAN_RXMsg_t msg;

    for(;;) {
        if (osMessageQueueGet(canfd_rx_queueHandle, &msg, NULL, osWaitForever) == osOK) {
            Process_CAN_Command(msg.id, msg.data);
        }
    }
}

void start_wheel_speed(void *argument)
{
  float rxFrequency = 0;
  for (;;)
  {
    // Wait for 500ms. Car is stopped if no pulses received.
    osStatus_t status = osMessageQueueGet(wheelSpeedFrequencyHandle, &rxFrequency, NULL, 500);

    osMutexAcquire(nodeDataMutexHandle, osWaitForever);
    if (status == osOK) {
        nodeData.wheelSpeed = (uint32_t)rxFrequency; // Convert freq to RPM/Speed as needed
    } else {
        nodeData.wheelSpeed = 0; // Timeout reached, car is stopped
    }
    osMutexRelease(nodeDataMutexHandle);
  }
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

        // Average the center 6x6 block (rows 3-8, cols 5-10)
        float sum = 0.0f;
        for (int row = 3; row < 9; row++) {
            for (int col = 5; col < 11; col++) {
                sum += tempGrid[row * 16 + col];
            }
        }
        float avgTemp = sum / 36.0f;

        osMutexAcquire(nodeDataMutexHandle, osWaitForever);
        nodeData.tireTemperature = (uint8_t)avgTemp;
        osMutexRelease(nodeDataMutexHandle);
    }
}

void StandaloneTimer_Callback(void *argument)
{
  osEventFlagsSet(systemEventFlagsHandle, FLAG_TIMER_TICK);
}
