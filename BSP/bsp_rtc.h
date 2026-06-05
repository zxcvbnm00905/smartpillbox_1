/**
 * @file    bsp_rtc.h
 * @brief   STM32内部RTC驱动
 */

#ifndef __BSP_RTC_H
#define __BSP_RTC_H

#include "stm32f10x.h"
#include <stdio.h>

/* 时间结构体 */
typedef struct {
    uint16_t year;    /* 年 (2000~2099) */
    uint8_t  month;   /* 月 (1~12) */
    uint8_t  day;     /* 日 (1~31) */
    uint8_t  hour;    /* 时 (0~23) */
    uint8_t  minute;  /* 分 (0~59) */
    uint8_t  second;  /* 秒 (0~59) */
    uint8_t  week;    /* 星期 (0=日, 1~6) */
} RTC_Time;

/* 闹钟结构体 */
typedef struct {
    uint8_t  hour;
    uint8_t  minute;
    uint8_t  enabled;   /* 0=禁用, 1=启用 */
    char     label[16]; /* 闹钟标签, 如 "早餐后" */
} RTC_Alarm;

#define MAX_ALARMS  6   /* 最多6组闹钟 */

/* 函数声明 */
void RTC_Init(void);
void RTC_SetTime(RTC_Time *time);
void RTC_GetTime(RTC_Time *time);
void BSP_RTC_Alarm(uint8_t index, uint8_t hour, uint8_t minute, uint8_t enabled);
void RTC_GetAlarm(uint8_t index, RTC_Alarm *alarm);
uint8_t RTC_CheckAlarm(RTC_Time *now);
void RTC_TimeToString(RTC_Time *time, char *buf);
void RTC_WeekToString(uint8_t week, char *buf);

#endif /* __BSP_RTC_H */
