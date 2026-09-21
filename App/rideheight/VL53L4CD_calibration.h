#ifndef VL53L4CD_CALIBRATION_H_
#define VL53L4CD_CALIBRATION_H_

#include "VL53L4CD_api.h"

/* Offset calibration: target at TargetDistInMm (50..1000 mm), nb_samples 5..255.
 * The result is written to the sensor and returned so it can be reapplied with VL53L4CD_SetOffset(). */
VL53L4CD_Error VL53L4CD_CalibrateOffset(Dev_t dev, int16_t TargetDistInMm, int16_t *p_measured_offset_mm, int16_t nb_samples);

/* Crosstalk calibration, only needed with a cover window: target at TargetDistInMm (50..5000 mm), nb_samples 5..255.
 * The result is written to the sensor and returned so it can be reapplied with VL53L4CD_SetXtalk(). */
VL53L4CD_Error VL53L4CD_CalibrateXtalk(Dev_t dev, int16_t TargetDistInMm, uint16_t *p_measured_xtalk_kcps, int16_t nb_samples);

#endif /* VL53L4CD_CALIBRATION_H_ */
