#ifndef __DGUS_TOOLS_H__
#define __DGUS_TOOLS_H__

#include "sys.h"

/*****************************************
*			中断中是否访问VP      *
*****************************************/
// #define INTVPACTION   //CAN 通讯 需要 关中断

void Read_Vp_byU16(unsigned int addr,unsigned char *buf,unsigned int len);
void Write_Vp_byU16(unsigned int addr,unsigned char *buf,unsigned int len);
void Write_Vp_byU8(unsigned int addr,unsigned char *buf,unsigned int len);

u16  Read_Vp(u16 Dgus_Addr);
void Write_Vp(u16 Dgus_Addr,u16 Val);

void Clear_Vp_byU16(u16 Dgus_Addr,u16 Len);
void Copy_Vp_byU16(u16 from_vp, u16 to_vp, u16 Len);

u8 Read_Vp_H(u16 Dgus_Addr);
u8 Read_Vp_L(u16 Dgus_Addr);

void Write_Vp_H(u16 Dgus_Addr, u8 Val);
void Write_Vp_L(u16 Dgus_Addr, u8 Val);

u16 Get_Vp(u16 Dgus_Addr);
u8 Get_Vp_H(u16 Dgus_Addr);
u8 Get_Vp_L(u16 Dgus_Addr);

u8 Get_Vp_Bit(u16 Dgus_Addr, u8 Bit);
void Set_Vp_Bit(u16 Dgus_Addr, u8 Bit);
void Clear_Vp_Bit(u16 Dgus_Addr, u8 Bit);

void page_change(u16 Page);

#endif