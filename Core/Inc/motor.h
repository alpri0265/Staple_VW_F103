#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    MOTOR_IDLE,
    MOTOR_MOVING_UP,
    MOTOR_MOVING_DOWN,
    MOTOR_STOPPING,
    MOTOR_ERROR
} MotorState;

void motor_init(void);
void motor_update(void);
void motor_tim_tick(void);

void motor_move_up(uint16_t speed);
void motor_move_down(uint16_t speed);
void motor_stop(void);
void motor_emergency_stop(void);
void motor_clear_error(void);

bool motor_is_limit_top(void);
bool motor_is_limit_bot(void);
bool motor_is_running(void);

float motor_get_position_mm(void);
void  motor_reset_position(void);
MotorState motor_get_state(void);

#endif /* __MOTOR_H */
