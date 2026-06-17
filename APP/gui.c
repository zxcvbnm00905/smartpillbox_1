/**
 * Smart Pill Box GUI - dynamic UI matched to target visual style
 */
#include "gui.h"
#include "bsp_lcd.h"
#include "bsp_cfont.h"
#include "bsp_extfont.h"
#include <stdio.h>
#include <string.h>

#define UI_BG             0xF7DF
#define UI_BAR            0x2C79
#define UI_PRIMARY        0x2D9F
#define UI_TEXT           0x18A8
#define UI_MUTED          0x6B4D
#define UI_BORDER         0xE73C
#define UI_BUTTON_BORDER  0xBDF7
#define UI_SOFT_BLUE      0xEF7F
#define UI_TRACK          0xE71C
#define UI_GREEN          0x05D0
#define UI_GREEN_BG       0xEFBD
#define UI_ORANGE         0xEA60
#define UI_ORANGE_BG      0xFDDD

#define UI_TXT_DAY "\xE6\x97\xA5"
#define UI_TXT_ONE "\xE4\xB8\x80"
#define UI_TXT_TWO "\xE4\xBA\x8C"
#define UI_TXT_THREE "\xE4\xB8\x89"
#define UI_TXT_FOUR "\xE5\x9B\x9B"
#define UI_TXT_FIVE "\xE4\xBA\x94"
#define UI_TXT_SIX "\xE5\x85\xAD"
#define UI_TXT_MONTH "\xE6\x9C\x88"
#define UI_TXT_WEEK "\xE5\x91\xA8"
#define UI_TXT_WIFI_OK "\x57\x69\x46\x69\x20\x4F\x4B"
#define UI_TXT_WIFI_WAIT "\x57\x69\x46\x69\x20\x2E\x2E\x2E"
#define UI_TXT_NEXT_DOSE "\xE4\xB8\x8B\xE6\xAC\xA1\xE6\x9C\x8D\xE8\x8D\xAF"
#define UI_TXT_GENERIC_MED "\xE8\x8D\xAF\xE5\x93\x81"
#define UI_TXT_DOSE "\xE5\x89\x82\xE9\x87\x8F\x3A"
#define UI_TXT_REMAIN "\xE8\xBF\x98\xE6\x9C\x89"
#define UI_TXT_STATUS "\xE7\x8A\xB6\xE6\x80\x81"
#define UI_TXT_TAKEN_DONE "\xE5\xB7\xB2\xE6\x9C\x8D\xE8\x8D\xAF"
#define UI_TXT_RECORDED "\xE5\xB7\xB2\xE8\xAE\xB0\xE5\xBD\x95\xE6\x9C\xAC\xE6\xAC\xA1\xE6\x9C\x8D\xE8\x8D\xAF"
#define UI_TXT_MISSED "\xE6\x9C\xAA\xE6\x9C\x8D\xE8\x8D\xAF"
#define UI_TXT_MISSED_RECORDED "\xE5\xB7\xB2\xE8\xAE\xB0\xE5\xBD\x95\xE6\x9C\xAC\xE6\xAC\xA1\xE6\x9C\x8D\xE8\x8D\xAF"
#define UI_TXT_LATER "\xE7\xA8\x8D\xE5\x90\x8E"
#define UI_TXT_AFTER_REMIND "\xE5\xB0\x86\xE5\x9C\xA8"
#define UI_TXT_AFTER_REMIND_END "\xE5\x88\x86\xE9\x92\x9F\xE5\x90\x8E\xE6\x8F\x90\xE9\x86\x92"
#define UI_TXT_HOURS "\xE5\xB0\x8F\xE6\x97\xB6"
#define UI_TXT_MINUTES "\xE5\x88\x86\xE9\x92\x9F"
#define UI_TXT_SMART_BOX "\xE6\x99\xBA\xE8\x83\xBD\xE8\x8D\xAF\xE7\x9B\x92"
#define UI_TXT_TODAY_PROGRESS "\xE4\xBB\x8A\xE6\x97\xA5\xE8\xBF\x9B\xE5\xBA\xA6"
#define UI_TXT_CONFIRM "\xE7\xA1\xAE\xE8\xAE\xA4\xE6\x9C\x8D\xE8\x8D\xAF"
#define UI_TXT_SNOOZE "\xE7\xA8\x8D\xE5\x90\x8E\xE6\x9C\x8D\xE8\x8D\xAF"
#define UI_TXT_TAKE_NOW "\xE7\xAB\x8B\xE5\x8D\xB3\xE6\x9C\x8D\xE8\x8D\xAF"
#define UI_TXT_BACK_HOME "\xE8\xBF\x94\xE5\x9B\x9E\xE9\xA6\x96\xE9\xA1\xB5"

