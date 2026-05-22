#include "input.h"
#include "main.h"
#include "config.h"

typedef struct {
    uint8_t counter;
    bool state;
    bool prev;
} DebounceBtn_t;

static DebounceBtn_t s_joy_up;
static DebounceBtn_t s_joy_down;
static DebounceBtn_t s_enc_sw;

static volatile int8_t s_enc_delta = 0;
static volatile bool s_stop_flag = false;

static void debounce_tick(DebounceBtn_t *btn, bool raw_pressed)
{
    if (raw_pressed) {
        if (btn->counter < DEBOUNCE_MS) {
            btn->counter++;
        }
    } else {
        btn->counter = 0;
    }
    btn->prev = btn->state;
    btn->state = (btn->counter >= DEBOUNCE_MS);
}

void input_init(void)
{
    s_joy_up = (DebounceBtn_t){0};
    s_joy_down = (DebounceBtn_t){0};
    s_enc_sw = (DebounceBtn_t){0};
    s_enc_delta = 0;
    s_stop_flag = false;
}

void input_debounce_tick(void)
{
    debounce_tick(&s_joy_up, HAL_GPIO_ReadPin(JOY_UP_GPIO_Port, JOY_UP_Pin) == GPIO_PIN_RESET);
    debounce_tick(&s_joy_down, HAL_GPIO_ReadPin(JOY_DOWN_GPIO_Port, JOY_DOWN_Pin) == GPIO_PIN_RESET);
    debounce_tick(&s_enc_sw, HAL_GPIO_ReadPin(ENC_SW_GPIO_Port, ENC_SW_Pin) == GPIO_PIN_RESET);
}

void input_enc_isr(void)
{
    if (HAL_GPIO_ReadPin(ENC_DT_GPIO_Port, ENC_DT_Pin) == GPIO_PIN_SET) {
        if (s_enc_delta < 127) s_enc_delta++;
    } else {
        if (s_enc_delta > -127) s_enc_delta--;
    }
}

void input_update(void)
{
    (void)0;
}

bool input_joy_up(void)
{
    return s_joy_up.state;
}

bool input_joy_down(void)
{
    return s_joy_down.state;
}

bool input_enc_sw_pressed(void)
{
    if (s_enc_sw.state && !s_enc_sw.prev) {
        return true;
    }
    return false;
}

int8_t input_enc_get_delta(void)
{
    int8_t delta;
    __disable_irq();
    delta = s_enc_delta;
    s_enc_delta = 0;
    __enable_irq();
    return delta;
}

bool input_stop_pressed(void)
{
    return s_stop_flag;
}

void input_stop_clear(void)
{
    s_stop_flag = false;
}

void input_stop_set(void)
{
    s_stop_flag = true;
}
