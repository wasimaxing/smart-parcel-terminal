# DGUS 变量地址表（原型暂定）

此表用于界面与固件协同开发。地址是项目自行规划的 VP 地址，不表示已写入现有 DWIN 工程；最终以 DGUS 工程配置为准。

## 主控板 → 屏幕

| VP 地址 | 名称 | 类型/长度 | 单位 | 说明 |
|---|---|---|---|---|
| 0x1000 | `weight_g` | S32 / 2 words | g | 去皮后的净重 |
| 0x1002 | `price_cent` | U32 / 2 words | 分 | 当前应付金额 |
| 0x1005 | `status_flags` | U16 bitset | - | bit0 稳定、bit1 传感器正常、bit2 未超量程、bit3 参数有效 |
| 0x1006 | `first_weight_g` | U32 / 2 words | g | 首重重量 |
| 0x1008 | `first_price_cent` | U32 / 2 words | 分 | 首重费用 |
| 0x100A | `additional_step_g` | U32 / 2 words | g | 续重单位 |
| 0x100C | `additional_price_cent` | U32 / 2 words | 分 | 每续重单位费用 |
| 0x100E | `height_mm` | U16 | mm | 后期扩展，0 表示无数据 |
| 0x100F | `error_code` | U16 | - | 0 正常，其他值见协议文档 |
| 0x1010 | `tare_g` | U32 / 2 words | g | 当前皮重 |
| 0x0010～0x0013 | `rtc` | DGUS系统RTC / 4 words | - | 年月、日星期、时分、秒；由STC的DS1302同步 |
| 0x1030 | `region_id` | U16 | - | 1 省内 / 2 普通省外 / 3 偏远地区 |
| 0x1031 | `system_state` | U16 | - | 0 IDLE / 1 WEIGHING / 2 STABLE / 3 BILLING / 4 ERROR，仅作数值状态码 |
| 0x1200 | `region_text` | ASCII / 10 bytes | - | 地区英文标识，内部/调试使用 |
| 0x1210 | `rule_text` | ASCII / 20 bytes | - | 地区费率英文标识，内部/调试使用 |
| 0x1220 | `state_text` | ASCII / 10 bytes | - | 主页面短状态文本 |
| 0x1230 | `status_text` | ASCII / 20 bytes | - | 主页面状态说明或当前地区费率 |

## 屏幕 → 主控板

| VP 地址 | 名称 | 类型 | 值 | 说明 |
|---|---|---|---|---|
| 0x1100 | `action_code` | U16 | 1～3、5～8 | 清零/去皮/重新计费/标定/省内/普通省外/偏远地区；4 保留不用 |
| 0x1101 | `action_seq` | U16 | 递增 | 防止同一触摸指令重复执行 |
| 0x1118 | `calibration_mass_g` | U32 | g | 标定时放置的已知砝码质量 |

## 约定

- 多字节整数统一使用大端顺序；
- 金额只用“分”，重量只用“克”，固件不使用浮点数；
- 通信是否正常由显示侧根据 2 秒接收超时自行判断，不占用主控状态标志；
- 计费规则固定为三档，设置页参数只读，不使用 0x1110～0x1117，也不发送 action_code=4；
- action_seq 每次操作加 1，主控只执行新序号；
- 屏幕显示的小数点由控件格式化，不改变寄存器中的整数值。
- 文本控件统一使用 0x00（8 位 ASCII）编码；文本缓冲区固定长度，短文本后补空格，避免残留上一次内容。
