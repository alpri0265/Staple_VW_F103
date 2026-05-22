#include "display.h"
#include "main.h"
#include "i2c.h"
#include "config.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define LCD_RS        0x01
#define LCD_RW        0x02
#define LCD_EN        0x04
#define LCD_BACKLIGHT 0x08

#define LCD_COLS 20
#define LCD_ROWS 4

static const uint8_t ROW_ADDR[4] = { 0x00, 0x40, 0x14, 0x54 };

static DisplayScreen s_screen = SCREEN_MAIN;
static float s_force = 0.0f;
static float s_target = FORCE_DEFAULT_KG;
static bool s_dirty = true;
static uint8_t s_menu_item = 0;

static char s_lcd_buf[LCD_ROWS][LCD_COLS + 1];
static char s_new_buf[LCD_ROWS][LCD_COLS + 1];
static char s_calib_lines[LCD_ROWS][LCD_COLS + 1];
static char s_error_msg[LCD_COLS + 1];

static void pcf_write(uint8_t byte)
{
    HAL_I2C_Master_Transmit(&hi2c1, LCD_I2C_ADDR, &byte, 1, 5);
}

static void lcd_nibble(uint8_t nibble, uint8_t flags)
{
    uint8_t b = (nibble & 0xF0) | flags | LCD_BACKLIGHT;
    pcf_write(b | LCD_EN);
    pcf_write(b);
}

static void lcd_byte(uint8_t data, uint8_t flags)
{
    lcd_nibble(data & 0xF0, flags);
    lcd_nibble((data << 4) & 0xF0, flags);
}

static void lcd_cmd(uint8_t cmd)
{
    lcd_byte(cmd, 0);
}

static void lcd_data(uint8_t ch)
{
    lcd_byte(ch, LCD_RS);
}

static void lcd_set_cursor(uint8_t col, uint8_t row)
{
    lcd_cmd(0x80 | (ROW_ADDR[row & 3U] + col));
}

static void lcd_print(const char *str)
{
    while (*str) {
        lcd_data((uint8_t)*str++);
    }
}

static void lcd_hw_init(void)
{
    HAL_Delay(50);

    lcd_nibble(0x30, 0); HAL_Delay(5);
    lcd_nibble(0x30, 0); HAL_Delay(2);
    lcd_nibble(0x30, 0); HAL_Delay(2);
    lcd_nibble(0x20, 0); HAL_Delay(2);

    lcd_cmd(0x28); HAL_Delay(1);
    lcd_cmd(0x0C); HAL_Delay(1);
    lcd_cmd(0x01); HAL_Delay(2);
    lcd_cmd(0x06); HAL_Delay(1);
}

static void make_line_padded(char *dst, const char *src)
{
    int len = (int)strlen(src);
    if (len > LCD_COLS) len = LCD_COLS;
    memcpy(dst, src, (size_t)len);
    for (int i = len; i < LCD_COLS; i++) dst[i] = ' ';
    dst[LCD_COLS] = '\0';
}

static void build_screen_main(void)
{
    char tmp[LCD_COLS + 1];

    make_line_padded(s_new_buf[0], "VW NF PRESS");

    snprintf(tmp, sizeof(tmp), "Force: %6.1f kg", (double)s_force);
    make_line_padded(s_new_buf[1], tmp);

    snprintf(tmp, sizeof(tmp), "Target:%6.1f kg", (double)s_target);
    make_line_padded(s_new_buf[2], tmp);

    make_line_padded(s_new_buf[3], "[^v] ENC    STOP");
}

#define MENU_ITEMS 4
static const char *MENU_LABELS[MENU_ITEMS] = {
    "1.Work mode",
    "2.Calibration",
    "3.Settings",
    "4.Home (zero)"
};

