#include "cmsis_os.h"
#include "stm32h5xx_hal.h"

extern ADC_HandleTypeDef hadc1;

uint32_t sharedPotValue = 0;

osMutexId_t potMutexHandle;

const osMutexAttr_t potMutex_attributes = {
  .name = "potentiometerMutex",
  .attr_bits = osMutexRecursive | osMutexPrioInherit,
  .cb_mem = NULL,
  .cb_size = 0u
};

void potMutex_init(void)
{
  potMutexHandle = osMutexNew(&potMutex_attributes);

  if(potMutexHandle == NULL)
  {
    for(;;)
  }
}

void StartPotReadTask(void *argument)
{
  for(;;)
  {
    HAL_ADC_Start(&hadc1);

    if(HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
      uint32_t rawValue = HAL_ADC_GetValue(&hadc1);

      if(osMutexAcquire(potMutexHandle, osWaitForever) == osOk)
      {
        sharedPotValue = rawValue;
        osMutexRelease(potMutexHandle);
      }
    }

    HAL_ADC_Stop(&hadc1);
    osDelay(20);
  }
}