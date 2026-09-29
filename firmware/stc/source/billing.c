#include "billing.h"

#ifdef __C51__
code region_tariff_t g_region_tariffs[REGION_COUNT] =
#else
const region_tariff_t g_region_tariffs[REGION_COUNT] =
#endif
{
    {REGION_PROVINCIAL, {1000UL,  800UL, 1000UL, 200UL}},
    {REGION_OUTSIDE,    {1000UL, 1200UL, 1000UL, 400UL}},
    {REGION_REMOTE,     {1000UL, 1800UL, 1000UL, 800UL}}
};

pp_u8 billing_region_params(pp_u8 region_id, billing_params_t *params)
{
    pp_u8 i;
    if (params == 0)
        return 0U;
    for (i = 0U; i < REGION_COUNT; ++i)
    {
        if (g_region_tariffs[i].region_id == region_id)
        {
            params->first_weight_g = g_region_tariffs[i].params.first_weight_g;
            params->first_price_cent = g_region_tariffs[i].params.first_price_cent;
            params->step_weight_g = g_region_tariffs[i].params.step_weight_g;
            params->step_price_cent = g_region_tariffs[i].params.step_price_cent;
            return 1U;
        }
    }
    return 0U;
}

pp_u8 billing_params_valid(const billing_params_t *params)
{
    if (params == 0)
        return 0U;
    if (params->first_weight_g == 0UL || params->step_weight_g == 0UL)
        return 0U;
    if (params->first_weight_g > 100000UL || params->step_weight_g > 100000UL)
        return 0U;
    if (params->first_price_cent > 1000000UL || params->step_price_cent > 1000000UL)
        return 0U;
    return 1U;
}

pp_u8 billing_calculate(pp_s32 weight_g, const billing_params_t *params,
                        pp_u32 *price_cent)
{
    pp_u32 weight;
    pp_u32 excess;
    pp_u32 steps;

    if (price_cent == 0 || !billing_params_valid(params))
        return 0U;
    if (weight_g <= 0L)
    {
        *price_cent = 0UL;
        return 1U;
    }

    weight = (pp_u32)weight_g;
    if (weight <= params->first_weight_g)
    {
        *price_cent = params->first_price_cent;
        return 1U;
    }

    excess = weight - params->first_weight_g;
    steps = excess / params->step_weight_g;
    if ((excess % params->step_weight_g) != 0UL)
        ++steps;

    if (params->step_price_cent != 0UL &&
        steps > (0xFFFFFFFFUL - params->first_price_cent) / params->step_price_cent)
        return 0U;

    *price_cent = params->first_price_cent + steps * params->step_price_cent;
    return 1U;
}
