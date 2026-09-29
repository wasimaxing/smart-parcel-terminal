#include "sys.h"
#include "uart.h"
#include "timer.h"
#include "canbus.h"

#define IRQ_EXIT_0 0    // 外部中断0中断号
#define IRQ_EXIT_1 2    // 外部中断1中断号
#define IRQ_TIMER_0 1   // 定时器0中断号
#define IRQ_TIMER_1 3   // 定时器1中断号
#define IRQ_TIMER_2 5   // 定时器2中断号
#define IRQ_CAN 9       // CAN中断号
#define IRQ_UART2_TR 4  // 串口2中断号
#define IRQ_UART3_TR 16 // 串口3中断号
#define IRQ_UART4_TX 10 // 串口4发送中断号
#define IRQ_UART4_RX 11 // 串口4接收中断号
#define IRQ_UART5_TX 12 // 串口5发送中断号
#define IRQ_UART5_RX 13 // 串口5接收中断号

// 外部中断0
void EX0_ISR_PC(void) interrupt IRQ_EXIT_0
{

}

// 外部中断1
void EX1_ISR_PC(void) interrupt IRQ_EXIT_1
{

}

// 定时器中断0
void T0_ISR_PC(void) interrupt IRQ_TIMER_0
{
    u16 i;
    
    // 关闭中断
	EA = 0;	

    // 重装载定时器0
	TH0 = T1MS >> 8;
	TL0 = T1MS;

    // 定时器延时计数器
	if(SysTick)	SysTick--;

    // 软定时器计数器
	for(i = 0; i < TIMER_NUMBER; i++)
	{
	    if(T_number_arr[i]&&T_number_en[i])	T_number_arr[i]--;
	}

    // 喂狗
	WDT_RST();

    // 开启中断
	EA = 1;
}

// 定时器中断1
void T1_ISR_PC(void) interrupt IRQ_TIMER_1
{

}

// 定时器中断2
void T2_ISR_PC(void) interrupt IRQ_TIMER_2
{

}

// CAN中断
void CAN_ISR_PC(void) interrupt IRQ_CAN
{
	u8 tmp;

	EA = 0;
	if((CAN_IR&0x80) == 0x80)
	{
		CAN_IR &= 0x3F;	//清空远程帧标记位			
	}
	if((CAN_IR&0x40) == 0x40)
	{
		CAN_IR &= 0xBF;	//清空数据帧标记位
		ADR_H = 0xFF;
		ADR_M = 0x00;
		ADR_L = 0x68;
		ADR_INC = 1;
		RAMMODE = 0xAF;		//读操作
		while(!APP_ACK);
		APP_EN = 1;
		while(APP_EN);
		tmp = DATA3 ;
		CanData.BusRXbuf[CanData.CanRxHead].status=tmp;
			APP_EN = 1;
			while(APP_EN);			
			CanData.BusRXbuf[CanData.CanRxHead].ID <<= 8;
			CanData.BusRXbuf[CanData.CanRxHead].ID |= DATA3;
			CanData.BusRXbuf[CanData.CanRxHead].ID <<= 8;
			CanData.BusRXbuf[CanData.CanRxHead].ID |= DATA2;
			CanData.BusRXbuf[CanData.CanRxHead].ID <<= 8;
			CanData.BusRXbuf[CanData.CanRxHead].ID |= DATA1;
			CanData.BusRXbuf[CanData.CanRxHead].ID <<= 8;
			CanData.BusRXbuf[CanData.CanRxHead].ID |= DATA0;
				CanData.BusRXbuf[CanData.CanRxHead].ID=CanData.BusRXbuf[CanData.CanRxHead].ID>>3;		
			if((tmp & 0xc0)==0xc0)//远程帧
			{

			}

			if(0==(tmp&0x80))//标准帧ID还需要右移18位
			{
				CanData.BusRXbuf[CanData.CanRxHead].ID >>= 18;
			}
		if(0==(tmp&0x40))//数据帧才需要读取数据
		{
			APP_EN = 1;
			while(APP_EN);
			CanData.BusRXbuf[CanData.CanRxHead].candata[0] = DATA3;
			CanData.BusRXbuf[CanData.CanRxHead].candata[1] = DATA2;
			CanData.BusRXbuf[CanData.CanRxHead].candata[2] = DATA1;
			CanData.BusRXbuf[CanData.CanRxHead].candata[3] = DATA0;
			APP_EN = 1;
			while(APP_EN);
			CanData.BusRXbuf[CanData.CanRxHead].candata[4] = DATA3;
			CanData.BusRXbuf[CanData.CanRxHead].candata[5] = DATA2;
			CanData.BusRXbuf[CanData.CanRxHead].candata[6] = DATA1;
			CanData.BusRXbuf[CanData.CanRxHead].candata[7] = DATA0;		
			}

			RAMMODE = 0;
			CanData.CanRxHead++;

	}
	if((CAN_IR&0x20) == 0x20)
	{
		CAN_IR &= ~(0x20);	//清空发送帧标记位
	}
	if((CAN_IR&0x10) == 0x10)
	{
		CAN_IR &= 0xEF;	//清空接收溢出标记位
	}
	if((CAN_IR&0x08) == 0x08)
	{
		CAN_IR &= 0xF7;	//清空错误标记位
	}
	if((CAN_IR&0x04) == 0x04)
	{
		CAN_IR &= 0xFB;	//清空仲裁失败标记位
		CAN_CR |= 0x04;	//重新启动发送	
	}
	CAN_ET=0;
	EA = 1;  
}

