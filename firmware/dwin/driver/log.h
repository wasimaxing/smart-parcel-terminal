#ifndef __LOG_H__
#define __LOG_H__
#include "sys.h"

// 定义日志等级
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_NONE  // 不输出任何日志
} LogLevel;

#define CURRENT_LOG_LEVEL LOG_LEVEL_NONE

void LOG_DEBUG(const char* fmt, ...);
void LOG_INFO(const char* fmt, ...);
void LOG_WARN(const char* fmt, ...);
void LOG_ERROR(const char* fmt, ...);

void printHexArray(unsigned char *buffer,int len);
#endif