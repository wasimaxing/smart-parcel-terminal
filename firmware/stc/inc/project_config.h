#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

/* 1: send an obvious fixed value while checking the RS-485/DWIN link.
 * 0: use calibrated EXT weight. Change to 0 only after zero/span calibration.
 */
#define LINK_TEST_MODE              0

/*
 * 1: imitate the supplied legacy scale HEX: use abs(GetWeight()-zero)
 *    directly as the displayed gram value, without a known-mass span step.
 * 0: require the normal two-point calibration before reporting grams.
 * Raw-count mode is convenient for bring-up, but its accuracy is hardware-dependent.
 */
#define WEIGHT_RAW_COUNT_MODE       1

#define RS485_BAUD                  9600UL
#define STATUS_PERIOD_10MS          50U
#define WEIGHT_SAMPLE_PERIOD_10MS   2U

#define DEFAULT_FIRST_WEIGHT_G      1000UL
#define DEFAULT_FIRST_PRICE_CENT    1200UL
#define DEFAULT_STEP_WEIGHT_G       1000UL
#define DEFAULT_STEP_PRICE_CENT     200UL

#define STABLE_WINDOW_SIZE          8U
#define STABLE_RANGE_G              5L
#define STABLE_REQUIRED_WINDOWS     5U
#define MAX_WEIGHT_G                30000L

#endif