typedef enum {
    UI_STATE_NORMAL = 0,
    UI_STATE_LATER,
    UI_STATE_CONFIRMED,
    UI_STATE_MISSED
} UI_State;

static GUI_Page g_Page = PAGE_MAIN;
static UI_State g_LastState = (UI_State)0xFFu;
static uint8_t g_NeedFullDraw = 1u;
static uint8_t g_WifiOnline = 0u;
static uint8_t g_ForceHome = 0u;
static const char* wk[7] = {
    UI_TXT_DAY, UI_TXT_ONE, UI_TXT_TWO, UI_TXT_THREE,
    UI_TXT_FOUR, UI_TXT_FIVE, UI_TXT_SIX
};

GUI_Page GUI_GetPage(void)
{
    return g_Page;
}

void GUI_SetWiFiOnline(uint8_t online)
{
    if(g_WifiOnline != online) {
        g_WifiOnline = online;
        g_NeedFullDraw = 1u;
    }
}

void GUI_ReturnHome(void)
{
    g_Page = PAGE_MAIN;
    g_ForceHome = 1u;
    g_NeedFullDraw = 1u;
    g_LastState = (UI_State)0xFFu;
}

static void FillRoundRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                          uint16_t r, uint16_t color)
{
    LCD_Fill((uint16_t)(x1 + r), y1, (uint16_t)(x2 - r), y2, color);
    LCD_Fill(x1, (uint16_t)(y1 + r), x2, (uint16_t)(y2 - r), color);
    LCD_FillCircle((uint16_t)(x1 + r), (uint16_t)(y1 + r), r, color);
    LCD_FillCircle((uint16_t)(x2 - r), (uint16_t)(y1 + r), r, color);
    LCD_FillCircle((uint16_t)(x1 + r), (uint16_t)(y2 - r), r, color);
    LCD_FillCircle((uint16_t)(x2 - r), (uint16_t)(y2 - r), r, color);
}

static void DrawRoundRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                          uint16_t r, uint16_t color)
{
    LCD_DrawLine((uint16_t)(x1 + r), y1, (uint16_t)(x2 - r), y1, color);
    LCD_DrawLine((uint16_t)(x1 + r), y2, (uint16_t)(x2 - r), y2, color);
    LCD_DrawLine(x1, (uint16_t)(y1 + r), x1, (uint16_t)(y2 - r), color);
    LCD_DrawLine(x2, (uint16_t)(y1 + r), x2, (uint16_t)(y2 - r), color);
}

static void Button(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
                   const char *text, uint16_t bg, uint16_t fg, uint16_t border)
{
    FillRoundRect(x1, y1, x2, y2, 11, bg);
    if(bg != WHITE) {
        DrawRoundRect(x1, y1, x2, y2, 11, border);
    }
    CFont_ShowTextCenter(x1, (uint16_t)(y1 + 10u), x2, text, fg, bg, 16);
}

static void DrawRemainText(uint16_t x, uint16_t y, int16_t minutes,
                           uint16_t fg, uint16_t bg)
{
    char h[8];
    char m[8];
    uint16_t pos = x;
    int16_t hour;
    int16_t min;

    if(minutes < 0)minutes = 0;
    hour = (int16_t)(minutes / 60);
    min = (int16_t)(minutes % 60);

    sprintf(h, "%d", hour);
    sprintf(m, "%d", min);

    CFont_ShowText(pos, y, h, fg, bg, 16);
    pos = (uint16_t)(pos + CFont_TextWidth(h, 16));
    CFont_ShowText(pos, y, UI_TXT_HOURS, fg, bg, 16);
    pos = (uint16_t)(pos + CFont_TextWidth(UI_TXT_HOURS, 16));
    CFont_ShowText(pos, y, m, fg, bg, 16);
    pos = (uint16_t)(pos + CFont_TextWidth(m, 16));
    CFont_ShowText(pos, y, UI_TXT_MINUTES, fg, bg, 16);
}

