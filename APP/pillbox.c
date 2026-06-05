/**
 * Smart Pill Box - Core Logic
 * Medicine management, alarm checking, daily reset
 */
#include "pillbox.h"
#include "bsp_beep.h"
#include "bsp_led.h"
#include <stdio.h>
#include <string.h>

static Medicine g_Medicines[MAX_MEDICINES] = {
    {"Amoxicillin",  7,30,1,0},
    {"Cold Medicine",12,30,1,0},
    {"Vitamin C",   18,30,1,0},
    {"BP Med",      21, 0,0,0},
};

static PillBox_State g_State = STATE_NORMAL;
static uint8_t  g_TriggeredIdx = 0xFF;
static uint32_t g_AlarmTick = 0;
static uint8_t  g_LastDay = 0xFF;

void PillBox_Init(void)
{
    g_State = STATE_NORMAL;
    g_TriggeredIdx = 0xFF;
    for(uint8_t i=0;i<MAX_MEDICINES;i++){
        BSP_RTC_Alarm(i,g_Medicines[i].alarmHour,
                      g_Medicines[i].alarmMinute,g_Medicines[i].enabled);
        g_Medicines[i].taken=0;
    }
}

PillBox_State PillBox_GetState(void){return g_State;}

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
    g_TriggeredIdx=0xFF;g_State=STATE_NORMAL;
}

void PillBox_Snooze(void)
{
    BEEP_OFF();LED_OFF();
    g_TriggeredIdx=0xFF;g_State=STATE_NORMAL;
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

void PillBox_Process(void)
{
    RTC_Time now;RTC_GetTime(&now);
    if(g_LastDay==0xFF)g_LastDay=now.day;
    if(now.day!=g_LastDay){PillBox_ResetDaily();g_LastDay=now.day;}

    switch(g_State){
    case STATE_NORMAL:{
        static uint8_t ls=0xFF;
        if(now.second!=ls){ls=now.second;
            for(uint8_t i=0;i<MAX_MEDICINES;i++){
                if(!g_Medicines[i].enabled||g_Medicines[i].taken)continue;
                if(g_Medicines[i].alarmHour==now.hour&&
                   g_Medicines[i].alarmMinute==now.minute&&now.second==0){
                    g_State=STATE_ALARM;g_TriggeredIdx=i;g_AlarmTick=0;break;
                }
            }
        }
        break;
    }
    case STATE_ALARM:{
        g_AlarmTick++;
        if(g_AlarmTick%300==0)LED_TOGGLE();
        if(g_AlarmTick%400<200)BEEP_ON();else BEEP_OFF();
        if(g_AlarmTick>120000){BEEP_OFF();LED_OFF();g_State=STATE_NORMAL;g_TriggeredIdx=0xFF;}
        break;
    }
    case STATE_SETTING:break;
    }
}
