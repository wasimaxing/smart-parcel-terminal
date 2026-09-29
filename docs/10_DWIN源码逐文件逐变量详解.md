# DWIN 源码逐文件、逐变量详解

本文对应仓库内正式工程 `firmware/dwin`，说明 T5L OS 核 C51 程序。DGUS 的背景、触摸控件和显示控件属于 `DWIN_SET` 资源，不是 C 源码；本文只在解释 VP 交互时引用它们。

## 一、实际编译文件与执行主线

Keil 工程编译 5 个 `user` 文件、12 个 `driver` 文件、2 个 `sys` 文件和 `STARTUP_M5.A51`：

| 层 | 文件 | 当前项目中的角色 |
|---|---|---|
| 业务 | `user/parcel_link.c` | STC 通信、状态显示、触摸命令、ACK/NACK 与重试，核心文件 |
| 入口 | `user/main.c` | 初始化并循环调度各模块 |
| 页面 | `user/page.c` | 通用页面进入/循环/离开回调框架 |
| 模板演示 | `user/deal.c` | 文本写入与移动演示，非快递业务，但当前仍被调用 |
| 模板演示 | `user/key_scan.c` | 通用按键 VP 扫描，当前未调用 |
| 系统 | `sys/sys.c`、`sys/sys_isr.c` | CPU、端口和中断处理 |
| 驱动 | `uart.c`、`timer.c`、`dgus_tools.c` | 当前核心运行依赖 |
| 驱动 | `dgus_8283.c`、`crc16.c` | 官方 5A A5/82/83 串口协议；UART4 不走此协议 |
| 驱动 | `canbus.c`、`t5l_io.c` | CAN 模板；初始化和轮询仍在运行，但没有快递业务 |
| 驱动 | `rtc.c` | OS 核外接 RTC；当前配置为关闭，由 STC 的 DS1302 同步时间 |
| 驱动 | `save_data_dgus.c`、`norflash.c` | VP 掉电保存模板；当前调用被注释且保存数量为 0 |
| 驱动 | `log.c` | 串口日志；等级为 NONE，普通日志关闭 |
| 驱动 | `other_tools.c` | 8/16 位位操作工具 |
| 启动 | `STARTUP_M5.A51` | 复位、存储空间、堆栈与 C 入口 |

正常业务主线为：

```text
UART4 接收中断写入环形缓冲
       ↓
parcel_link_poll 扫描 AA 55 定长帧
       ↓
校验地址、长度、CRC8、命令
       ↓
STATUS/PARAMS/RTC → 写 DGUS VP
ACK/NACK          → 结束或标记待确认命令

DGUS 触摸写 VP 0x1100/0x1101
       ↓
process_touch_action 形成 26 字节命令帧
       ↓
UART4 半双工 RS-485 发送
       ↓
120 ms 无确认重发，最多发送 3 次
```

---

## 二、T5L C51 类型和存储关键字

`sys/sys.h` 定义：`u8/u16/u32` 是 8/16/32 位无符号整数，`s8/s16/s32` 是对应有符号整数。

- `data`：8051 内部低速小容量 RAM，适合频繁访问的计数器。
- `xdata`：扩展数据空间，适合 18 字节载荷、1 KB UART 缓冲等较大对象。
- `code`：程序 Flash 中的只读常量，如提示文本和 CAN 波特率表。
- `sfr`：把一个 C 名称直接映射到 T5L 特殊功能寄存器地址。
- `sbit`：把名称映射到某个可位寻址寄存器的一位。
- `interrupt N`：声明函数是编号 N 的中断服务函数。
- `static`：文件内私有；函数内静态变量则跨调用保留值。

特别注意：`TR4` 在本工程不是 Timer4 启停位，而是 `P0.0` 的别名，用作 UART4 的 RS-485 收发方向控制：1 发送，0 接收。

`sys.h` 第 10 行再次包含自身是冗余写法，头文件保护会阻止无限递归，不影响结果。

---

## 三、`sys/Parameter_Config.h`：所有编译开关

### RTC 与日志

| 宏 | 值 | 当前效果 |
|---|---:|---|
| `SELECT_RTC_TYPE` | 0 | 不编译 RX8130/SD2058 的具体驱动路径；`rtc_init/rtc_deal` 成为空函数 |
| `DEBUG_UART` | 3 | `printf` 和日志从 UART3 输出 |

DWIN 页面仍可显示 RTC。这里关闭的只是 OS 核外接 I²C RTC；真正时间由 STC 板 DS1302 通过项目 `RTC` 帧写入 VP `0x0010～0x0013`。

### UART 开关

UART2、3、4、5 的 `*_ENABLE` 都是 1，所以四个串口都会初始化并建立 1024 字节接收缓冲。

- UART2/3/5 的 `*_8283_ENABLE=1`：按官方 `5A A5 + 0x82/0x83` 协议解析。
- UART4 的 `UART4_8283_ENABLE=0`：不让通用解析器取走 UART4 数据，交给 `parcel_link.c` 解析 `AA 55` 项目帧。
- `UART4_485_ENABLE=1`：P0.0 控制 UART4 的 485 方向。
- `UART5_485_ENABLE=1`：P0.1 控制 UART5 的 485 方向。
- UART2 波特率倍速开启。

