#include  "rtc.h"
#include "dgus_tools.h"

#if SELECT_RTC_TYPE
// 引脚定义
sbit RTC_SDA	=	P3^3;
sbit RTC_SCL	=	P3^2;

// 50
void RTC_SDA_IN(void)
{
	P3MDOUT = P3MDOUT & 0xF7;
}

void RTC_SDA_OUT(void)
{
	P3MDOUT = P3MDOUT | 0x08;
}
#endif

#if SELECT_RTC_TYPE == 1
void RTC_Delay_us_rx8130(void)
{
	u8 i;
	for(i=0;i<80;i++);
}

void RTC_start_rx8130(void)
{	
	RTC_SDA_OUT();
	RTC_SDA=1;
	RTC_SCL=1;
	RTC_Delay_us_rx8130();  //15
	RTC_SDA=0;
	RTC_Delay_us_rx8130();  //15
	RTC_SCL=0;
	RTC_Delay_us_rx8130();  //15
}
//================================
void RTC_stop_rx8130(void)
{	
	RTC_SDA_OUT();
	RTC_SDA=0;
	RTC_SCL=1;
	RTC_Delay_us_rx8130();//15
	RTC_SDA=1;
	RTC_Delay_us_rx8130();//15
	RTC_SDA_IN();
}
//================================
void RTC_mark_rx8130(void)
{	
	RTC_SDA_OUT();
	RTC_SDA=0;
	RTC_Delay_us_rx8130();
	RTC_SCL=1;
	RTC_Delay_us_rx8130();
	RTC_SCL=0;
	RTC_Delay_us_rx8130();
}
//=================================
void RTC_mnak_rx8130(void)
{	
	RTC_SDA_OUT();
	RTC_SDA=1;
	RTC_Delay_us_rx8130();
	RTC_SCL=1;
	RTC_Delay_us_rx8130();
	RTC_SCL=0;
	RTC_Delay_us_rx8130();
}
//=================================
void RTC_cack_rx8130()
{	
	u8 i;
	RTC_SDA_IN();
	RTC_SDA=1;
	RTC_Delay_us_rx8130();
	RTC_SCL=1;
	RTC_Delay_us_rx8130();
	for(i=0;i<50;i++)
	{   
		if(!RTC_SDA) break;
		RTC_Delay_us_rx8130();
	}
	RTC_SCL=0;
	RTC_Delay_us_rx8130();
	RTC_SDA_OUT();
}
//=========================================
void RTC_write_byte_rx8130(u8 dat)
{	
	u8 i;
	RTC_SDA_OUT();
	for(i=0;i<8;i++)
	{	
		RTC_SDA = (dat & 0x80) ? 1 : 0;
		dat=(dat<<1);
		RTC_Delay_us_rx8130();
		RTC_SCL=1;
		RTC_Delay_us_rx8130();
		RTC_SCL=0;
		RTC_Delay_us_rx8130();
	}
	RTC_cack_rx8130();
}
//====================================================
u8 RTC_read_byte_rx8130()
{	
	u8 i, Dat;
	RTC_SDA_IN();
	for(i=0;i<8;i++)
	{	
		RTC_Delay_us_rx8130();
		RTC_SCL=1;
		RTC_Delay_us_rx8130();
		Dat=(Dat<<1);
		if(RTC_SDA)
		{
			Dat=Dat|0x01;
		}
		else
		{
			Dat=Dat&0xFE;
		}
		RTC_SCL=0;
		RTC_Delay_us_rx8130();
	}
	return(Dat);
}
//====================================================
u8 RTC_get_week_rx8130(u8 years,u8 month,u8 day)
{	
	u8 weak;
	u16 year;
	year = years+2000;
	if(month==1||month==2)
	{
		month += 12;
		year--;
	}
	if((year<1752)||(year==1752&&month<9)||(year==1752&&month==9&&day<3))
	{
		weak = (day + 2*month + 3*(month+1)/5 + year + year/4 + 6)%7;
	}
	else
	{
		weak = (day +1 + 2*month + 3*(month+1)/5 + year + year/4 -year/100 + year/400)%7;
	}
	return weak;
}
//把RTC读取并处理，写到DGUS对应的变量空间
void RTC_read_time_rx8130()
{
	u8 i,N,M; 
	u8 Rtcdata[]={0,0,0,0,0,0,0,0};
	RTC_start_rx8130();
	RTC_write_byte_rx8130(0x64);
	RTC_write_byte_rx8130(0x10);
	RTC_stop_rx8130();
	RTC_start_rx8130();
	RTC_write_byte_rx8130(0x65);
	for(i=6;i>0;i--)
	{	
		Rtcdata[i]=RTC_read_byte_rx8130();
		RTC_mark_rx8130();
	}
	Rtcdata[0]=RTC_read_byte_rx8130(); RTC_mnak_rx8130();
	RTC_stop_rx8130();
	for(i=0;i<3;i++)	//年月日转换成HEX
	{	
		N=Rtcdata[i]/16;
		M=Rtcdata[i]%16;
		Rtcdata[i]=N*10+M;
	}
	for(i=4;i<7;i++)	//时分秒转换成HEX
	{	
		N=Rtcdata[i]/16;
		M=Rtcdata[i]%16;
		Rtcdata[i]=N*10+M;
	}
	Rtcdata[3]=RTC_get_week_rx8130(Rtcdata[0],Rtcdata[1],Rtcdata[2]);//周
	Write_Vp(0X0010,(Rtcdata[0]<<8)+Rtcdata[1]);
	Write_Vp(0X0011,(Rtcdata[2]<<8)+Rtcdata[3]);
	Write_Vp(0X0012,(Rtcdata[4]<<8)+Rtcdata[5]);
	Write_Vp(0X0013,(Rtcdata[6]<<8)+Rtcdata[7]);
}

