#include "loadcell.h"
#include "main.h"
#include "config.h"

#define AVG_SAMPLES 8

static float s_scale = 1.0f;
static int32_t s_offset = 0;
static float s_force_kg = 0.0f;

static int32_t s_avg_buf[AVG_SAMPLES];
static uint8_t s_avg_idx = 0;
static int32_t s_avg_sum = 0;
static uint8_t s_avg_count = 0;

static uint32_t s_last_tick = 0;
static bool s_has_error = false;

static inline void hx_clk_hi(void)
{
    HX_CLK_GPIO_Port->BSRR = HX_CLK_Pin;
}

static inline void hx_clk_lo(void)
{
    HX_CLK_GPIO_Port->BSRR = (uint32_t)HX_CLK_Pin << 16U;
}

static inline bool hx_dat_lo(void)
{
    return (HX_DAT_GPIO_Port->IDR & HX_DAT_Pin) == 0U;
}

static inline void hx_delay(void)
{
    for (volatile uint32_t i = 0; i < 16U; i++) {
        __NOP();
    }
}

static int32_t hx711_read(void)
{
    int32_t data = 0;

    for (int i = 0; i < 24; i++) {
        hx_clk_hi();
        hx_delay();
        data = (data << 1);
        if ((HX_DAT_GPIO_Port->IDR & HX_DAT_Pin) != 0U) {
            data |= 1;
        }
        hx_clk_lo();
        hx_delay();
    }

    hx_clk_hi();
    hx_delay();
    hx_clk_lo();
    hx_delay();

    if (data & 0x800000) {
        data |= (int32_t)0xFF000000;
    }

    return data;
}

void loadcell_init(void)
{
    hx_clk_lo();
    s_scale = 1.0f;
    s_offset = 0;
    s_avg_idx = 0;
    s_avg_sum = 0;
    s_avg_count = 0;
    s_has_error = false;
    s_last_tick = HAL_GetTick();
}

bool loadcell_is_ready(void)
{
    return hx_dat_lo();
}

void loadcell_update(void)
{
    if (!hx_dat_lo()) {
        if ((HAL_GetTick() - s_last_tick) > HX711_TIMEOUT_MS) {
            s_has_error = true;
        }
        return;
    }

    s_has_error = false;
    s_last_tick = HAL_GetTick();

    int32_t raw = hx711_read();

    s_avg_sum -= s_avg_buf[s_avg_idx];
    s_avg_buf[s_avg_idx] = raw;
    s_avg_sum += raw;
    s_avg_idx = (s_avg_idx + 1U) % AVG_SAMPLES;
    if (s_avg_count < AVG_SAMPLES) s_avg_count++;

    int32_t avg = s_avg_sum / (int32_t)s_avg_count;

    if (s_scale != 0.0f) {
        s_force_kg = (float)(avg - s_offset) / s_scale;
    } else {
        s_force_kg = 0.0f;
    }
}

float loadcell_get_kg(void)
{
    return s_force_kg;
}

bool loadcell_has_error(void)
{
    return s_has_error;
}

void loadcell_tare(void)
{
    if (s_avg_count > 0U) {
        s_offset = s_avg_sum / (int32_t)s_avg_count;
    }
}

void loadcell_set_scale(float scale)
{
    s_scale = scale;
}

float loadcell_get_scale(void)
{
    return s_scale;
}

int32_t loadcell_get_offset(void)
{
    return s_offset;
}

void loadcell_set_offset(int32_t offset)
{
    s_offset = offset;
}

int32_t loadcell_read_raw(void)
{
    if (s_avg_count > 0U) {
        return s_avg_sum / (int32_t)s_avg_count;
    }
    return 0;
}