波特率为 UART2/3/5 115200，UART4 9600。只有 UART4 与 STC 快递终端直接相关。

`RESPONSE_UARTx` 控制官方 0x82 写 VP 命令是否回复 `OK`；`DATA_UPLOAD_UARTx` 控制 DGUS 自动上传是否转发到该串口；`CRC_CHECK_UARTx` 控制官方 82/83 帧的 CRC16。四个 CRC16 开关当前都为 0。

非常重要：`CRC_CHECK_UART4=0` 不会关闭项目帧 CRC8。`parcel_link.c` 始终独立计算和检查 CRC-8/ATM。

---

## 四、`user/main.c`：入口与调度

### 初始化顺序

1. `INIT_CPU()`：关闭并复位中断优先级，设置基本定时器模式和端口初值。
2. `PORT_Init()`：设置引脚复用、驱动电流和推挽/输入模式。
3. `T0_Init()`：建立 1 ms 系统节拍并开启中断。
4. `uart_init()`：初始化四个启用的串口。
5. `parcel_link_init()`：初始化快递链路的软件状态和诊断 VP。
6. `CanBus_Init(can_baud_250K)`：初始化 250 kbit/s CAN。当前快递业务不使用 CAN，但这行仍实际执行。
7. `WDT_ON()`：开启看门狗。
8. `rtc_init()`：当前 `SELECT_RTC_TYPE=0`，实际为空。
9. `Write_Vp(0x16, 0x5A02)`：把 DGUS 系统触摸状态初始化为“抬起”。
10. `page_deal_init()`：当前页和上一页都设为 0。
11. `test_init()`：运行官方文本演示；这不是快递业务，且会写业务 VP `0x1000`。

`data_save_init()` 被注释，不执行。

### 软定时器

| 定时器 | 周期 | 到期工作 |
|---|---:|---|
| `SOFT_TIMER_RTC=0` | 250 ms | 调用 `rtc_deal()`，当前为空 |
| `SOFT_TIMER_1=1` | 50 ms | 调用模板 `test_func()` 移动文本控件 |
| `SOFT_TIMER_2=2` | 1000 ms | `parcel_link_1s()` 做通信超时累计 |
| `SOFT_TIMER_3=3` | 按需 120 ms | 等待命令 ACK；只在发送控制命令时启动 |
| `SOFT_TIMER_PRINTF=7` | 5000 ms | 调用 `LOG_DEBUG`；当前日志等级使其直接返回 |

### 无限循环顺序

`parcel_link_poll()` 最先消费 UART4 项目帧和触摸动作；随后 `uart_handle_all()` 处理 UART2/3/5 的官方协议；`Read_0xF00()` 处理 DGUS 自动上传；页面框架和 CAN 接收继续运行；最后轮询各软定时器。

这些函数都应快速返回。没有 RTOS 线程，所谓“并行”来自中断收字节和主循环分时轮询。

### 当前值得注意的实际行为

`test_init()` 和 `test_func()` 没有被注释：前者会在启动时向 `0x1000` 写演示文本数据，后者每 50 ms 调整 SP `0x2000` 对应文本控件位置。第一帧有效 STC 状态会重新覆盖重量 VP，但启动瞬间可能出现错误内容。若正式发布，应评估是否移除这两处调用。

---

## 五、`user/parcel_link.c/.h`：快递终端核心

头文件只公开四个函数：初始化、主循环轮询、1 秒超时任务和命令重试任务。内部细节全部用 `static` 隐藏。

### 1. 协议常量

帧固定 26 字节：

```text
AA 55 | DST | SRC | CMD | SEQ | LEN | DATA[18] | CRC8
```

地址：STC=`0x01`、DWIN=`0x02`、广播=`0xFF`。命令与 STC 端一致：STATUS `01`、PARAMS `02`、RTC `03`、ZERO `10`、TARE `11`、REBILL `12`、CALIBRATE `14`、SET_REGION `15`、ACK `7E`、NACK `7F`。

### 2. 所有 VP 宏

| VP | 名称 | 内容 |
|---:|---|---|
| `0x1000` | `VP_WEIGHT_G` | S32 净重，连续两个 VP |
| `0x1002` | `VP_PRICE_CENT` | U32 价格，单位分 |
| `0x1005` | `VP_STATUS_FLAGS` | STC 状态位集合 |
| `0x1006/08/0A/0C` | 四项费率 VP | 每项 U32，占两个 VP |
| `0x100E` | `VP_HEIGHT_MM` | 原预留高度；当前实际装接收诊断阶段和帧计数 |
| `0x100F` | `VP_ERROR_CODE` | 错误码 |
| `0x1010` | `VP_TARE_G` | S32 皮重 |
| `0x1030` | `VP_REGION_ID` | 地区 1/2/3 |
| `0x1031` | `VP_SYSTEM_STATE` | STC 状态 0～4 |
| `0x1200` | `VP_REGION_TEXT` | 10 字节地区 ASCII |
| `0x1210` | `VP_RULE_TEXT` | 20 字节规则 ASCII |
| `0x1220` | `VP_STATE_TEXT` | 10 字节状态 ASCII |
| `0x1230` | `VP_STATUS_TEXT` | 20 字节说明 ASCII |
| `0x1100` | `VP_ACTION_CODE` | DGUS 触摸写入的动作码 |
| `0x1101` | `VP_ACTION_SEQ` | DGUS 动作序号 |
| `0x1102` | `VP_ACTION_ECHO` | 固件是否读到动作的诊断回显 |
| `0x1103` | `VP_COMMAND_STATE` | 命令等待、成功、拒绝或超时状态 |
| `0x1104` | `VP_FIRMWARE_MARK` | 固件标记，固定 `0xD104` |
| `0x1118` | `VP_CAL_MASS_G` | 标定砝码质量 U32 |

