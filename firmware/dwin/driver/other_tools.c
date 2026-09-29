#include "other_tools.h"

// 读取特定u8类型数据特定位
u8 get_bit_u8(u8 temp16, u8 bit_position) 
{
    // 使用位掩码检查特定位是否为1
    return (temp16 & (1 << bit_position)) != 0;
}

// 设置u8类型的数据中的特定位
u8 set_bit_u8(u8 value, u8 bit_position, u8 bit_value) 
{
    // 异常直接返回
    if(bit_position > 7)
        return value;
    // 使用位掩码置位或清位特定位
    if (bit_value == 1) 
	{
        return value | (1 << bit_position); // 置位
    } 
	else 
	{
        return value & ~(1 << bit_position); // 清位
    }
}

// 取反u8类型的数据中的特定位
u8 toggle_bit_u8(u8 value, u8 bit_position)
{
    // 使用位掩码取反特定位
    return value ^ (1 << bit_position);
}

// 读取特定u16类型数据特定位
u8 get_bit_u16(u16 temp16, u8 bit_position) 
{
    // 使用位掩码检查特定位是否为1
    return (temp16 & (1 << bit_position)) != 0;
}

// 设置u16类型的数据中的特定位
u16 set_bit_u16(u16 value, u8 bit_position, u8 bit_value) 
{
    // 异常直接返回
    if(bit_position > 15)
        return value;
    // 使用位掩码置位或清位特定位
    if (bit_value == 1) 
	{
        return value | (1 << bit_position); // 置位
    } 
	else 
	{
        return value & ~(1 << bit_position); // 清位
    }
}

// 取反u16类型的数据中的特定位
u16 toggle_bit_u16(u16 value, u8 bit_position)
{
    // 使用位掩码取反特定位
    return value ^ (1 << bit_position);
}

