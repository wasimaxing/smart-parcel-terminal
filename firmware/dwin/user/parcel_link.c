#include "parcel_link.h"
#include "uart.h"
#include "dgus_tools.h"
#include "timer.h"

#define PP_HEADER_0       0xAA
#define PP_HEADER_1       0x55
#define PP_DATA_SIZE      18
#define PP_FRAME_SIZE     26
#define PP_ADDR_STC       0x01
#define PP_ADDR_DWIN      0x02
#define PP_ADDR_BROADCAST 0xFF

#define PP_CMD_STATUS     0x01
#define PP_CMD_PARAMS     0x02
#define PP_CMD_RTC        0x03
#define PP_CMD_ZERO       0x10
#define PP_CMD_TARE       0x11
#define PP_CMD_REBILL     0x12
#define PP_CMD_CALIBRATE  0x14
#define PP_CMD_SET_REGION 0x15
#define PP_CMD_ACK        0x7E
#define PP_CMD_NACK       0x7F

#define VP_WEIGHT_G       0x1000
#define VP_PRICE_CENT     0x1002
#define VP_REGION_TEXT    0x1200
#define VP_STATUS_FLAGS   0x1005
#define VP_FIRST_WEIGHT   0x1006
#define VP_FIRST_PRICE    0x1008
#define VP_STEP_WEIGHT    0x100A
#define VP_STEP_PRICE     0x100C
#define VP_HEIGHT_MM      0x100E
#define VP_ERROR_CODE     0x100F
#define VP_TARE_G         0x1010
#define VP_RULE_TEXT      0x1210
#define VP_REGION_ID      0x1030
#define VP_SYSTEM_STATE   0x1031
#define VP_STATE_TEXT     0x1220
#define VP_STATUS_TEXT    0x1230

#define VP_ACTION_CODE    0x1100
#define VP_ACTION_SEQ     0x1101
#define VP_ACTION_ECHO    0x1102
#define VP_COMMAND_STATE  0x1103
#define VP_FIRMWARE_MARK  0x1104
#define VP_CAL_MASS_G     0x1118

#define ACTION_ZERO       1
#define ACTION_TARE       2
#define ACTION_REBILL     3
#define ACTION_CALIBRATE  5
#define ACTION_REGION_1   6
#define ACTION_REGION_2   7
#define ACTION_REGION_3   8

#define ERROR_CALIBRATION 0x0003
#define ERROR_LINK_LOST   0x0100

#define COMMAND_STATE_IDLE     0x0000
#define COMMAND_STATE_WAIT_ACK 0x0001
#define COMMAND_STATE_ACK      0x0002
#define COMMAND_STATE_NACK     0xE000
#define COMMAND_STATE_TIMEOUT  0xFFFF
#define COMMAND_RETRY_MS       120
#define COMMAND_MAX_ATTEMPTS   3

static u8 g_tx_seq;
static u8 g_seconds_without_status;
static u8 g_error_page_forced;
static u8 g_pending_cmd;
static u8 g_pending_len;
static u8 g_pending_seq;
static u8 g_pending_attempts;
static u8 g_displayed_region;
static u8 xdata g_pending_payload[PP_DATA_SIZE];

static char code TEXT_IDLE[] = "IDLE";
static char code TEXT_WEIGHING[] = "WEIGHING";
static char code TEXT_STABLE[] = "STABLE";
static char code TEXT_BILLING[] = "BILLING";
static char code TEXT_ERROR[] = "ERROR";
static char code TEXT_LOCAL_RATE[] = "LOCAL 8+2 YUAN/KG";
static char code TEXT_OUTSIDE_RATE[] = "OUTSIDE 12+4 Y/KG";
static char code TEXT_REMOTE_RATE[] = "REMOTE 18+8 Y/KG";
static char code TEXT_SENSOR_ERROR[] = "SCALE SENSOR ERROR";
static char code TEXT_OVER_RANGE[] = "WEIGHT OVER RANGE";
static char code TEXT_CALIBRATION[] = "CALIBRATION NEEDED";
static char code TEXT_PARAM_ERROR[] = "BILLING PARAM ERROR";
static char code TEXT_SAVE_ERROR[] = "SAVE FAILED";
static char code TEXT_UNKNOWN_CMD[] = "UNKNOWN COMMAND";
static char code TEXT_STATE_ERROR[] = "OPERATION DENIED";
static char code TEXT_LINK_TIMEOUT[] = "LINK TIMEOUT";
static char code TEXT_UNKNOWN_ERROR[] = "UNKNOWN ERROR";

