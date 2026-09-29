#include "save_data_dgus.h"
#include "log.h"


#define	SAVE_NUMBER                 0           // 需要保存的地址数量
#define INITIALIZATION_MARK 	    0XABCD      // 初始化标记
#define INITIALIZATION_MARK_ADDR    0x0000      // norflash中初始化标记存放地址

// 配置需要保存的地址以及第一次上电的初始值(需要从小到大排列)
static Save_Data_t Save_Data[] = 
{
	{0x2010,0},
	{0x2011,1},
	{0x2013,1},
	{0x1004,4},
	{0x1005,5},
	{0x1006,6},
	{0x1007,7},
	{0x1008,0},
	{0x1009,0},
	{0x100A,0},
	{0x100B,0},
	{0x100C,0},
	{0x100D,0},
	{0x100E,0},
	{0x100F,0},
	{0x1010,0},
	{0x1011,0},
	{0x1012,0},
	{0x1013,0}
};

// 初始化数据:第一次启动将初始值加载进片内flash，后续每次启动从片内flash读取数据
void data_save_init(void)
{
	u32 init_mark;
	u16 i;
	u32 norflash_addr;

	norflash_read(INITIALIZATION_MARK_ADDR, (u8*)&init_mark, 2);

	if(init_mark!= INITIALIZATION_MARK)
	{
		// 第一次启动，将初始标记写入片内flash
		init_mark = INITIALIZATION_MARK;
		norflash_write(INITIALIZATION_MARK_ADDR, (u8*)&init_mark, 2);

		// 写入初始值
		for(i = 0; i < SAVE_NUMBER; i++)
		{
			Write_Vp(Save_Data[i].Address, Save_Data[i].Data);

			Write_Vp(CACHE_ADDR, Save_Data[i].Data);
			Write_Vp(CACHE_ADDR + 1, 0);

			// 地址做放大处理
			norflash_addr = (u32)((u32)Save_Data[i].Address * 2);
			T5L_Flash(NORFLASH_WRITE, CACHE_ADDR, norflash_addr, 2);
		}
	}
	else
	{
		// 后续启动，从片内flash读取数据
		for(i = 0; i < SAVE_NUMBER; i++)
		{
			norflash_addr = (u32)((u32)Save_Data[i].Address * 2);
			// 先读到缓存区,因为一次会读出来32位,但只需要16位
			T5L_Flash(NORFLASH_READ, CACHE_ADDR, norflash_addr, 2);
			Save_Data[i].Data = Read_Vp(CACHE_ADDR);
			Write_Vp(Save_Data[i].Address, Save_Data[i].Data);

			LOG_DEBUG("vp_addr:0x%h04x, data:0x%h04x\n", Save_Data[i].Address, Save_Data[i].Data);
		}
	}
}

// 改变数据:将改变后的数据保存到片内flash
void data_change_sava(void)
{
	u16 i, tmp_u16;
	u32 norflash_addr;

	// 读取触摸状态为抬起时才保存数据
	if(Read_Vp_L(0x16) != 0x02)
		return;
	for(i = 0; i < SAVE_NUMBER; i++)
	{
		tmp_u16 = Read_Vp(Save_Data[i].Address);
		if(Save_Data[i].Data != tmp_u16)
		{
			Save_Data[i].Data = tmp_u16;

			// 写到缓存地址
			Write_Vp(CACHE_ADDR, Save_Data[i].Data);
			Write_Vp(CACHE_ADDR + 1, 0);

			// 地址做放大处理
			norflash_addr = (u32)((u32)Save_Data[i].Address * 2);
			T5L_Flash(NORFLASH_WRITE, CACHE_ADDR, norflash_addr, 2);
		}
	}
}
