#ifndef __PARAMETER_CONFIG_H__
#define __PARAMETER_CONFIG_H__

#define	      SELECT_RTC_TYPE			0		        // 不启用OS核I2C RTC驱动；由STC板DS1302同步DGUS RTC区

#define       DEBUG_UART                3               // 调试串口

#define	      UART2_ENABLE              1               // T5L  串口2开启关闭设置，1开启，0关闭
#define	      UART3_ENABLE              1               // T5L  串口3开启关闭设置，1开启，0关闭
#define	      UART4_ENABLE              1               // T5L  串口4开启关闭设置，1开启，0关闭
#define	      UART5_ENABLE              1               // T5L  串口5开启关闭设置，1开启，0关闭

#define	      UART2_8283_ENABLE         1               // T5L  串口2 8283使能
#define	      UART3_8283_ENABLE         1               // T5L  串口3 8283使能
#define	      UART4_8283_ENABLE         0               // UART4 专用于项目 RS-485 帧
#define	      UART5_8283_ENABLE         1               // T5L  串口5 8283使能

#define       UART2_SMOD_ENABLE	        1               // T5L  串口2波特率倍速
#define       UART4_485_ENABLE	        1               // T5L  串口4 485使能
#define       UART5_485_ENABLE	        1               // T5L  串口5 485使能

#define       BAUD_UART2	            115200          // T5L  串口2波特率设置
#define       BAUD_UART3	            115200          // T5L  串口3波特率设置
#define       BAUD_UART4	            9600            // 与 STC 板载 RS-485 一致
#define       BAUD_UART5	            115200          // T5L  串口5波特率设置

#define       RESPONSE_UART2            1               // 串口2应答4F4B开启关闭设置，RESPONSE_UART2=1开启，RESPONSE_UART2=0关闭
#define       RESPONSE_UART3            1               // 串口3应答4F4B开启关闭设置，RESPONSE_UART3=1开启，RESPONSE_UART3=0关闭
#define       RESPONSE_UART4            1               // 串口4应答4F4B开启关闭设置，RESPONSE_UART4=1开启，RESPONSE_UART4=0关闭
#define       RESPONSE_UART5            1               // 串口5应答4F4B开启关闭设置，RESPONSE_UART5=1开启，RESPONSE_UART5=0关闭

#define       DATA_UPLOAD_UART2         1               // 串口2数据自动上传设置，1上传，0不上传
#define       DATA_UPLOAD_UART3         1               // 串口3数据自动上传设置，1上传，0不上传
#define       DATA_UPLOAD_UART4         1               // 串口4数据自动上传设置，1上传，0不上传
#define       DATA_UPLOAD_UART5         1               // 串口5数据自动上传设置，1上传，0不上传

#define       CRC_CHECK_UART2           0               // 串口2数据CRC检验设置，1开启CRC，0关闭CRC
#define       CRC_CHECK_UART3           0               // 串口3数据CRC检验设置，1开启CRC，0关闭CRC
#define       CRC_CHECK_UART4           0               // 串口4数据CRC检验设置，1开启CRC，0关闭CRC
#define       CRC_CHECK_UART5           0               // 串口5数据CRC检验设置，1开启CRC，0关闭CRC

#endif