### 3. 动作和命令状态

动作码 1 清零、2 去皮、3 重计费、5 标定、6/7/8 选择地区 1/2/3；动作 4 被固定三档版本保留不用。

`VP_COMMAND_STATE`：`0x0000` 空闲、`0x0001` 等 ACK、`0x0002` ACK 成功、`0xE000|错误码` NACK、`0xFFFF` 三次均未确认。

`COMMAND_RETRY_MS=120` 是每次确认等待时间；`COMMAND_MAX_ATTEMPTS=3` 包含首次发送，所以一般在 t=0、120、240 ms 发送，约 t=360 ms 判超时。

### 4. 每个文件静态变量

| 变量 | 作用 |
|---|---|
| `g_tx_seq` | 当 DGUS 没提供动作序号时生成 8 位序号 |
| `g_seconds_without_status` | 距最后一帧有效 STATUS 的秒数，饱和到 255 |
| `g_error_page_forced` | 1 表示程序曾因错误强制进入页面 2，恢复后需强制回页面 0 |
| `g_pending_cmd` | 当前等待确认的命令；0 同时作为“没有待确认命令”的标志 |
| `g_pending_len` | 待确认命令有效载荷长度 |
| `g_pending_seq` | 待确认命令序号，用于匹配 ACK/NACK |
| `g_pending_attempts` | 已发送次数，首次发送后为 1 |
| `g_displayed_region` | 当前已经写入文本 VP 的地区缓存，避免重复改写 |
| `g_pending_payload[18]` | `xdata` 中保存的重发载荷；局部数组退出后仍可重发 |

`TEXT_*` 全部存放在 `code` 区，避免占 RAM。显示使用英文 ASCII，是为了避开字体编码和 GBK/UTF-8 差异。

### 5. `crc8/get_u16/get_u32/put_u32`

`crc8()` 与 STC 完全同算法：多项式 `0x07`、初值 0、逐字节从最高位计算。`i` 是字节下标，`bit_index` 是位下标，`crc` 是当前余数。

`get_u16/get_u32` 从大端字节恢复整数；`put_u32` 把本机整数写成大端载荷。所有跨板多字节字段都不依赖 CPU 本机字节序。

### 6. `write_u32_vp()` 与 `write_fixed_ascii()`

一个 DGUS VP 是 16 位，因此 32 位数拆成高 16 位写 `vp`、低 16 位写 `vp+1`。

固定文本先把最多 20 字节缓冲全部填空格，再复制到字符串结尾或固定长度，防止新短文本后残留旧长文本。`length>20` 会被截到 20。

### 7. `update_status_text()`

局部 `state_text/status_text` 是指向 `code` 区字符串的指针。错误非零时状态固定显示 ERROR，并把错误 1～7、链路错误 `0x0100` 映射成具体说明；无错误时，状态 0/1/2/3 映射 IDLE/WEIGHING/STABLE/BILLING，说明文本显示当前地区费率。

未知状态或未知地区显示 UNKNOWN ERROR。最后分别写入 10 字节状态文本和 20 字节说明文本。

### 8. `update_region_display()`

`region_text[10]` 和 `rule_text[20]` 是局部 ASCII 缓冲，`i` 用于填空格。地区必须为 1～3，且不能等于缓存的已显示地区；否则直接返回。

三档写入 `LOCAL/OUTSIDE/REMOTE` 和相应简写费率，再更新数值地区 VP 与 `g_displayed_region`。缓存减少 VP 写入，但也意味着外部若把文本 VP 改坏，只要地区号没变，本函数不会主动恢复。

### 9. `update_error_page()`

错误 0 和标定错误 3 不视为必须停留在错误页，因为操作员需要在正常页完成清零/标定。如果此前程序强制进入过错误页，则恢复时切回页面 0。其他错误令 `g_error_page_forced=1` 并切到页面 2。

### 10. UART4 环形缓冲辅助函数

- `uart4_available()`：用 `(tail+size-head)%size` 计算待处理字节数。
- `uart4_peek(offset)`：不移动 `head` 地查看后续字节。
- `uart4_drop(count)`：移动 `head`，正式丢弃已消费数据。

`head` 是主循环读指针，`tail` 是接收中断写指针。缓冲大小 1024。驱动没有“缓冲已满”检测；若生产速度长期超过消费速度，`tail` 追上 `head` 时会表现成空缓冲并覆盖旧数据。

