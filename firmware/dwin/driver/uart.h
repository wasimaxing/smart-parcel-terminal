#ifndef __UART_H__
#define __UART_H__

#include "sys.h"
#include "crc16.h"
#include "dgus_tools.h"

#define UART2			(2)
#define UART3			(3)
#define UART4			(4)
#define UART5			(5)

// 串口接收缓存区大小
#define UART2_RX_SIZE (0x400)
#define UART3_RX_SIZE (0x400)
#define UART4_RX_SIZE (0x400)
#define UART5_RX_SIZE (0x400)

typedef struct
{
	u8 buf[UART2_RX_SIZE];
	u16 head;
	u16 tail;
}uart2_rx_t;

typedef struct
{
	u8 buf[UART3_RX_SIZE];
	u16 head;
	u16 tail;
}uart3_rx_t;

typedef struct
{
	u8 buf[UART4_RX_SIZE];
	u16 head;
	u16 tail;
}uart4_rx_t;

typedef struct
{
	u8 buf[UART5_RX_SIZE];
	u16 head;
	u16 tail;
}uart5_rx_t;

#if UART2_ENABLE
extern uart2_rx_t uart2;
#endif
#if UART3_ENABLE
extern uart3_rx_t uart3;
#endif
#if UART4_ENABLE
extern uart4_rx_t uart4;
#endif
#if UART5_ENABLE
extern uart5_rx_t uart5;
#endif


void uart_handle_all(void);
void Read_0xF00();
void uart_init();

void uart_send_byte(u8 uart_number,u8 Dat);
void uart_send_array(u8 uart_number,u8* Str,u16 Len);

void uart2_buf_to_temp_buf(u8* buf, u16 len);
void uart3_buf_to_temp_buf(u8* buf, u16 len);
void uart4_buf_to_temp_buf(u8* buf, u16 len);
void uart5_buf_to_temp_buf(u8* buf, u16 len);

#endif