void RTC_set_time_rx8130()
{					    
	if(Read_Vp(0x009C) == 0x5AA5)
	{			
		u8 tmp_u8=0;
		u8 TimeS[]={0,0,0,0,0,0,0};
		u16 time_set[3], tmp_u16;
		Read_Vp_byU16(0x009D,(u8*)time_set,3);
		tmp_u16 = time_set[0];
		tmp_u8=(u8)(tmp_u16>>8);TimeS[0]=((tmp_u8/10)*16)+(tmp_u8%10);
		tmp_u8=(u8)(tmp_u16);   TimeS[1]=((tmp_u8/10)*16)+(tmp_u8%10);
		tmp_u16 = time_set[1];
		tmp_u8=(u8)(tmp_u16>>8);TimeS[2]=((tmp_u8/10)*16)+(tmp_u8%10);
		tmp_u8=(u8)(tmp_u16);   TimeS[4]=((tmp_u8/10)*16)+(tmp_u8%10);
		tmp_u16 = time_set[2];
		tmp_u8=(u8)(tmp_u16>>8);TimeS[5]=((tmp_u8/10)*16)+(tmp_u8%10);
		tmp_u8=(u8)(tmp_u16);   TimeS[6]=((tmp_u8/10)*16)+(tmp_u8%10);
		Write_Vp(0x009c,0);	

		RTC_start_rx8130();		//30=00
		RTC_write_byte_rx8130(0x64);
		RTC_write_byte_rx8130(0x30);
		RTC_write_byte_rx8130(0x00);
		RTC_stop_rx8130();
		RTC_start_rx8130();		//1C-1F=48 00 40 10
		RTC_write_byte_rx8130(0x64);
		RTC_write_byte_rx8130(0x1C);
		RTC_write_byte_rx8130(0x48);
		RTC_write_byte_rx8130(0x00);
		RTC_write_byte_rx8130(0x40);
		RTC_write_byte_rx8130(0x10);
		RTC_stop_rx8130();
		RTC_start_rx8130();		//10-16=RTC设置值 BCD格式
		RTC_write_byte_rx8130(0x64);
		RTC_write_byte_rx8130(0x10);
		RTC_write_byte_rx8130(TimeS[6]);	//秒
		RTC_write_byte_rx8130(TimeS[5]);	//分
		RTC_write_byte_rx8130(TimeS[4]);	//时
		RTC_write_byte_rx8130(TimeS[3]);	//星期
		RTC_write_byte_rx8130(TimeS[2]);	//日
		RTC_write_byte_rx8130(TimeS[1]);	//月
		RTC_write_byte_rx8130(TimeS[0]);	//年
		RTC_stop_rx8130();
		RTC_start_rx8130();		//1E-1F 00 10
		RTC_write_byte_rx8130(0x64);
		RTC_write_byte_rx8130(0x1E);
		RTC_write_byte_rx8130(0x00);	
		RTC_write_byte_rx8130(0x10);			
		RTC_stop_rx8130(); 
	}
}