8051 对 16 位 `tail` 的读写不是天然原子操作，当前 `uart4_available/peek` 没有像通用 UART 处理函数那样短暂关中断复制 `tail`；在非常巧合的字节到达时刻存在读到不一致值的理论窗口。

### 11. `uart4_send_frame()`

`TR4=1` 后一次发送完整 26 字节，每个字节写 `SBUF2_TX`，轮询 `SCON2T bit0` 等待发送完成并清标志；整帧结束才令 `TR4=0` 回接收。

它没有调用通用 `uart_send_array()`，因为通用 `uart4_send_byte()` 会为每个字节切换一次 485 方向，而项目帧需要整帧期间持续保持发送方向。

### 12. `send_command/begin_command/finish_pending_command`

`send_command()` 的局部 `frame[26]` 是完整发送缓冲，`i` 填满 18 字节数据区并补零，最后计算覆盖 `frame[2..24]` 的 CRC8。

`begin_command()` 把命令、长度、序号和载荷复制到全局待确认状态，发送第一次，写 WAIT_ACK，并启动 120 ms 定时器。

`finish_pending_command()` 只需把 `g_pending_cmd` 清零即可关闭活动事务，再停止定时器并写最终状态；其余缓存值可以保留，因为是否有效只看 `g_pending_cmd`。

当前没有检查“是否已有命令在等待 ACK”。如果用户在等待期间触发另一个动作，新 `begin_command()` 会覆盖旧事务；界面设计应避免连续触发，或后续代码增加忙状态判断。

### 13. `apply_status()`

局部 `flags/region_id/state/error` 分别承接状态标志、地区、状态码和错误码。载荷映射：

| 下标 | 内容 | 写入 VP |
|---:|---|---|
| 0～3 | 重量 | `0x1000/1001` |
| 4～7 | 价格 | `0x1002/1003` |
| 8 | 状态 | `0x1031` |
| 9～10 | flags | `0x1005` |
| 11～12 | 诊断阶段/计数 | `0x100E` |
| 13 | 错误 | `0x100F` |
| 14～17 | 皮重 | `0x1010/1011` |

地区从 `flags` 高字节取得。只有合法 STATUS 才把 `g_seconds_without_status` 清零，因此收到 PARAMS、RTC 或 ACK 不能掩盖状态链路中断。

### 14. `apply_params()` 与 `apply_rtc()`

PARAMS 前 16 字节写四个 U32 费率；长度至少 17 时再取地区。接收条件允许 `len>=16`，兼容旧版没有地区字节的参数帧。

RTC 的 7 字节按“年月、日星期、时分、秒空白”打包到系统 VP `0x0010～0x0013`，供 DGUS RTC 控件直接显示。

### 15. `apply_frame()`

按顺序检查 `LEN<=18`、目的为 DWIN 或广播、源为 STC、CRC8 正确。任何一项失败都静默丢弃。

- STATUS 必须 `len==18`。
- PARAMS 要求 `len>=16`。
- RTC 必须 `len==7`。
- NACK/ACK 至少 2 字节。

ACK 只有序号、原命令码和结果 0 都匹配当前事务才结束等待。NACK 匹配时写 `0xE000|result`；此外任何合法 NACK 都会把结果写入错误 VP 并更新错误页。

### 16. `receive_frames()`

局部 `frame[26]` 保存一帧快照，`i` 负责复制。循环扫描缓冲：不是 `AA 55` 就丢一个字节继续找；找到帧头但不足 26 字节就等待下一轮；足够时复制并先丢掉 26 字节，再交给 `apply_frame()`。

无效 CRC 帧也整帧丢弃。如果噪声中恰好出现假 `AA 55`，可能先错丢 26 字节，之后再从剩余数据继续同步。

### 17. `process_touch_action()`

局部变量：`action` 是 VP 动作码，`action_seq` 是触摸序号，`payload[16]` 是临时载荷，`cmd` 是协议命令，`len` 是有效长度。

清零/去皮/重计费没有载荷；标定读取 `0x1118/1119` 的 U32 克数并生成 4 字节；动作 6～8 转成地区 1～3的一字节载荷。

无论动作是否认识，先写 `0xA000|action` 到回显 VP，最后都把动作码 VP 清零。命令序号优先用非零 `action_seq` 的低 8 位；若为 0 才用 `++g_tx_seq`。

### 18. 四个公开函数

- `parcel_link_init()`：清序号和待确认状态；地区缓存设 0；上电先显示 LINK TIMEOUT；诊断回显 `0xA500`、命令状态 IDLE、固件标记 `0xD104`、错误 VP `0x0100`。秒计数初值 3、强制页标志初值 1，使系统在未收到状态时保持故障语义。
- `parcel_link_poll()`：每轮先收帧，再看触摸。
- `parcel_link_1s()`：秒数饱和递增；达到 2 秒写链路错误文本并切页面 2。
- `parcel_link_retry()`：无待确认命令直接返回；已发送 3 次则 TIMEOUT；否则次数加 1、重发相同命令/载荷/序号并重新计时。

