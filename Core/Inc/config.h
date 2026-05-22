#ifndef __CONFIG_H
#define __CONFIG_H

#define SCREW_PITCH_MM      4.0f
#define GEAR_RATIO          10.0f
#define MICROSTEP           8
#define MOTOR_STEPS_REV     200

#define STEPS_PER_MM        4000.0f

#define SPEED_FAST          1000
#define SPEED_SLOW          400
#define SPEED_ENC           200
#define ACCEL_STEPS         2000

#define FORCE_MAX_KG        500.0f
#define FORCE_DEFAULT_KG    15.0f
#define FORCE_STEP_KG       0.1f
#define SLOWDOWN_THRESHOLD  0.80f
#define OVERLOAD_FACTOR     1.10f

#define EEPROM_MAGIC_VALUE  0xAB

#define HX711_TIMEOUT_MS    500

#define LCD_I2C_ADDR        (0x27 << 1)

#define DEBOUNCE_MS         20

#endif /* __CONFIG_H */
