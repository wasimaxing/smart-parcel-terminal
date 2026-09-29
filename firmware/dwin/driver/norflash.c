#include "norflash.h"

//检查指定DGUS变量地址的DATA3清零的话就退出
static void wait_ok(unsigned int addr)
{	
	ADR_H=0x00;
	ADR_M=(unsigned char)(addr>>8);
	ADR_L=(unsigned char)(addr);
	ADR_INC=0x00;
	do
	{ 
		for(addr=0;addr<1000;addr++)	//释放变量空间一段时间
			DATA2=DATA1;
		RAMMODE=0xAF;
		while(APP_ACK==0);
		APP_EN=1;
		while(APP_EN==1);
		RAMMODE=0x00;
	}	
	while(DATA3!=0);
}

/*****************************************************************************
 函 数 名  : T5L_Flash
 功能描述  : 读取Flash的值到DGUS地址、或 将DGUS地址的值写入Flash
 输入参数  : mod=读或写模式  addr=DGUS地址  addr_flash=Flash地址  len=读或写长度
 输出参数  : None
*****************************************************************************/
void T5L_Flash(unsigned char mod,unsigned int addr,long addr_flash,unsigned int len)
{	
	ADR_H=0x00;
	ADR_M=0x00;
	ADR_L=0x04;
	ADR_INC=0x01;
	RAMMODE=0x8F;		//启动读Flash
	while(APP_ACK==0);
	DATA3=mod;
	DATA2=(unsigned char)(addr_flash>>16);
	DATA1=(unsigned char)(addr_flash>>8);
	DATA0=(unsigned char)(addr_flash&0xFE);
	APP_EN=1;
	while(APP_EN==1);	
	DATA3=(unsigned char)(addr>>8);
	DATA2=(unsigned char)(addr&0xFE);
	DATA1=(unsigned char)(len>>8);
	DATA0=(unsigned char)(len&0xFE);
	APP_EN=1;
	while(APP_EN==1);
	RAMMODE=0x00;
	wait_ok(0x0004);
}	//等待数据读取OK

/*****************************************************************************
 函 数 名  : norflash_read
 功能描述  : 读取flash
 输入参数  : flash_addr=flash地址  buf=读数据存放地址  len=读数据长度（必须是偶数）
 输出参数  : None
*****************************************************************************/
void norflash_read(u32 flash_addr,u8* buf,u16 len)
{
	T5L_Flash(NORFLASH_READ,CACHE_ADDR,flash_addr,len);// 将flash读到的数据先入转换地址CACHE_ADDR
	Read_Vp_byU16(CACHE_ADDR,buf,len);// 读转换地址CACHE_ADDR的数据到buf
}
/*****************************************************************************
 函 数 名  : norflash_write
 功能描述  : 写入flash
 输入参数  : flash_addr=flash地址  buf=写数据存放地址  len=写数据长度（必须是偶数）
 输出参数  : None
*****************************************************************************/
void norflash_write(u32 flash_addr,u8* buf,u16 len)
{
	Write_Vp_byU16(CACHE_ADDR,buf,len);
	T5L_Flash(NORFLASH_WRITE,CACHE_ADDR,flash_addr,len);
}