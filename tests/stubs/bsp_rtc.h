#ifndef BSP_RTC_H
#define BSP_RTC_H

#include "stm32f10x.h"

#define MAX_ALARMS 6

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t week;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} RTC_Time;

typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t enabled;
    char label[16];
} RTC_Alarm;

void RTC_GetTime(RTC_Time *time);
void BSP_RTC_Alarm(uint8_t index, uint8_t hour, uint8_t minute, uint8_t enabled);

#endif
