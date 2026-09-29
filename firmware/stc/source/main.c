#include "STC15F2K60S2.H"
#include "sys.H"
#include "uart2.H"
#include "EXT.H"
#include "key.h"
#include "M24C02.h"
#include "DS1302.H"
#include "project_config.h"
#include "parcel_protocol.h"
#include "billing.h"
#include "weight_service.h"

code unsigned long SysClock = 11059200UL;

enum app_state
{
    APP_IDLE = 0,
    APP_WEIGHING = 1,
    APP_STABLE = 2,
    APP_BILLING = 3,
    APP_ERROR = 4
};

enum app_error
{
    ERR_NONE = 0,
    ERR_SENSOR = 1,
    ERR_OVER_RANGE = 2,
    ERR_CALIBRATION = 3,
    ERR_PARAMS = 4,
    ERR_SAVE = 5,
    ERR_UNKNOWN_CMD = 6,
    ERR_STATE = 7
};

#define NVM_REGION_ADDR 0xE0U
#define NVM_REGION_TAG  0xA0U

xdata pp_u8 g_rx_frame[PP_FRAME_SIZE];
xdata pp_u8 g_tx_frame[PP_FRAME_SIZE];
xdata pp_u8 g_frame_header[2];
xdata weight_service_t g_weight;
xdata billing_params_t g_billing;

volatile pp_u8 g_tick10_event;
volatile pp_u8 g_rx_event;
xdata pp_u8 g_tick_divider;
xdata pp_u8 g_status_ticks;
xdata pp_u8 g_status_pending;
xdata pp_u8 g_params_pending;
xdata pp_u8 g_rtc_ticks;
xdata pp_u8 g_rtc_pending;
xdata pp_u8 g_tx_seq;
pp_s32 g_latest_raw;
xdata pp_u8 g_region_id;
xdata pp_u8 g_last_rx_valid;
xdata pp_u8 g_last_rx_cmd;
xdata pp_u8 g_last_rx_seq;
xdata pp_u8 g_last_rx_result;
xdata pp_u8 g_rx_diag_stage;
xdata pp_u8 g_rx_frame_count;

static void on_10ms(void)
{
    g_tick10_event = 1U;
}

static void on_uart2_rx(void)
{
    g_rx_event = 1U;
    ++g_rx_frame_count;
    g_rx_diag_stage = 1U; /* Full 26-byte frame reached the BSP callback. */
}

static pp_u8 send_frame(pp_u8 cmd, pp_u8 seq, const pp_u8 *payload, pp_u8 len)
{
    if (GetUart2TxStatus() != enumUart2TxFree)
        return 0U;
    if (!pp_build(PP_ADDR_DWIN, PP_ADDR_STC, cmd, seq, payload, len, g_tx_frame))
        return 0U;
    return (Uart2Print(g_tx_frame, PP_FRAME_SIZE) == enumUart2TxOK) ? 1U : 0U;
}

static pp_u8 send_reply(pp_u8 request_cmd, pp_u8 request_seq, pp_u8 result)
{
    pp_u8 payload[2];
    payload[0] = request_cmd;
    payload[1] = result;
    return send_frame((result == 0U) ? PP_CMD_ACK : PP_CMD_NACK,
                      request_seq, payload, 2U);
}

static pp_u8 apply_command(const pp_packet_t *packet)
{
    pp_u32 known_mass;

    switch (packet->cmd)
    {
        case PP_CMD_ZERO:
            weight_zero(&g_weight, g_latest_raw);
            g_status_pending = 1U;
            return ERR_NONE;

        case PP_CMD_TARE:
            if (!g_weight.calibrated && !LINK_TEST_MODE)
                return ERR_CALIBRATION;
            weight_tare(&g_weight);
            g_status_pending = 1U;
            return ERR_NONE;

        case PP_CMD_REBILL:
            g_status_pending = 1U;
            return ERR_NONE;

        case PP_CMD_CALIBRATE:
            if (packet->len != 4U)
                return ERR_CALIBRATION;
            known_mass = pp_get_u32(&packet->payload[0]);
            if (!weight_calibrate(&g_weight, g_latest_raw, known_mass))
                return ERR_CALIBRATION;
            g_status_pending = 1U;
            return ERR_NONE;

        case PP_CMD_SET_REGION:
            if (packet->len != 1U ||
                !billing_region_params(packet->payload[0], &g_billing))
                return ERR_PARAMS;
            g_region_id = packet->payload[0];
            M24C02_Write(NVM_REGION_ADDR,
                         (pp_u8)(NVM_REGION_TAG | g_region_id));
            g_params_pending = 1U;
            g_status_pending = 1U;
            return ERR_NONE;

        case PP_CMD_HEARTBEAT:
            return ERR_NONE;

        default:
            return ERR_UNKNOWN_CMD;
    }
}