---

## 六、`user/page.c/.h`：页面回调框架

`PAGE_ON=0xAA` 表示某回调启用，`PAGE_OFF=0xBB` 表示关闭；不是普通 0/1 布尔。`PAGE_NUM=8` 允许索引 0～7。

`s_page_t`：`page_last` 是上一个页面号，`page_now` 是当前页面号。全局实例为 `s_page`。

`page_void` 是“无参数、无返回值函数”的指针类型。`s_page_deal_t` 为每页保存三个使能字节和三个函数指针：进入页 `page_init`、停留页循环 `page_deal`、离开页 `page_finish`。

`page_deal[8]` 仅显式配置页面 0 的三个空函数；页面 1 显式 OFF；其余 6 项由 C 自动补零，零也不等于 `PAGE_ON`，所以都不调用。

`page_deal_init()` 把当前和上一页设 0。`page_deal_func()` 从 DGUS 系统 VP `0x0014` 读当前页：页变化时先保存旧页，若旧页有效且 finish 开启就调用，再对新页调用 init；页未变化时调用当前页 deal。

上电实际就在页面 0 时不会触发 `page_0_init()`，而会直接进入空的 `page_0_deal()`；只有离开后再次回到页面 0 才执行 init。

---

## 七、`user/deal.c/.h`：官方文本演示

`t_str` 和被整段注释的 `hello()` 不运行。`test_str` 是演示字符串；源码当前字符编码显示为乱码，编译后的具体中文取决于源文件编码。

`set_text_pos(sp_addr, x, y, width, height)` 用 `buf_s16[4]` 形成左上角 `(x,y)` 和右下角 `(x+width,y+height)`，写入文本控件 SP 描述区。前两项写 `sp_addr+1`，四项再写 `sp_addr+4`，遵循 DGUS 控件描述结构。

`test_init()`：建立 64 字节缓冲，先填 `0xFF`，复制演示文字，使用不受日志等级控制的 `printHexArray()` 输出十六进制，再从 VP `0x1000` 开始写入，并把 SP `0x2000` 的控件放在 `(0,0)`、尺寸 268×74。

`test_func()` 的 `location_x/y` 是函数内静态变量，每次调用各加 1，到 600/400 回零；当系统 VP `0x0015` 非零时停止移动，否则更新文本位置。主程序每 50 ms 调用它，因此它当前不是“未执行的示例”，而是仍在运行的模板逻辑。

---

## 八、`user/key_scan.c/.h`：通用按键 VP 模板

`KEY_SCAN_VP=0xA000`。`scan_key_func()` 读取该 VP 到 `key_value`；非零时记录日志，当前只为 `0x0001` 留空分支，最后清零 VP。主程序没有调用本函数，且日志关闭，因此它对现有快递触摸无作用。快递触摸使用 `parcel_link.c` 的 `0x1100`，两套机制不要混淆。

---

## 九、`sys/sys.c/.h` 与寄存器头

### `INIT_CPU()`

`IEN0/1/2=0` 关闭各组中断，`IP0/1=0` 恢复默认优先级；P0～P3 输出锁存值清零；`TMOD=0x11` 令 T0、T1 都为 16 位模式 1；`TCON=0x05` 令外部中断 0、1 使用边沿触发。

### `PORT_Init()`

先清 `MUX_SEL`，随后各驱动再打开所需 UART/CAN 复用。`PORTDRV=0x01` 选择约 ±8 mA 驱动；`P0MDOUT/P1MDOUT/P2MDOUT/P3MDOUT` 决定相应引脚为推挽输出还是输入/非推挽模式。

### 时钟和看门狗

未定义 `T5L_2K` 时 `FOSC=206438400 Hz`；`T1MS=65536-FOSC/12/1000` 是 T0 每 1 ms 重装值。

`WDT_ON/OFF/RST` 分别操作 `MUX_SEL` bit1、清 bit1、置 bit0。看门狗在 T0 中断中每毫秒喂一次。因此只要 T0 中断继续工作，即使主循环卡死，看门狗仍可能一直被喂而不能复位；它主要能覆盖连中断都无法运行的严重故障。

### `T5LOS8051.h`

该芯片头文件用 `sfr/sbit` 声明端口、定时器、串口、中断、CAN、DGUS RAM 接口等硬件寄存器。业务关键组：

- `RAMMODE/APP_ACK/APP_EN/ADR_*/DATA*`：OS 核访问 DGUS VP 的桥接接口。
- `SCON2T/SCON2R/BODE2_DIV_*/SBUF2_TX/SBUF2_RX`：UART4。
- `TR4=P0^0`：UART4 485 方向。
- `IEN0/IEN1/IEN2/EA`：中断使能。
- `TMOD/TCON/TH0/TL0`：1 ms 定时器。
- `CAN_CR/CAN_IR/CAN_ET`：CAN 控制与状态。

其余寄存器是芯片公共定义，只有被某个驱动读写时才进入本项目行为。

---

## 十、`driver/timer.c/.h` 与 T0 中断

`SysTick` 是阻塞延时计数；`T_number_arr[8]` 保存每个软定时器剩余毫秒；`T_number_en[8]` 保存启用标志。

