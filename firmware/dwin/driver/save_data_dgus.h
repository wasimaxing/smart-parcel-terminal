#ifndef __SAVE_DATA_DGUS_H__
#define __SAVE_DATA_DGUS_H__

#include "sys.h"
#include "uart.h"
#include "dgus_tools.h"
#include "norflash.h"

// 存放掉电存储的地址和数据
typedef struct
{
	u16 Address;	// 地址
	u16 Data;		// 数据 && 初始值
}Save_Data_t;

void data_save_init();
void data_change_sava();
#endif 
