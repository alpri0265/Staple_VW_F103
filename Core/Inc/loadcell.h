#ifndef __LOADCELL_H
#define __LOADCELL_H

#include <stdbool.h>
#include <stdint.h>

void    loadcell_init(void);
void    loadcell_update(void);
float   loadcell_get_kg(void);
bool    loadcell_is_ready(void);
bool    loadcell_has_error(void);
void    loadcell_tare(void);
void    loadcell_set_scale(float s);
float   loadcell_get_scale(void);
int32_t loadcell_get_offset(void);
void    loadcell_set_offset(int32_t offset);
int32_t loadcell_read_raw(void);

#endif /* __LOADCELL_H */
