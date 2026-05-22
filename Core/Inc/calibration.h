#ifndef __CALIBRATION_H
#define __CALIBRATION_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    CALIB_STEP_IDLE = 0,
    CALIB_STEP_TARE,
    CALIB_STEP_LOAD,
    CALIB_STEP_INPUT_MASS,
    CALIB_STEP_CALCULATE,
    CALIB_STEP_SAVE,
    CALIB_STEP_VERIFY,
    CALIB_STEP_DONE
} CalibStep;

void      calib_init(void);
void      calib_update(void);
void      calib_start(float target_kg);
void      calib_confirm(void);
void      calib_adjust(int8_t d);
bool      calib_is_active(void);
CalibStep calib_get_step(void);

bool  flash_load(float *scale, int32_t *offset, float *target);
bool  flash_save(float scale, int32_t offset, float target);

#endif /* __CALIBRATION_H */