static void DrawPillIcon(void)
{
    FillRoundRect(34, 65, 94, 125, 30, UI_SOFT_BLUE);
    FillRoundRect(47, 81, 81, 113, 6, WHITE);
    DrawRoundRect(47, 81, 81, 113, 6, UI_PRIMARY);
    LCD_DrawLine(64, 82, 64, 113, UI_PRIMARY);
    LCD_DrawLine(48, 97, 81, 97, UI_PRIMARY);
    LCD_FillCircle(84, 76, 9, 0x2EBA);
    LCD_FillCircle(84, 76, 6, WHITE);
    CFont_ShowTextCenter(34, 128, 94, UI_TXT_SMART_BOX, UI_MUTED, WHITE, 16);
}

static void DrawMedicineName(uint16_t x, uint16_t y, Medicine *med)
{
    if(med == 0)return;
    if(CFont_CanShowText(med->name)) {
        CFont_ShowText(x, y, med->name, UI_TEXT, WHITE, 24);
        return;
    }
    if(med->nameGbkLen > 0u && ExtFont_ShowGbkText16(x, y, med->nameGbk,
                                                     med->nameGbkLen,
                                                     UI_TEXT, WHITE)) {
        return;
    }
    CFont_ShowText(x, y, UI_TXT_GENERIC_MED, UI_TEXT, WHITE, 24);
}

static void DrawHeader(RTC_Time *t)
{
    char b[16];

    FillRoundRect(9, 10, 311, 40, 9, UI_BAR);
    sprintf(b, "%02d", t->month);
    CFont_ShowText(20, 18, b, WHITE, UI_BAR, 16);
    CFont_ShowText(32, 18, UI_TXT_MONTH, WHITE, UI_BAR, 16);
    sprintf(b, "%02d", t->day);
    CFont_ShowText(48, 18, b, WHITE, UI_BAR, 16);
    CFont_ShowText(60, 18, UI_TXT_DAY, WHITE, UI_BAR, 16);
    CFont_ShowText(82, 18, UI_TXT_WEEK, WHITE, UI_BAR, 16);
    CFont_ShowText(98, 18, wk[t->week], WHITE, UI_BAR, 16);

    CFont_ShowText(218, 18, g_WifiOnline ? UI_TXT_WIFI_OK : UI_TXT_WIFI_WAIT,
                   0xBDF7, UI_BAR, 16);
    sprintf(b, "%02d:%02d", t->hour, t->minute);
    CFont_ShowText(276, 18, b, WHITE, UI_BAR, 16);
}

static Medicine* FindNextMedicine(RTC_Time *t, int16_t *remainMin)
{
    Medicine *m = PillBox_GetMedicines();
    Medicine *nx = 0;
    int16_t nowMin = (int16_t)t->hour * 60 + t->minute;
    int16_t best = 32767;
    uint8_t i;

    for(i = 0; i < MAX_MEDICINES; i++) {
        int16_t medMin;
        int16_t diff;
        if(!m[i].enabled)continue;
        medMin = (int16_t)m[i].alarmHour * 60 + m[i].alarmMinute;
        diff = medMin - nowMin;
        if(m[i].taken) {
            diff = (int16_t)(diff + 1440);
        } else if(diff < 0) {
            diff = (int16_t)(diff + 1440);
        }
        if(diff < best) {
            best = diff;
            nx = &m[i];
        }
    }

    if(remainMin != 0)*remainMin = best;
    return nx;
}

static UI_State GetState(void)
{
    uint8_t snoozeIdx;
    uint16_t snoozeRemain;
    uint8_t total;
    uint8_t taken;

    if(PillBox_GetState() == STATE_ALARM) {
        g_ForceHome = 0u;
        return UI_STATE_NORMAL;
    }

    if(g_ForceHome) {
        return UI_STATE_NORMAL;
    }

    if(PillBox_GetSnoozeRemaining(&snoozeIdx, &snoozeRemain)) {
        return UI_STATE_LATER;
    }

    if(PillBox_GetState() == STATE_MISSED) {
        return UI_STATE_MISSED;
    }

    total = PillBox_GetTotalEnabled();
    taken = PillBox_GetTakenCount();
    if(total > 0u && taken >= total) {
        return UI_STATE_CONFIRMED;
    }

    return UI_STATE_NORMAL;
}