static u8 crc8(const u8 *bytes, u8 len)
{
    u8 crc = 0;
    u8 i;
    u8 bit_index;
    for (i = 0; i < len; ++i)
    {
        crc ^= bytes[i];
        for (bit_index = 0; bit_index < 8; ++bit_index)
        {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

static u16 get_u16(const u8 *p)
{
    return ((u16)p[0] << 8) | p[1];
}

static u32 get_u32(const u8 *p)
{
    u32 value;
    value = (u32)p[0] << 24;
    value |= (u32)p[1] << 16;
    value |= (u32)p[2] << 8;
    value |= p[3];
    return value;
}

static void put_u32(u8 *p, u32 value)
{
    p[0] = value >> 24;
    p[1] = value >> 16;
    p[2] = value >> 8;
    p[3] = value;
}

static void write_u32_vp(u16 vp, u32 value)
{
    Write_Vp(vp, (u16)(value >> 16));
    Write_Vp(vp + 1, (u16)value);
}

static void write_fixed_ascii(u16 vp, u8 length, const char code *text)
{
    u8 buffer[20];
    u8 i;

    if (length > sizeof(buffer))
        length = sizeof(buffer);
    for (i = 0; i < length; ++i)
        buffer[i] = ' ';
    for (i = 0; i < length && text[i] != 0; ++i)
        buffer[i] = (u8)text[i];
    Write_Vp_byU8(vp, buffer, length);
}

static void update_status_text(u8 state, u16 error, u8 region_id)
{
    const char code *state_text;
    const char code *status_text;

    if (error != 0)
    {
        state_text = TEXT_ERROR;
        if (error == 1)
            status_text = TEXT_SENSOR_ERROR;
        else if (error == 2)
            status_text = TEXT_OVER_RANGE;
        else if (error == 3)
            status_text = TEXT_CALIBRATION;
        else if (error == 4)
            status_text = TEXT_PARAM_ERROR;
        else if (error == 5)
            status_text = TEXT_SAVE_ERROR;
        else if (error == 6)
            status_text = TEXT_UNKNOWN_CMD;
        else if (error == 7)
            status_text = TEXT_STATE_ERROR;
        else if (error == ERROR_LINK_LOST)
            status_text = TEXT_LINK_TIMEOUT;
        else
            status_text = TEXT_UNKNOWN_ERROR;
    }
    else
    {
        if (state == 0)
            state_text = TEXT_IDLE;
        else if (state == 1)
            state_text = TEXT_WEIGHING;
        else if (state == 2)
            state_text = TEXT_STABLE;
        else if (state == 3)
            state_text = TEXT_BILLING;
        else
            state_text = TEXT_ERROR;

        if (region_id == 1)
            status_text = TEXT_LOCAL_RATE;
        else if (region_id == 2)
            status_text = TEXT_OUTSIDE_RATE;
        else if (region_id == 3)
            status_text = TEXT_REMOTE_RATE;
        else
            status_text = TEXT_UNKNOWN_ERROR;
    }

    write_fixed_ascii(VP_STATE_TEXT, 10, state_text);
    write_fixed_ascii(VP_STATUS_TEXT, 20, status_text);
}

static void update_region_display(u8 region_id)
{
    u8 region_text[10];
    u8 rule_text[20];
    u8 i;

    if (region_id < 1 || region_id > 3 || region_id == g_displayed_region)
        return;
    for (i = 0; i < 10; ++i)
        region_text[i] = ' ';
    for (i = 0; i < 20; ++i)
        rule_text[i] = ' ';

    if (region_id == 1)
    {
        region_text[0] = 'L'; region_text[1] = 'O'; region_text[2] = 'C';
        region_text[3] = 'A'; region_text[4] = 'L';
        rule_text[0] = '1'; rule_text[1] = 'k'; rule_text[2] = 'g';
        rule_text[3] = ':'; rule_text[4] = '8'; rule_text[5] = 'y';
        rule_text[6] = ' '; rule_text[7] = '+'; rule_text[8] = '2';
        rule_text[9] = 'y'; rule_text[10] = '/'; rule_text[11] = 'k';
        rule_text[12] = 'g';
    }
    else if (region_id == 2)
    {
        region_text[0] = 'O'; region_text[1] = 'U'; region_text[2] = 'T';
        region_text[3] = 'S'; region_text[4] = 'I'; region_text[5] = 'D';
        region_text[6] = 'E';
        rule_text[0] = '1'; rule_text[1] = 'k'; rule_text[2] = 'g';
        rule_text[3] = ':'; rule_text[4] = '1'; rule_text[5] = '2';
        rule_text[6] = 'y'; rule_text[7] = ' '; rule_text[8] = '+';
        rule_text[9] = '4'; rule_text[10] = 'y'; rule_text[11] = '/';
        rule_text[12] = 'k'; rule_text[13] = 'g';
    }
    else
    {
        region_text[0] = 'R'; region_text[1] = 'E'; region_text[2] = 'M';
        region_text[3] = 'O'; region_text[4] = 'T'; region_text[5] = 'E';
        rule_text[0] = '1'; rule_text[1] = 'k'; rule_text[2] = 'g';
        rule_text[3] = ':'; rule_text[4] = '1'; rule_text[5] = '8';
        rule_text[6] = 'y'; rule_text[7] = ' '; rule_text[8] = '+';
        rule_text[9] = '8'; rule_text[10] = 'y'; rule_text[11] = '/';
        rule_text[12] = 'k'; rule_text[13] = 'g';
    }

    Write_Vp_byU8(VP_REGION_TEXT, region_text, 10);
    Write_Vp_byU8(VP_RULE_TEXT, rule_text, 20);
    Write_Vp(VP_REGION_ID, region_id);
    g_displayed_region = region_id;
}

/*
 * Calibration is an expected commissioning state, not a broken link.
 * Keep the normal page available so the operator can zero and calibrate.
 */
static void update_error_page(u16 error)
{
    if (error == 0 || error == ERROR_CALIBRATION)
    {
        if (g_error_page_forced)
        {
            g_error_page_forced = 0;
            page_change(0);
        }
    }
    else
    {
        g_error_page_forced = 1;
        page_change(2);
    }
}

static u32 read_u32_vp(u16 vp)
{
    return ((u32)Read_Vp(vp) << 16) | Read_Vp(vp + 1);
}

static u16 uart4_available(void)
{
    return (uart4.tail + UART4_RX_SIZE - uart4.head) % UART4_RX_SIZE;
}

static u8 uart4_peek(u16 offset)
{
    return uart4.buf[(uart4.head + offset) % UART4_RX_SIZE];
}

static void uart4_drop(u16 count)
{
    uart4.head = (uart4.head + count) % UART4_RX_SIZE;
}

/* Hold the 485 direction for the complete frame. */
static void uart4_send_frame(const u8 *frame)
{
    u8 i;
    TR4 = 1;
    for (i = 0; i < PP_FRAME_SIZE; ++i)
    {
        SBUF2_TX = frame[i];
        while ((SCON2T & 0x01) == 0)
            ;
        SCON2T &= 0xFE;
    }
    TR4 = 0;
}

static void send_command(u8 cmd, const u8 *payload, u8 len, u8 seq)
{
    u8 frame[PP_FRAME_SIZE];
    u8 i;
    frame[0] = PP_HEADER_0;
    frame[1] = PP_HEADER_1;
    frame[2] = PP_ADDR_STC;
    frame[3] = PP_ADDR_DWIN;
    frame[4] = cmd;
    frame[5] = seq;
    frame[6] = len;
    for (i = 0; i < PP_DATA_SIZE; ++i)
        frame[7 + i] = (i < len && payload != 0) ? payload[i] : 0;
    frame[25] = crc8(&frame[2], 23);
    uart4_send_frame(frame);
}

static void finish_pending_command(u16 state)
{
    g_pending_cmd = 0;
    stop_timer(SOFT_TIMER_3);
    Write_Vp(VP_COMMAND_STATE, state);
}

static void begin_command(u8 cmd, const u8 *payload, u8 len, u8 seq)
{
    u8 i;
    g_pending_cmd = cmd;
    g_pending_len = len;
    g_pending_seq = seq;
    g_pending_attempts = 1;
    for (i = 0; i < PP_DATA_SIZE; ++i)
        g_pending_payload[i] = (i < len && payload != 0) ? payload[i] : 0;

    Write_Vp(VP_COMMAND_STATE, COMMAND_STATE_WAIT_ACK);
    send_command(g_pending_cmd, g_pending_payload, g_pending_len, g_pending_seq);
    start_timer(SOFT_TIMER_3, COMMAND_RETRY_MS);
}

static void apply_status(const u8 *payload)
{
    u16 flags;
    u8 region_id;
    u8 state;
    u16 error;

    write_u32_vp(VP_WEIGHT_G, get_u32(&payload[0]));
    write_u32_vp(VP_PRICE_CENT, get_u32(&payload[4]));
    state = payload[8];
    error = payload[13];
    Write_Vp(VP_SYSTEM_STATE, state);
    flags = get_u16(&payload[9]);
    Write_Vp(VP_STATUS_FLAGS, flags);
    region_id = (u8)(flags >> 8);
    update_region_display(region_id);
    update_status_text(state, error, region_id);
    Write_Vp(VP_HEIGHT_MM, get_u16(&payload[11]));
    Write_Vp(VP_ERROR_CODE, error);
    write_u32_vp(VP_TARE_G, get_u32(&payload[14]));
    g_seconds_without_status = 0;

    update_error_page(error);
}

static void apply_params(const u8 *payload, u8 len)
{
    write_u32_vp(VP_FIRST_WEIGHT, get_u32(&payload[0]));
    write_u32_vp(VP_FIRST_PRICE, get_u32(&payload[4]));
    write_u32_vp(VP_STEP_WEIGHT, get_u32(&payload[8]));
    write_u32_vp(VP_STEP_PRICE, get_u32(&payload[12]));
    if (len >= 17)
        update_region_display(payload[16]);
}

static void apply_rtc(const u8 *payload)
{
    /* DGUS RTC area: YY/MM, DD/weekday, HH/MM, SS/unused. */
    Write_Vp(0x0010, ((u16)payload[0] << 8) | payload[1]);
    Write_Vp(0x0011, ((u16)payload[2] << 8) | payload[3]);
    Write_Vp(0x0012, ((u16)payload[4] << 8) | payload[5]);
    Write_Vp(0x0013, ((u16)payload[6] << 8));
}

static void apply_frame(const u8 *frame)
{
    u8 len = frame[6];
    if (len > PP_DATA_SIZE)
        return;
    if (frame[2] != PP_ADDR_DWIN && frame[2] != PP_ADDR_BROADCAST)
        return;
    if (frame[3] != PP_ADDR_STC)
        return;
    if (frame[25] != crc8(&frame[2], 23))
        return;

    if (frame[4] == PP_CMD_STATUS && len == 18)
        apply_status(&frame[7]);
    else if (frame[4] == PP_CMD_PARAMS && len >= 16)
        apply_params(&frame[7], len);
    else if (frame[4] == PP_CMD_RTC && len == 7)
        apply_rtc(&frame[7]);
    else if (frame[4] == PP_CMD_NACK && len >= 2)
    {
        if (g_pending_cmd != 0 && frame[5] == g_pending_seq &&
            frame[7] == g_pending_cmd)
            finish_pending_command(COMMAND_STATE_NACK | frame[8]);
        Write_Vp(VP_ERROR_CODE, frame[8]);
        update_error_page(frame[8]);
    }
    else if (frame[4] == PP_CMD_ACK && len >= 2)
    {
        if (g_pending_cmd != 0 && frame[5] == g_pending_seq &&
            frame[7] == g_pending_cmd && frame[8] == 0)
            finish_pending_command(COMMAND_STATE_ACK);
    }
}

static void receive_frames(void)
{
    u8 frame[PP_FRAME_SIZE];
    u8 i;
    while (uart4_available() >= 2)
    {
        if (uart4_peek(0) != PP_HEADER_0 || uart4_peek(1) != PP_HEADER_1)
        {
            uart4_drop(1);
            continue;
        }
        if (uart4_available() < PP_FRAME_SIZE)
            return;
        for (i = 0; i < PP_FRAME_SIZE; ++i)
            frame[i] = uart4_peek(i);
        uart4_drop(PP_FRAME_SIZE);
        apply_frame(frame);
    }
}

static void process_touch_action(void)
{
    u16 action = Read_Vp(VP_ACTION_CODE);
    u16 action_seq;
    u8 payload[16];
    u8 cmd;
    u8 len = 0;

    if (action == 0)
        return;
    action_seq = Read_Vp(VP_ACTION_SEQ);
    cmd = 0;
    if (action == ACTION_ZERO)
        cmd = PP_CMD_ZERO;
    else if (action == ACTION_TARE)
        cmd = PP_CMD_TARE;
    else if (action == ACTION_REBILL)
        cmd = PP_CMD_REBILL;
    else if (action == ACTION_CALIBRATE)
    {
        cmd = PP_CMD_CALIBRATE;
        put_u32(&payload[0], read_u32_vp(VP_CAL_MASS_G));
        len = 4;
    }
    else if (action >= ACTION_REGION_1 && action <= ACTION_REGION_3)
    {
        cmd = PP_CMD_SET_REGION;
        payload[0] = action - ACTION_REGION_1 + 1;
        len = 1;
    }

    Write_Vp(VP_ACTION_ECHO, 0xA000 | action);
    if (cmd != 0)
        begin_command(cmd, payload, len,
                      (u8)(action_seq ? action_seq : ++g_tx_seq));
    Write_Vp(VP_ACTION_CODE, 0);
}

void parcel_link_init(void)
{
    g_tx_seq = 0;
    g_seconds_without_status = 3;
    g_error_page_forced = 1;
    g_pending_cmd = 0;
    g_pending_len = 0;
    g_pending_seq = 0;
    g_pending_attempts = 0;
    g_displayed_region = 0;
    update_status_text(4, ERROR_LINK_LOST, 0);
    Write_Vp(VP_ACTION_ECHO, 0xA500);
    Write_Vp(VP_COMMAND_STATE, COMMAND_STATE_IDLE);
    Write_Vp(VP_FIRMWARE_MARK, 0xD104);
    Write_Vp(VP_ERROR_CODE, ERROR_LINK_LOST);
}

void parcel_link_poll(void)
{
    receive_frames();
    process_touch_action();
}

void parcel_link_1s(void)
{
    if (g_seconds_without_status < 255)
        ++g_seconds_without_status;
    if (g_seconds_without_status >= 2)
    {
        Write_Vp(VP_ERROR_CODE, ERROR_LINK_LOST);
        update_status_text(4, ERROR_LINK_LOST, g_displayed_region);
        g_error_page_forced = 1;
        page_change(2);
    }
}

void parcel_link_retry(void)
{
    if (g_pending_cmd == 0)
        return;

    if (g_pending_attempts >= COMMAND_MAX_ATTEMPTS)
    {
        finish_pending_command(COMMAND_STATE_TIMEOUT);
        return;
    }

    ++g_pending_attempts;
    send_command(g_pending_cmd, g_pending_payload, g_pending_len, g_pending_seq);
    start_timer(SOFT_TIMER_3, COMMAND_RETRY_MS);
}
