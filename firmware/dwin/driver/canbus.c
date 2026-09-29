#include "canbus.h"
#include "dgus_tools.h"
#include "log.h"
#include "t5l_io.h"
#include "timer.h"

/********************************** 常量定义 ***********************************/

#if defined(CAN_T5L0)
u8 code can_baud[4][4]={{17,0x4C,0x1F,0x00},{0x08,0x25,0x0F,0x00},{0x11,0x25,0x0F,0x00},{0x04,0x24,0x0E,0x00}};
#else
u8 code can_baud[4][4]={{0x3F,0x40,0x72,0x00},{0x1F,0x40,0x72,0x00},{0x0F,0x40,0x72,0x00},{0x07,0x40,0x72,0x00}};
#endif

u32 code CanBustxaddr[27]={
    0x1C1F1830<<3,0x1C1F1831<<3,0x1C1F1832<<3,0x1C1F1833<<3,0x1C1F1834<<3,
    0x1C1F1835<<3,0x1C1F1836<<3,0x1C1F183D<<3,0x1C1F183E<<3,0x1C1F183F<<3,
    0x1C1F1840<<3,0x1C1F1841<<3,0x1C1F1842<<3,0x1C1F1843<<3,0x1C1F1850<<3,
    0x1C1F1851<<3,0x1C1F1852<<3,0x1C1F1853<<3,0x1C1F1854<<3,0x1C1F1855<<3,
	0x1C1F1800<<3,0x1C1F1822<<3,0x1C1F1825<<3,0x1C1F1860<<3,0x1C1F1722<<3,
	0x1C1F2010<<3,0x1C1F1826<<3,
};
/********************************** 全局变量 ***********************************/
CANBUSUNIT   CanData;
//https://blog.csdn.net/weixin_44536482/article/details/89030152

// CAN初始化, dgus_tools.h  必须增加如下宏定义  #define INTVPACTION
void CanBus_Init(u16 bdtid)
{
	u16 i;

	SetPinOut(0,2);
	SetPinIn(0,3);
	// PinOutput(0,2,1);
	P02 = 1;

	MUX_SEL |= 0x80;		//将CAN接口引出到P0.2,P0.3	
	ADR_H = 0xFF;
	ADR_M = 0x00;
	ADR_L = 0x60;
	ADR_INC = 1;
	RAMMODE = 0x8F;		//写操作
	while(!APP_ACK);
	#if 0
	DATA3 = 17;
	DATA2 = 0x4c;
	DATA1 = 0x1f;
	DATA0 = 0x00;	
	#else
	i=bdtid;
	DATA3 = can_baud[i][0];
	DATA2 = can_baud[i][1];
	DATA1 = can_baud[i][2];
	DATA0 = can_baud[i][3];	 		
	#endif		
	APP_EN = 1;
	while(APP_EN);//0060
	DATA3 = 0;
	DATA2 = 0;
	DATA1 = 0;
	DATA0 = 0;	 		//配置验收寄存器ACR
	APP_EN = 1;	  
	while(APP_EN);//0061
	DATA3 = 0xFF;
	DATA2 = 0xFF;
	DATA1 = 0xFF;
	DATA0 = 0xFF;	 	//配置AMR
	APP_EN = 1;	
	while(APP_EN);//0062
	RAMMODE = 0;
	CAN_CR = 0xA0;
	while(CAN_CR&0x20);	//执行配置FF0060-FF0062动作
	ECAN = 1;			//打开CAN中断	
//	CanRTRFlag = 1;	
	CanData.CanRxTail = 0;
	CanData.CanRxHead = 0;
}


/**************************************************************

D3  1  CAN_RX_BUFFER  [7] IDE ，[6]RTR， [3:0]—DLC，帧数据长度。
0xFF:0068
D2:D0  3  未定义 
ID  ID，扩展帧时 29bit 有效，标准帧时 11bit 有效。
D3  1  ID 第一个字节，标准帧与扩展帧。
D2  1  ID 第二个字节，[7:5]为标准帧的高 3bit，扩展帧第 2 字节。
D1  1  ID 第三个字节，标准帧无效，扩展帧第 3 字节。
0xFF:0069
D0  1  ID 第四个字节，标准帧无效，[7:3]-扩展帧的高 5bit。
0xFF:006A  D3:D0  4  数据  接收数据，DATA1-DATA4。
0xFF:006B  D3:D0  4  数据  接收数据，DATA5-DATA8。


******************************************************************/
void CanErrorReset(void)//对于T5L1和T5L2必须在main函数，while(1)中调用
{
	// EA=0;
	if(CAN_ET&0X20)
	{
		CAN_ET &= 0XDF;
		CAN_CR |= 0X40;
		delay_us(1000);
		CAN_CR &= 0XBF;  
		CanData.CanTxFlag = 0;
	}
	// EA=1;
}


void CanRx(void)//主程序调用--接收数据
{
	if(CanData.CanRxHead != CanData.CanRxTail)
	{
		switch(CanData.BusRXbuf[CanData.CanRxTail].ID)
		{
			// 测试
			case 0x180100F4:
				// Write_Vp_byU16(0x1000,CanData.BusRXbuf[CanData.CanRxTail].candata,4);
				break;

			// case ......

			default:break;
		}	
		CanData.CanRxTail++;
	}
}


void CAN_tx(u32 ID,  u8 *str)
{

	EA = 0;

	ADR_H = 0xFF;
	ADR_M = 0x00;
	ADR_L = 0x64;
	ADR_INC = 1;
	RAMMODE = 0x8F;		//写操作
	while(!APP_ACK);
	DATA3 = 0x88;			//帧类长度型以及数据
	DATA2 = 0;
	DATA1 = 0;
	DATA0 = 0;	 		
	APP_EN = 1;
	while(APP_EN);		//写入RTR,IDE,DLC等数据
	DATA3 = (u8)(ID>>24);
	DATA2 = (u8)(ID>>16);
	DATA1 = (u8)(ID>>8);
	DATA0 = (u8)(ID>>0);	 		
	APP_EN = 1;
	while(APP_EN);		//写入ID数据
	DATA3 = str[0];
	DATA2 = str[1];
	DATA1 = str[2];
	DATA0 = str[3];	 		
	APP_EN = 1;
	while(APP_EN);		//写入发送数据前4字节
	DATA3 = str[4];
	DATA2 = str[5];
	DATA1 = str[6];
	DATA0 = str[7];	 		
	APP_EN = 1;
	while(APP_EN);		//写入发送数据后4字节
	EA = 1;
	RAMMODE = 0;
	CAN_CR |= 0x04;		//启动发送			

	delay_ms(2);//


}


void Can_test_func(void)
{
	u8 str_test[8]={0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
	
	CanErrorReset();						/* CAN总线错误清除 */
	CAN_tx(CanBustxaddr[0],str_test);		/* 发送数据 */
}