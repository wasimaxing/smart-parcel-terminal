#ifndef __OTHER_TOOLS_H__
#define __OTHER_TOOLS_H__

#include "sys.h"

u8 get_bit_u8(u8 temp16, u8 bit_position);
u8 set_bit_u8(u8 value, u8 bit_position, u8 bit_value);
u8 toggle_bit_u8(u8 value, u8 bit_position);

u8 get_bit_u16(u16 temp16, u8 bit_position);
u16 set_bit_u16(u16 value, u8 bit_position, u8 bit_value);
u16 toggle_bit_u16(u16 value, u8 bit_position);

#endif