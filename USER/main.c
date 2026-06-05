/**
 * Smart Pill Box - Main
 * Wildfire Guide STM32F103VET6 + 3.2" ILI9341 TFT
 */
#include "stm32f10x.h"
#include "bsp_lcd.h"
/*#include "bsp_touch.h"
#include "bsp_rtc.h"*/
#include "bsp_led.h"
#include "bsp_beep.h"
#include "bsp_key.h"
#include "pillbox.h"
#include "gui.h"
#include <stdio.h>

static RTC_Time  g_CurTime;
static uint8_t   g_PageDirty = 1;

static void SystemClock_Config(void)
{
    RCC_DeInit();
    RCC_HSEConfig(RCC_HSE_ON);
    if(RCC_WaitForHSEStartUp() == SUCCESS) {
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
        FLASH_SetLatency(FLASH_Latency_2);
        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        RCC_PCLK2Config(RCC_HCLK_Div1);
        RCC_PCLK1Config(RCC_HCLK_Div2);
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
        RCC_PLLCmd(ENABLE);
        while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        while(RCC_GetSYSCLKSource() != 0x08);
    }
}

volatile uint32_t g_SysTickCount = 0;
static void SysTick_Init(void) { SysTick_Config(SystemCoreClock / 1000); }
void SysTick_Handler(void)     { g_SysTickCount++; }

static void Delay_ms(uint32_t ms)
{
    uint32_t end = g_SysTickCount + ms;
    while(g_SysTickCount < end);
}

static void ProcessTouch(void)
{
    static TouchState last = {0,0,0};
    TouchState t = Touch_Read();
    if(t.pressed && !last.pressed) {
        uint8_t btn = GUI_CheckButton(t.x, t.y);
        switch(btn) {
        case BTN_CONFIRM_TAKEN: PillBox_ConfirmTaken(); g_PageDirty = 1; break;
        case BTN_SNOOZE:        PillBox_Snooze();       g_PageDirty = 1; break;
        default: g_PageDirty = 1; break;
        }
    }
    last = t;
}

static void ProcessKey(void)
{
    uint8_t k = Key_Scan();
    if(k == KEY1_DOWN && PillBox_GetState() == STATE_ALARM)
        { PillBox_ConfirmTaken(); g_PageDirty = 1; }
    if(k == KEY2_DOWN && PillBox_GetState() == STATE_ALARM)
        { PillBox_Snooze(); g_PageDirty = 1; }
}

static void RenderUI(void)
{
    RTC_GetTime(&g_CurTime);
    if(PillBox_GetState() == STATE_ALARM) {
        Medicine *meds = PillBox_GetMedicines();
        for(uint8_t i = 0; i < MAX_MEDICINES; i++) {
            if(meds[i].enabled && !meds[i].taken &&
               meds[i].alarmHour == g_CurTime.hour &&
               meds[i].alarmMinute == g_CurTime.minute) {
                GUI_ShowAlarmAlert(&meds[i]); break;
            }
        }
    } else {
        GUI_ShowMainPage(&g_CurTime);
    }
    g_PageDirty = 0;
}

int main(void)
{
    SystemClock_Config();
    SysTick_Init();

    LCD_Init();
    Touch_Init();
    RTC_Init();
    LED_Init();
    BEEP_Init();
    Key_Init();

    GUI_Init();
    PillBox_Init();

    g_CurTime.year = 2024; g_CurTime.month = 6; g_CurTime.day = 1;
    g_CurTime.hour = 7; g_CurTime.minute = 25; g_CurTime.second = 0;
    RTC_SetTime(&g_CurTime);

    g_PageDirty = 1;

    while(1) {
        static uint32_t lastTick = 0;
        if(g_SysTickCount - lastTick >= 1000) {
            lastTick = g_SysTickCount;
            g_PageDirty = 1;
        }
        PillBox_Process();
        ProcessTouch();
        ProcessKey();
        if(g_PageDirty) RenderUI();
    }
}