static pp_u8 inspect_rx_frame(const pp_u8 *frame)
{
    if (frame[0] != PP_HEADER_0 || frame[1] != PP_HEADER_1)
        return 0xD1U;
    if (frame[6] > PP_DATA_SIZE)
        return 0xD2U;
    if (frame[2] != PP_ADDR_STC && frame[2] != PP_ADDR_BROADCAST)
        return 0xD3U;
    if (frame[25] != pp_crc8(&frame[2], 23U))
        return 0xD4U;
    return 0U;
}

/*
 * Compatibility path for a zero-only frame whose command fields and all
 * reserved payload bytes are intact, but whose final CRC byte is rejected.
 * No other command is allowed to bypass CRC validation.
 */
static pp_u8 is_safe_zero_frame(const pp_u8 *frame)
{
    pp_u8 i;
    if (frame[0] != PP_HEADER_0 || frame[1] != PP_HEADER_1 ||
        frame[2] != PP_ADDR_STC || frame[3] != PP_ADDR_DWIN ||
        frame[4] != PP_CMD_ZERO || frame[6] != 0U)
        return 0U;
    for (i = 7U; i < 25U; ++i)
    {
        if (frame[i] != 0U)
            return 0U;
    }
    return 1U;
}

static void process_rx(void)
{
    pp_packet_t packet;
    pp_u8 result;
    pp_u8 frame_error;
    pp_u8 i;

    if (!g_rx_event)
        return;
    g_rx_event = 0U;

    frame_error = inspect_rx_frame(g_rx_frame);
    if (frame_error != 0U)
    {
        if (frame_error == 0xD4U && is_safe_zero_frame(g_rx_frame))
        {
            weight_zero(&g_weight, g_latest_raw);
            g_status_pending = 1U;
            if (send_reply(PP_CMD_ZERO, g_rx_frame[5], ERR_NONE))
                g_rx_diag_stage = 0xC4U; /* Zero accepted; CRC-only fallback. */
            else
                g_rx_diag_stage = 0xE3U;
            return;
        }
        g_rx_diag_stage = frame_error;
        g_status_pending = 1U;
        return;
    }

    /*
     * Decode locally after validation.  Keeping the xdata receive buffer and
     * automatic packet in one C51 module avoids a generic-pointer memory-model
     * mismatch observed when calling pp_parse() across modules.
     */
    packet.dst = g_rx_frame[2];
    packet.src = g_rx_frame[3];
    packet.cmd = g_rx_frame[4];
    packet.seq = g_rx_frame[5];
    packet.len = g_rx_frame[6];
    for (i = 0U; i < PP_DATA_SIZE; ++i)
        packet.payload[i] = g_rx_frame[7U + i];
    g_rx_diag_stage = 2U;
    if (packet.src != PP_ADDR_DWIN)
    {
        g_rx_diag_stage = 0xE2U; /* Unexpected source address. */
        g_status_pending = 1U;
        return;
    }
    g_rx_diag_stage = 3U;
    if (g_last_rx_valid && packet.cmd == g_last_rx_cmd &&
        packet.seq == g_last_rx_seq)
    {
        result = g_last_rx_result;
    }
    else
    {
        result = apply_command(&packet);
        g_last_rx_valid = 1U;
        g_last_rx_cmd = packet.cmd;
        g_last_rx_seq = packet.seq;
        g_last_rx_result = result;
    }
    g_rx_diag_stage = 4U;
    if (send_reply(packet.cmd, packet.seq, result))
        g_rx_diag_stage = 5U; /* ACK/NACK accepted by the UART2 BSP. */
    else
        g_rx_diag_stage = 0xE3U; /* UART2 was busy when replying. */
    g_status_pending = 1U;
}

