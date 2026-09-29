#include "Uart.h"
#include "dgus_8283.h"

#if UART2_ENABLE
uart2_rx_t uart2;
#endif
#if UART3_ENABLE
uart3_rx_t uart3;
#endif
#if UART4_ENABLE
uart4_rx_t uart4;
#endif
#if UART5_ENABLE
uart5_rx_t uart5;
#endif

#if UART2_ENABLE
// 串口2初始化
void uart2_Init(u32 baud_rate)
{
	u16 bode_div_cache;

	// 串口2数据初始化
	uart2.head = uart2.tail = 0;
	memset(uart2.buf, 0, UART2_RX_SIZE);

#if UART2_SMOD_ENABLE
	
	PCON |= 0x80;								// 启用串口2的倍速模式
	bode_div_cache = 1024-FOSC/32/baud_rate;	// bode_div_cache = 1024 - FOSC/32/波特率
#else
	PCON &= 0x7F;								// 关闭串口2的倍速模式
	bode_div_cache = 1024-FOSC/64/baud_rate;	// bode_div_cache = 1024 - FOSC/64/波特率
#endif

	SREL0H = (u8)(bode_div_cache>>8);		
	SREL0L = (u8)bode_div_cache;

	MUX_SEL |= 0x40;	// 引出uart2到P0.4,P0.5

	// P0.4 TXD 推挽
	P0MDOUT &= 0xCF;	// 1100 1111
	P0MDOUT |= 0x10;	// 0001 0000

	ADCON = 0x80;			//UART2 8N1 115200
	SCON0 = 0x50;

	ES0 = 1;
	EA=1;	
}
#endif

#if UART3_ENABLE
// 串口3初始化
void uart3_Init(u32 baud_rate)
{
	u16 bode_div_cache;

	// 串口3数据初始化
	uart3.head = uart3.tail = 0;
	memset(uart3.buf, 0, UART3_RX_SIZE);

	bode_div_cache = 1024-FOSC/32/baud_rate;	// bode_div_cache = 1024 - FOSC/32/波特率
	SREL1H = (u8)(bode_div_cache>>8);		
	SREL1L = (u8)bode_div_cache;

	MUX_SEL |= 0x20;    	// UART3引出到P0.6,P0.7

	// P0.6 TXD 推挽 
	P0MDOUT &= 0x3F;		// 0011 1111
	P0MDOUT |= 0x40;	  	// 0100 0000
	SCON1 = 0x90;     		// 接受使能和模式设置
	IEN2 |= 0x01;	  		// 中断使能

	EA  = 1;
}
#endif

#if UART4_ENABLE
// 串口4初始化
void uart4_Init(u32 baud_rate)
{
	u16 bode_div_cache;

	// 串口4数据初始化
	uart4.head = uart4.tail = 0;
	memset(uart4.buf, 0, UART4_RX_SIZE);

#if UART4_485_ENABLE
	// 启用485
	P0MDOUT |= 0x01;
	TR4 = 0;
#endif

	// 设置波特率
#ifdef T5L_2K
	bode_div_cache = FOSC/16/baud_rate;		// bode_div_cache = CPU 主频/(16*波特率) 
#else
	bode_div_cache = FOSC/8/baud_rate;		// bode_div_cache = CPU 主频/(8*波特率) 
#endif
	BODE2_DIV_H = (u8)(bode_div_cache>>8);
	BODE2_DIV_L = (u8)bode_div_cache;

	// 配置传输方式和中断
	SCON2T= 0x80;							// 发送使能、8bit Mode
	SCON2R= 0x80;							// 接收使能
	ES2R = 1;								// 中断接收使能
  	ES2T = 1;								// 中断发送使能

	EA=1;	
}
#endif

#if UART5_ENABLE
// 串口5初始化
void uart5_Init(u32 baud_rate)
{
	u16 bode_div_cache;

	// 串口5数据初始化
	uart5.head = uart5.tail = 0;
	memset(uart5.buf, 0, UART5_RX_SIZE);

#if UART5_485_ENABLE
	// 启用485
	P0MDOUT |= 0x02;
	TR5 = 0;
#endif

	// 设置波特率
#ifdef T5L_2K
	bode_div_cache = FOSC/16/baud_rate;		// bode_div_cache = CPU 主频/(16*波特率) 
#else
	bode_div_cache = FOSC/8/baud_rate;		// bode_div_cache = CPU 主频/(8*波特率) 
#endif
	BODE3_DIV_H = (u8)(bode_div_cache>>8);
	BODE3_DIV_L = (u8)bode_div_cache;

	// 配置传输方式和中断
	SCON3T= 0x80;							// 发送使能、8bit Mode
	SCON3R= 0x80;							// 接收使能
	ES3R = 1;								// 中断接收使能
  	ES3T = 1;								// 中断发送使能

	EA=1;	
}
#endif

