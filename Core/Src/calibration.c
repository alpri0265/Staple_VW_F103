#include "calibration.h"
#include "loadcell.h"
#include "display.h"
#include "config.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

#define FLASH_BASE_ADDR 0x08000000UL
#define FLASH_SIZE_REG_ADDR 0x1FFFF7E0UL

#pragma pack(1)
typedef struct {
    float scale;
    int32_t offset;
    float target_force;
    uint8_t magic;
} FlashData_t;
#pragma pack()

static uint32_t flash_eeprom_addr(void)
{
    uint32_t flash_kb = *(const uint16_t *)FLASH_SIZE_REG_ADDR;
    if (flash_kb == 0U || flash_kb > 1024U) {
        flash_kb = 64U;
    }
    uint32_t flash_end = FLASH_BASE_ADDR + flash_kb * 1024UL;
    return flash_end - FLASH_PAGE_SIZE;
}

bool flash_load(float *scale, int32_t *offset, float *target)
{
    uint32_t addr = flash_eeprom_addr();
    const FlashData_t *p = (const FlashData_t *)addr;
    if (p->magic != EEPROM_MAGIC_VALUE) return false;
    if (scale)  *scale  = p->scale;
    if (offset) *offset = p->offset;
    if (target) *target = p->target_force;
    return true;
}

bool flash_save(float scale, int32_t offset, float target)
{
    uint32_t addr = flash_eeprom_addr();

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = addr;
    erase.NbPages = 1;

    uint32_t page_error = 0;
    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        HAL_FLASH_Lock();
        return false;
    }

    FlashData_t data = {
        .scale = scale,
        .offset = offset,
        .target_force = target,
        .magic = EEPROM_MAGIC_VALUE
    };

    const uint8_t *src = (const uint8_t *)&data;
    size_t len = sizeof(FlashData_t);
    size_t i = 0;
    while (i < len) {
        uint16_t half = src[i];
        if ((i + 1U) < len) {
            half |= (uint16_t)src[i + 1U] << 8;
        } else {
            half |= 0xFF00U;
        }
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr + i, half) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
        i += 2U;
    }

    HAL_FLASH_Lock();
    return true;
}

static CalibStep s_step = CALIB_STEP_IDLE;
static float s_known_kg = 1.0f;
static int32_t s_raw_tare = 0;
static int32_t s_raw_load = 0;
static float s_target_for_save = FORCE_DEFAULT_KG;

static void update_display(void)
{
    char line[21];

    switch (s_step) {
        case CALIB_STEP_TARE:
            display_set_calib_text(0, "=== CALIBRATION ===");
            display_set_calib_text(1, "Remove all weight");
            display_set_calib_text(2, "then press ENC BTN");
            display_set_calib_text(3, "");
            break;

        case CALIB_STEP_LOAD:
            display_set_calib_text(0, "=== CALIBRATION ===");
            display_set_calib_text(1, "Place known weight");
            display_set_calib_text(2, "then press ENC BTN");
            display_set_calib_text(3, "");
            break;

        case CALIB_STEP_INPUT_MASS:
            snprintf(line, sizeof(line), "Mass: %.2f kg", (double)s_known_kg);
            display_set_calib_text(0, "=== CALIBRATION ===");
            display_set_calib_text(1, "Set weight mass:");
            display_set_calib_text(2, line);
            display_set_calib_text(3, "ENC=adjust  BTN=ok");
            break;

        case CALIB_STEP_CALCULATE:
        case CALIB_STEP_SAVE:
            display_set_calib_text(0, "=== CALIBRATION ===");
            display_set_calib_text(1, "Saving...");
            display_set_calib_text(2, "");
            display_set_calib_text(3, "");
            break;

        case CALIB_STEP_VERIFY: {
            float sc = loadcell_get_scale();
            snprintf(line, sizeof(line), "Scale: %.1f", (double)sc);
            display_set_calib_text(0, "Calibration done!");
            display_set_calib_text(1, line);
            snprintf(line, sizeof(line), "Force: %.2f kg", (double)loadcell_get_kg());
            display_set_calib_text(2, line);
            display_set_calib_text(3, "BTN=exit");
            break;
        }

        default:
            break;
    }
}

void calib_init(void)
{
    s_step = CALIB_STEP_IDLE;
    s_known_kg = 1.0f;
}

void calib_start(float target_kg)
{
    s_step = CALIB_STEP_TARE;
    s_known_kg = 1.0f;
    s_target_for_save = target_kg;
    display_set_screen(SCREEN_CALIBRATION);
    update_display();
}

bool calib_is_active(void)
{
    return (s_step != CALIB_STEP_IDLE && s_step != CALIB_STEP_DONE);
}

CalibStep calib_get_step(void)
{
    return s_step;
}

void calib_confirm(void)
{
    switch (s_step) {
        case CALIB_STEP_TARE:
            loadcell_tare();
            s_raw_tare = loadcell_read_raw();
            s_step = CALIB_STEP_LOAD;
            update_display();
            break;

        case CALIB_STEP_LOAD:
            s_raw_load = loadcell_read_raw();
            s_step = CALIB_STEP_INPUT_MASS;
            update_display();
            break;

        case CALIB_STEP_INPUT_MASS:
            s_step = CALIB_STEP_CALCULATE;
            break;

        case CALIB_STEP_VERIFY:
            s_step = CALIB_STEP_DONE;
            display_set_screen(SCREEN_MAIN);
            break;

        default:
            break;
    }
}

void calib_adjust(int8_t delta)
{
    if (s_step == CALIB_STEP_INPUT_MASS) {
        s_known_kg += (float)delta * 0.1f;
        if (s_known_kg < 0.1f) s_known_kg = 0.1f;
        if (s_known_kg > 50.0f) s_known_kg = 50.0f;
        update_display();
    }
}

void calib_update(void)
{
    switch (s_step) {
        case CALIB_STEP_CALCULATE: {
            int32_t delta = s_raw_load - s_raw_tare;
            if (delta != 0 && s_known_kg > 0.0f) {
                float new_scale = (float)delta / s_known_kg;
                loadcell_set_scale(new_scale);
                loadcell_set_offset(s_raw_tare);
            }
            s_step = CALIB_STEP_SAVE;
            update_display();
            break;
        }

        case CALIB_STEP_SAVE: {
            flash_save(loadcell_get_scale(), loadcell_get_offset(), s_target_for_save);
            s_step = CALIB_STEP_VERIFY;
            update_display();
            break;
        }

        default:
            break;
    }
}
