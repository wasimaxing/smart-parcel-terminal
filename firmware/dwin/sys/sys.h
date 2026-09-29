#ifndef __SYS_H__
#define __SYS_H__

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include "stdarg.h"
#include "t5los8051.h"
#include "sys.h"

/************************************* 数据类型定义 ************************************/
typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned long   u32;
typedef char            s8;
typedef short           s16;
typedef long            s32;


/*************************************** 宏定义 ***************************************/
#define	WDT_ON()	MUX_SEL|=0x02		/* 开启看门狗 */
#define	WDT_OFF()	MUX_SEL&=0xFD		/* 关闭看门狗 */
#define	WDT_RST()	MUX_SEL|=0x01		/* 喂狗 */

// #define T5L_2K

#ifdef T5L_2K
    #define FOSC     383385600UL            /* 主频 */
#else
    #define FOSC     206438400UL            /* 主频 */
#endif

#define T1MS    (65536-FOSC/12/1000)

/************************************* 函数声明 **************************************/

void INIT_CPU(void);
void PORT_Init(void);
#endif

