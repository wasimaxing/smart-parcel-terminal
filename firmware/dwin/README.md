# DWIN T5L OS 与 DGUS 工程

本目录同时包含 T5L OS 核 Keil C51 工程和 DGUS 页面工程。

## 入口文件

- `parcel-terminal-dwin.uvproj`：OS 核固件；
- `DWprj.hmi`：DGUS 页面工程；
- `DWIN_SET/`：可直接复制到 SD 卡的下载包。

## 构建 OS 核

用 Keil µVision 打开 `parcel-terminal-dwin.uvproj` 并构建。构建成功后会运行 `build-dwin-set.bat`，由 `tools/hex_to_bin.ps1` 把 `obj/ParcelTerminal_DWIN.hex` 转为 `DWIN_SET/T5L51.bin`。转换脚本是纯 PowerShell，不需要额外提交可执行工具。

UART4 的关键配置位于 `sys/Parameter_Config.h`：

```c
#define UART4_ENABLE       1
#define UART4_485_ENABLE   1
#define UART4_8283_ENABLE  0
#define BAUD_UART4         9600
```

## 页面与触摸

- 页面 0：主页面；
- 页面 1：三档地区计费设置；
- 页面 2：通信中断/严重错误；
- 动作 VP `0x1100`：清零、去皮、重新计费、标定和地区选择；
- 序号 VP `0x1101`：用于命令去重；
- 诊断 VP `0x1102`～`0x1104`：动作回显、命令状态和固件标记。

如果用 DGUS Tool 重新生成了页面资源，可能覆盖已经修正的地区设置页。可重新运行：

```powershell
./tools/patch_region_dgus.ps1 -GeneratedPng ./assets/01_balanced_tier_cards_imagegen.png
```

该脚本会更新页面 1 背景、触摸区、状态文本 VP 与 `32bg.icl`。备份基线保存在 `assets/`。

详细说明见仓库根目录下的 `docs/03_DWIN工程使用说明.md`。
