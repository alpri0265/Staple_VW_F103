#include "motor.h"
#include "main.h"
#include "config.h"

static volatile MotorState s_state = MOTOR_IDLE;
static volatile int32_t s_step_pos = 0;

static volatile uint16_t s_step_period = 0;
static volatile uint16_t s_step_timer = 0;
static volatile bool s_step_state = false;

static volatile uint16_t s_period_target = 0;
static volatile uint16_t s_period_current = 0;
static volatile uint16_t s_accel_timer = 0;

static void motor_limit_stop_isr(void)
{
    s_step_period = 0;
    s_state = MOTOR_IDLE;
    STEP_GPIO_Port->BSRR = (uint32_t)STEP_Pin << 16U;
    s_step_state = false;
}

static uint16_t speed_to_period(uint16_t speed_steps_per_sec)
{
    if (speed_steps_per_sec == 0) return 0;
    uint32_t p = 1000U / speed_steps_per_sec;
    return (p < 1U) ? 1U : (uint16_t)p;
}

static void set_direction(bool up)
{
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, up ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void motor_init(void)
{
    HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET);

    s_state = MOTOR_IDLE;
    s_step_pos = 0;
    s_step_period = 0;
    s_step_timer = 0;
    s_step_state = false;
    s_period_target = 0;
    s_period_current = 0;
}

void motor_clear_error(void)
{
    __disable_irq();
    if (s_state == MOTOR_ERROR) {
        s_state = MOTOR_IDLE;
    }
    __enable_irq();

    HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET);
}

void motor_move_up(uint16_t speed)
{
    if (s_state == MOTOR_ERROR) return;
    if (motor_is_limit_top()) return;

    set_direction(true);

    uint16_t target = speed_to_period(speed);
    uint16_t start_period = speed_to_period(50);
    if (start_period < target) start_period = target;

    __disable_irq();
    s_period_target = target;
    s_period_current = start_period;
    s_step_period = start_period;
    s_step_timer = start_period;
    s_accel_timer = 5;
    s_state = MOTOR_MOVING_UP;
    __enable_irq();
}

void motor_move_down(uint16_t speed)
{
    if (s_state == MOTOR_ERROR) return;
    if (motor_is_limit_bot()) return;

    set_direction(false);

    uint16_t target = speed_to_period(speed);
    uint16_t start_period = speed_to_period(50);
    if (start_period < target) start_period = target;

    __disable_irq();
    s_period_target = target;
    s_period_current = start_period;
    s_step_period = start_period;
    s_step_timer = start_period;
    s_accel_timer = 5;
    s_state = MOTOR_MOVING_DOWN;
    __enable_irq();
}

void motor_stop(void)
{
    __disable_irq();
    s_step_period = 0;
    s_state = MOTOR_IDLE;
    STEP_GPIO_Port->BSRR = (uint32_t)STEP_Pin << 16U;
    s_step_state = false;
    __enable_irq();
}

void motor_emergency_stop(void)
{
    s_step_period = 0;
    s_state = MOTOR_ERROR;
    STEP_GPIO_Port->BSRR = (uint32_t)STEP_Pin << 16U;
    s_step_state = false;

    HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_SET);
}

void motor_tim_tick(void)
{
    if (s_step_period == 0) return;

    if (s_state == MOTOR_MOVING_UP && motor_is_limit_top()) {
        motor_limit_stop_isr();
        return;
    }
    if (s_state == MOTOR_MOVING_DOWN && motor_is_limit_bot()) {
        motor_limit_stop_isr();
        return;
    }

    if (s_accel_timer > 0) {
        s_accel_timer--;
    } else {
        s_accel_timer = 5;
        if (s_period_current > s_period_target) {
            s_period_current--;
            s_step_period = s_period_current;
        }
    }

    if (s_step_timer > 0) {
        s_step_timer--;
    } else {
        s_step_timer = s_step_period;

        if (!s_step_state) {
            STEP_GPIO_Port->BSRR = STEP_Pin;
            s_step_state = true;

            if (s_state == MOTOR_MOVING_UP) {
                s_step_pos++;
            } else if (s_state == MOTOR_MOVING_DOWN) {
                s_step_pos--;
            }
        } else {
            STEP_GPIO_Port->BSRR = (uint32_t)STEP_Pin << 16U;
            s_step_state = false;
        }
    }
}

void motor_update(void)
{
    if (s_state == MOTOR_ERROR) return;

    if (s_state == MOTOR_MOVING_UP && motor_is_limit_top()) {
        motor_stop();
    }
    if (s_state == MOTOR_MOVING_DOWN && motor_is_limit_bot()) {
        motor_stop();
    }
}

bool motor_is_limit_top(void)
{
    return HAL_GPIO_ReadPin(LIMIT_TOP_GPIO_Port, LIMIT_TOP_Pin) == GPIO_PIN_RESET;
}

bool motor_is_limit_bot(void)
{
    return HAL_GPIO_ReadPin(LIMIT_BOT_GPIO_Port, LIMIT_BOT_Pin) == GPIO_PIN_RESET;
}

bool motor_is_running(void)
{
    return (s_state == MOTOR_MOVING_UP || s_state == MOTOR_MOVING_DOWN);
}

float motor_get_position_mm(void)
{
    int32_t pos;
    __disable_irq();
    pos = s_step_pos;
    __enable_irq();
    return (float)pos / STEPS_PER_MM;
}

void motor_reset_position(void)
{
    __disable_irq();
    s_step_pos = 0;
    __enable_irq();
}

MotorState motor_get_state(void)
{
    return s_state;
}
