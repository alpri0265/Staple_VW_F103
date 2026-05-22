#include "calibration.h"
#include "loadcell.h"
#include "display.h"
#include "config.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

#define FLASH_BASE_ADDR 0x08000000UL
#define FLASH_SIZE_REG_ADDR 0x1FFFF7E0UL

#define EEPROM_MAGIC32 0x31574656UL
#define EEPROM_VERSION 1U

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t len;
    uint32_t seq;
    uint32_t crc;
} FlashRecHdr_t;

typedef struct {
    float scale;
    int32_t offset;
    float target_force;
} FlashPayloadV1_t;

static uint32_t crc32_compute(const uint8_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint32_t)data[i];
        for (uint32_t b = 0; b < 8U; b++) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1UL);
            crc = (crc >> 1U) ^ (0xEDB88320UL & mask);
        }
    }
    return ~crc;
}

static size_t flash_record_size(void)
{
    size_t sz = sizeof(FlashRecHdr_t) + sizeof(FlashPayloadV1_t);
    if ((sz & 1U) != 0U) sz++;
    return sz;
}

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
    uint32_t base = flash_eeprom_addr();
    uint32_t end = base + FLASH_PAGE_SIZE;

    bool found = false;
    uint32_t best_seq = 0;
    FlashPayloadV1_t best = {0};

    size_t rec_sz = flash_record_size();
    for (uint32_t addr = base; (addr + rec_sz) <= end; addr += (uint32_t)rec_sz) {
        uint32_t magic = *(const uint32_t *)addr;
        if (magic == 0xFFFFFFFFUL) {
            break;
        }
        if (magic != EEPROM_MAGIC32) {
            break;
        }

        FlashRecHdr_t hdr;
        FlashPayloadV1_t payload;
        memcpy(&hdr, (const void *)addr, sizeof(hdr));
        memcpy(&payload, (const void *)(addr + sizeof(hdr)), sizeof(payload));

        if (hdr.version != EEPROM_VERSION) continue;
        if (hdr.len != (uint16_t)sizeof(FlashPayloadV1_t)) continue;

        uint8_t crc_buf[2 + 2 + 4 + sizeof(FlashPayloadV1_t)];
        memcpy(&crc_buf[0], &hdr.version, 2);
        memcpy(&crc_buf[2], &hdr.len, 2);
        memcpy(&crc_buf[4], &hdr.seq, 4);
        memcpy(&crc_buf[8], &payload, sizeof(payload));

        uint32_t crc = crc32_compute(crc_buf, sizeof(crc_buf));
        if (crc != hdr.crc) continue;

        if (!found || hdr.seq > best_seq) {
            found = true;
            best_seq = hdr.seq;
            best = payload;
        }
    }

    if (found) {
        if (scale)  *scale  = best.scale;
        if (offset) *offset = best.offset;
        if (target) *target = best.target_force;
        return true;
    }

    typedef struct {
        float scale;
        int32_t offset;
        float target_force;
        uint8_t magic;
    } FlashDataOld_t;

    const FlashDataOld_t *oldp = (const FlashDataOld_t *)base;
    if (oldp->magic != EEPROM_MAGIC_VALUE) return false;
    if (scale)  *scale  = oldp->scale;
    if (offset) *offset = oldp->offset;
    if (target) *target = oldp->target_force;
    return true;
}

bool flash_save(float scale, int32_t offset, float target)
{
    uint32_t base = flash_eeprom_addr();
    uint32_t end = base + FLASH_PAGE_SIZE;
    size_t rec_sz = flash_record_size();

    uint32_t write_addr = base;
    uint32_t last_seq = 0;
    bool page_empty = true;

    for (uint32_t addr = base; (addr + rec_sz) <= end; addr += (uint32_t)rec_sz) {
        uint32_t magic = *(const uint32_t *)addr;
        if (magic == 0xFFFFFFFFUL) {
            write_addr = addr;
            break;
        }
        page_empty = false;
        if (magic != EEPROM_MAGIC32) {
            write_addr = addr;
            break;
        }
        FlashRecHdr_t hdr;
        memcpy(&hdr, (const void *)addr, sizeof(hdr));
        if (hdr.version == EEPROM_VERSION && hdr.len == (uint16_t)sizeof(FlashPayloadV1_t)) {
            if (hdr.seq > last_seq) last_seq = hdr.seq;
        }
        write_addr = addr + (uint32_t)rec_sz;
    }

    bool need_erase = false;
    if (write_addr + rec_sz > end) {
        need_erase = true;
        write_addr = base;
    }
    if (page_empty) {
        write_addr = base;
    }

    FlashPayloadV1_t payload = {
        .scale = scale,
        .offset = offset,
        .target_force = target
    };

    FlashRecHdr_t hdr = {
        .magic = EEPROM_MAGIC32,
        .version = EEPROM_VERSION,
        .len = (uint16_t)sizeof(FlashPayloadV1_t),
        .seq = last_seq + 1U,
        .crc = 0U
    };

    uint8_t crc_buf[2 + 2 + 4 + sizeof(FlashPayloadV1_t)];
    memcpy(&crc_buf[0], &hdr.version, 2);
    memcpy(&crc_buf[2], &hdr.len, 2);
    memcpy(&crc_buf[4], &hdr.seq, 4);
    memcpy(&crc_buf[8], &payload, sizeof(payload));
    hdr.crc = crc32_compute(crc_buf, sizeof(crc_buf));

    uint8_t out[sizeof(FlashRecHdr_t) + sizeof(FlashPayloadV1_t)];
    memcpy(&out[0], &hdr, sizeof(hdr));
    memcpy(&out[sizeof(hdr)], &payload, sizeof(payload));

    HAL_FLASH_Unlock();

    if (need_erase) {
        FLASH_EraseInitTypeDef erase = {0};
        erase.TypeErase = FLASH_TYPEERASE_PAGES;
        erase.PageAddress = base;
        erase.NbPages = 1;
        uint32_t page_error = 0;
        if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
    }

    size_t i = 0;
    while (i < sizeof(out)) {
        uint16_t half = out[i];
        half |= (uint16_t)out[i + 1U] << 8;
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, write_addr + i, half) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
        i += 2U;
    }

    HAL_FLASH_Lock();

    FlashRecHdr_t verify_hdr;
    FlashPayloadV1_t verify_payload;
    memcpy(&verify_hdr, (const void *)write_addr, sizeof(verify_hdr));
    memcpy(&verify_payload, (const void *)(write_addr + sizeof(verify_hdr)), sizeof(verify_payload));

    if (verify_hdr.magic != EEPROM_MAGIC32 ||
        verify_hdr.version != EEPROM_VERSION ||
        verify_hdr.len != (uint16_t)sizeof(FlashPayloadV1_t) ||
        verify_hdr.seq != hdr.seq) {
        return false;
    }

    memcpy(&crc_buf[0], &verify_hdr.version, 2);
    memcpy(&crc_buf[2], &verify_hdr.len, 2);
    memcpy(&crc_buf[4], &verify_hdr.seq, 4);
    memcpy(&crc_buf[8], &verify_payload, sizeof(verify_payload));

    return (crc32_compute(crc_buf, sizeof(crc_buf)) == verify_hdr.crc);
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