// 串口2中断
void Uart2_TX_RX_ISR_PC(void) interrupt IRQ_UART2_TR
{
	if (RI0)
    {
#if UART2_ENABLE
		uart2.buf[uart2.tail] = SBUF0;
		uart2.tail++;
		uart2.tail %= UART2_RX_SIZE;
#endif
        RI0 = 0;
    }
	// if (TI0)
    // {
    //     // user code
    //     TI0 = 0;
    // }
}

// 串口3中断
void Uart3_TX_RX_ISR_PC(void) interrupt IRQ_UART3_TR
{
	IEN2 &= 0xFE;// 关闭串口3中断
    if (SCON1 & 0x01) // 接收中断
    {
#if UART3_ENABLE
		uart3.buf[uart3.tail] = SBUF1;
		uart3.tail++;
		uart3.tail %= UART3_RX_SIZE;
#endif
        SCON1 &= 0x0FE; // 必须清除标志位2次
        SCON1 &= 0x0FE;
    }
    // if (SCON1 & 0x02) // 发送中断
    // {
    //     // user code
    //     SCON1 &= 0x0FD; // 清除接收标志
    //     SCON1 &= 0x0FD;
    // }
    IEN2 |= 0x01;// 开启串口3中断
}

// 串口4发送中断
void Uart4_TX_ISR_PC(void) interrupt IRQ_UART4_TX
{
	// user code
    // SCON2T &= 0xFE; // 清除接收标志
}

// 串口4接收中断
void Uart4_RX_ISR_PC(void) interrupt IRQ_UART4_RX
{
	ES2R = 0;// 关闭串口4接收中断
	if(SCON2R&0x01)
	{
#if UART4_ENABLE
		uart4.buf[uart4.tail] = SBUF2_RX;
		uart4.tail++;
		uart4.tail %= UART4_RX_SIZE;
#endif
		SCON2R &= 0xFE; // 清除接收标志
	}
    ES2R = 1;// 开启串口4接收中断
}

// 串口5发送中断
void Uart5_TX_ISR_PC(void) interrupt IRQ_UART5_TX
{
	// user code
    // SCON3T &= 0xFE; // 清除接收标志
}

// 串口5接收中断
void Uart5_RX_ISR_PC(void) interrupt IRQ_UART5_RX
{
	ES3R = 0;
	if(SCON3R&0x01)
	{
#if UART5_ENABLE
		uart5.buf[uart5.tail] = SBUF3_RX;
		uart5.tail++;
		uart5.tail %= UART5_RX_SIZE;
#endif
		SCON3R &= 0xFE; // 清除接收标志
	}
	ES3R = 1;
}