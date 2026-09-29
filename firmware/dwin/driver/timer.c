#include "timer.h"

u8 data SysTick=0;
u16 data T_number_arr[TIMER_NUMBER]={0};
u8 data T_number_en[TIMER_NUMBER]={0};

// 定时器0初始化
void T0_Init(void)
{
	TMOD |= 0x01;
	TH0 = T1MS>>8;        //1ms定时器
	TL0 = T1MS;
	ET0 = 1;              //开启定时器0中断
	TR0 = 1;              //开启定时器
	EA = 1;               //开总中断
}

// 启动一个软定时器,定时单位ms
void start_timer(u8 ID, u16 nTime)
{
    T_number_en[ID]= 1;   //定时器使能
    T_number_arr[ID]= nTime; 
}

// 停止一个软定时器
void stop_timer(u8 ID)
{
    T_number_en[ID]= 0;   //定时器关闭
}

// 获取定时器超时标志
u8 get_time_out_flag(u8 ID)
{
	if(T_number_en[ID])
	{
		if(T_number_arr[ID] == 0)
		{
			stop_timer(ID);
			return 1;
		}
		else
        {
            return 0;
        }
	}
	return 0;	
}

// 定时器延时函数,单位ms
void delay_timer_ms(u16 delay_time)
{
	SysTick = delay_time;
	while(SysTick);
}

// for循环实现延时函数,单位ms
void delay_ms(u16 delay_time)
{
    int data i,j;
    for(i = 0; i < delay_time; i++)
        for(j = 0 ;j < 8600; j++);
}

// for循环实现延时函数,单位us
void delay_us(u16 delay_time)
{
    int data i,j;
    for(i = 0;i < delay_time; i++)
        for(j = 0;j < 5; j++);
}