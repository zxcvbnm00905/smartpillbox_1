/**
 * Smart Pill Box - Main
 * Wildfire Guide STM32F103VET6 + 3.2" ILI9341 TFT
 */
#include "stm32f10x.h"
#include "bsp_lcd.h"
#include "bsp_touch.h"
#include "bsp_rtc.h"
#include "bsp_led.h"
#include "bsp_beep.h"
#include "bsp_key.h"
#include "bsp_ir.h"
#include "pillbox.h"
#include "gui.h"
#include <stdio.h>

static RTC_Time  g_CurTime;
static uint8_t   g_PageDirty = 1;
static uint32_t  g_AlarmStartTick = 0;
static uint8_t   g_LastAlarmGuardState = STATE_NORMAL;

#define FORCE_START_TIME_ON_BOOT  1

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


static void Alarm_UpdateGuard(void)
{
    uint8_t state = PillBox_GetState();
    if(state == STATE_ALARM && g_LastAlarmGuardState != STATE_ALARM) {
        g_AlarmStartTick = g_SysTickCount;
    }
    g_LastAlarmGuardState = state;
}

static uint8_t Alarm_CanDismiss(void)
{
    return (PillBox_GetState() == STATE_ALARM &&
            (g_SysTickCount - g_AlarmStartTick) >= 2000);
}
static void ProcessTouch(void)
{
    static TouchState last = {0,0,0};
    TouchState t = Touch_Read();
    if(t.pressed && !last.pressed) {
        uint8_t btn = GUI_CheckButton(t.x, t.y);
        switch(btn) {
        case BTN_CONFIRM_TAKEN: if(Alarm_CanDismiss()) PillBox_ConfirmTaken(); g_PageDirty = 1; break;
        case BTN_SNOOZE:        if(PillBox_GetState()==STATE_ALARM) PillBox_Snooze();       g_PageDirty = 1; break;
        default: g_PageDirty = 1; break;
        }
    }
    last = t;
}

static void ProcessKey(void)
{
    uint8_t k = Key_Scan();
    if(k == KEY1_DOWN && Alarm_CanDismiss())
        { PillBox_ConfirmTaken(); g_PageDirty = 1; }
    if(k == KEY2_DOWN && PillBox_GetState() == STATE_ALARM)
        { PillBox_Snooze(); g_PageDirty = 1; }
}


static void ProcessInfrared(void)
{
    uint8_t state = PillBox_GetState();
    uint8_t current_mask = IR_ReadMask();

    static uint8_t  dynamic_baseline = 0;   
    static uint8_t  ir_debounce_count = 0;  
    static uint32_t last_check_tick = 0;    

    if (state != STATE_ALARM) {
        dynamic_baseline = current_mask; 
        ir_debounce_count = 0;
    } 
		
    else {
        if ((g_SysTickCount - last_check_tick) >= 20) {
            last_check_tick = g_SysTickCount;

            if (current_mask != dynamic_baseline) {
                ir_debounce_count++;
  
                if (ir_debounce_count >= 3) {
                    PillBox_ConfirmTaken(); 
                    g_PageDirty = 1;       
                    ir_debounce_count = 0;
                }
            } else {
                ir_debounce_count = 0; 
            }
        }
    }
}

static void RenderIRDebug(void)
{
    char b[16];
    uint8_t mask = IR_ReadMask();

    sprintf(b, "IR:%d", (mask & 0x01) ? 1 : 0);
    LCD_Fill(246, 32, 319, 46, WHITE);
    LCD_ShowString(246, 32, b, MAGENTA, WHITE, 12);
}
static void RenderUI(void)
{
    RTC_GetTime(&g_CurTime);
    if(PillBox_GetState() == STATE_ALARM) {
        Medicine *meds = PillBox_GetMedicines();
        uint8_t idx = PillBox_GetTriggeredIndex();
        if(idx < MAX_MEDICINES)GUI_ShowAlarmAlert(&meds[idx]);
    } else if(GUI_GetPage() == PAGE_MED_SETTING) {
        GUI_ShowMedSetting(PillBox_GetMedicines(), MAX_MEDICINES, 0);
    } else {
        GUI_ShowMainPage(&g_CurTime);
    }
    //RenderIRDebug();
    g_PageDirty = 0;
}

int main(void)
{
    uint8_t last_min = 255;
    uint8_t last_ir_mask = 0xFF;
    uint8_t last_state = 255;
    uint8_t ir_mask;

    SystemClock_Config();
    SysTick_Init();

    LCD_Init();
    Touch_Init();
    RTC_Init();	
    LED_Init();
    BEEP_Init();
    Key_Init();
    IR_Init();

    GUI_Init();
    PillBox_Init();


    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    if (FORCE_START_TIME_ON_BOOT || BKP_ReadBackupRegister(BKP_DR1) != 0x5050) {

        g_CurTime.year = 2026; 
        g_CurTime.month = 6; 
        g_CurTime.day = 6;
        g_CurTime.hour = 07;  
        g_CurTime.minute = 29;
        g_CurTime.second = 50;
        RTC_SetTime(&g_CurTime);

        BKP_WriteBackupRegister(BKP_DR1, 0x5050);
    }

    g_PageDirty = 1;

    while(1) {
 
        RTC_GetTime(&g_CurTime); 

        if(g_CurTime.minute != last_min) {
            last_min = g_CurTime.minute;
            g_PageDirty = 1;
        }

        PillBox_Process();
        Alarm_UpdateGuard();
        ProcessTouch();
        ProcessKey();
        ProcessInfrared();

        ir_mask = IR_ReadMask();
        if(ir_mask != last_ir_mask) {
            last_ir_mask = ir_mask;
            g_PageDirty = 1;
        }

        if (PillBox_GetState() != last_state) {
            last_state = PillBox_GetState();
            g_PageDirty = 1;
        }

        if(g_PageDirty) {
            RenderUI();
        }
    }
	}
