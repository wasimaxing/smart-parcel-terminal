#include "log.h"
#include "uart.h"
#include <stdio.h>
#include <stdarg.h>

// 日志缓存大小
#define LOG_BUFFER_SIZE 1024

const LogLevel current_log = CURRENT_LOG_LEVEL;

char putchar(char ch)
{
    uart_send_byte(DEBUG_UART,ch);
    return ch;
}

// 打印十六进制数组
void printHexArray(unsigned char *buffer,int len)
{
    u16 i;
    printf("\n");
	printf("DATA_LEN = %d\n",len);

    for(i = 0; i < len; i++)
    {
		if(i%32==0)
        {
			printf("\n");
		}
		printf("%02bx ",(u8)buffer[i]);
	}
	printf("\n");
}

// 日志等级对应的字符串
const char* log_level_strings[] = {
    "[DEBUG]",
    "[-INFO]",
    "[-WARN]",
    "[ERROR]"
};

void LOG_DEBUG(const char* fmt, ...)
{
    if (LOG_LEVEL_DEBUG < current_log)
    {
        return; // 如果当前等级低于设置的等级，则不打印
    }
    else
    {
        char buffer[LOG_BUFFER_SIZE];
        u16 i;
        va_list args;
        va_start(args, fmt);
        vsprintf(buffer, fmt, args);
        va_end(args);
        
        uart_send_array(DEBUG_UART,log_level_strings[0],7);

        for (i = 0; buffer[i] != '\0'; i++)
        {
            uart_send_byte(DEBUG_UART,buffer[i]);
        }
    }
}

void LOG_INFO(const char* fmt, ...)
{
    if (LOG_LEVEL_INFO < current_log)
    {
        return; // 如果当前等级低于设置的等级，则不打印
    }
    else
    {
        char buffer[LOG_BUFFER_SIZE];
        u16 i;
        va_list args;
        va_start(args, fmt);
        vsprintf(buffer, fmt, args);
        va_end(args);
        
        uart_send_array(DEBUG_UART,log_level_strings[1],7);

        for (i = 0; buffer[i] != '\0'; i++)
        {
            uart_send_byte(DEBUG_UART,buffer[i]);
        }
    }
}

void LOG_WARN(const char* fmt, ...)
{
    if (LOG_LEVEL_WARN < current_log)
    {
        return; // 如果当前等级低于设置的等级，则不打印
    }
    else
    {
        char buffer[LOG_BUFFER_SIZE];
        u16 i;
        va_list args;
        va_start(args, fmt);
        vsprintf(buffer, fmt, args);
        va_end(args);
        
        uart_send_array(DEBUG_UART,log_level_strings[2],7);

        for (i = 0; buffer[i] != '\0'; i++)
        {
            uart_send_byte(DEBUG_UART,buffer[i]);
        }
    }
}

void LOG_ERROR(const char* fmt, ...)
{
    if (LOG_LEVEL_ERROR < current_log)
    {
        return; // 如果当前等级低于设置的等级，则不打印
    }
    else
    {
        char buffer[LOG_BUFFER_SIZE];
        u16 i;
        va_list args;
        va_start(args, fmt);
        vsprintf(buffer, fmt, args);
        va_end(args);
        
        uart_send_array(DEBUG_UART,log_level_strings[3],7);

        for (i = 0; buffer[i] != '\0'; i++)
        {
            uart_send_byte(DEBUG_UART,buffer[i]);
        }
    }
}