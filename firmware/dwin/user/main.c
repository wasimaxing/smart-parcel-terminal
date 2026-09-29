#include "sys.h"
#include "uart.h"
#include "timer.h"
#include "save_data_dgus.h"
#include "dgus_tools.h"
#include "norflash.h"
#include "log.h"
#include "deal.h"
#include "rtc.h"
#include "page.h"
#include "canbus.h"
#include "parcel_link.h"

void main(void)
{
	INIT_CPU();  						/* 系统初始化 */
	PORT_Init(); 						/* IO口初始化 */
	T0_Init();							/* 定时器0初始化 */
	uart_init();						/* 串口初始化 */
	parcel_link_init();				/* STC UART4/RS-485 链路 */
	CanBus_Init(can_baud_250K);			/* CAN总线初始化 */

	WDT_ON();							/* 打开开门狗(喂狗在定时器T0中断中) */
	rtc_init();							/* RTC初始化 */
	Write_Vp(0x16, 0x5a02); 			/* 初始化初始触摸状态为抬起 */
	// data_save_init();					/* 数据改变自动保存初始化 */
	page_deal_init();					/* 页面处理初始化 */
	test_init();

	LOG_DEBUG("Firmware build time[%s %s]\n", __DATE__, __TIME__);

	start_timer(SOFT_TIMER_RTC, SOFT_TIMER_RTC_TIMEOUT);
	start_timer(SOFT_TIMER_1, SOFT_TIMER_1_TIMEOUT);
	start_timer(SOFT_TIMER_PRINTF, SOFT_TIMER_PRINTF_TIMEOUT);
	start_timer(SOFT_TIMER_2, 1000);
	
	while(1)
	{
		parcel_link_poll();				/* 优先取走 UART4 的 AA55 项目帧 */
		uart_handle_all();				/* 其他串口数据处理 */
		Read_0xF00();					/* 数据自动上传 */
		page_deal_func();				/* 页面处理 */
		CanRx();						/* CAN总线接收 */

		if(get_time_out_flag(SOFT_TIMER_RTC))
		{
			rtc_deal();					/* RTC处理 */
			start_timer(SOFT_TIMER_RTC, SOFT_TIMER_RTC_TIMEOUT);
		}
		
		if(get_time_out_flag(SOFT_TIMER_1))
		{
			// data_change_sava();			/* 数据改变自动保存 */
			start_timer(SOFT_TIMER_1, SOFT_TIMER_1_TIMEOUT);
				test_func();
		}

		if(get_time_out_flag(SOFT_TIMER_PRINTF))
		{
			LOG_DEBUG("code is running\n");

			// Can_test_func();				/* CAN总线测试 */
			start_timer(SOFT_TIMER_PRINTF, SOFT_TIMER_PRINTF_TIMEOUT);
		}

		if(get_time_out_flag(SOFT_TIMER_2))
		{
			parcel_link_1s();
			start_timer(SOFT_TIMER_2, 1000);
		}

		if(get_time_out_flag(SOFT_TIMER_3))
		{
			parcel_link_retry();
		}
	}
}
