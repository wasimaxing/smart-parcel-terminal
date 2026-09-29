# STC工程使用说明

## 打开与编译

1. 使用 Keil µVision 打开 `firmware/stc/parcel-terminal-stc.uvproj`。
2. 目标应为 `IAP15F2K61S2`。
3. 点击 Build/Rebuild。
4. 成功后 HEX 位于 `firmware/stc/outputs/ParcelTerminal_STC.hex`。

当前正式配置在 `firmware/stc/inc/project_config.h`：

- `LINK_TEST_MODE=0`：使用真实电子秤；设为1仅用于固定数据链路测试。
- `WEIGHT_RAW_COUNT_MODE=1`：使用原始变化量兼容现有电子秤，不依赖已知砝码跨度标定。
- 状态发送周期500 ms，称重采样周期20 ms。
- 最大重量30 kg。

## 关键模块

| 文件 | 作用 |
|---|---|
| `source/main.c` | 初始化、任务调度、命令执行、状态/参数/RTC发送 |
| `source/weight_service.c` | 清零、去皮、标定换算、移动平均和稳定判定 |
| `source/billing.c` | 三档固定费率和向上取整计费 |
| `source/parcel_protocol.c` | 26字节帧、CRC8和大端整数工具 |
| `inc/project_config.h` | 链路测试、称重模式、周期与量程配置 |

## 三档费率

| 地区 | 首重 | 续重 |
|---|---|---|
| 省内 | 1 kg / 8元 | 2元/kg |
| 普通省外 | 1 kg / 12元 | 4元/kg |
| 偏远地区 | 1 kg / 18元 | 8元/kg |

续重不足1 kg按1 kg计算。屏幕不能修改费率，只能发送地区编号1、2、3。M24C02地址 `0xE0` 保存带 `0xA0` 标记的地区编号。

## BSP与硬件接口

- `EXTInit(enumEXTWeight)`、`GetWeight()`：课程BSP提供的称重接口。
- `Uart2Init(..., Uart2Usedfor485)`：STC侧RS-485串口。
- `GetKeyAct(enumKey1)`：板载K1后备清零入口。
- `M24C02_Read/Write()`：地区掉电保存。
- `DS1302Init()`、`RTC_Read()`：当前RTC读取入口。

这些函数来自课程库，不应在不知道硬件映射的情况下随意修改。