void uart_init(void)
{
#if UART2_ENABLE
	uart2_Init(BAUD_UART2);
#endif	
#if UART3_ENABLE
	uart3_Init(BAUD_UART3);
#endif	
#if UART4_ENABLE
	uart4_Init(BAUD_UART4);
#endif	
#if UART5_ENABLE
	uart5_Init(BAUD_UART5);
#endif	
}

#if UART2_ENABLE
// 串口2发送单个字符
void uart2_send_byte(char Data)
{
	SBUF0 = Data;
	while (!TI0)
		;
	TI0 = 0;
}
#endif

#if UART3_ENABLE
// 串口3发送单个字符
void uart3_send_byte(char Data)
{
	SBUF1 = Data;
	while ((SCON1 & 0x02) == 0x00)
		;
	SCON1 &= 0xFD; // 需写两次
	SCON1 &= 0xFD;
}
#endif

#if UART4_ENABLE
// 串口4发送单个字符
void uart4_send_byte(char Data)
{
#if UART4_485_ENABLE
	TR4 = 1;
#endif

	SBUF2_TX = Data;
	while ((SCON2T & 0x01) == 0x00)
		;
	SCON2T &= 0xFE;

#if UART4_485_ENABLE
	TR4 = 0;
#endif
}
#endif

#if UART5_ENABLE
// 串口5发送单个字符
void uart5_send_byte(char Data)
{
#if UART5_485_ENABLE
	TR5 = 1;
#endif

	SBUF3_TX = Data;
	while ((SCON3T & 0x01) == 0x00)
		;
	SCON3T &= 0xFE;

#if UART5_485_ENABLE
	TR5 = 0;
#endif
}
#endif

// 无发送
void uart_send_null(char Data)
{
	Data = Data;
}

// 串口发送单字符函数指针
typedef void (*UART_SEND_BYTE)(char);
UART_SEND_BYTE uart_send_byte_func[6] = {
	uart_send_null,
	uart_send_null,
#if UART2_ENABLE
	uart2_send_byte,
#else
	uart_send_null,
#endif
#if UART3_ENABLE
	uart3_send_byte,
#else
	uart_send_null,
#endif
#if UART4_ENABLE
	uart4_send_byte,
#else
	uart_send_null,
#endif
#if UART5_ENABLE
	uart5_send_byte,
#else
	uart_send_null,
#endif
};

// 串口发送单个字符函数封装,根据uart_number选择串口发送
void uart_send_byte(u8 uart_number,u8 Dat)
{
	uart_send_byte_func[uart_number](Dat);
}

// 串口发送数组函数封装,根据Uart_number选择串口发送
void uart_send_array(u8 uart_number,u8* Str,u16 Len)
{
	u16 i=0;

	for(i = 0; i < Len; i++)
	{
		uart_send_byte(uart_number,*Str++);
	}
}

#if UART2_ENABLE
// 串口2循环队列数据读到临时数组，并且队列头指针向后移
void uart2_buf_to_temp_buf(u8* buf, u16 len)
{
    u16 i;
    for(i = 0; i < len; i++)
    {
        buf[i] = uart2.buf[uart2.head];
        uart2.head ++;
        uart2.head %= UART2_RX_SIZE;
    }
}
#endif

#if UART3_ENABLE
// 串口3循环队列数据读到临时数组，并且队列头指针向后移
void uart3_buf_to_temp_buf(u8* buf, u16 len)
{
    u16 i;
    for(i = 0; i < len; i++)
    {
        buf[i] = uart3.buf[uart3.head];
        uart3.head ++;
        uart3.head %= UART3_RX_SIZE;
    }
}
#endif

#if UART4_ENABLE
// 串口4循环队列数据读到临时数组，并且队列头指针向后移
void uart4_buf_to_temp_buf(u8* buf, u16 len)
{
    u16 i;
    for(i = 0; i < len; i++)
    {
        buf[i] = uart4.buf[uart4.head];
        uart4.head ++;
        uart4.head %= UART4_RX_SIZE;
    }
}
#endif

#if UART5_ENABLE
// 串口5循环队列数据读到临时数组，并且队列头指针向后移
void uart5_buf_to_temp_buf(u8* buf, u16 len)
{
    u16 i;
    for(i = 0; i < len; i++)
    {
        buf[i] = uart5.buf[uart5.head];
        uart5.head ++;
        uart5.head %= UART5_RX_SIZE;
    }
}
#endif