static void build_screen_menu(void)
{
    make_line_padded(s_new_buf[0], "=== MENU ===");

    for (int i = 0; i < 3; i++) {
        uint8_t item = (s_menu_item <= 1U) ? (uint8_t)i : (uint8_t)(s_menu_item - 1U + (uint8_t)i);
        if (item >= MENU_ITEMS) {
            make_line_padded(s_new_buf[i + 1], "");
        } else {
            char tmp[LCD_COLS + 1];
            snprintf(tmp, sizeof(tmp), "%c%s", (item == s_menu_item) ? '>' : ' ', MENU_LABELS[item]);
            make_line_padded(s_new_buf[i + 1], tmp);
        }
    }
}

static void build_screen_calib(void)
{
    for (int i = 0; i < LCD_ROWS; i++) {
        make_line_padded(s_new_buf[i], s_calib_lines[i]);
    }
}

static void build_screen_settings(void)
{
    make_line_padded(s_new_buf[0], "=== SETTINGS ===");
    make_line_padded(s_new_buf[1], "Not yet available");
    make_line_padded(s_new_buf[2], "");
    make_line_padded(s_new_buf[3], "BTN=back");
}

static void build_screen_error(void)
{
    make_line_padded(s_new_buf[0], "!!! ERROR !!!");
    make_line_padded(s_new_buf[1], s_error_msg);
    make_line_padded(s_new_buf[2], "");
    make_line_padded(s_new_buf[3], "Press STOP to reset");
}

static void flush_screen(void)
{
    for (int row = 0; row < LCD_ROWS; row++) {
        if (memcmp(s_new_buf[row], s_lcd_buf[row], LCD_COLS) != 0) {
            lcd_set_cursor(0, (uint8_t)row);
            lcd_print(s_new_buf[row]);
            memcpy(s_lcd_buf[row], s_new_buf[row], LCD_COLS + 1);
        }
    }
}

void display_init(void)
{
    memset(s_lcd_buf, 0, sizeof(s_lcd_buf));
    memset(s_new_buf, 0, sizeof(s_new_buf));
    memset(s_calib_lines, 0, sizeof(s_calib_lines));
    memset(s_error_msg, 0, sizeof(s_error_msg));

    lcd_hw_init();
    s_dirty = true;
}

void display_update(void)
{
    if (!s_dirty) return;
    s_dirty = false;

    switch (s_screen) {
        case SCREEN_MAIN:         build_screen_main();     break;
        case SCREEN_MENU:         build_screen_menu();     break;
        case SCREEN_CALIBRATION:  build_screen_calib();    break;
        case SCREEN_SETTINGS:     build_screen_settings(); break;
        case SCREEN_ERROR:        build_screen_error();    break;
        default:                  build_screen_main();     break;
    }

    flush_screen();
}

DisplayScreen display_get_screen(void)
{
    return s_screen;
}

void display_set_screen(DisplayScreen s)
{
    if (s_screen != s) {
        s_screen = s;
        memset(s_lcd_buf, 0, sizeof(s_lcd_buf));
        s_dirty = true;
    }
}

void display_set_force(float current, float target)
{
    if (s_force != current || s_target != target) {
        s_force = current;
        s_target = target;
        if (s_screen == SCREEN_MAIN) s_dirty = true;
    }
}

void display_show_error(const char *msg)
{
    strncpy(s_error_msg, msg, LCD_COLS);
    s_error_msg[LCD_COLS] = '\0';
    display_set_screen(SCREEN_ERROR);
    s_dirty = true;
}

void display_set_calib_text(uint8_t line, const char *text)
{
    if (line >= LCD_ROWS) return;
    strncpy(s_calib_lines[line], text, LCD_COLS);
    s_calib_lines[line][LCD_COLS] = '\0';
    if (s_screen == SCREEN_CALIBRATION) s_dirty = true;
}

void display_menu_next(void)
{
    if (s_menu_item < (MENU_ITEMS - 1U)) {
        s_menu_item++;
        if (s_screen == SCREEN_MENU) s_dirty = true;
    }
}

void display_menu_prev(void)
{
    if (s_menu_item > 0U) {
        s_menu_item--;
        if (s_screen == SCREEN_MENU) s_dirty = true;
    }
}

void display_menu_select(void)
{
    (void)0;
}

uint8_t display_menu_get_item(void)
{
    return s_menu_item;
}
