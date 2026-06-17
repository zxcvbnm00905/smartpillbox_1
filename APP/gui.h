/**
 * @file    gui.h
 * @brief   智能药盒 GUI - 3.2寸 TFT 触摸屏界面
 */

#ifndef __GUI_H
#define __GUI_H

#include "stm32f10x.h"
#include "bsp_rtc.h"
#include "pillbox.h"

/* ==================== 页面枚举 ==================== */
typedef enum {
    PAGE_MAIN = 0,         /* 主页: 时间 + 药品清单 + 状态 */
    PAGE_ALARM_ALERT,      /* 闹钟提醒弹窗 */
    PAGE_MED_SETTING       /* 药品/闹钟设置页面 */
} GUI_Page;

/* ==================== 触摸按钮定义 ==================== */
#define BTN_NONE            0
#define BTN_CONFIRM_TAKEN   1   /* "确认取药" / "已服药" */
#define BTN_SNOOZE          2   /* "稍后提醒" */
#define BTN_SET_MEDS        3   /* 进入药品设置 */
#define BTN_BACK            4   /* 返回主页 */
#define BTN_TOGGLE_ENABLE   5   /* 启用/禁用某药品 */
#define BTN_HOUR_UP         6   /* 时间+ */
#define BTN_HOUR_DOWN       7   /* 时间- */
#define BTN_MIN_UP          8   /* 分钟+ */
#define BTN_MIN_DOWN        9   /* 分钟- */
#define BTN_SELECT_NEXT     10  /* 下一个药品 */
#define BTN_SELECT_PREV     11  /* 上一个药品 */

/* ==================== 函数声明 ==================== */
void GUI_Init(void);
GUI_Page GUI_GetPage(void);
void GUI_SetWiFiOnline(uint8_t online);
void GUI_ReturnHome(void);
void GUI_ShowMainPage(RTC_Time *time);
void GUI_ShowAlarmAlert(Medicine *med);
void GUI_ShowMedSetting(Medicine *meds, uint8_t count, uint8_t selected);
uint8_t GUI_CheckButton(uint16_t x, uint16_t y);

#endif /* __GUI_H */