static void DrawStaticLayout(RTC_Time *t, UI_State state)
{
    LCD_Clear(UI_BG);
    DrawHeader(t);
    FillRoundRect(13, 50, 307, 178, 15, WHITE);
    DrawRoundRect(13, 50, 307, 178, 15, UI_BORDER);
    DrawPillIcon();

    CFont_ShowText(108, 65, UI_TXT_NEXT_DOSE, UI_MUTED, WHITE, 16);
    CFont_ShowText(34, 150, UI_TXT_TODAY_PROGRESS, UI_MUTED, WHITE, 16);

    if(state == UI_STATE_NORMAL) {
        Button(22, 190, 147, 226, UI_TXT_CONFIRM, UI_PRIMARY, WHITE, UI_PRIMARY);
        Button(164, 190, 302, 226, UI_TXT_SNOOZE, WHITE, UI_TEXT, UI_BUTTON_BORDER);
    } else if(state == UI_STATE_LATER) {
        Button(22, 190, 147, 226, UI_TXT_TAKE_NOW, UI_ORANGE, WHITE, UI_ORANGE);
        Button(164, 190, 302, 226, UI_TXT_BACK_HOME, WHITE, UI_TEXT, UI_BUTTON_BORDER);
    } else {
        Button(22, 190, 147, 226, UI_TXT_CONFIRM,
               state == UI_STATE_MISSED ? UI_ORANGE : UI_GREEN,
               WHITE,
               state == UI_STATE_MISSED ? UI_ORANGE : UI_GREEN);
        Button(164, 190, 302, 226, UI_TXT_BACK_HOME, WHITE, UI_TEXT, UI_BUTTON_BORDER);
    }
}

static void DrawDynamicContent(RTC_Time *t, UI_State state)
{
    char b[32];
    Medicine *nx;
    int16_t remain = 0;
    uint8_t total;
    uint8_t taken;
    uint8_t snoozeIdx;
    uint16_t snoozeRemain;
    uint16_t color = UI_PRIMARY;
    uint16_t boxBg = 0xF79F;
    uint16_t progress;

    nx = FindNextMedicine(t, &remain);
    total = PillBox_GetTotalEnabled();
    taken = PillBox_GetTakenCount();

    LCD_Fill(96, 82, 212, 140, WHITE);
    if(nx != 0) {
        DrawMedicineName(108, 86, nx);
        CFont_ShowText(109, 120, UI_TXT_DOSE, UI_TEXT, WHITE, 16);
        CFont_ShowText(157, 120, nx->dose, UI_TEXT, WHITE, 16);
    }

    if(state == UI_STATE_LATER) {
        color = UI_ORANGE;
        boxBg = UI_ORANGE_BG;
    } else if(state == UI_STATE_CONFIRMED) {
        color = UI_GREEN;
        boxBg = UI_GREEN_BG;
    } else if(state == UI_STATE_MISSED) {
        color = UI_ORANGE;
        boxBg = UI_ORANGE_BG;
    }

    FillRoundRect(202, 69, 300, 117, 12, boxBg);
    DrawRoundRect(202, 69, 300, 117, 12,
                  state == UI_STATE_LATER || state == UI_STATE_MISSED ? 0xF5CC : (state == UI_STATE_CONFIRMED ? 0xA7B4 : 0xCF3F));

    if(state == UI_STATE_CONFIRMED) {
        CFont_ShowTextCenter(202, 77, 300, UI_TXT_STATUS, UI_TEXT, boxBg, 16);
        CFont_ShowTextCenter(202, 94, 300, UI_TXT_TAKEN_DONE, color, boxBg, 16);
        LCD_Fill(96, 139, 260, 157, WHITE);
        CFont_ShowText(96, 140, UI_TXT_RECORDED, color, WHITE, 16);
    } else if(state == UI_STATE_MISSED) {
        CFont_ShowTextCenter(202, 77, 300, UI_TXT_STATUS, UI_TEXT, boxBg, 16);
        CFont_ShowTextCenter(202, 94, 300, UI_TXT_MISSED, color, boxBg, 16);
        LCD_Fill(96, 139, 260, 157, WHITE);
        CFont_ShowText(96, 140, UI_TXT_MISSED_RECORDED, color, WHITE, 16);
    } else if(state == UI_STATE_LATER) {
        if(PillBox_GetSnoozeRemaining(&snoozeIdx, &snoozeRemain)) {
            remain = (int16_t)snoozeRemain;
        } else {
            remain = 10;
        }
        CFont_ShowTextCenter(202, 77, 300, UI_TXT_LATER, UI_TEXT, boxBg, 16);
        DrawRemainText(207, 94, remain, color, boxBg);
        LCD_Fill(96, 139, 260, 157, WHITE);
        CFont_ShowText(96, 140, UI_TXT_AFTER_REMIND, color, WHITE, 16);
        sprintf(b, "%d", remain);
        CFont_ShowText(144, 140, b, color, WHITE, 16);
        CFont_ShowText((uint16_t)(144u + CFont_TextWidth(b, 16)), 140,
                       UI_TXT_AFTER_REMIND_END, color, WHITE, 16);
    } else {
        CFont_ShowTextCenter(202, 77, 300, UI_TXT_REMAIN, UI_TEXT, boxBg, 16);
        DrawRemainText(207, 94, remain, color, boxBg);
    }

    sprintf(b, "%u / %u", taken, total);
    LCD_Fill(262, 153, 300, 168, WHITE);
    CFont_ShowText(270, 150, b, color, WHITE, 16);
    FillRoundRect(23, 171, 297, 176, 3, UI_TRACK);
    if(total > 0u) {
        progress = (uint16_t)((uint32_t)taken * 274u / total);
        if(progress < 28u && taken > 0u)progress = 28u;
        if(state == UI_STATE_NORMAL && progress < 28u)progress = 28u;
        FillRoundRect(23, 171, (uint16_t)(23u + progress), 176, 3, color);
    }
}

