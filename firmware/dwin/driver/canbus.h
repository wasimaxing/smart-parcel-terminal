#ifndef __CANBUS_H__
#define __CANBUS_H__

#include "sys.h"

// #define CAN_T5L0

#define can_baud_125K 0
#define can_baud_250K 1
#define can_baud_500K 2
#define can_baud_1M 3

typedef struct _candataunit
{
	u32 ID;
	u8 status;//bit7   IDE  0表示标准帧  1表示扩展帧     bit6 RTR  0表示数据帧，1表示远程帧   bit3_0 发送数据长度，0-8.	
	u8 candata[8];
}CANUNIT;

typedef struct _candataunitbuf
{
	CANUNIT BusRXbuf[256];
	CANUNIT BusTXbuf[256];
	u8 CanRxHead,CanRxTail;
	u8 CanTxHead,CanTxTail,CanTxFlag;
}CANBUSUNIT;



extern CANBUSUNIT CanData;
#define CAN_frame_NUM 35

void CanBus_Init(u16 bdt);
void CAN_tx(u32 ID, u8 *str);
void CanRx(void);
void CanErrorReset(void);
void Can_test_func(void);

#endif