`T0_Init()` 写 1 ms 重装值，开启 T0 中断和全局中断。`start_timer(ID,nTime)` 置使能并写倒计时；`stop_timer()` 只清使能；`get_time_out_flag()` 发现启用且计数为 0 时自动停表并返回 1，所以软定时器默认是一次性的，调用者必须重新启动。

接口没有检查 `ID<8`，传错编号会越界写内存。

`delay_timer_ms()` 把 `SysTick` 设为毫秒并忙等，递减依赖 T0 中断；`delay_ms/delay_us` 用空循环近似延时，精度依赖主频和编译优化。

`T0_ISR_PC()` 每毫秒重装 TH0/TL0，递减 `SysTick` 和所有已启用的软定时器，并喂看门狗。局部 `i` 遍历 8 个定时器。函数开头清 EA、结尾再置 EA；进入 8051 中断时硬件本已阻止同级响应，这种模板写法还会在退出前重新允许中断。

---

## 十一、`driver/uart.c/.h` 与 UART 中断

### 环形缓冲结构

UART2/3/4/5 各有一个结构体，成员均为 `buf[1024]`、读指针 `head` 和写指针 `tail`。对应全局变量 `uart2/3/4/5` 只在启用宏为 1 时存在。

接收中断把字节写到 `tail` 后令其模 1024 递增；主循环从 `head` 消费。没有满缓冲标志，因此最多可靠区分 0～1023 个待处理字节。

### 初始化函数

各 `uartN_Init()` 清 head/tail 和缓冲，计算波特率分频，设置引脚复用、推挽输出、收发模式和中断。UART4 额外把 P0.0 配成输出并令 `TR4=0` 进入接收。

`bode_div_cache` 是根据主频和目标波特率算出的分频暂存值。`uart_init()` 根据宏依次调用四个初始化函数。

### 发送函数

四个 `uartN_send_byte()` 都是阻塞发送：写发送寄存器，忙等完成标志并清除。UART4/5 的单字节函数会在每字节前后切换 485 方向。

`UART_SEND_BYTE` 是函数指针类型，`uart_send_byte_func[6]` 用串口号作为下标映射具体发送函数；0、1 映射空函数。`uart_send_byte()` 没有检查 `uart_number<6`，错误串口号会越界调用。`uart_send_array()` 用 `i` 循环发送并递增数据指针。

### 缓冲搬运与 82/83 解析

四个 `*_buf_to_temp_buf()` 从环形缓冲复制指定长度并推进 head。

四个 `*_rx_deal()` 逻辑相同：短暂关中断取得 tail 快照；至少 6 字节后寻找 `5A A5`；长度字节加 3 得整帧长度；命令 0x82/0x83 分派给 `dgus_8283.c`，未知帧头或命令只丢一个字节继续同步。

当前 `uart_handle_all()` 只调用 UART2、UART3、UART5 处理；UART4 条件是 `UART4_ENABLE && UART4_8283_ENABLE`，后者为 0，所以不会与快递解析器争抢数据。

### `Read_0xF00()`

`buf_u8[128]` 接收 DGUS 自动上传描述，`len` 是字数，`vp_addr` 是触发地址。若 `0x0F00` 开头标志为 `0x5A`，读取地址和长度，调用 `uart_auto_upload_deal()` 转发，最后清触发标志。

主程序每轮调用。当前 UART2/3/5 会接收自动上传，UART4 不会。

---

## 十二、`sys/sys_isr.c`：全部中断

中断号宏把外部中断、T0/T1/T2、CAN、UART2/3/4/5 收发映射到 T5L 向量。EX0、EX1、T1、T2 和 UART4/5 发送 ISR 当前为空。

- T0 ISR：1 ms 节拍、软定时器递减、看门狗喂狗。
- UART2 ISR：若 `RI0` 置位，把 `SBUF0` 写入 UART2 环形缓冲并清接收标志。
- UART3 ISR：临时关闭 UART3 中断，检查接收位，从 `SBUF1` 入队并按芯片要求清标志两次。
- UART4 RX ISR：临时清 `ES2R`，从 `SBUF2_RX` 写入 `uart4.buf[tail]`，移动 tail，清 `SCON2R bit0`，再开接收中断。这是 STC 链路每个字节的入口。
- UART5 RX ISR：与 UART4 同类。
- CAN ISR：处理远程帧/数据帧/发送完成/错误等标志，从 DGUS/CAN 硬件缓冲读取 ID、状态和 8 字节数据到 `CanData.BusRXbuf`，再推进接收 head。

CAN ISR 中的 `tmp` 保存硬件帧状态；`status` bit7 为扩展帧 IDE、bit6 为远程帧 RTR、低四位为 DLC。

---

## 十三、`driver/dgus_tools.c/.h`：VP 内存访问

T5L OS 核不能像普通数组一样直接访问 DGUS VP，而是通过 `ADR_H/M/L` 指定地址、`RAMMODE` 指定读写宽度、`APP_ACK` 等待接口就绪、`APP_EN` 发起一次传输、`DATA3..0` 搬运数据。

