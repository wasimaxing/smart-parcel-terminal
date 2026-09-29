#ifndef BILLING_H
#define BILLING_H

#include "parcel_protocol.h"

typedef struct
{
    pp_u32 first_weight_g;
    pp_u32 first_price_cent;
    pp_u32 step_weight_g;
    pp_u32 step_price_cent;
} billing_params_t;

#define REGION_PROVINCIAL  1U
#define REGION_OUTSIDE     2U
#define REGION_REMOTE      3U
#define REGION_DEFAULT     REGION_PROVINCIAL
#define REGION_COUNT       3U

typedef struct
{
    pp_u8 region_id;
    billing_params_t params;
} region_tariff_t;

pp_u8 billing_region_params(pp_u8 region_id, billing_params_t *params);

pp_u8 billing_params_valid(const billing_params_t *params);
pp_u8 billing_calculate(pp_s32 weight_g, const billing_params_t *params,
                        pp_u32 *price_cent);

#endif
