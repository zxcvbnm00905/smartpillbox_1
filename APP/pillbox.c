/**
 * Smart Pill Box - Core Logic
 * Medicine management, alarm checking, daily reset
 */
#include "pillbox.h"
#include "bsp_beep.h"
#include "bsp_led.h"
#include <stdio.h>
#include <string.h>

#define SNOOZE_MINUTES  5

static Medicine g_Medicines[MAX_MEDICINES] = {
    {"Amoxicillin",  7,30,1,0,"1 tab"},
    {"Cold Medicine",12,30,1,0,"2 tabs"},
    {"Vitamin C",   18,30,1,0,"1 tab"},
    {"BP Med",      21, 0,0,0,"1 tab"},
};

static PillBox_State g_State = STATE_NORMAL;
static uint8_t  g_TriggeredIdx = 0xFF;
static uint8_t  g_LastDay = 0xFF;
static uint8_t  g_LastAlarmIdx = 0xFF;
static uint8_t  g_LastAlarmDay = 0xFF;
static uint8_t  g_LastAlarmHour = 0xFF;
static uint8_t  g_LastAlarmMinute = 0xFF;
static uint8_t  g_AlarmElapsedSec = 0;
static uint8_t  g_AlarmLastSecond = 0xFF;
static uint8_t  g_SnoozeIdx = 0xFF;
static uint32_t g_SnoozeDueMinute = 0;


static uint32_t PillBox_NowMinute(RTC_Time *time)
{
    return (uint32_t)time->day * 1440u + (uint32_t)time->hour * 60u + time->minute;
}

static void PillBox_StartAlarm(uint8_t idx, RTC_Time *now)
{
    g_State = STATE_ALARM;
    g_TriggeredIdx = idx;
    g_LastAlarmIdx = idx;
    g_LastAlarmDay = now->day;
    g_LastAlarmHour = now->hour;
    g_LastAlarmMinute = now->minute;
    g_AlarmElapsedSec = 0;
    g_AlarmLastSecond = now->second;
    BEEP_ON();
    LED_ON();
}
void PillBox_Init(void)
{
    g_State = STATE_NORMAL;
    g_TriggeredIdx = 0xFF;
    g_AlarmElapsedSec = 0;
    g_AlarmLastSecond = 0xFF;
    g_SnoozeIdx = 0xFF;
    g_SnoozeDueMinute = 0;
    for(uint8_t i=0;i<MAX_MEDICINES;i++){
        BSP_RTC_Alarm(i,g_Medicines[i].alarmHour,
                      g_Medicines[i].alarmMinute,g_Medicines[i].enabled);
        g_Medicines[i].taken=0;
    }
}

PillBox_State PillBox_GetState(void){return g_State;}
uint8_t PillBox_GetTriggeredIndex(void){return g_TriggeredIdx;}

Medicine* PillBox_GetMedicines(void){return g_Medicines;}
uint8_t PillBox_GetMedicineCount(void){return MAX_MEDICINES;}

void PillBox_SetMedicine(uint8_t idx,const char*name,uint8_t h,uint8_t m,uint8_t en)
{
    if(idx>=MAX_MEDICINES)return;
    strncpy(g_Medicines[idx].name,name,MED_NAME_LEN-1);
    g_Medicines[idx].name[MED_NAME_LEN-1]='\0';
    g_Medicines[idx].alarmHour=h;
    g_Medicines[idx].alarmMinute=m;
    g_Medicines[idx].enabled=en;
    g_Medicines[idx].taken=0;
    if(g_SnoozeIdx == idx){
        g_SnoozeIdx=0xFF;
        g_SnoozeDueMinute=0;
    }
    if(g_LastAlarmIdx == idx){
        g_LastAlarmIdx=0xFF;
        g_LastAlarmDay=0xFF;
        g_LastAlarmHour=0xFF;
        g_LastAlarmMinute=0xFF;
        g_SnoozeIdx=0xFF;
        g_SnoozeDueMinute=0;
    }
    BSP_RTC_Alarm(idx,h,m,en);
}

uint8_t PillBox_GetTakenCount(void){
    uint8_t c=0;for(uint8_t i=0;i<MAX_MEDICINES;i++)
        if(g_Medicines[i].enabled&&g_Medicines[i].taken)c++;return c;
}

uint8_t PillBox_GetTotalEnabled(void){
    uint8_t c=0;for(uint8_t i=0;i<MAX_MEDICINES;i++)
        if(g_Medicines[i].enabled)c++;return c;
}

void PillBox_ConfirmTaken(void)
{
    if(g_State==STATE_ALARM&&g_TriggeredIdx<MAX_MEDICINES)
        g_Medicines[g_TriggeredIdx].taken=1;
    BEEP_OFF();LED_OFF();
    g_SnoozeIdx=0xFF;g_SnoozeDueMinute=0;g_AlarmElapsedSec=0;g_AlarmLastSecond=0xFF;g_TriggeredIdx=0xFF;g_State=STATE_NORMAL;
}