//====================================================================
void RTC_init_rx8130()
{	
	u8 i;
	P3MDOUT = 0x0d;		//P3 0000 1111
	RTC_start_rx8130();
	RTC_write_byte_rx8130(0x64);
	RTC_write_byte_rx8130(0x1d);
	RTC_stop_rx8130();
	RTC_start_rx8130();
	RTC_write_byte_rx8130(0x65);
	i=RTC_read_byte_rx8130();
	RTC_mark_rx8130();
	RTC_read_byte_rx8130();
	RTC_mnak_rx8130();
	RTC_stop_rx8130();
	if((i&0x02)==0x02)
	{	
		//重新配置时间
		RTC_start_rx8130();		//30=00
		RTC_write_byte_rx8130(0x64);
		RTC_write_byte_rx8130(0x30);
		RTC_write_byte_rx8130(0x00);
		RTC_stop_rx8130();
		RTC_start_rx8130();		//1C-1F=48 00 40 10
		RTC_write_byte_rx8130(0x64);
		RTC_write_byte_rx8130(0x1C);
		RTC_write_byte_rx8130(0x48);
		RTC_write_byte_rx8130(0x00);
		RTC_write_byte_rx8130(0x40);
		RTC_write_byte_rx8130(0x10);
		RTC_stop_rx8130();
		RTC_start_rx8130();		//10-16=RTC设置值 BCD格式
		RTC_write_byte_rx8130(0x64);
		RTC_write_byte_rx8130(0x10);
		RTC_write_byte_rx8130(0x00);	//秒
		RTC_write_byte_rx8130(0x00);	//分
		RTC_write_byte_rx8130(0x00);	//时
		RTC_write_byte_rx8130(0x01);	//星期
		RTC_write_byte_rx8130(0x01);	//日
		RTC_write_byte_rx8130(0x01);	//月
		RTC_write_byte_rx8130(0x17);	//年
		RTC_stop_rx8130();
		RTC_start_rx8130();		//1E-1F 00 10
		RTC_write_byte_rx8130(0x64);
		RTC_write_byte_rx8130(0x1E);
		RTC_write_byte_rx8130(0x00);	
		RTC_write_byte_rx8130(0x10);			
		RTC_stop_rx8130();
	}
}
//=========================================

//========================================================================================================================================
//**********************SD2058接口程序，SDA 10K上拉到3.3V**************
		

#elif SELECT_RTC_TYPE == 2

void RTC_Delay_us_sd2058(unsigned char t)
{	
	char i;
	while(t)
	{	
		for(i=0;i<50;i++)
		{
			i=i;
		}
		t--;
	}
}

void RTC_start_sd2058(void)
{	
	RTC_SDA_OUT();
	RTC_SDA=1;
	RTC_SCL=1;
	RTC_Delay_us_sd2058(15);
	RTC_SDA=0;
	RTC_Delay_us_sd2058(15);
	RTC_SCL=0;
	RTC_Delay_us_sd2058(15);
}

void RTC_stop_sd2058(void)
{	
	RTC_SDA_OUT();
	RTC_SDA=0;
	RTC_SCL=1;
	RTC_Delay_us_sd2058(15);
	RTC_SDA=1;
	RTC_Delay_us_sd2058(15);
	RTC_SDA_IN();
}

