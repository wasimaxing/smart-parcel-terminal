#include "dgus_tools.h"
#include "timer.h"
#include "other_tools.h"

// 读VP，按字读
void Read_Vp_byU16(unsigned int addr,unsigned char *buf,unsigned int len)
{
	unsigned char i;

#ifdef INTVPACTION	
	EA = 0;
#endif

	i = (unsigned char)(addr & 0x01);
	addr = addr / 2;
	ADR_H = 0x00;
	ADR_M = (unsigned char)(addr >> 8);
	ADR_L = (unsigned char)(addr);
	ADR_INC = 0x01;
	RAMMODE = 0xAF;
	while(APP_ACK == 0);
	while(len > 0)
	{
		APP_EN = 1;
		while(APP_EN == 1);
		if((i==0)&&(len>0))
		{
			*buf++ = DATA3;
			*buf++ = DATA2;
			i = 1;
			len--;
		}
		if((i == 1) && (len > 0))
		{
			*buf++ = DATA1;
			*buf++ = DATA0;
			i = 0;
			len--;
		}
	}
	RAMMODE=0x00;

#ifdef INTVPACTION	
	EA = 1;
#endif
}

// 写VP，按字写
void Write_Vp_byU16(unsigned int addr,unsigned char *buf,unsigned int len)
{
	unsigned char i;
//	bit EA_Bak;
//	EA_Bak = EA;
//	EA = 0;
#ifdef INTVPACTION	
	EA = 0;
#endif

	i = (unsigned char)(addr & 0x01);
	addr = addr / 2;
	ADR_H = 0x00;
	ADR_M = (unsigned char)(addr >> 8);
	ADR_L = (unsigned char)(addr);
	ADR_INC = 0x01;
	RAMMODE = 0x8F;
	while(APP_ACK == 0);
	if(i && len > 0)
	{
		RAMMODE = 0x83;
		DATA1 = *buf++;
		DATA0 = *buf++;
		APP_EN = 1;
		while(APP_EN == 1);
		len--;
	}
	RAMMODE = 0x8F;
	while(len >= 2)
	{
		DATA3 =* buf++;
		DATA2 =* buf++;
		DATA1 =* buf++;
		DATA0 =* buf++;
		APP_EN = 1;
		while(APP_EN == 1);
		len = len - 2;
	}
	if(len)
	{
		RAMMODE = 0x8C;
		DATA3 =* buf++;
		DATA2 =* buf++;
		APP_EN = 1;
		while(APP_EN == 1);
	}
	RAMMODE = 0x00;
//	EA = EA_Bak;

#ifdef INTVPACTION	
	EA = 1;
#endif
}

// 写VP，按字节写
void Write_Vp_byU8(unsigned int addr,unsigned char *buf,unsigned int len)
{
	unsigned char i;
//	bit EA_Bak;
//	EA_Bak = EA;
//	EA = 0;
#ifdef INTVPACTION	
	EA = 0;
#endif
	i = (unsigned char)(addr & 0x01);
	addr = addr / 2;
	ADR_H = 0x00;
	ADR_M = (unsigned char)(addr >> 8);
	ADR_L = (unsigned char)(addr);
	ADR_INC = 0x01;
	RAMMODE = 0x8F;
	while(APP_ACK == 0);
	if(i && len > 0){
		if(len == 1){
			RAMMODE = 0x82;
			DATA1 = *buf++;
			APP_EN = 1;
			while(APP_EN == 1);
			len--;
		} else {
			RAMMODE = 0x83;
			DATA1 = *buf++;
			DATA0 = *buf++;
			APP_EN = 1;
			while(APP_EN == 1);
			len -= 2;
		}
	}
	RAMMODE = 0x8F;
	while(len >= 4){
		DATA3 = *buf++;
		DATA2 = *buf++;
		DATA1 = *buf++;
		DATA0 = *buf++;
		APP_EN = 1;
		while(APP_EN == 1);
		len -= 4;
	}
	
	if(len == 3){
		RAMMODE = 0x8E;
		DATA3 = *buf++;
		DATA2 = *buf++;
		DATA1 = *buf++;
		APP_EN = 1;
		while(APP_EN == 1);
	} else if(len == 2){
		RAMMODE = 0x8C;
		DATA3 =* buf++;
		DATA2 =* buf++;
		APP_EN = 1;
		while(APP_EN == 1);
	} else if(len == 1){
		RAMMODE = 0x88;
		DATA3 = *buf++;
		APP_EN = 1;
		while(APP_EN == 1);
	}
	RAMMODE = 0x00;
//	EA = EA_Bak;

#ifdef INTVPACTION	
	EA = 1;
#endif
}

// 读一个VP，按字读
u16 Read_Vp(u16 Dgus_Addr)
{	   	   	
	u16 R_Dgus=0;

#ifdef INTVPACTION	
	EA = 0;
#endif
	ADR_H = 0x00;
	ADR_M = (u8)((Dgus_Addr/2)>>8);
	ADR_L = (u8)(Dgus_Addr/2);
	RAMMODE = 0xAF;		//读操作
	while(!APP_ACK);
	APP_EN = 1;
	while(APP_EN);
	if(Dgus_Addr%2)	R_Dgus = (DATA1<<8)+DATA0; 
	else   R_Dgus=(DATA3<<8)+DATA2;
	RAMMODE = 0;

#ifdef INTVPACTION	
	EA = 1;
#endif

	return   R_Dgus;		
}

