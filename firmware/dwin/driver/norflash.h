#ifndef __NORFLASH_H__
#define __NORFLASH_H__

#include "sys.h"
#include "dgus_tools.h"


#define NORFLASH_READ		(0x5a)
#define NORFLASH_WRITE		(0xa5)
#define CACHE_ADDR			(0xf000)	/* norflash读写转接地址 [注意：不要使用这片DGUS地址！！！] */

void T5L_Flash(unsigned char mod,unsigned int addr,long addr_flash,unsigned int len);
void norflash_read(u32 flash_addr,u8* buf,u16 len);
void norflash_write(u32 flash_addr,u8* buf,u16 len);

#endif

// 0x1000~0xFFFF 0x1000~0xF000

// 0x000000~0x03FFFF 512KB