void RTC_mack_sd2058(void)
{	
	RTC_SDA_OUT();
	RTC_SDA=0;
	RTC_Delay_us_sd2058(5);
	RTC_SCL=1;
	RTC_Delay_us_sd2058(5);
	RTC_SCL=0;
	RTC_Delay_us_sd2058(5);
}

void RTC_mnak_sd2058(void)
{	
	RTC_SDA_OUT();
	RTC_SDA=1;
	RTC_Delay_us_sd2058(5);
	RTC_SCL=1;
	RTC_Delay_us_sd2058(5);
	RTC_SCL=0;
	RTC_Delay_us_sd2058(5);
}

void RTC_cack_sd2058(void)
{	
	unsigned char i;
	RTC_SDA_IN();
	RTC_SDA=1;
	RTC_Delay_us_sd2058(5);
	RTC_SCL=1;
	RTC_Delay_us_sd2058(5);
	for(i=0;i<50;i++)
	{   
		if(!RTC_SDA) 
		{
			break;
		}
		RTC_Delay_us_sd2058(5);
	}
	RTC_SCL=0;
	RTC_Delay_us_sd2058(5);
	RTC_SDA_OUT();
}

//I2C 写入1个字节
void RTC_write_byte_sd2058(unsigned char dat)
{	
	char i;
	RTC_SDA_OUT();
	for(i=0;i<8;i++)
	{	
		if(dat&0x80)
		{
			RTC_SDA=1;
		}
		else 
		{
			RTC_SDA=0;
		}
		dat=(dat<<1);
		RTC_Delay_us_sd2058(5);
		RTC_SCL=1;
		RTC_Delay_us_sd2058(5);
		RTC_SCL=0;
		RTC_Delay_us_sd2058(5);
	}
	RTC_cack_sd2058();
}

//i2c 读取1个字节数据
unsigned char RTC_read_byte_sd2058(void)
{	
	char i;
	unsigned char dat;
	RTC_SDA_IN();
	for(i=0;i<8;i++)
	{	
		RTC_Delay_us_sd2058(5);
		RTC_SCL=1;
		RTC_Delay_us_sd2058(5);
		dat=(dat<<1);
		if(RTC_SDA) 
		{
			dat=dat|0x01;
		}
		else 
		{
			dat=dat&0xFE;
		}
		//dat=(dat<<1);
		RTC_SCL=0;
		RTC_Delay_us_sd2058(5);
	}
	return(dat);
}

