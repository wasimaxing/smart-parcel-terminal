#ifndef __TIMER_H__
#define __TIMER_H__

#include "sys.h"


// 软定时器
#define TIMER_NUMBER   8  
#define SOFT_TIMER_RTC  0
#define SOFT_TIMER_RTC_TIMEOUT  250
#define SOFT_TIMER_1  1
#define SOFT_TIMER_1_TIMEOUT  50
#define SOFT_TIMER_2  2
#define SOFT_TIMER_2_TIMEOUT  1000
#define SOFT_TIMER_3  3
#define SOFT_TIMER_3_TIMEOUT  2000
#define SOFT_TIMER_PRINTF  7
#define SOFT_TIMER_PRINTF_TIMEOUT  5000

extern u8 data SysTick;
extern u16 data T_number_arr[TIMER_NUMBER];
extern u8 data T_number_en[TIMER_NUMBER];


void T0_Init(void);
void delay_timer_ms(u16 delay_time);
void delay_ms(u16 delay_time);
void delay_us(u16 delay_time);
void start_timer(u8 ID, u16 nTime);
void stop_timer(u8 ID);
u8 get_time_out_flag(u8 ID);

#endif