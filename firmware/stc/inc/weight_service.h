#ifndef WEIGHT_SERVICE_H
#define WEIGHT_SERVICE_H

#include "parcel_protocol.h"
#include "project_config.h"

typedef struct
{
    pp_s32 zero_raw;
    pp_s32 span_raw;
    pp_u32 span_mass_g;
    pp_s32 tare_g;
    pp_s32 samples[STABLE_WINDOW_SIZE];
    pp_s32 filtered_g;
    pp_u8 sample_count;
    pp_u8 sample_index;
    pp_u8 stable_windows;
    pp_u8 calibrated;
    pp_u8 stable;
    pp_u8 raw_count_mode;
} weight_service_t;

void weight_init(weight_service_t *service, pp_s32 initial_raw);
void weight_use_raw_counts(weight_service_t *service);
void weight_zero(weight_service_t *service, pp_s32 raw);
pp_u8 weight_calibrate(weight_service_t *service, pp_s32 raw, pp_u32 known_mass_g);
void weight_tare(weight_service_t *service);
void weight_push_raw(weight_service_t *service, pp_s32 raw);

#endif