//检查SD2058有没有掉电，掉电则初始化SD2058，设置时间为2021.01.01 星期五 00:00:00
void RTC_init_sd2058(void)
{	
	unsigned char dat1,dat2;
	//检查有没有掉电
	P3MDOUT = 0x0d;		//P3 0000 1111
	RTC_start_sd2058();
	RTC_write_byte_sd2058(0x64);
	RTC_write_byte_sd2058(0x0f);		//0x0f最低位
	RTC_start_sd2058();
	RTC_write_byte_sd2058(0x65);
	dat2 = RTC_read_byte_sd2058();
	RTC_mack_sd2058();
	dat1 = RTC_read_byte_sd2058();
	RTC_mnak_sd2058();
	RTC_stop_sd2058();
	if(dat2&0x01)
	{
		if(dat2&0x84)		//WRTC2 WRTC3是否为0，是的话，写0
		{
			dat2 &= ~0x84;
			RTC_start_sd2058();		
			RTC_write_byte_sd2058(0x64);
			RTC_write_byte_sd2058(0x0f);
			RTC_write_byte_sd2058(dat2);
			RTC_stop_sd2058();
		}
		dat2 &= ~0x01;
		//读WRTC1是否为0
		if(dat1 & 0x80)		//WRTC1是否为1
		{
			dat1 &= ~0x80;
			RTC_start_sd2058();		
			RTC_write_byte_sd2058(0x64);
			RTC_write_byte_sd2058(0x10);
			RTC_write_byte_sd2058(dat1);
			RTC_stop_sd2058();
		}
		
		//写使能
		dat1 |= 0x80;
		RTC_start_sd2058();		
		RTC_write_byte_sd2058(0x64);
		RTC_write_byte_sd2058(0x10);
		RTC_write_byte_sd2058(dat1);
		RTC_stop_sd2058();
		dat2 |= 0x84;
		RTC_start_sd2058();		
		RTC_write_byte_sd2058(0x64);
		RTC_write_byte_sd2058(0x0f);
		RTC_write_byte_sd2058(dat2);
		RTC_stop_sd2058();
		
		
//		//重新配置时间
//		RTC_start_sd2058();		//10-16=RTC设置值 BCD格式
//		RTC_write_byte_sd2058(0x64);
//		RTC_write_byte_sd2058(0x00);
//		RTC_write_byte_sd2058(0x00);	//秒
//		RTC_write_byte_sd2058(0x00);	//分
//		RTC_write_byte_sd2058(0x80);	//时，小时最高位是24-12小时配置位
//		RTC_write_byte_sd2058(0x05);	//星期
//		RTC_write_byte_sd2058(0x01);	//日
//		RTC_write_byte_sd2058(0x01);	//月
//		RTC_write_byte_sd2058(0x21);	//年
//		RTC_stop_sd2058();
		dat2 &= ~0x84;
		dat1 &= ~0x80;
		RTC_start_sd2058();		
		RTC_write_byte_sd2058(0x64);
		RTC_write_byte_sd2058(0x10);
		RTC_write_byte_sd2058(dat1);
		RTC_stop_sd2058();
		RTC_start_sd2058();		
		RTC_write_byte_sd2058(0x64);
		RTC_write_byte_sd2058(0x0f);
		RTC_write_byte_sd2058(dat2);
		RTC_stop_sd2058();
	}
}



void RTC_config_sd2058(u8* prtc_set)
{
	u8 dat,dat1;
	RTC_start_sd2058();
	RTC_write_byte_sd2058(0x64);
	RTC_write_byte_sd2058(0x0f);		//0x10
	RTC_start_sd2058();
	RTC_write_byte_sd2058(0x65);
	dat = RTC_read_byte_sd2058();
	RTC_mack_sd2058();
	dat1 = RTC_read_byte_sd2058();
	RTC_mnak_sd2058();
	RTC_stop_sd2058();
	dat1 |= 0x80;
	RTC_start_sd2058();		
	RTC_write_byte_sd2058(0x64);
	RTC_write_byte_sd2058(0x10);
	RTC_write_byte_sd2058(dat1);
	RTC_stop_sd2058();
	dat |= 0x84;
	RTC_start_sd2058();		
	RTC_write_byte_sd2058(0x64);
	RTC_write_byte_sd2058(0x0f);
	RTC_write_byte_sd2058(dat);
	RTC_stop_sd2058();
	

	RTC_start_sd2058();		//10-16=RTC设置值 BCD格式
	RTC_write_byte_sd2058(0x64);
	RTC_write_byte_sd2058(0x00);
	RTC_write_byte_sd2058(prtc_set[6]);	//秒
	RTC_write_byte_sd2058(prtc_set[5]);	//分
	RTC_write_byte_sd2058(prtc_set[4]);	//时
	RTC_write_byte_sd2058(prtc_set[3]);	//星期
	RTC_write_byte_sd2058(prtc_set[2]);	//日
	RTC_write_byte_sd2058(prtc_set[1]);	//月
	RTC_write_byte_sd2058(prtc_set[0]);	//年
	RTC_stop_sd2058();
	dat &= ~0x84;
	dat1 &= ~0x80;
	RTC_start_sd2058();		
	RTC_write_byte_sd2058(0x64);
	RTC_write_byte_sd2058(0x10);
	RTC_write_byte_sd2058(dat1);
	RTC_stop_sd2058();
	RTC_start_sd2058();		
	RTC_write_byte_sd2058(0x64);
	RTC_write_byte_sd2058(0x0f);
	RTC_write_byte_sd2058(dat);
	RTC_stop_sd2058();
}