// 写一个VP，按字写
void Write_Vp(u16 Dgus_Addr,u16 Val)
{         

#ifdef INTVPACTION	
	EA = 0;
#endif

	ADR_H = 0x00;
	ADR_M = (u8)((Dgus_Addr/2)>>8);
	ADR_L = (u8)(Dgus_Addr/2);
	RAMMODE = 0xAF;		//读操作
	while(!APP_ACK);
	APP_EN = 1;
	while(APP_EN);
	RAMMODE = 0;
	ADR_M = (u8)((Dgus_Addr/2)>>8);
	ADR_L = (u8)(Dgus_Addr/2);
	RAMMODE = 0x8F;		//写操作
	while(!APP_ACK);
	if(Dgus_Addr%2){DATA1=(u8)(Val>>8); DATA0=(u8)(Val); }
	else { DATA3 =(u8)(Val>>8);	DATA2 =(u8)(Val);}
	APP_EN = 1;
	while(APP_EN);
	RAMMODE = 0;

#ifdef INTVPACTION	
	EA = 1;
#endif
}

// 清除VP，按字清除，一次最大0x1000字
void Clear_Vp_byU16(u16 Dgus_Addr,u16 Len)
{
	u16 buf_u16[0x1000];
	memset(buf_u16, 0, 0x1000 * 2);
	Write_Vp_byU16(Dgus_Addr, (unsigned char *)buf_u16, Len);
}

// 拷贝VP，按字拷贝，一次最大0x1000字
void Copy_Vp_byU16(u16 from_vp, u16 to_vp, u16 Len)
{
	u16 buf_u16[0x1000];
	Read_Vp_byU16(from_vp, (unsigned char *)buf_u16, Len);
	Write_Vp_byU16(to_vp, (unsigned char *)buf_u16, Len);
}

// 读VP高字节
u8 Read_Vp_H(u16 Dgus_Addr)
{
	u8 buf_u8[2];
	Read_Vp_byU16(Dgus_Addr, buf_u8, 1);
	return buf_u8[0];
}

// 读VP低字节
u8 Read_Vp_L(u16 Dgus_Addr)
{
	u8 buf_u8[2];
	Read_Vp_byU16(Dgus_Addr, buf_u8, 1);
	return buf_u8[1];
}

// 写VP高字节
void Write_Vp_H(u16 Dgus_Addr, u8 Val)
{
	u8 buf_u8[2];
	Read_Vp_byU16(Dgus_Addr, buf_u8, 1);
	buf_u8[0] = Val;
	Write_Vp_byU16(Dgus_Addr, buf_u8, 1);
}

// 写VP低字节
void Write_Vp_L(u16 Dgus_Addr, u8 Val)
{
	u8 buf_u8[2];
	Read_Vp_byU16(Dgus_Addr, buf_u8, 1);
	buf_u8[1] = Val;
	Write_Vp_byU16(Dgus_Addr, buf_u8, 1);
}

// 获取VP值并清除VP值
u16 Get_Vp(u16 Dgus_Addr)
{
	u16 tmp_u16;
	tmp_u16 = Read_Vp(Dgus_Addr);
	Write_Vp(Dgus_Addr, 0);
	return tmp_u16;
}

// 获取VP高字节值并清除VP高字节值
u8 Get_Vp_H(u16 Dgus_Addr)
{
	u8 tmp_u8;
	tmp_u8 = Read_Vp_H(Dgus_Addr);
	Write_Vp_H(Dgus_Addr, 0);
	return tmp_u8;
}

// 获取VP低字节值并清除VP低字节值
u8 Get_Vp_L(u16 Dgus_Addr)
{
	u8 tmp_u8;
	tmp_u8 = Read_Vp_L(Dgus_Addr);
	Write_Vp_L(Dgus_Addr, 0);
	return tmp_u8;
}

// 获取VP特定位
u8 Get_Vp_Bit(u16 Dgus_Addr, u8 Bit)
{
	return get_bit_u16(Read_Vp(Dgus_Addr), Bit);
}

// 设置VP特定位
void Set_Vp_Bit(u16 Dgus_Addr, u8 Bit)
{
	Write_Vp(Dgus_Addr, set_bit_u16(Read_Vp(Dgus_Addr), Bit, 1));
}

// 清除VP特定位
void Clear_Vp_Bit(u16 Dgus_Addr, u8 Bit)
{
	Write_Vp(Dgus_Addr, set_bit_u16(Read_Vp(Dgus_Addr), Bit, 0));
}

// 页面切换
void page_change(u16 Page)
{
	Write_Vp(0x85, Page); 
	Write_Vp(0x84,0x5A01); 
	do
	{
		delay_us(500);
	} while(Read_Vp(0x84) & 0xFF00);
}