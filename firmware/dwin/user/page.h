#ifndef __PAGE_H__
#define __PAGE_H__

#include "sys.h"

#define PAGE_ON  0xAA   // 页使能位
#define PAGE_OFF 0xBB   // 页失能位

// 页面数量
#define PAGE_NUM 8

typedef struct // 页面结构体
{
    u16 page_last;
    u16 page_now;
} s_page_t;

extern s_page_t s_page;

// 页处理函数指针
typedef void (*page_void)(void);

typedef struct // 页处理函数结构体
{
    unsigned char page_init_is_enable;    // 页初始化函数使能位
    unsigned char page_deal_is_enable;    // 页处理函数使能位
    unsigned char page_finish_is_enable;  // 页结束函数使能位
    page_void page_init;                  // 页初始化函数指针
    page_void page_deal;                  // 页处理函数指针
    page_void page_finish;                // 页结束函数指针
} s_page_deal_t;


void page_deal_init(void);
void page_deal_func(void);
#endif