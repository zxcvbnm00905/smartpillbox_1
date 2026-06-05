/**
 * @file    bsp_rtc.c
 * @brief   STM32内部RTC驱动 - 使用外部32.768kHz晶振
 * @note    PCLK1=36MHz, RTC预分频=35999 -> 1秒
 */

#include "bsp_rtc.h"
#include <stdio.h>
#include <string.h>

/* ==================== 闹钟存储 (模拟EEPROM, 实际项目中写入AT24C02或Flash) ==================== */
static RTC_Alarm g_Alarms[MAX_ALARMS];

/* ==================== RTC 初始化 ==================== */
void RTC_Init(void)
{
    /* 使能 PWR 和 BKP 时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

    /* 允许访问备份域 */
    PWR_BackupAccessCmd(ENABLE);

    /* 复位备份域 */
    BKP_DeInit();

    /* 使能 LSE (32.768kHz) */
    RCC_LSEConfig(RCC_LSE_ON);
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);

    /* 选择 LSE 为 RTC 时钟源 */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
    RCC_RTCCLKCmd(ENABLE);

    /* 等待 RTC 寄存器同步 */
    RTC_WaitForSynchro();
    RTC_WaitForLastTask();

    /* 设置预分频: 32768 - 1 = 32767 (1Hz) */
    RTC_SetPrescaler(32767);
    RTC_WaitForLastTask();

    /* 初始化闹钟为空 */
    for (uint8_t i = 0; i < MAX_ALARMS; i++) {
        g_Alarms[i].hour    = 0;
        g_Alarms[i].minute  = 0;
        g_Alarms[i].enabled = 0;
        sprintf(g_Alarms[i].label, "闹钟%d", i + 1);
    }
}

/* ==================== 时间设置/读取 ==================== */
void RTC_SetTime(RTC_Time *time)
{
    /* 计算从2000-01-01到目标日期的天数 */
    uint32_t days = 0;
    const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    for (uint16_t y = 2000; y < time->year; y++) {
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))
            days += 366;
        else
            days += 365;
    }

    for (uint8_t m = 1; m < time->month; m++) {
        days += daysInMonth[m - 1];
        if (m == 2 && ((time->year % 4 == 0 && time->year % 100 != 0) ||
                        (time->year % 400 == 0)))
            days += 1;
    }
    days += time->day - 1;

    /* 计算星期几 (2000-01-01是星期六) */
    time->week = (days + 6) % 7;

    /* 计算秒数 */
    uint32_t seconds = days * 86400;
    seconds += (uint32_t)time->hour * 3600;
    seconds += (uint32_t)time->minute * 60;
    seconds += time->second;

    /* 写入RTC计数器 */
    RTC_WaitForLastTask();
    RTC_SetCounter(seconds);
    RTC_WaitForLastTask();
}

void RTC_GetTime(RTC_Time *time)
{
    uint32_t seconds;
    uint32_t days;
    uint32_t yearDays;

    RTC_WaitForLastTask();
    seconds = RTC_GetCounter();

    /* 分解秒 -> 日/时/分/秒 */
    time->second = seconds % 60;
    time->minute = (seconds / 60) % 60;
    time->hour   = (seconds / 3600) % 24;
    days = seconds / 86400;

    /* 分解天数 -> 年/月/日 */
    time->year = 2000;
    while (1) {
        if ((time->year % 4 == 0 && time->year % 100 != 0) ||
            (time->year % 400 == 0))
            yearDays = 366;
        else
            yearDays = 365;
        if (days < yearDays) break;
        days -= yearDays;
        time->year++;
    }

    time->week = (days + 6) % 7;  /* 2000-01-01是星期六 */

    const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    time->month = 1;
    for (uint8_t m = 0; m < 12; m++) {
        uint8_t dim = daysInMonth[m];
        if (m == 1 && yearDays == 366) dim = 29;
        if (days < dim) break;
        days -= dim;
        time->month++;
    }
    time->day = days + 1;
}

/* ==================== 闹钟管理 ==================== */
void BSP_RTC_Alarm(uint8_t index, uint8_t hour, uint8_t minute, uint8_t enabled)
{
    if (index >= MAX_ALARMS) return;
    g_Alarms[index].hour    = hour;
    g_Alarms[index].minute  = minute;
    g_Alarms[index].enabled = enabled;
}

void RTC_GetAlarm(uint8_t index, RTC_Alarm *alarm)
{
    if (index >= MAX_ALARMS) return;
    memcpy(alarm, &g_Alarms[index], sizeof(RTC_Alarm));
}

/**
 * @brief  检查是否有闹钟触发
 * @return 触发的闹钟索引 (0~5), 0xFF 表示无触发
 */
uint8_t RTC_CheckAlarm(RTC_Time *now)
{
    /* 只比较时和分, 秒变化时检查一次避免重复触发 */
    if (now->second != 0) return 0xFF;

    for (uint8_t i = 0; i < MAX_ALARMS; i++) {
        if (g_Alarms[i].enabled &&
            g_Alarms[i].hour == now->hour &&
            g_Alarms[i].minute == now->minute) {
            return i;  /* 闹钟 i 触发 */
        }
    }
    return 0xFF;
}

/* ==================== 字符串转换 ==================== */
void RTC_TimeToString(RTC_Time *time, char *buf)
{
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
            time->year, time->month, time->day,
            time->hour, time->minute, time->second);
}

void RTC_WeekToString(uint8_t week, char *buf)
{
    const char *weekNames[] = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"};
    if (week < 7)
        strcpy(buf, weekNames[week]);
    else
        strcpy(buf, "??");
}