static void process_10ms(void)
{
    unsigned char key_action;

    if (!g_tick10_event)
        return;
    g_tick10_event = 0U;

    /* The original scale program zeros on the local K1 (P3.2) press. */
    key_action = GetKeyAct(enumKey1);
    if (key_action == enumKeyPress)
    {
        weight_zero(&g_weight, g_latest_raw);
        g_status_pending = 1U;
    }

    ++g_tick_divider;
    ++g_status_ticks;
    ++g_rtc_ticks;
    if (g_tick_divider >= WEIGHT_SAMPLE_PERIOD_10MS)
    {
        g_tick_divider = 0U;
        g_latest_raw = (pp_s32)GetWeight();
        weight_push_raw(&g_weight, g_latest_raw);
    }
    if (g_status_ticks >= STATUS_PERIOD_10MS)
    {
        g_status_ticks = 0U;
        g_status_pending = 1U;
    }
    if (g_rtc_ticks >= 100U)
    {
        g_rtc_ticks = 0U;
        g_rtc_pending = 1U;
    }
}

static void build_runtime_status(pp_s32 *weight_g, pp_u32 *price_cent,
                                 pp_u8 *state, pp_u16 *flags, pp_u8 *error)
{
#if LINK_TEST_MODE
    *weight_g = 1234L;
    *price_cent = 0UL;
    billing_calculate(*weight_g, &g_billing, price_cent);
    *state = APP_BILLING;
    *flags = 0x001FU; /* stable/sensor/range/params + link-test */
    *error = ERR_NONE;
#else
    *weight_g = g_weight.filtered_g;
    *price_cent = 0UL;
    *flags = 0x0006U; /* sensor present, within range until checked below */
    *error = ERR_NONE;

    if (billing_params_valid(&g_billing))
        *flags |= 0x0008U;
    else
        *error = ERR_PARAMS;
    if (!g_weight.calibrated)
        *error = ERR_CALIBRATION;
    if (*weight_g > MAX_WEIGHT_G || *weight_g < -100L)
    {
        *flags &= (pp_u16)~0x0004U;
        *error = ERR_OVER_RANGE;
    }
    if (g_weight.stable)
        *flags |= 0x0001U;
    *flags |= (pp_u16)((pp_u16)g_region_id << 8);

    if (*error != ERR_NONE)
        *state = APP_ERROR;
    else if (*weight_g <= 0L)
        *state = APP_IDLE;
    else if (!g_weight.stable)
        *state = APP_WEIGHING;
    else
    {
        *state = APP_BILLING;
        if (!billing_calculate(*weight_g, &g_billing, price_cent))
        {
            *state = APP_ERROR;
            *error = ERR_PARAMS;
        }
    }
#endif
}

static void try_send_status(void)
{
    pp_u8 payload[PP_DATA_SIZE];
    pp_s32 weight_g;
    pp_u32 price_cent;
    pp_u8 state;
    pp_u16 flags;
    pp_u8 error;

    if (!g_status_pending || GetUart2TxStatus() != enumUart2TxFree)
        return;
    build_runtime_status(&weight_g, &price_cent, &state, &flags, &error);
    pp_put_u32(&payload[0], (pp_u32)weight_g);
    pp_put_u32(&payload[4], price_cent);
    payload[8] = state;
    pp_put_u16(&payload[9], flags);
    /* Reserved height field carries RX diagnostic stage/count during bring-up. */
    pp_put_u16(&payload[11],
               ((pp_u16)g_rx_diag_stage << 8) | g_rx_frame_count);
    payload[13] = error;
    pp_put_u32(&payload[14], (pp_u32)g_weight.tare_g);
    if (send_frame(PP_CMD_STATUS, g_tx_seq++, payload, PP_DATA_SIZE))
        g_status_pending = 0U;
}

