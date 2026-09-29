#ifndef PARCEL_PROTOCOL_H
#define PARCEL_PROTOCOL_H

#ifdef __C51__
typedef unsigned char  pp_u8;
typedef unsigned int   pp_u16;
typedef unsigned long  pp_u32;
typedef signed long    pp_s32;
#else
#include <stdint.h>
typedef uint8_t  pp_u8;
typedef uint16_t pp_u16;
typedef uint32_t pp_u32;
typedef int32_t  pp_s32;
#endif

#define PP_HEADER_0       0xAAU
#define PP_HEADER_1       0x55U
#define PP_DATA_SIZE      18U
#define PP_FRAME_SIZE     26U

#define PP_ADDR_STC       0x01U
#define PP_ADDR_DWIN      0x02U
#define PP_ADDR_BROADCAST 0xFFU

#define PP_CMD_STATUS     0x01U
#define PP_CMD_PARAMS     0x02U
#define PP_CMD_RTC        0x03U
#define PP_CMD_ZERO       0x10U
#define PP_CMD_TARE       0x11U
#define PP_CMD_REBILL     0x12U
#define PP_CMD_SET_PRICE  0x13U /* Reserved: fixed-tier build rejects it. */
#define PP_CMD_CALIBRATE  0x14U
#define PP_CMD_SET_REGION 0x15U
#define PP_CMD_HEARTBEAT  0x20U
#define PP_CMD_ACK        0x7EU
#define PP_CMD_NACK       0x7FU

typedef struct
{
    pp_u8 dst;
    pp_u8 src;
    pp_u8 cmd;
    pp_u8 seq;
    pp_u8 len;
    pp_u8 payload[PP_DATA_SIZE];
} pp_packet_t;

pp_u8 pp_crc8(const pp_u8 *bytes, pp_u8 len);
pp_u8 pp_build(pp_u8 dst, pp_u8 src, pp_u8 cmd, pp_u8 seq,
               const pp_u8 *payload, pp_u8 len, pp_u8 *frame);

void pp_put_u16(pp_u8 *p, pp_u16 value);
void pp_put_u32(pp_u8 *p, pp_u32 value);
pp_u16 pp_get_u16(const pp_u8 *p);
pp_u32 pp_get_u32(const pp_u8 *p);

#endif