void PillBox_Snooze(void)
{
    RTC_Time now;
    RTC_GetTime(&now);

    if(g_State==STATE_ALARM && g_TriggeredIdx<MAX_MEDICINES){
        g_SnoozeIdx = g_TriggeredIdx;
        g_SnoozeDueMinute = PillBox_NowMinute(&now) + SNOOZE_MINUTES;
    }

    BEEP_OFF();LED_OFF();
    g_AlarmElapsedSec=0;g_AlarmLastSecond=0xFF;g_TriggeredIdx=0xFF;g_State=STATE_NORMAL;
}

void PillBox_ResetDaily(void)
{
    for(uint8_t i=0;i<MAX_MEDICINES;i++)g_Medicines[i].taken=0;
}

void PillBox_GetNextDoseString(char*buf)
{
    RTC_Time now;RTC_GetTime(&now);
    int16_t minDiff=9999;uint8_t ni=0,found=0;
    for(uint8_t i=0;i<MAX_MEDICINES;i++){
        if(!g_Medicines[i].enabled)continue;
        int16_t am=(int16_t)g_Medicines[i].alarmHour*60+g_Medicines[i].alarmMinute;
        int16_t nm=(int16_t)now.hour*60+now.minute;
        int16_t diff=am-nm;
        if(diff>0&&diff<minDiff){minDiff=diff;ni=i;found=1;}
    }
    if(found)sprintf(buf,"Next: %s %02d:%02d",
                     g_Medicines[ni].name,g_Medicines[ni].alarmHour,g_Medicines[ni].alarmMinute);
    else sprintf(buf,"All done for today!");
}


uint8_t PillBox_GetSnoozeRemaining(uint8_t *idx, uint16_t *remainMin)
{
    RTC_Time now;
    uint32_t nowMinute;

    if(g_SnoozeIdx >= MAX_MEDICINES)return 0;

    RTC_GetTime(&now);
    nowMinute = PillBox_NowMinute(&now);
    if(g_SnoozeDueMinute <= nowMinute){
        if(idx)*idx = g_SnoozeIdx;
        if(remainMin)*remainMin = 0;
        return 1;
    }

    if(idx)*idx = g_SnoozeIdx;
    if(remainMin)*remainMin = (uint16_t)(g_SnoozeDueMinute - nowMinute);
    return 1;
}
void PillBox_Process(void)
{
    RTC_Time now;RTC_GetTime(&now);
    if(g_LastDay==0xFF)g_LastDay=now.day;
    if(now.day!=g_LastDay){
        PillBox_ResetDaily();
        g_LastDay=now.day;
        g_LastAlarmIdx=0xFF;
        g_LastAlarmDay=0xFF;
        g_LastAlarmHour=0xFF;
        g_LastAlarmMinute=0xFF;
    }

    switch(g_State){
    case STATE_NORMAL:{
        uint32_t nowMinute = PillBox_NowMinute(&now);

        if(g_SnoozeIdx < MAX_MEDICINES){
            if(!g_Medicines[g_SnoozeIdx].enabled || g_Medicines[g_SnoozeIdx].taken){
                g_SnoozeIdx = 0xFF;
                g_SnoozeDueMinute = 0;
            }else if(nowMinute >= g_SnoozeDueMinute){
                uint8_t idx = g_SnoozeIdx;
                g_SnoozeIdx = 0xFF;
                g_SnoozeDueMinute = 0;
                PillBox_StartAlarm(idx, &now);
                break;
            }
        }

        for(uint8_t i=0;i<MAX_MEDICINES;i++){
            uint8_t alreadyAlarmed = (g_LastAlarmIdx == i &&
                                      g_LastAlarmDay == now.day &&
                                      g_LastAlarmHour == now.hour &&
                                      g_LastAlarmMinute == now.minute);
            if(!g_Medicines[i].enabled || g_Medicines[i].taken || alreadyAlarmed)continue;
            if(g_Medicines[i].alarmHour == now.hour &&
               g_Medicines[i].alarmMinute == now.minute){
                PillBox_StartAlarm(i, &now);
                break;
            }
        }
        break;
    }
    case STATE_ALARM:{
        if(g_TriggeredIdx >= MAX_MEDICINES){
            BEEP_OFF();
            LED_OFF();
            g_State = STATE_NORMAL;
            g_TriggeredIdx = 0xFF;
            g_AlarmElapsedSec = 0;
            g_AlarmLastSecond = 0xFF;
            break;
        }

        if(now.second != g_AlarmLastSecond){
            g_AlarmLastSecond = now.second;
            if(g_AlarmElapsedSec < 255)g_AlarmElapsedSec++;
        }

        if(g_AlarmElapsedSec >= 10 ||
           now.hour != g_LastAlarmHour ||
           now.minute != g_LastAlarmMinute){
            BEEP_OFF();
            LED_OFF();
            g_State = STATE_NORMAL;
            g_TriggeredIdx = 0xFF;
            g_AlarmElapsedSec = 0;
            g_AlarmLastSecond = 0xFF;
            break;
        }

        if(now.second % 2 == 0){
            BEEP_ON();
            LED_ON();
        }else{
            BEEP_OFF();
            LED_OFF();
        }
        break;
    }
    case STATE_SETTING:break;
    }
}