void GUI_ShowMainPage(RTC_Time *time)
{
    UI_State state = GetState();
    g_Page = PAGE_MAIN;

    if(g_NeedFullDraw || state != g_LastState) {
        DrawStaticLayout(time, state);
        g_LastState = state;
        g_NeedFullDraw = 0u;
    } else {
        DrawHeader(time);
    }

    DrawDynamicContent(time, state);
}

void GUI_ShowAlarmAlert(Medicine *med)
{
    RTC_Time now;
    (void)med;
    RTC_GetTime(&now);
    g_Page = PAGE_ALARM_ALERT;
    g_ForceHome = 0u;
    if(g_NeedFullDraw || g_LastState != UI_STATE_NORMAL) {
        DrawStaticLayout(&now, UI_STATE_NORMAL);
        g_LastState = UI_STATE_NORMAL;
        g_NeedFullDraw = 0u;
    }
    DrawDynamicContent(&now, UI_STATE_NORMAL);
}

void GUI_ShowMedSetting(Medicine *meds, uint8_t count, uint8_t selected)
{
    RTC_Time now;
    (void)meds;
    (void)count;
    (void)selected;
    RTC_GetTime(&now);
    g_Page = PAGE_MED_SETTING;
    g_NeedFullDraw = 1u;
    GUI_ShowMainPage(&now);
}

static uint8_t GUI_CheckButtonDirect(int16_t x, int16_t y)
{
    if(x < 0 || y < 0 || x >= 320 || y >= 240)return BTN_NONE;

    if(x >= 22 && x <= 147 && y >= 190 && y <= 226) {
        g_NeedFullDraw = 1u;
        return BTN_CONFIRM_TAKEN;
    }
    if(x >= 164 && x <= 302 && y >= 190 && y <= 226) {
        g_NeedFullDraw = 1u;
        return BTN_SNOOZE;
    }

    return BTN_NONE;
}

static uint8_t GUI_CheckAlarmTouch(uint16_t x, uint16_t y)
{
    /*
     * Alarm page raw touch calibration on the real board:
     * - Confirm button: measured around X=205..228, Y=164..165.
     * - The two bottom buttons are separated mainly by raw Y after the
     *   LCD/touch rotation, so do not use the generic transform here.
     */
    if(x >= 140u && x <= 300u && y >= 125u && y <= 225u) {
        g_NeedFullDraw = 1u;
        return BTN_CONFIRM_TAKEN;
    }

    if(x >= 140u && x <= 300u && y >= 35u && y < 125u) {
        g_NeedFullDraw = 1u;
        return BTN_SNOOZE;
    }

    return BTN_NONE;
}

