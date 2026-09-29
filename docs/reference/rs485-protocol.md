# STC-DWIN RS-485 协议 v0.2

## 串口参数

- 9600 bps，8 数据位，1 停止位，无校验；
- 半双工；发送前拉高 P3.7，最后一个停止位发完后再拉低；
- STC 主控地址 `0x01`，DWIN 地址 `0x02`；
- 默认由主控板周期发送状态，显示侧只在触摸或回复时发送，避免总线竞争。

## 帧格式

为适配课程 BSP 的 `SetUart2Rxd()` 定长接收接口，所有帧统一为 26 字节：

```text
AA 55 | DST | SRC | CMD | SEQ | LEN | DATA[18] | CRC8
  2       1     1     1     1     1       18        1  = 26 bytes
```

| 字段 | 字节数 | 说明 |
|---|---:|---|
| `AA 55` | 2 | 帧头 |
| `DST` | 1 | 目标地址，`0xFF` 为广播 |
| `SRC` | 1 | 源地址 |
| `CMD` | 1 | 命令 |
| `SEQ` | 1 | 0-255 循环序号 |
| `LEN` | 1 | 数据区长度，MVP 最大 24 |
| `DATA` | 18 | 有效内容由 LEN 指定，剩余字节必须补 0；多字节整数为大端 |
| `CRC8` | 1 | 对 DST 到完整的 18 字节 DATA 做 CRC-8/ATM，多项式 `0x07`，初值 `0x00` |

STC 端可直接使用 `SetUart2Rxd(rx, 26, "\xAA\x55", 2)`。接收端先找 `AA 55`，再接收固定 26 字节；LEN 大于 18、CRC 错误或目标地址不符时丢弃。

## 命令

| CMD | 方向 | 名称 | DATA |
|---:|---|---|---|
| 0x01 | A→B | `STATUS_SNAPSHOT` | 完整状态快照 |
| 0x02 | A→B | `PARAM_SNAPSHOT` | 当前计费参数 |
| 0x03 | A→B | `RTC_SNAPSHOT` | DS1302年月日、星期、时分秒 |
| 0x10 | B→A | `ZERO_CMD` | 无 |
| 0x11 | B→A | `TARE_CMD` | 无 |
| 0x12 | B→A | `REBILL_CMD` | 无 |
| 0x13 | B→A | 保留 | 固定三档版本不接受此命令 |
| 0x14 | B→A | `CALIBRATE_CMD` | 已知砝码质量 U32（g） |
| 0x15 | B→A | `SET_REGION_CMD` | 地区编号 U8（1/2/3） |
| 0x20 | 双向 | `HEARTBEAT` | uptime_s(U32) + state(U8) |
| 0x7E | 回复 | `ACK` | original_cmd(U8) + result(U8) |
| 0x7F | 回复 | `NACK` | original_cmd(U8) + error(U8) |

## STATUS_SNAPSHOT 数据区

```text
weight_g        S32  4 bytes
price_cent      U32  4 bytes
system_state    U8   1 byte
status_flags    U16  2 bytes
height_mm       U16  2 bytes
error_code      U8   1 byte
tare_g          U32  4 bytes
```

总长 18 字节。状态标志：bit0 稳定、bit1 传感器正常、bit2 未超量程、bit3 参数有效。通信状态由显示侧根据接收超时判断。

## RTC_SNAPSHOT 数据区

```text
year       U8   0-99
month      U8   1-12
day        U8   1-31
week       U8   1-7
hour       U8   0-23
minute     U8   0-59
second     U8   0-59
```

总长 7 字节。STC 从 DS1302 读取 BCD 后转换为普通二进制数，每秒发送一次。DWIN 依次写入 DGUS 系统 RTC 区 `0x0010-0x0013`，供 RTC 显示控件使用。

计费参数不从屏幕输入。屏幕只通过 `SET_REGION_CMD` 选择省内、普通省外或偏远地区，STC 从程序中的固定三档价目表加载参数，并通过 `PARAM_SNAPSHOT` 回传只读显示值。收到保留命令 `0x13` 时，STC 返回未知命令错误。

## 超时与重发

- 主控板每 500 ms 发送一次 `STATUS_SNAPSHOT`；
- 2 s 未收到有效状态帧，显示侧提示通信中断；
- 触摸命令等待 300 ms，未收到同 SEQ 的 ACK/NACK 则重发，最多 2 次；
- 主控板记录最近一次 `SRC + CMD + SEQ`，重复帧只重发上次结果，不重复执行清零/去皮；
- 通信恢复后，主控板立即发送完整状态和参数快照。

## 错误码

| 值 | 含义 |
|---:|---|
| 0x00 | 正常 |
| 0x01 | 传感器无响应 |
| 0x02 | 重量超量程 |
| 0x03 | 标定数据无效 |
| 0x04 | 计费参数无效 |
| 0x05 | 参数保存失败 |
| 0x06 | 未知命令 |
| 0x07 | 当前状态禁止操作 |
