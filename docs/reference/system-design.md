# 系统总体设计（原型 v0.1）

## 已冻结的 MVP 结构

```text
称重传感器 + HX710/HX711
          │ DOUT / SCK
          ▼
┌────────────────────────────┐
│ 主控板 A                   │
│ 采样 → 去皮/标定 → 滤波    │
│      → 稳定判断 → 整数计费 │
└─────────────┬──────────────┘
              │ RS-485：A/B + 共地
              ▼
┌────────────────────────────┐
│ EKT043_EDU                 │
│ T5L OS CPU UART4/RS-485    │
│ 协议解析 → DGUS VP 更新    │
└─────────────┬──────────────┘
              │ DGUS VP 变量
              ▼
      迪文屏显示与触摸操作
```

课程不强制两块 STC 同时参与主流程，因此不再设置“显示板 B”。电子秤仍连接 STC 的 EXT，并把 P1.0/P1.1 当作称重接口；STC UART2 映射到 P4.6/P4.7 驱动板载 RS-485。第二块 STC 作为备件或后期 RFID/超声波扩展控制器。

## 主控板 A 模块

| 模块 | 输入 | 输出 | 周期/触发 |
|---|---|---|---|
| `weight_driver` | HX711 DOUT/SCK | 16 位有符号原始值 | 采样就绪 |
| `weight_calibration` | 原始值、零点、标定系数 | 净重（g） | 每次采样 |
| `weight_filter` | 净重样本 | 滤波重量（g） | 20-50 ms |
| `stability` | 最近 N 个样本 | 稳定标志 | 100 ms |
| `billing` | 稳定重量、计费参数 | 金额（分） | 稳定边沿/重新计费 |
| `system_state` | 重量、稳定、异常、通信 | 状态与错误码 | 状态变化 |
| `rs485_link` | 状态快照/屏幕命令 | 协议帧 | 100 ms + 事件 |
| `parameter_store` | 新参数 | EEPROM/IAP 数据 | 保存命令 |

## DWIN T5L OS 模块

| 模块 | 职责 |
|---|---|
| `rs485_link` | 通过 UART4/RS-485 接收状态、回复确认、发送触摸命令、监测超时 |
| `dwin_adapter` | 将状态快照写入 VP 地址，将 VP 触摸事件变成命令 |
| `page_controller` | 主页面、设置页、错误页切换 |
| `ui_cache` | 只在值变化时更新屏幕，减少串口负载 |

## 状态机

```text
上电
 │
 ▼
IDLE ──放置包裹──► WEIGHING ──连续稳定──► STABLE ──计算成功──► BILLING
 ▲                     │                                      │
 └────净重归零─────────┴──────────重量再次变化────────────────┘

任意状态 ──传感器/超量程/通信异常──► ERROR
ERROR ──异常解除且自检通过──► IDLE 或 WEIGHING
```

## 稳定判定建议

- 采样周期：20-50 ms；
- 先对 5 点做中值滤波，再做 8 点滑动平均；
- 最近 10 个滤波值的最大值与最小值差不超过阈值（例如 5 g），持续 500 ms 后判定稳定；
- 阈值必须结合实物噪声测试调整，不写死为最终值；
- 由“不稳定→稳定”的边沿触发一次计费，避免每个循环重复计费。

## 整数计费

```text
weight_g <= 0:
    price_cent = 0
0 < weight_g <= first_weight_g:
    price_cent = first_price_cent
weight_g > first_weight_g:
    steps = (weight_g - first_weight_g + additional_step_g - 1)
            / additional_step_g
    price_cent = first_price_cent + steps * additional_price_cent
```

所有变量使用克和分；`steps` 使用整数除法向上取整。