static void try_send_params(void)
{
    pp_u8 payload[17];
    if (!g_params_pending || GetUart2TxStatus() != enumUart2TxFree)
        return;
    pp_put_u32(&payload[0], g_billing.first_weight_g);
    pp_put_u32(&payload[4], g_billing.first_price_cent);
    pp_put_u32(&payload[8], g_billing.step_weight_g);
    pp_put_u32(&payload[12], g_billing.step_price_cent);
    payload[16] = g_region_id;
    if (send_frame(PP_CMD_PARAMS, g_tx_seq++, payload, 17U))
        g_params_pending = 0U;
}

static pp_u8 bcd_to_dec(pp_u8 bcd)
{
    return (pp_u8)(((bcd >> 4) * 10U) + (bcd & 0x0FU));
}

static void try_send_rtc(void)
{
    pp_u8 payload[7];
    struct_DS1302_RTC rtc;

    if (!g_rtc_pending || GetUart2TxStatus() != enumUart2TxFree)
        return;
    rtc = RTC_Read();
    payload[0] = bcd_to_dec(rtc.year);
    payload[1] = bcd_to_dec(rtc.month & 0x1FU);
    payload[2] = bcd_to_dec(rtc.day & 0x3FU);
    payload[3] = bcd_to_dec(rtc.week & 0x07U);
    payload[4] = bcd_to_dec(rtc.hour & 0x3FU);
    payload[5] = bcd_to_dec(rtc.minute & 0x7FU);
    payload[6] = bcd_to_dec(rtc.second & 0x7FU);
    if (send_frame(PP_CMD_RTC, g_tx_seq++, payload, 7U))
        g_rtc_pending = 0U;
}

static void app_init(void)
{
    pp_u8 stored_region;

    g_latest_raw = (pp_s32)GetWeight();
    weight_init(&g_weight, g_latest_raw);
#if WEIGHT_RAW_COUNT_MODE
    weight_use_raw_counts(&g_weight);
#endif
    stored_region = M24C02_Read(NVM_REGION_ADDR);
    if ((stored_region & 0xF0U) == NVM_REGION_TAG)
        g_region_id = stored_region & 0x0FU;
    else
        g_region_id = REGION_DEFAULT;
    if (!billing_region_params(g_region_id, &g_billing))
    {
        g_region_id = REGION_DEFAULT;
        billing_region_params(g_region_id, &g_billing);
    }
    g_tick10_event = 0U;
    g_rx_event = 0U;
    g_tick_divider = 0U;
    g_status_ticks = 0U;
    g_status_pending = 1U;
    g_params_pending = 1U;
    g_rtc_ticks = 0U;
    g_rtc_pending = 1U;
    g_tx_seq = 0U;
    g_last_rx_valid = 0U;
    g_last_rx_cmd = 0U;
    g_last_rx_seq = 0U;
    g_last_rx_result = 0U;
    g_rx_diag_stage = 0xA0U; /* Boot marker: diagnostic firmware is running. */
    g_rx_frame_count = 0U;
}

void main(void)
{
    struct_DS1302_RTC initial_time = {
        0x00U, /* second */
        0x00U, /* minute */
        0x12U, /* hour */
        0x05U, /* day */
        0x09U, /* month */
        0x06U, /* week: Saturday */
        0x26U  /* year: 2026 */
    };

    EXTInit(enumEXTWeight);
    KeyInit();
    DS1302Init(initial_time);
    Uart2Init(RS485_BAUD, Uart2Usedfor485);
    g_frame_header[0] = PP_HEADER_0;
    g_frame_header[1] = PP_HEADER_1;
    SetUart2Rxd(g_rx_frame, PP_FRAME_SIZE, g_frame_header, 2U);
    SetEventCallBack(enumEventSys10mS, on_10ms);
    SetEventCallBack(enumEventUart2Rxd, on_uart2_rx);
    app_init();
    MySTC_Init();

    while (1)
    {
        MySTC_OS();
        process_10ms();
        process_rx();
        try_send_params();
        try_send_status();
        try_send_rtc();
    }
}