static uint8_t GUI_CheckResultPageTouch(uint16_t x, uint16_t y)
{
    uint8_t btn;

    /*
     * Measured on the result page when pressing "return home":
     * around X=272, Y=078.
     * The right-side button maps mainly to the smaller raw-Y area on
     * this panel, so keep the raw-Y check wide enough for the snooze
     * result page too.
     */
    if(y <= 170u) {
        g_NeedFullDraw = 1u;
        return BTN_BACK;
    }

    btn = GUI_CheckButtonDirect((int16_t)x, (int16_t)y);
    if(btn == BTN_SNOOZE)return BTN_BACK;
    btn = GUI_CheckButtonDirect((int16_t)(319 - (int16_t)x), (int16_t)y);
    if(btn == BTN_SNOOZE)return BTN_BACK;
    btn = GUI_CheckButtonDirect((int16_t)x, (int16_t)(239 - (int16_t)y));
    if(btn == BTN_SNOOZE)return BTN_BACK;
    btn = GUI_CheckButtonDirect((int16_t)(319 - (int16_t)x), (int16_t)(239 - (int16_t)y));
    if(btn == BTN_SNOOZE)return BTN_BACK;
    btn = GUI_CheckButtonDirect((int16_t)y, (int16_t)x);
    if(btn == BTN_SNOOZE)return BTN_BACK;
    btn = GUI_CheckButtonDirect((int16_t)(319 - (int16_t)y), (int16_t)x);
    if(btn == BTN_SNOOZE)return BTN_BACK;
    btn = GUI_CheckButtonDirect((int16_t)y, (int16_t)(239 - (int16_t)x));
    if(btn == BTN_SNOOZE)return BTN_BACK;
    btn = GUI_CheckButtonDirect((int16_t)(319 - (int16_t)y), (int16_t)(239 - (int16_t)x));
    if(btn == BTN_SNOOZE)return BTN_BACK;

    return BTN_NONE;
}

uint8_t GUI_CheckButton(uint16_t x, uint16_t y)
{
    uint8_t btn;

    switch(g_Page) {
    case PAGE_ALARM_ALERT:
        btn = GUI_CheckAlarmTouch(x, y);
        if(btn != BTN_NONE)return btn;
        break;
    case PAGE_MAIN:
        if(GetState() != UI_STATE_NORMAL) {
            btn = GUI_CheckResultPageTouch(x, y);
            if(btn != BTN_NONE)return btn;
            return BTN_NONE;
        }
        btn = GUI_CheckButtonDirect((int16_t)x, (int16_t)y);
        if(btn != BTN_NONE)return btn;
        btn = GUI_CheckButtonDirect((int16_t)(319 - (int16_t)x), (int16_t)y);
        if(btn != BTN_NONE)return btn;
        btn = GUI_CheckButtonDirect((int16_t)x, (int16_t)(239 - (int16_t)y));
        if(btn != BTN_NONE)return btn;
        btn = GUI_CheckButtonDirect((int16_t)(319 - (int16_t)x), (int16_t)(239 - (int16_t)y));
        if(btn != BTN_NONE)return btn;
        btn = GUI_CheckButtonDirect((int16_t)y, (int16_t)x);
        if(btn != BTN_NONE)return btn;
        btn = GUI_CheckButtonDirect((int16_t)(319 - (int16_t)y), (int16_t)x);
        if(btn != BTN_NONE)return btn;
        btn = GUI_CheckButtonDirect((int16_t)y, (int16_t)(239 - (int16_t)x));
        if(btn != BTN_NONE)return btn;
        btn = GUI_CheckButtonDirect((int16_t)(319 - (int16_t)y), (int16_t)(239 - (int16_t)x));
        if(btn != BTN_NONE)return btn;
        break;
    case PAGE_MED_SETTING:
        g_Page = PAGE_MAIN;
        g_NeedFullDraw = 1u;
        return BTN_BACK;
    default:
        break;
    }

    return BTN_NONE;
}

void GUI_Init(void)
{
    g_Page = PAGE_MAIN;
    g_LastState = (UI_State)0xFFu;
    g_NeedFullDraw = 1u;
    g_WifiOnline = 0u;
}
