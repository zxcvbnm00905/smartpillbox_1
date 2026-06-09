/**
 * Smart Pill Box GUI - Original Landscape 320x240
 */
#include "gui.h"
#include "bsp_lcd.h"
#include "bsp_touch.h"
#include <stdio.h>
#include <string.h>

static GUI_Page g_Page = PAGE_MAIN;
static const char* wk[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

GUI_Page GUI_GetPage(void){ return g_Page; }

static void Btn(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                const char* t, uint16_t bg, uint16_t fg)
{
    LCD_Fill(x1, y1, x2, y2, bg);
    LCD_DrawRect(x1, y1, x2, y2, GRAY);
    uint16_t tw = strlen(t) * 6;
    uint16_t cx = x1 + ((x2-x1)-tw)/2;
    uint16_t cy = y1 + ((y2-y1)-12)/2;
    LCD_ShowString(cx, cy, (char*)t, fg, bg, 12);
}

void GUI_ShowMainPage(RTC_Time *t)
{
    char b[60];
    g_Page = PAGE_MAIN;
    Medicine *m = PillBox_GetMedicines();
    uint8_t total = PillBox_GetTotalEnabled();
    uint8_t taken = PillBox_GetTakenCount();

    /* ????:???????????? (Y?? 30 ? 185) */
    /* ?????? LCD_Clear,???????????! */
    LCD_Fill(0, 30, 320, 185, WHITE);

    /* top bar */
    LCD_Fill(0, 0, 320, 30, DARKBLUE);
    sprintf(b, "%02d-%02d %s", t->month, t->day, wk[t->week]);
    LCD_ShowString(10, 7, b, WHITE, DARKBLUE, 12);
    sprintf(b, "%02d:%02d", t->hour, t->minute);
    LCD_ShowString(260, 7, b, WHITE, DARKBLUE, 12);

    uint8_t snoozeIdx;
    uint16_t snoozeRemain;

    /* next dose */
    Medicine *nx = NULL;
    for(uint8_t i=0; i<MAX_MEDICINES; i++)
        if(m[i].enabled && !m[i].taken) { nx = &m[i]; break; }

    if(nx) {
        LCD_ShowString(90, 45, ">>> NEXT DOSE <<<", GRAY, WHITE, 12);
        uint16_t nw = strlen(nx->name)*6;
        LCD_ShowString((320-nw)/2, 70, nx->name, BLACK, WHITE, 12);
        sprintf(b, "Dose: %s", nx->dose);
        LCD_ShowString(110, 95, b, DARKBLUE, WHITE, 12);

        int16_t ml = (int16_t)nx->alarmHour*60 + nx->alarmMinute
                   - ((int16_t)t->hour*60 + t->minute);
        if(ml > 0) {
            sprintf(b, "Scheduled: %02d:%02d", nx->alarmHour, nx->alarmMinute);
            LCD_ShowString(80, 118, b, DARKBLUE, WHITE, 12);
            sprintf(b, "in %d min", ml);
            LCD_ShowString(120, 138, b, RED, WHITE, 12);
        } else {
            LCD_ShowString(120, 130, "DUE NOW!", RED, WHITE, 12);
        }
    } else {
        LCD_ShowString(110, 80, "ALL DONE!", GREEN, WHITE, 12);
    }

    if(PillBox_GetSnoozeRemaining(&snoozeIdx, &snoozeRemain)) {
        sprintf(b, "Snooze: %s", m[snoozeIdx].name);
        LCD_ShowString(80, 145, b, RED, WHITE, 12);
        sprintf(b, "Remind again in %d min", snoozeRemain);
        LCD_ShowString(65, 160, b, RED, WHITE, 12);
    }

    /* progress */
    LCD_DrawRect(30, 172, 290, 183, GRAY);
    if(total > 0) {
        uint16_t bw = (uint16_t)((uint32_t)taken*258/total);
        LCD_Fill(31, 173, 31+bw, 182, (taken==total)?GREEN:LIGHTBLUE);
    }
    sprintf(b, "Taken: %d / %d", taken, total);
    LCD_ShowString(110, 172, b, DARKBLUE, WHITE, 12);

    /* buttons */
    LCD_DrawLine(0, 185, 320, 185, GRAY);
    LCD_Fill(0, 186, 320, 240, LIGHTGRAY);
    Btn(90, 195, 230, 230, "SETTINGS", BLUE, WHITE);
}

void GUI_ShowAlarmAlert(Medicine *med)
{
    char b[48];
    g_Page = PAGE_ALARM_ALERT;
    LCD_Clear(WHITE);
    LCD_Fill(0, 0, 320, 40, RED);
    LCD_ShowString(50, 12, "!!! TIME TO TAKE MEDS !!!", WHITE, RED, 12);

    uint16_t nw = strlen(med->name)*6;
    LCD_ShowString((320-nw)/2, 65, med->name, BLACK, WHITE, 12);
    sprintf(b, "Dose: %s", med->dose);
    LCD_ShowString(110, 95, b, DARKBLUE, WHITE, 12);
    sprintf(b, "Scheduled: %02d:%02d", med->alarmHour, med->alarmMinute);
    LCD_ShowString(80, 120, b, DARKBLUE, WHITE, 12);
    LCD_ShowString(40, 150, "Please take your medicine!", RED, WHITE, 12);

    LCD_DrawLine(0, 185, 320, 185, GRAY);
    LCD_Fill(0, 186, 320, 240, LIGHTGRAY);
    Btn(20, 195, 150, 230, "SNOOZE", RED, WHITE);
    Btn(170, 195, 300, 230, "TAKEN", GREEN, WHITE);
}

void GUI_ShowMedSetting(Medicine *meds, uint8_t n, uint8_t sel)
{
    char b[48];
    g_Page = PAGE_MED_SETTING;
    LCD_Clear(WHITE);
    LCD_Fill(0, 0, 320, 30, DARKBLUE);
    LCD_ShowString(100, 7, "Alarm Settings", WHITE, DARKBLUE, 12);

    for(uint8_t i=0; i<n; i++) {
        uint16_t y = 35 + i*45;
        uint16_t bg = (i==sel)?LIGHTBLUE:WHITE;
        LCD_Fill(0, y, 320, y+43, bg);

        sprintf(b, "%d. %-8s %02d:%02d %s", i+1, meds[i].name, meds[i].alarmHour, meds[i].alarmMinute, meds[i].dose);
        LCD_ShowString(10, y+15, b, BLACK, bg, 12);

        Btn(180, y+10, 210, y+35, "+H", (i==sel)?BLUE:GRAY, WHITE);
        Btn(220, y+10, 250, y+35, "+M", (i==sel)?BLUE:GRAY, WHITE);
        LCD_ShowString(265, y+15, meds[i].enabled?"[ON]":"[OFF]",
                       meds[i].enabled?GREEN:RED, bg, 12);
    }

    LCD_Fill(0, 210, 320, 240, LIGHTGRAY);
    LCD_ShowString(70, 220, "Tap top bar to EXIT", DARKBLUE, LIGHTGRAY, 12);
}

uint8_t GUI_CheckButton(uint16_t x, uint16_t y)
{
    Medicine *meds = PillBox_GetMedicines();
    switch(g_Page) {
    case PAGE_MAIN:
        if(y<=30 || (x>=90 && x<=230 && y>=195 && y<=230)) {
            g_Page=PAGE_MED_SETTING;
            return BTN_SET_MEDS;
        }
        break;
    case PAGE_ALARM_ALERT:
        if(x>=20 && x<=150 && y>=195 && y<=230) {
            g_Page=PAGE_MAIN;
            return BTN_SNOOZE;
        }
        if(x>=170 && x<=300 && y>=195 && y<=230) {
            g_Page=PAGE_MAIN;
            return BTN_CONFIRM_TAKEN;
        }
        break;
    case PAGE_MED_SETTING:
        if(y<=30 || y>=210) { g_Page=PAGE_MAIN; return BTN_BACK; }
        for(uint8_t i=0; i<MAX_MEDICINES; i++) {
            uint16_t iy = 35 + i*45;
            if(y>=iy && y<=iy+43) {
                if(x>=265) {
                    meds[i].enabled = !meds[i].enabled;
                    PillBox_SetMedicine(i, meds[i].name, meds[i].alarmHour, meds[i].alarmMinute, meds[i].enabled);
                    return BTN_TOGGLE_ENABLE;
                }
                if(x>=180 && x<=210) {
                    meds[i].alarmHour = (meds[i].alarmHour+1)%24;
                    PillBox_SetMedicine(i, meds[i].name, meds[i].alarmHour, meds[i].alarmMinute, meds[i].enabled);
                    return BTN_HOUR_UP;
                }
                if(x>=220 && x<=250) {
                    meds[i].alarmMinute = (meds[i].alarmMinute+5)%60;
                    PillBox_SetMedicine(i, meds[i].name, meds[i].alarmHour, meds[i].alarmMinute, meds[i].enabled);
                    return BTN_MIN_UP;
                }
                return BTN_SELECT_NEXT;
            }
        }
        break;
    }
    return BTN_NONE;
}

void GUI_Init(void){ g_Page = PAGE_MAIN; }
