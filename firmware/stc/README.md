# STC 主控工程

目标芯片按实物选择 `IAP15F2K61S2 / 20I-P44`。工程沿用课程 BSP 3.6b 的 `STC15F2K60S2.H` 和 `STCBSP_V3.6b.LIB`。

## 构建

1. 用 Keil µVision 打开 `parcel-terminal-stc.uvproj`；
2. 确认 Include Path 为 `./inc`；
3. 构建目标 `IAP15F2K61S2`；
4. 用 STC-ISP 以 11.0592 MHz 下载频率烧录生成的 HEX。

项目设置为将构建输出写到 `outputs/`，该目录已被 Git 忽略。`release/ParcelTerminal_STC.hex` 是整理仓库时保留的已验证固件。

## 运行要点

- `project_config.h` 默认 `LINK_TEST_MODE=0`，读取 EXT 电子秤真实数据；
- `WEIGHT_RAW_COUNT_MODE=1` 时无需砝码即可观察重量变化，但单位精度取决于硬件；
- 精确计量时将该宏改为 0，并使用已知质量完成标定；
- K1（P3.2）和 DWIN 命令均可触发清零；
- UART2 使用板载 MAX485，不能配置为 `Uart2UsedforEXT`；
- DS1302 是系统时间源，M24C02 保存当前地区。

详细说明见仓库根目录下的 `docs/02_STC工程使用说明.md`。