//把RTC读取并处理，写到DGUS对应的变量空间，主程序中每0.5秒调用一次
void RTC_read_time_sd2058(void)
{	
	unsigned char rtcdata[8];
	unsigned char i,n,m;
	RTC_start_sd2058();
	RTC_write_byte_sd2058(0x64);
	RTC_write_byte_sd2058(0x00);
	RTC_start_sd2058();
	RTC_write_byte_sd2058(0x65);
	for(i=6;i>0;i--)
	{	
		rtcdata[i]=RTC_read_byte_sd2058();
		RTC_mack_sd2058();
	}
	rtcdata[0]=RTC_read_byte_sd2058();
	RTC_mnak_sd2058();
	RTC_stop_sd2058();
	rtcdata[4] &= 0x7F;
	for(i=0;i<3;i++)	//年月日转换成HEX
	{	
		n=rtcdata[i]/16;
		m=rtcdata[i]%16;
		rtcdata[i]=n*10+m;
	}
	for(i=4;i<7;i++)	//时分秒转换成HEX
	{	
		n=rtcdata[i]/16;
		m=rtcdata[i]%16;
		rtcdata[i]=n*10+m;
	}
	//星期可以不用处理
	rtcdata[7]=0;
	Write_Vp_byU8(0x0010,(u8*)rtcdata,8);	//写入DGUS变量空间
}

//void btn_click(u16 x,u16 y)
//{
//	u16 btn_check[4];
//	btn_check[0]=0x5aa5;
//	btn_check[1]=0x0004;
//	btn_check[2]=x;
//	btn_check[3]=y;
//	Write_Dgusii_Vp_byChar(0x00D4,(u8*)&btn_check[0],4);
//}

u8 BCD(u8 dat)
{
	  return ((dat/10)<<4)|(dat%10);
}

//u8 IBCD(u8 dat)
//{
//	 return (dat>>4)*10+(dat&0x0f);
//}





u8 RTC_get_week_sd2058(u8 year,u8 month,u8 day)
{	
	u16 tmp,mon,y;
	u8 week;
	if((month == 1) || (month == 2))
	{
		mon = month + 12;
		y = year - 1;
	}
	else 
	{
		mon = month;
		y = year;
	}
	tmp = y + (y / 4) +(((mon + 1) * 26) / 10) + day - 36;
	week = tmp % 7;
	return week;
}





void RTC_set_time_sd2058(void)
{
	u8 rtc_parm[8],rtc_set[8];
	Read_Vp_byU16(0x9c,rtc_parm,4);
	if((rtc_parm[0] == 0x5A) && (rtc_parm[1] == 0xA5))		//启动配置
	{
		rtc_set[0] = BCD(rtc_parm[2]);
		rtc_set[1] = BCD(rtc_parm[3]);
		rtc_set[2] = BCD(rtc_parm[4]);
		rtc_set[3] = RTC_get_week_sd2058(rtc_parm[2],rtc_parm[3],rtc_parm[4]);
		rtc_set[4] = BCD(rtc_parm[5]);
		rtc_set[4] |= 0x80;
		rtc_set[5] = BCD(rtc_parm[6]);
		rtc_set[6] = BCD(rtc_parm[7]);
		RTC_config_sd2058(rtc_set);
		rtc_parm[0] = 0;
		rtc_parm[1] = 0;
		Write_Vp_byU8(0x9c,rtc_parm,2);
	}
}

#endif

	   
//========================================================================================================================================

void rtc_init(void)
{
#if SELECT_RTC_TYPE == 1
	RTC_init_rx8130();
#elif SELECT_RTC_TYPE == 2
	RTC_init_sd2058();
#endif
}

void rtc_deal(void)
{
#if SELECT_RTC_TYPE == 1
	RTC_read_time_rx8130();
	RTC_set_time_rx8130();
#elif SELECT_RTC_TYPE == 2
	RTC_read_time_sd2058();
	RTC_set_time_sd2058();
#endif
}