#if UART2_ENABLE
// 串口2接收处理
void uart2_rx_deal(void)
{
	u16 tail_tmp;

	EA = 0;
	tail_tmp = uart2.tail;
	EA = 1;

	// 判断是否队空
	if(uart2.head != tail_tmp)
	{
		u8 buf[UART2_RX_SIZE];
		u16 uart_len, data_len, frame_head, cmd;

		// 计算接收长度
		uart_len = (uart2.tail + UART2_RX_SIZE - uart2.head) % UART2_RX_SIZE;

		// 接收数据大于一定长度再处理
		if(uart_len < 6)
		{
			return;
		}
		frame_head = (uart2.buf[uart2.head] << 8) | (uart2.buf[(uart2.head + 1) % UART2_RX_SIZE]);
		// 判断头部
		if(frame_head == 0x5AA5)
		{
			// 获取一个数据帧长度
			data_len = uart2.buf[(uart2.head + 2) % UART2_RX_SIZE] + 3;
			// 获取命令
			cmd = uart2.buf[(uart2.head + 3) % UART2_RX_SIZE];

			// 根据不同命令处理
			if(cmd == 0x82)
			{
				if(data_len <= uart_len)
				{
					uart2_buf_to_temp_buf(buf, data_len);
					#if UART2_8283_ENABLE
					deal_82_cmd(UART2, buf, RESPONSE_UART2, CRC_CHECK_UART2);
					#endif
				}
			}
			else if(cmd == 0x83)
			{
				if(data_len <= uart_len)
				{
					uart2_buf_to_temp_buf(buf, data_len);
					#if UART2_8283_ENABLE
					deal_83_cmd(UART2, buf, CRC_CHECK_UART2);
					#endif
				}
			}
			else
			{
				// 没有搜索到对应的指令，继续搜寻
				uart2.head = (uart2.head + 1) % UART2_RX_SIZE;

				// LOG_WARN("undefine cmd : 0x%h04x\n", cmd);
			}
		}
		else
		{
			// 没有搜索到对应的帧头，继续搜寻
			uart2.head = (uart2.head + 1) % UART2_RX_SIZE;

			// LOG_WARN("frame head error : 0x%h04x\n", frame_head);
		}
	}
}
#endif

#if UART3_ENABLE
// 串口3接收处理
void uart3_rx_deal(void)
{
	u16 tail_tmp;

	EA = 0;
	tail_tmp = uart3.tail;
	EA = 1;

	// 判断是否队空
	if(uart3.head != tail_tmp)
	{
		u8 buf[UART3_RX_SIZE];
		u16 uart_len, data_len, frame_head, cmd;

		// 计算接收长度
		uart_len = (uart3.tail + UART3_RX_SIZE - uart3.head) % UART3_RX_SIZE;

		// 接收数据大于一定长度再处理
		if(uart_len < 6)
		{
			return;
		}

		frame_head = (uart3.buf[uart3.head] << 8) | (uart3.buf[(uart3.head + 1) % UART3_RX_SIZE]);

		if(frame_head == 0x5AA5)
		{
			data_len = uart3.buf[(uart3.head + 2) % UART3_RX_SIZE] + 3;

			cmd = uart3.buf[(uart3.head + 3) % UART3_RX_SIZE];

			if(cmd == 0x82)
			{
				if(data_len <= uart_len)
				{
					uart3_buf_to_temp_buf(buf, data_len);
					#if UART3_8283_ENABLE
					deal_82_cmd(UART3, buf, RESPONSE_UART3, CRC_CHECK_UART3);
					#endif
				}
			}
			else if(cmd == 0x83)
			{
				if(data_len <= uart_len)
				{
					uart3_buf_to_temp_buf(buf, data_len);
					#if UART3_8283_ENABLE
					deal_83_cmd(UART3, buf, CRC_CHECK_UART3);
					#endif
				}
			}
			else
			{
				// 没有搜索到对应的指令，继续搜寻
				uart3.head = (uart3.head + 1) % UART3_RX_SIZE;

				// LOG_WARN("undefine cmd : 0x%h04x\n", cmd);
			}
		}
		else
		{
			// 没有搜索到对应的帧头，继续搜寻
			uart3.head = (uart3.head + 1) % UART3_RX_SIZE;

			// LOG_WARN("frame head error : 0x%h04x\n", frame_head);
		}
	}
}
#endif

