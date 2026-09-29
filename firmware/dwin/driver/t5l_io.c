#include "t5l_io.h"

// 设置引脚为输出模式
void SetPinOut(u8 Port,u8 Pin)
{
    switch(Port)
    {
        case 0: P0MDOUT|=(1<<Pin);break;        
        case 1: P1MDOUT|=(1<<Pin);break;
        case 2: P2MDOUT|=(1<<Pin);break;
        case 3: P3MDOUT|=(1<<Pin);break;
        default:break;
    }
}

// 设置引脚为输入模式
void SetPinIn(u8 Port,u8 Pin)
{
    switch(Port)
    {
        case 0: P0MDOUT&=~(1<<Pin);break;
        case 1: P1MDOUT&=~(1<<Pin);break;
        case 2: P2MDOUT&=~(1<<Pin);break;
        case 3: P3MDOUT&=~(1<<Pin);break;
        default:break;
    }
}