内部桥一次访问 32 位，包含两个 16 位 VP，所以函数用 `addr/2` 定位 32 位槽，用 `addr&1` 选择前半或后半。

- `Read_Vp_byU16/Write_Vp_byU16`：按 16 位字数批量读写。
- `Write_Vp_byU8`：按字节数写，按剩余 1/2/3/4 字节选择 RAMMODE，项目文本更新依赖它。
- `Read_Vp/Write_Vp`：读写单个 16 位 VP。
- `Clear_Vp_byU16/Copy_Vp_byU16`：批量清零/复制；局部数组可容纳 0x1000 个 U16，调用者仍应遵守最大长度。
- `Read/Write_Vp_H/L`：只访问一个 VP 的高或低字节，写之前先读回另一字节。
- `Get_Vp/Get_Vp_H/Get_Vp_L`：读取后立即清零，适合一次性事件。
- `Get/Set/Clear_Vp_Bit`：操作单个 VP 的一位。
- `page_change(Page)`：先写目标页到 `0x85`，再写 `0x5A01` 到系统命令 VP `0x84`，轮询命令高字节清零后返回。

若定义 `INTVPACTION`，多数 VP 操作前后会关闭/开启全局中断；当前头文件把它注释掉，因此 VP 访问期间中断保持可响应。

---

## 十四、`driver/dgus_8283.c` 与 `driver/crc16.c`

这是官方 DGUS 串口协议，不是 STC 的 AA55 项目协议。

`response_buf_crc16/response_buf_nocrc16` 是 0x82 写命令的固定 `OK` 回复。`uart_auto_upload_deal()` 从 VP 读取触发数据，构造 `5A A5 LEN 83 ADDR WORDS DATA`，按各串口开关发送。

其局部 `send_flag` 表示是否至少一个串口允许上传，`crc_flag` 表示是否至少一个要求 CRC，`send_len` 是无 CRC 帧长，`crc_count` 是 CRC16，`buf_u8[128]` 是组帧缓冲。

`deal_82_cmd()` 校验可选 CRC16后把收到的数据写 VP，并按 `response_flag` 回复；`deal_83_cmd()` 读取指定 VP 并回传。`uart_number` 选择出口，`rx_buf` 是帧，`response_flag/check_flag` 控制回复和校验。

当前模板有一处通用性问题：给 UART3/4/5 计算发送长度时仍使用 `CRC_CHECK_UART2`，而不是各自开关。当前所有 CRC 开关均为 0，所以现配置不受影响。

`crc16.c` 的两张 256 项只读表分别给高、低 CRC 字节查表；`crc16table()` 的 `crchi/crclo` 初值均为 `0xFF`，`index` 由低字节与当前输入异或得到，最终组合成 16 位 Modbus CRC。它与项目帧使用的 8 位 CRC 完全不同。

---

## 十五、`driver/save_data_dgus.c` 与 `driver/norflash.c`

`Save_Data_t` 只有 `Address` 和 `Data`，表示要保存的 VP 及其缓存/初始值。虽然 `Save_Data[]` 列了多项，`SAVE_NUMBER` 当前为 0，所以两个保存函数的循环都执行 0 次；主程序也把两处调用注释了。

`INITIALIZATION_MARK=0xABCD` 用于判断 Flash 是否首次初始化，地址为 0。`data_save_init()` 首次应写标志及默认值，后续从 Flash 恢复；`data_change_sava()` 只在触摸抬起时比较 VP 和缓存，变化才写 Flash，减少擦写。

因为功能关闭，列表内容不会覆盖快递业务 VP。若以后启用，必须先修正 `SAVE_NUMBER` 并审查列表地址是否与重量、状态等实时 VP 冲突。

`norflash.c` 使用 DGUS VP `0xF000` 作中转缓存。`T5L_Flash(mod,addr,addr_flash,len)` 把读写命令写到 DGUS 系统地址 `0x0004` 并由 `wait_ok()` 轮询完成；Flash 地址和长度最低位被清零，要求偶数字节对齐。`norflash_read/write` 把该流程封装成缓冲区接口。

---

## 十六、`driver/log.c/.h`

日志等级枚举从 DEBUG、INFO、WARN、ERROR 到 NONE。`CURRENT_LOG_LEVEL=NONE`，所以四个 `LOG_*` 函数都会在格式化前返回。

`current_log` 是只读当前阈值；`log_level_strings[]` 是四个前缀。每个日志函数的 `fmt` 是格式串，`args` 接收可变参数，`buffer[1024]` 保存 `vsprintf` 结果，`i` 逐字节发 UART3。

`putchar()` 把标准 `printf` 重定向到 `DEBUG_UART=3`。`printHexArray()` 直接调用 `printf`，不检查 `current_log`；因此 `test_init()` 的十六进制输出仍会发生，即使普通日志关闭。

`vsprintf` 没有长度限制，若未来传入超过 1023 字节的格式化结果会溢出局部缓冲；当前日志内容很短。

---

## 十七、`driver/canbus.c/.h` 与 `driver/t5l_io.c`

