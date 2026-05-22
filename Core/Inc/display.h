#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <stdint.h>

typedef enum {
    SCREEN_MAIN,
    SCREEN_MENU,
    SCREEN_CALIBRATION,
    SCREEN_SETTINGS,
    SCREEN_ERROR
} DisplayScreen;

void display_init(void);
void display_update(void);

void          display_set_screen(DisplayScreen s);
DisplayScreen display_get_screen(void);
void          display_set_force(float current, float target);
void          display_show_error(const char *msg);
void          display_set_calib_text(uint8_t line, const char *text);

void display_menu_next(void);
void display_menu_prev(void);
void display_menu_select(void);
uint8_t display_menu_get_item(void);

#endif /* __DISPLAY_H */
