#ifndef __DGUS_8283_H__
#define __DGUS_8283_H__

#include "sys.h"

void uart_auto_upload_deal(u16 vp_addr, u8 len);
void deal_82_cmd(u8 uart_number, u8* rx_buf, u8 response_flag, u8 check_flag);
void deal_83_cmd(u8 uart_number, u8* rx_buf, u8 check_flag);
#endif