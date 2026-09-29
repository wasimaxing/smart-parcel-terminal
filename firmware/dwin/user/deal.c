#include "deal.h"
#include "dgus_tools.h"
#include "log.h"
const u8 t_str[]="你好hello";
//在文�?显示区域显示hello
/*void hello(void) {
    u8 buf_w[64];
    memset(buf_w,0xff,sizeof(buf_w));
    memcpy(buf_w,t_str,sizeof(t_str));
    printHexArray(buf_w,64);

    Write_Vp_byU8(0x1000,buf_w,sizeof(t_str));
	
}*/


const u8 test_str[] = "���1234567!";

// 设置文本控件位置
void set_text_pos(u16 sp_addr, s16 location_x, s16 location_y, u16 width, u16 height)
{
    s16 buf_s16[4];
    buf_s16[0] = location_x;
    buf_s16[1] = location_y;
    Write_Vp_byU16((u16)(sp_addr + 0x01), (u8*)buf_s16, 2);

    buf_s16[2] = buf_s16[0] + width;
    buf_s16[3] = buf_s16[1] + height;

    Write_Vp_byU16((u16)(sp_addr + 0x04), (u8*)buf_s16, 4);
}


void test_init(void)
{
    u8 buf_w[64];

    memset(buf_w, 0xFF, sizeof(buf_w));

    memcpy(buf_w, test_str, sizeof(test_str));

    printHexArray(buf_w, 64);

    Write_Vp_byU16(0x1000, buf_w, sizeof(test_str));

    set_text_pos(0x2000, 0, 0, 268, 74);

    // dht11_init();
}

void test_func(void)
{    
    static s16 location_x = 0;
    static s16 location_y = 0;

    location_x += 1;
    location_y += 1;

    if(location_x >= 600) location_x = 0;

    if(location_y >= 400) location_y = 0;

    if(Read_Vp(0x0015) != 0x0000) return;

    set_text_pos(0x2000, location_x, location_y, 268, 74);

    
}