`CANUNIT`：`ID` 为 11/29 位标识；`status` 的 bit7=IDE、bit6=RTR、bit3..0=DLC；`candata[8]` 为数据。

`CANBUSUNIT` 含 256 个接收单元、256 个发送单元以及 RX/TX 的 head/tail/flag。全局实例 `CanData` 由 ISR 和主循环共享。8 位 head/tail 自然在 255 后回到 0，但同样没有单独满标志。

`can_baud[4][4]` 是四档硬件分频参数；`CanBustxaddr[27]` 是模板发送 ID 表。`CanBus_Init()` 设置 P0.2/P0.3、CAN 复用、波特率、验收码/掩码并开 CAN 中断。

`CanRx()` 当前只识别 `0x180100F4`，处理代码也被注释，所以仅消费队列；`CAN_tx()` 把状态、ID 和 8 字节写硬件发送缓存并触发发送；`CanErrorReset()` 清总线错误；`Can_test_func()` 发送固定 01～08，但主程序未调用。

`t5l_io.c` 的 `SetPinOut/SetPinIn` 根据端口号 0～3 对 `PxMDOUT` 的指定位进行置位或清位。`Pin` 没有范围检查，调用者应限于 0～7。当前只被 CAN 初始化用于 P0.2 输出、P0.3 输入。

---

## 十八、`driver/rtc.c/.h`

该文件包含两整套软件 I²C RTC：`SELECT_RTC_TYPE==1` 为 RX8130，`==2` 为 SD2058。当前值为 0，预处理后两套实现都不进入固件执行路径，只留下空的 `rtc_init()` 和 `rtc_deal()`。

若选择非零：P3.3 是 SDA、P3.2 是 SCL；`RTC_SDA_IN/OUT` 切换 SDA 方向；各芯片的 start/stop、ACK/NACK、read/write byte 实现软件 I²C；初始化检查掉电状态；读时把 BCD 转十进制并写 DGUS `0x0010～0x0013`；设置时从 `0x009C` 附近读取页面设置值并写 RTC。

RX8130 的 `RTC_get_week_rx8130()` 和 SD2058 的 `RTC_get_week_sd2058()` 用日期公式计算星期；局部 year/month/day 和中间量只参与计算。`BCD()` 把普通十进制转 BCD。

本项目不应把 `SELECT_RTC_TYPE` 随意改成 1/2，因为当前统一时间源是 STC 的 DS1302，且 P3.2/P3.3 可能与实际硬件资源冲突。

---

## 十九、`driver/other_tools.c`、头文件和启动汇编

`get_bit_u8/u16` 返回某位是否为 1；`set_bit_u8/u16` 按 `bit_value` 置位或清位，并分别检查位置不超过 7/15；`toggle_bit_u8/u16` 用异或翻转。`get` 和 `toggle` 没有范围检查，错误移位参数应由调用者避免。

各 `.h` 文件主要提供头文件保护、类型/结构体、宏、`extern` 全局声明和函数原型。`extern` 只声明对象在别处定义，不重复分配存储。

`STARTUP_M5.A51` 在复位地址 0 跳转到启动代码，并在 `0x00F8` 放置 `DWINT5` 识别信息；设置 64 KB code 与 32 KB xdata 页、1T CPU 模式、T2 自动重装、UART2/3 复用和 RAMMODE；清 `0x80` 字节 IDATA；XDATA/PDATA 批量清零长度均为 0；三个可重入函数软件栈关闭；为普通栈段保留 30 字节并设置 SP；最后跳到 Keil `?C_START`，由 C 运行库初始化并调用 `main()`。

---

## 二十、当前固件状态和关键注意点

| 事项 | 当前真实状态 |
|---|---|
| STC 通信 | UART4、9600、RS-485、项目 CRC8，正在使用 |
| 官方 82/83 协议 | UART2/3/5 使用；UART4 明确关闭 |
| 通信超时 | 只由有效 STATUS 清零，2 秒进入页面 2 |
| 命令确认 | 120 ms 重试，总共发 3 次，约 360 ms 判超时 |
| 时间源 | STC DS1302；DWIN 外接 RTC 驱动关闭 |
| CAN | 初始化和接收轮询仍运行，但无快递业务 |
| VP 掉电保存 | 调用已注释，且 `SAVE_NUMBER=0` |
| 日志 | 普通 LOG 全关；`printHexArray` 仍可输出 |
| 页面回调 | 只有页面 0 配置了三个空回调 |
| `deal.c` 演示 | 当前仍运行，可能短暂覆盖重量 VP 并持续移动演示文本 |
| UART 环形缓冲 | 无满检测；UART4 的 16 位 tail 读取没有原子快照 |
| 待 ACK 命令 | 新触摸可覆盖旧待确认事务，代码未做忙保护 |

从维护角度看，快递业务的最小必要路径是 `main → timer/UART 中断 → parcel_link → dgus_tools`。页面模板、CAN、外接 RTC、Flash 保存、82/83 多串口桥和文本移动演示属于通用底座或遗留功能。阅读时应先掌握核心路径，再决定是否精简外围代码，不能仅因为文件参与编译就认为它正在承担快递终端功能。
