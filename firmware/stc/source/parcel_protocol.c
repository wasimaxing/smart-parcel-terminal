#include "parcel_protocol.h"

pp_u8 pp_crc8(const pp_u8 *bytes, pp_u8 len)
{
    pp_u8 crc;
    pp_u8 i;
    pp_u8 bit_index;

    crc = 0U;
    for (i = 0U; i < len; ++i)
    {
        crc ^= bytes[i];
        for (bit_index = 0U; bit_index < 8U; ++bit_index)
        {
            if ((crc & 0x80U) != 0U)
                crc = (pp_u8)((crc << 1) ^ 0x07U);
            else
                crc <<= 1;
        }
    }
    return crc;
}

pp_u8 pp_build(pp_u8 dst, pp_u8 src, pp_u8 cmd, pp_u8 seq,
               const pp_u8 *payload, pp_u8 len, pp_u8 *frame)
{
    pp_u8 i;

    if (frame == 0 || len > PP_DATA_SIZE)
        return 0U;

    frame[0] = PP_HEADER_0;
    frame[1] = PP_HEADER_1;
    frame[2] = dst;
    frame[3] = src;
    frame[4] = cmd;
    frame[5] = seq;
    frame[6] = len;
    for (i = 0U; i < PP_DATA_SIZE; ++i)
        frame[7U + i] = (payload != 0 && i < len) ? payload[i] : 0U;
    frame[25] = pp_crc8(&frame[2], 23U);
    return 1U;
}

void pp_put_u16(pp_u8 *p, pp_u16 value)
{
    p[0] = (pp_u8)(value >> 8);
    p[1] = (pp_u8)value;
}

void pp_put_u32(pp_u8 *p, pp_u32 value)
{
    p[0] = (pp_u8)(value >> 24);
    p[1] = (pp_u8)(value >> 16);
    p[2] = (pp_u8)(value >> 8);
    p[3] = (pp_u8)value;
}

pp_u16 pp_get_u16(const pp_u8 *p)
{
    return (pp_u16)(((pp_u16)p[0] << 8) | p[1]);
}

pp_u32 pp_get_u32(const pp_u8 *p)
{
    pp_u32 value;
    value = (pp_u32)p[0] << 24;
    value |= (pp_u32)p[1] << 16;
    value |= (pp_u32)p[2] << 8;
    value |= p[3];
    return value;
}
