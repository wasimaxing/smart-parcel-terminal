#include "dgus_8283.h"
#include "uart.h"
#include "log.h"

const u8 response_buf_crc16[] = {0x5A, 0xA5, 0X05, 0X82, 0X4F, 0X4B, 0XA5, 0XEF};
const u8 response_buf_nocrc16[] = {0x5A, 0xA5, 0X03, 0X82, 0X4F, 0X4B};

// 自动上传串口发送处理
void uart_auto_upload_deal(u16 vp_addr, u8 len)
{
    u16 send_flag, crc_flag, send_len, crc_count;
    u8 buf_u8[128];

    send_flag = DATA_UPLOAD_UART2 + DATA_UPLOAD_UART3 + DATA_UPLOAD_UART4 + DATA_UPLOAD_UART5;

    // 判断是否有串口允许自动上传
    if(send_flag)
    {
        buf_u8[0] = 0x5A;
        buf_u8[1] = 0xA5;
        buf_u8[2] = 4 + len * 2;
        buf_u8[3] = 0x83;
        buf_u8[4] = (u8)(vp_addr >> 8);
        buf_u8[5] = (u8)(vp_addr);
        buf_u8[6] = (u8)len;

        Read_Vp_byU16(vp_addr, &buf_u8[7], len);

        send_len = 7 + len * 2;

        crc_flag = CRC_CHECK_UART2 + CRC_CHECK_UART3 + CRC_CHECK_UART4 + CRC_CHECK_UART5;
        // 判断是否有串口需要CRC校验
        if(crc_flag)
        {
            crc_count = crc16table(&buf_u8[3], (4 + len * 2));
			buf_u8[2] = buf_u8[2] + 2;
            buf_u8[send_len + 1] = (u8)(crc_count >> 8);
            buf_u8[send_len] = (u8)(crc_count);
        }

#if UART2_8283_ENABLE
        uart_send_array(UART2, buf_u8, send_len + 2 * CRC_CHECK_UART2);
#endif
#if UART3_8283_ENABLE
        uart_send_array(UART3, buf_u8, send_len + 2 * CRC_CHECK_UART2);
#endif
#if UART4_8283_ENABLE
        uart_send_array(UART4, buf_u8, send_len + 2 * CRC_CHECK_UART2);
#endif
#if UART5_8283_ENABLE
        uart_send_array(UART5, buf_u8, send_len + 2 * CRC_CHECK_UART2);
#endif
    }
}

// 处理82指令
void deal_82_cmd(u8 uart_number, u8* rx_buf, u8 response_flag, u8 check_flag)
{
	// 带CRC校验
	if(check_flag)
	{
		u16 crc_rx, crc_count;
		crc_count = crc16table((u8*)(&rx_buf[3]), rx_buf[2] - 2);
		crc_rx = (u16)(rx_buf[3 + rx_buf[2] - 1] << 8) + (u16)(rx_buf[3 + rx_buf[2] - 2]);
		if(crc_rx == crc_count)
		{
			// 更新c代码(不清楚如何实现的,先保留,不稳定)
			if(rx_buf[4] == 0 && rx_buf[5] == 6)
			{
				rx_buf[7] = 0xA5;
			}
			Write_Vp_byU8((rx_buf[4] << 8) + rx_buf[5], rx_buf + 6, rx_buf[2] - 5);

			if(response_flag)
			{
				uart_send_array(uart_number, response_buf_crc16, 8);
			}
		}
	}
	//不含CRC检验
	else
	{
		// 更新c代码(不清楚如何实现的,先保留,不稳定)
		if(rx_buf[4] == 0 && rx_buf[5] == 6)
		{
			rx_buf[7] = 0xA5;
		}
		Write_Vp_byU8((rx_buf[4] << 8) + rx_buf[5], rx_buf + 6, rx_buf[2] - 3);

		if(response_flag)
		{
			uart_send_array(uart_number, response_buf_nocrc16, 6);
		}
	}
}

// 处理83指令
void deal_83_cmd(u8 uart_number, u8* rx_buf, u8 check_flag)
{
	if(check_flag)
	{
		// rx	5AA5 06 83 000F 01ED 90
		// tx	5A A5 08 83 00 0F 01 14 10 43 F0
		u16 crc_rx, crc_count;
		crc_count = crc16table((u8*)(&rx_buf[3]), rx_buf[2] - 2);
		crc_rx = (u16)(rx_buf[3 + rx_buf[2] - 1] << 8) + (u16)(rx_buf[3 + rx_buf[2] - 2]);

		if(crc_rx == crc_count)
		{
			Read_Vp_byU16((rx_buf[4] << 8) + rx_buf[5], (u8*)&rx_buf[7], rx_buf[6]);

			crc_count = crc16table((u8*)(&rx_buf[3]), 4 + rx_buf[6] * 2);

			rx_buf[7 + rx_buf[6] * 2 + 1] = (u8)(crc_count >> 8);
			rx_buf[7 + rx_buf[6] * 2] = (u8)(crc_count);

			rx_buf[2] = 7 + rx_buf[6] * 2 + 2 - 3;

			uart_send_array(uart_number, rx_buf, 7 + rx_buf[6] * 2 + 2);
		}
	}
	else
	{
		Read_Vp_byU16((rx_buf[4] << 8) + rx_buf[5], (u8*)&rx_buf[7], rx_buf[6]);

		rx_buf[2] = 7 + rx_buf[6] * 2 - 3;

		uart_send_array(uart_number, rx_buf, 7 + rx_buf[6] * 2);
	}
}
