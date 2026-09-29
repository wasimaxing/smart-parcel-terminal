#include "weight_service.h"

static pp_s32 raw_to_g(const weight_service_t *service, pp_s32 raw)
{
    pp_s32 delta;
    pp_s32 value;

    if (!service->calibrated || service->span_raw == 0L)
        return 0L;
    delta = raw - service->zero_raw;
    if (service->raw_count_mode && delta < 0L)
        delta = -delta;
    value = (delta * (pp_s32)service->span_mass_g) / service->span_raw;
    return value;
}

void weight_init(weight_service_t *service, pp_s32 initial_raw)
{
    pp_u8 i;
    service->zero_raw = initial_raw;
    service->span_raw = 0L;
    service->span_mass_g = 0UL;
    service->tare_g = 0L;
    service->filtered_g = 0L;
    service->sample_count = 0U;
    service->sample_index = 0U;
    service->stable_windows = 0U;
    service->calibrated = 0U;
    service->stable = 0U;
    service->raw_count_mode = 0U;
    for (i = 0U; i < STABLE_WINDOW_SIZE; ++i)
        service->samples[i] = 0L;
}

void weight_use_raw_counts(weight_service_t *service)
{
    service->span_raw = 1L;
    service->span_mass_g = 1UL;
    service->calibrated = 1U;
    service->raw_count_mode = 1U;
    service->sample_count = 0U;
    service->stable_windows = 0U;
    service->stable = 0U;
}

void weight_zero(weight_service_t *service, pp_s32 raw)
{
    service->zero_raw = raw;
    service->tare_g = 0L;
    service->sample_count = 0U;
    service->stable_windows = 0U;
    service->stable = 0U;
}

pp_u8 weight_calibrate(weight_service_t *service, pp_s32 raw, pp_u32 known_mass_g)
{
    pp_s32 span;
    if (known_mass_g == 0UL || known_mass_g > (pp_u32)MAX_WEIGHT_G)
        return 0U;
    span = raw - service->zero_raw;
    if (span > -10L && span < 10L)
        return 0U;
    service->span_raw = span;
    service->span_mass_g = known_mass_g;
    service->calibrated = 1U;
    service->raw_count_mode = 0U;
    service->sample_count = 0U;
    return 1U;
}

void weight_tare(weight_service_t *service)
{
    service->tare_g += service->filtered_g;
    service->sample_count = 0U;
    service->stable_windows = 0U;
    service->stable = 0U;
}

void weight_push_raw(weight_service_t *service, pp_s32 raw)
{
    pp_s32 gross;
    pp_s32 sum;
    pp_s32 min_value;
    pp_s32 max_value;
    pp_u8 i;

    if (!service->calibrated)
    {
        service->filtered_g = 0L;
        service->stable = 0U;
        return;
    }

    gross = raw_to_g(service, raw) - service->tare_g;
    if (gross < 0L && gross > -5L)
        gross = 0L;
    service->samples[service->sample_index] = gross;
    service->sample_index = (pp_u8)((service->sample_index + 1U) % STABLE_WINDOW_SIZE);
    if (service->sample_count < STABLE_WINDOW_SIZE)
        ++service->sample_count;

    sum = 0L;
    min_value = service->samples[0];
    max_value = service->samples[0];
    for (i = 0U; i < service->sample_count; ++i)
    {
        sum += service->samples[i];
        if (service->samples[i] < min_value)
            min_value = service->samples[i];
        if (service->samples[i] > max_value)
            max_value = service->samples[i];
    }
    service->filtered_g = sum / service->sample_count;

    if (service->sample_count == STABLE_WINDOW_SIZE &&
        (max_value - min_value) <= STABLE_RANGE_G)
    {
        if (service->stable_windows < STABLE_REQUIRED_WINDOWS)
            ++service->stable_windows;
    }
    else
    {
        service->stable_windows = 0U;
    }
    service->stable = (service->stable_windows >= STABLE_REQUIRED_WINDOWS) ? 1U : 0U;
}
