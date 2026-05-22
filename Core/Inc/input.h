#ifndef __INPUT_H
#define __INPUT_H

#include <stdbool.h>
#include <stdint.h>

void   input_init(void);
void   input_update(void);
void   input_debounce_tick(void);
void   input_enc_isr(void);

bool   input_joy_up(void);
bool   input_joy_down(void);
bool   input_enc_sw_pressed(void);
int8_t input_enc_get_delta(void);

bool   input_stop_pressed(void);
void   input_stop_clear(void);
void   input_stop_set(void);

#endif /* __INPUT_H */
