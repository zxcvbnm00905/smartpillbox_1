# 智能药盒 - 课程设计

## 硬件平台 (仅板载资源)

| 组件 | 型号 | 连接引脚 |
|------|------|----------|
| 主控 | 野火指南者 STM32F103VET6 | - |
| 下载器 | DAP-Link | SWD |
| 显示屏 | 3.2寸 TFT LCD (ILI9341) | FSMC 16位 |
| 触摸屏 | XPT2046 | SPI2 (PB12~PB15) |
| 蜂鸣器 | 板载有源蜂鸣器 | PB2 |
| LED | 板载蓝色LED | PB5 |
| 按键 | KEY1 (WK_UP) / KEY2 (TAMPER) | PA0 / PC13 |
| RTC | 外部32.768kHz晶振 | PC14/PC15 |

## 功能列表

| 功能 | 实现方式 |
|------|----------|
| ⏰ 实时时钟 | RTC + 32.768kHz晶振, 屏幕显示年月日/星期/时分秒 |
| 💊 药品清单 | 4种药品, 每种独立闹钟时间, 触摸屏设置 |
| 🔔 服药提醒 | 闹钟触发 → 蜂鸣器响 + LED闪烁 + 屏幕红色弹窗 |
| ✅ 取药确认 | 按 KEY1 或触摸屏 → 标记"已服", 关闭闹钟 |
| ⏭️ 稍后提醒 | 按 KEY2 或触摸屏 → 关闭闹钟, 下次继续提醒 |
| 📊 进度跟踪 | 显示今日服药进度 (n/m) 和每种药的倒计时 |
| ⚙️ 闹钟设置 | 触摸屏设置每种药的服药时间和启用/停用 |

## 操作说明

| 操作 | 方式 |
|------|------|
| 查看时间和药品 | 主页自动显示 |
| 确认服药 | 闹钟响时按 **KEY1** 或触摸 **[已服药,确认]** |
| 稍后提醒 | 闹钟响时按 **KEY2** 或触摸 **[稍后提醒]** |
| 设置药品闹钟 | 触摸主页 **[药品设置]** → 选药品 → 调时间 |
| 启用/停用药品 | 设置页触摸 **[启用]/[停用]** 切换 |

## 工程目录结构

```
SmartPillBox/
├── USER/
│   ├── main.c              # 主程序 (系统初始化+主循环)
│   ├── stm32f10x_it.c      # 中断服务函数
│   └── stm32f10x_it.h
├── BSP/
│   ├── bsp_lcd.c / .h      # TFT LCD 驱动 (ILI9341/FSMC)
│   ├── bsp_touch.c / .h    # 触摸屏驱动 (XPT2046/SPI)
│   ├── bsp_rtc.c / .h      # RTC 时钟驱动
│   ├── bsp_led.c / .h      # LED 驱动 (PB5)
│   ├── bsp_beep.c / .h     # 蜂鸣器驱动 (PB2)
│   ├── bsp_key.c / .h      # 按键驱动 (PA0/PC13)
│   └── font.h              # ASCII 字库
└── APP/
    ├── pillbox.c / .h      # 药盒逻辑 (药品清单/闹钟/服药记录)
    └── gui.c / .h          # GUI 界面 (主页/闹钟弹窗/设置页)
```

## Keil5 工程配置

### 1. 新建工程
- Device: **STM32F103VE**
- 添加 STM32F10x_StdPeriph_Driver 标准库

### 2. 源文件
将所有 `USER/`、`BSP/`、`APP/` 下的 `.c` 文件添加到工程

### 3. Include Paths
```
.\USER;.\BSP;.\APP
..\Libraries\STM32F10x_StdPeriph_Driver\inc
..\Libraries\CMSIS\CM3\DeviceSupport\ST\STM32F10x
..\Libraries\CMSIS\CM3\CoreSupport
```

### 4. 预定义宏 (C/C++ → Define)
```
USE_STDPERIPH_DRIVER, STM32F10X_HD
```

### 5. 调试器 (Debug)
- Use: **CMSIS-DAP Debugger**
- SWD 模式

### 6. 需添加的标准库文件
stm32f10x_rcc.c, stm32f10x_gpio.c, stm32f10x_tim.c,
stm32f10x_spi.c, stm32f10x_fsmc.c, stm32f10x_rtc.c,
stm32f10x_bkp.c, stm32f10x_pwr.c, stm32f10x_flash.c,
stm32f10x_exti.c, misc.c