#if UART4_ENABLE
// 串口4接收处理
void uart4_rx_deal(void)
{
	u16 tail_tmp;

	EA = 0;
	tail_tmp = uart4.tail;
	EA = 1;

	// 判断是否队空
	if(uart4.head != tail_tmp)
	{
		u8 buf[UART4_RX_SIZE];
		u16 uart_len, data_len, frame_head, cmd;

		// 计算接收长度
		uart_len = (uart4.tail + UART4_RX_SIZE - uart4.head) % UART4_RX_SIZE;

		// 接收数据大于一定长度再处理
		if(uart_len < 6)
		{
			return;
		}

		frame_head = (uart4.buf[uart4.head] << 8) | (uart4.buf[(uart4.head + 1) % UART4_RX_SIZE]);

		if(frame_head == 0x5AA5)
		{
			data_len = uart4.buf[(uart4.head + 2) % UART4_RX_SIZE] + 3;

			cmd = uart4.buf[(uart4.head + 3) % UART4_RX_SIZE];

			if(cmd == 0x82)
			{
				if(data_len <= uart_len)
				{
					uart4_buf_to_temp_buf(buf, data_len);
					#if UART4_8283_ENABLE
					deal_82_cmd(UART4, buf, RESPONSE_UART4, CRC_CHECK_UART4);
					#endif
				}
			}
			else if(cmd == 0x83)
			{
				if(data_len <= uart_len)
				{
					uart4_buf_to_temp_buf(buf, data_len);
					#if UART4_8283_ENABLE
					deal_83_cmd(UART4, buf, CRC_CHECK_UART4);
					#endif
				}
			}
			else
			{
				// 没有搜索到对应的指令，继续搜寻
				uart4.head = (uart4.head + 1) % UART4_RX_SIZE;

				// LOG_WARN("undefine cmd : 0x%h04x\n", cmd);
			}
		}
		else
		{
			// 没有搜索到对应的帧头，继续搜寻
			uart4.head = (uart4.head + 1) % UART4_RX_SIZE;

			// LOG_WARN("frame head error : 0x%h04x\n", frame_head);
		}
	}
}
#endif

#if UART5_ENABLE
// 串口5接收处理
void uart5_rx_deal(void)
{
	u16 tail_tmp;

	EA = 0;
	tail_tmp = uart5.tail;
	EA = 1;

	// 判断是否队空
	if(uart5.head != tail_tmp)
	{
		u8 buf[UART5_RX_SIZE];
		u16 uart_len, data_len, frame_head, cmd;

		// 计算接收长度
		uart_len = (uart5.tail + UART5_RX_SIZE - uart5.head) % UART5_RX_SIZE;

		// 接收数据大于一定长度再处理
		if(uart_len < 6)
		{
			return;
		}

		frame_head = (uart5.buf[uart5.head] << 8) | (uart5.buf[(uart5.head + 1) % UART5_RX_SIZE]);

		if(frame_head == 0x5AA5)
		{
			data_len = uart5.buf[(uart5.head + 2) % UART5_RX_SIZE] + 3;

			cmd = uart5.buf[(uart5.head + 3) % UART5_RX_SIZE];

			if(cmd == 0x82)
			{
				if(data_len <= uart_len)
				{
					uart5_buf_to_temp_buf(buf, data_len);
					#if UART5_8283_ENABLE
					deal_82_cmd(UART5, buf, RESPONSE_UART5, CRC_CHECK_UART5);
					#endif
				}
			}
			else if(cmd == 0x83)
			{
				if(data_len <= uart_len)
				{
					uart5_buf_to_temp_buf(buf, data_len);
					#if UART5_8283_ENABLE
					deal_83_cmd(UART5, buf, CRC_CHECK_UART5);
					#endif
				}
			}
			else
			{
				// 没有搜索到对应的指令，继续搜寻
				uart5.head = (uart5.head + 1) % UART5_RX_SIZE;

				// LOG_WARN("undefine cmd : 0x%h04x\n", cmd);
			}
		}
		else
		{
			// 没有搜索到对应的帧头，继续搜寻
			uart5.head = (uart5.head + 1) % UART5_RX_SIZE;

			// LOG_WARN("frame head error : 0x%h04x\n", frame_head);
		}
	}
}
#endif

// 数据自动上传
void Read_0xF00(void)
{
	u8 buf_u8[128];
	u8 len;
	u16 vp_addr;

	Read_Vp_byU16(0xF00, buf_u8, 2);

	// 判断标志位
	if(buf_u8[0] == 0x5A)
	{
		// 读取触发的地址和变量长度(字)
		vp_addr = (buf_u8[1]<<8) | buf_u8[2];
		len = buf_u8[3];
		
		// 根据地址做特殊处理
		switch(vp_addr)
		{
			// case 0x0000:

			default:break;
		}

#if UART2_8283_ENABLE+UART3_8283_ENABLE+UART4_8283_ENABLE+UART5_8283_ENABLE
		// 8283串口自动上传处理函数
		uart_auto_upload_deal(vp_addr, len);
#endif

		Clear_Vp_byU16(0xF00, 1);
	}
	
}

// 串口处理函数
void uart_handle_all(void)
{
#if UART2_ENABLE
	uart2_rx_deal();
#endif
#if UART3_ENABLE
	uart3_rx_deal();
#endif
#if UART4_ENABLE && UART4_8283_ENABLE
	uart4_rx_deal();
#endif
#if UART5_ENABLE
	uart5_rx_deal();
#endif
}
