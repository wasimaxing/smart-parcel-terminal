#include <assert.h>
#include <stdio.h>
#include "billing.h"

typedef struct
{
    pp_s32 weight_g;
    pp_u32 expected_cent[REGION_COUNT];
} boundary_case_t;

static const boundary_case_t cases[] =
{
    {0L,    {0UL,    0UL,    0UL}},
    {1L,    {800UL,  1200UL, 1800UL}},
    {1000L, {800UL,  1200UL, 1800UL}},
    {1001L, {1000UL, 1600UL, 2600UL}},
    {2000L, {1000UL, 1600UL, 2600UL}},
    {2001L, {1200UL, 2000UL, 3400UL}}
};

int main(void)
{
    billing_params_t params;
    pp_u32 price;
    unsigned int region;
    unsigned int i;

    for (region = REGION_PROVINCIAL; region <= REGION_REMOTE; ++region)
    {
        assert(billing_region_params((pp_u8)region, &params));
        assert(params.first_weight_g == 1000UL);
        assert(params.step_weight_g == 1000UL);
        for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        {
            assert(billing_calculate(cases[i].weight_g, &params, &price));
            assert(price == cases[i].expected_cent[region - 1U]);
        }
    }
    assert(!billing_region_params(0U, &params));
    assert(!billing_region_params(4U, &params));
    puts("billing region boundary tests passed");
    return 0;
}
