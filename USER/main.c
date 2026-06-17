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
#include "bsp_wifi.h"
#include "bsp_sntp.h"
#include "bsp_extfont.h"
#include "pillbox.h"
#include "gui.h"
#include <stdio.h>
#include <string.h>

static RTC_Time  g_CurTime;
static uint8_t   g_PageDirty = 1;
static char      g_WiFiDebugText[32] = "WiFi:IDLE";

#define FORCE_START_TIME_ON_BOOT  0

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

static void Action_ConfirmTaken(void)
{
    if(PillBox_GetState() == STATE_ALARM) {
        PillBox_ConfirmTaken();
        g_PageDirty = 1;
    }
}

static void Action_Snooze(void)
{
    if(PillBox_GetState() == STATE_ALARM) {
        PillBox_Snooze();
        g_PageDirty = 1;
    }
}

static void Action_ReturnHome(void)
{
    GUI_ReturnHome();
    g_PageDirty = 1;
}

static void ProcessTouch(void)
{
    static uint32_t last_touch_tick = 0;
    TouchState t;
    uint8_t btn = BTN_NONE;

    t = Touch_Read(); // ????????????????
    if(t.pressed) {
        btn = GUI_CheckButton(t.x, t.y);
    }

    if(t.pressed) {
        if(btn != BTN_NONE && (g_SysTickCount - last_touch_tick) > 250) {
            if(btn == BTN_CONFIRM_TAKEN) {
                Action_ConfirmTaken();
            } else if(btn == BTN_SNOOZE) {
                Action_Snooze();
            } else if(btn == BTN_BACK) {
                Action_ReturnHome();
            }
            last_touch_tick = g_SysTickCount;
        }
    }
}

static void ProcessKey(void)
{
    uint8_t k = Key_Scan();
    if(k == KEY1_DOWN) {
        Action_ConfirmTaken();
    }
    if(k == KEY2_DOWN) {
        Action_Snooze();
    }
}

static void ProcessInfrared(void)
{
    static uint8_t  dynamic_baseline = 0;
    static uint8_t  ir_debounce_count = 0;
    static uint32_t last_check_tick = 0;
    uint8_t state = PillBox_GetState();
    uint8_t current_mask = IR_ReadMask();

    if(state != STATE_ALARM) {
        dynamic_baseline = current_mask;
        ir_debounce_count = 0;
    } else {
        if((g_SysTickCount - last_check_tick) >= 20) {
            last_check_tick = g_SysTickCount;
            if(current_mask != dynamic_baseline) {
                ir_debounce_count++;
                if(ir_debounce_count >= 3) {
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

static void RenderWiFiDebug(void)
{
    GUI_SetWiFiOnline(strstr(g_WiFiDebugText, "OK") != 0 ||
                      strstr(g_WiFiDebugText, "TCP") != 0);
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
    RenderWiFiDebug();
    /* Enable this while debugging IR input. */
    /* RenderIRDebug(); */
    g_PageDirty = 0;
}

static WiFi_Status WiFi_SendPillBoxStatus(const char *event)
{
    char msg[256];
    uint8_t idx;
    uint8_t state;
    uint8_t taken;
    uint8_t total;
    Medicine *meds;

    RTC_GetTime(&g_CurTime);
    state = PillBox_GetState();
    idx = PillBox_GetTriggeredIndex();
    taken = PillBox_GetTakenCount();
    total = PillBox_GetTotalEnabled();
    meds = PillBox_GetMedicines();

    if(idx < MAX_MEDICINES) {
        snprintf(msg, sizeof(msg),
                 "SmartPillBox,%s,%04d-%02d-%02d %02d:%02d:%02d,state=%d,taken=%d/%d,med=%s\r\n",
                 event,
                 g_CurTime.year, g_CurTime.month, g_CurTime.day,
                 g_CurTime.hour, g_CurTime.minute, g_CurTime.second,
                 state, taken, total, meds[idx].name);
    } else {
        snprintf(msg, sizeof(msg),
                 "SmartPillBox,%s,%04d-%02d-%02d %02d:%02d:%02d,state=%d,taken=%d/%d\r\n",
                 event,
                 g_CurTime.year, g_CurTime.month, g_CurTime.day,
                 g_CurTime.hour, g_CurTime.minute, g_CurTime.second,
                 state, taken, total);
    }

    return WiFi_SendString(msg, 3000);
}

static void WiFi_ShowRxPreview(const char *payload)
{
    char preview[12];
    uint8_t i = 0;

    while(payload[i] != '\0' && i < 7u) {
        if(payload[i] == '\r' || payload[i] == '\n') {
            preview[i] = '_';
        } else {
            preview[i] = payload[i];
        }
        i++;
    }
    preview[i] = '\0';
    sprintf(g_WiFiDebugText, "WiFi:RX %s", preview);
}

static uint8_t Json_GetInt(const char *json, const char *key, int *value)
{
    char pattern[20];
    const char *p;

    sprintf(pattern, "\"%s\"", key);
    p = strstr(json, pattern);
    if(p == 0) {
        return 0;
    }

    p = strchr(p, ':');
    if(p == 0) {
        return 0;
    }

    p++;
    while(*p == ' ' || *p == '\t') {
        p++;
    }

    return (sscanf(p, "%d", value) == 1) ? 1 : 0;
}

static uint8_t Json_HexVal(char c)
{
    if(c >= '0' && c <= '9')return (uint8_t)(c - '0');
    if(c >= 'a' && c <= 'f')return (uint8_t)(c - 'a' + 10);
    if(c >= 'A' && c <= 'F')return (uint8_t)(c - 'A' + 10);
    return 0xFFu;
}

static uint8_t Json_AppendUtf8(uint16_t code, char *out, uint8_t *idx, uint8_t maxLen)
{
    if(code < 0x80u) {
        if(*idx + 1u >= maxLen)return 0;
        out[(*idx)++] = (char)code;
    } else if(code < 0x800u) {
        if(*idx + 2u >= maxLen)return 0;
        out[(*idx)++] = (char)(0xC0u | (code >> 6));
        out[(*idx)++] = (char)(0x80u | (code & 0x3Fu));
    } else {
        if(*idx + 3u >= maxLen)return 0;
        out[(*idx)++] = (char)(0xE0u | (code >> 12));
        out[(*idx)++] = (char)(0x80u | ((code >> 6) & 0x3Fu));
        out[(*idx)++] = (char)(0x80u | (code & 0x3Fu));
    }
    return 1;
}

static uint8_t Json_GetString(const char *json, const char *key, char *out, uint8_t maxLen)
{
    char pattern[20];
    const char *p;
    uint8_t i = 0;

    if(out == 0 || maxLen == 0) {
        return 0;
    }

    sprintf(pattern, "\"%s\"", key);
    p = strstr(json, pattern);
    if(p == 0) {
        return 0;
    }

    p = strchr(p, ':');
    if(p == 0) {
        return 0;
    }

    p++;
    while(*p == ' ' || *p == '\t') {
        p++;
    }

    if(*p != '\"') {
        return 0;
    }

    p++;
    while(*p != '\0' && *p != '\"' && i < (uint8_t)(maxLen - 1u)) {
        if(*p == '\\') {
            p++;
            if(*p == 'u') {
                uint8_t h0 = Json_HexVal(p[1]);
                uint8_t h1 = Json_HexVal(p[2]);
                uint8_t h2 = Json_HexVal(p[3]);
                uint8_t h3 = Json_HexVal(p[4]);
                if(h0 == 0xFFu || h1 == 0xFFu || h2 == 0xFFu || h3 == 0xFFu) {
                    break;
                }
                if(!Json_AppendUtf8((uint16_t)((h0 << 12) | (h1 << 8) | (h2 << 4) | h3),
                                    out, &i, maxLen)) {
                    break;
                }
                p += 5;
                continue;
            }
            if(*p == 'n') {
                out[i++] = ' ';
                p++;
                continue;
            }
            if(*p == 'r' || *p == 't') {
                out[i++] = ' ';
                p++;
                continue;
            }
            if(*p == '\"' || *p == '\\' || *p == '/') {
                out[i++] = *p++;
                continue;
            }
            out[i++] = '\\';
            continue;
        }
        out[i++] = *p++;
    }
    out[i] = '\0';

    return (i > 0) ? 1 : 0;
}

static uint8_t Json_GetHexBytes(const char *json, const char *key, uint8_t *out, uint8_t maxLen)
{
    char hex[MED_GBK_NAME_LEN * 2u + 1u];
    uint8_t i;
    uint8_t len;
    if(out == 0 || maxLen == 0u)return 0;
    if(!Json_GetString(json, key, hex, sizeof(hex)))return 0;
    len = (uint8_t)strlen(hex);
    if((len & 1u) != 0u)return 0;
    if((uint8_t)(len / 2u) > maxLen)return 0;
    for(i = 0; i < len; i += 2u) {
        uint8_t h = Json_HexVal(hex[i]);
        uint8_t l = Json_HexVal(hex[i + 1u]);
        if(h == 0xFFu || l == 0xFFu)return 0;
        out[i / 2u] = (uint8_t)((h << 4) | l);
    }
    return (uint8_t)(len / 2u);
}

static uint8_t ProcessServerJsonCommand(char *json, uint8_t wifi_connected)
{
    char cmd[12];
    char name[MED_NAME_LEN];
    char dose[MED_DOSE_LEN];
    uint8_t nameGbk[MED_GBK_NAME_LEN];
    uint8_t nameGbkLen;
    char ack[80];
    Medicine *meds;
    int idx;
    int hour;
    int minute;

    if(!Json_GetString(json, "cmd", cmd, sizeof(cmd)))return 0;

    if(strcmp(cmd, "add") == 0 || strcmp(cmd, "set") == 0) {
        if(!Json_GetInt(json, "idx", &idx) ||
           !Json_GetInt(json, "hour", &hour) ||
           !Json_GetInt(json, "min", &minute)) {
            sprintf(g_WiFiDebugText, "WiFi:CMD MISS");
            g_PageDirty = 1;
            return 1;
        }

        if(idx < 0 || idx >= MAX_MEDICINES ||
           hour < 0 || hour > 23 ||
           minute < 0 || minute > 59) {
            sprintf(g_WiFiDebugText, "WiFi:CMD RANGE");
            g_PageDirty = 1;
            return 1;
        }

        meds = PillBox_GetMedicines();
        strncpy(name, meds[idx].name, MED_NAME_LEN - 1u);
        name[MED_NAME_LEN - 1u] = '\0';
        Json_GetString(json, "name", name, sizeof(name));
        strncpy(dose, meds[idx].dose, MED_DOSE_LEN - 1u);
        dose[MED_DOSE_LEN - 1u] = '\0';
        Json_GetString(json, "dose", dose, sizeof(dose));

        PillBox_SetMedicine((uint8_t)idx, name, (uint8_t)hour, (uint8_t)minute, 1);
        strncpy(meds[idx].dose, dose, MED_DOSE_LEN - 1u);
        meds[idx].dose[MED_DOSE_LEN - 1u] = '\0';
        nameGbkLen = Json_GetHexBytes(json, "name_gbk", nameGbk, sizeof(nameGbk));
        if(nameGbkLen > 0u) {
            PillBox_SetMedicineGbkName((uint8_t)idx, nameGbk, nameGbkLen);
        }
        sprintf(g_WiFiDebugText, "WiFi:CMD ADD%d", idx);
        g_PageDirty = 1;

        if(wifi_connected) {
            sprintf(ack, "SmartPillBox,ACK,add,idx=%d,hour=%02d,min=%02d\r\n",
                    idx, hour, minute);
            WiFi_SendString(ack, 1000);
        }
        return 1;
    }

    if(strcmp(cmd, "del") == 0) {
        if(Json_GetInt(json, "idx", &idx) &&
           idx >= 0 && idx < MAX_MEDICINES) {
            meds = PillBox_GetMedicines();
            PillBox_SetMedicine((uint8_t)idx, meds[idx].name,
                                meds[idx].alarmHour, meds[idx].alarmMinute, 0);
            sprintf(g_WiFiDebugText, "WiFi:CMD DEL%d", idx);
            g_PageDirty = 1;
            return 1;
        }
    }

    sprintf(g_WiFiDebugText, "WiFi:CMD UNK");
    g_PageDirty = 1;
    return 1;
}

static uint8_t ProcessServerCommand(uint8_t wifi_connected)
{
    char payload[160];
    char oneJson[128];
    char *p;
    char *start;
    char *end;
    uint8_t handled = 0;
    uint16_t len;

    if(!WiFi_ReadTcpPayload(payload, sizeof(payload))) {
        return 0;
    }

    WiFi_ShowRxPreview(payload);
    g_PageDirty = 1;

    p = payload;
    while((start = strchr(p, '{')) != 0) {
        end = strchr(start, '}');
        if(end == 0) {
            break;
        }

        len = (uint16_t)(end - start + 1);
        if(len >= sizeof(oneJson)) {
            len = sizeof(oneJson) - 1;
        }
        memcpy(oneJson, start, len);
        oneJson[len] = '\0';

        if(ProcessServerJsonCommand(oneJson, wifi_connected)) {
            handled = 1;
        }

        p = end + 1;
    }

    if(!handled) {
        sprintf(g_WiFiDebugText, "WiFi:RX NO CMD");
        g_PageDirty = 1;
        return 1;
    }

    return 1;
}

static uint8_t WiFi_ShouldPauseForDose(void)
{
    RTC_Time now;
    Medicine *meds;
    uint16_t nowMin;
    uint16_t alarmMin;
    uint16_t diff;
    uint8_t i;

    if(PillBox_GetState() != STATE_NORMAL) {
        return (PillBox_GetState() == STATE_ALARM) ? 1u : 0u;
    }

    RTC_GetTime(&now);
    nowMin = (uint16_t)now.hour * 60u + now.minute;
    meds = PillBox_GetMedicines();

    for(i = 0; i < MAX_MEDICINES; i++) {
        if(!meds[i].enabled || meds[i].taken) {
            continue;
        }

        alarmMin = (uint16_t)meds[i].alarmHour * 60u + meds[i].alarmMinute;
        if(alarmMin >= nowMin) {
            diff = alarmMin - nowMin;
        } else {
            diff = (uint16_t)(1440u - nowMin + alarmMin);
        }

        if(diff <= 1u) {
            return 1;
        }
    }

    return 0;
}

static uint8_t WiFi_TryConnectAndReport(void)
{
    uint8_t i;
    uint8_t j;
    RTC_Time syncedTime;
    uint32_t baudList[2] = {WIFI_DEFAULT_BAUDRATE, WIFI_ALT_BAUDRATE};
    const char *serverIpList[2] = {WIFI_SERVER_IP, WIFI_SERVER_IP_ALT1};

    for(i = 0; i < 2; i++) {
        WiFi_SetBaudrate(baudList[i]);
        sprintf(g_WiFiDebugText, "WiFi:TRY %lu", baudList[i]);
        g_PageDirty = 1;
        RenderUI();

        if(WiFi_ConnectAPDefault() != WIFI_OK) {
            sprintf(g_WiFiDebugText, "WiFi:FAIL %s", WiFi_GetLastStepString());
            continue;
        }

        sprintf(g_WiFiDebugText, "WiFi:NTP");
        g_PageDirty = 1;
        RenderUI();

        if(SNTP_SyncRtc(&syncedTime) == WIFI_OK) {
            sprintf(g_WiFiDebugText, "NTP:%02d:%02d:%02d",
                    syncedTime.hour, syncedTime.minute, syncedTime.second);
            g_PageDirty = 1;
            RenderUI();
        } else {
            sprintf(g_WiFiDebugText, "NTP:FAIL");
            g_PageDirty = 1;
            RenderUI();
        }

        for(j = 0; j < 2u; j++) {
            sprintf(g_WiFiDebugText, "TCP:%u %lu", (unsigned)(j + 1u), baudList[i]);
            g_PageDirty = 1;
            RenderUI();

            if(WiFi_TCPConnect(serverIpList[j], WIFI_SERVER_PORT, 3000) != WIFI_OK) {
                WiFi_Close(500);
                continue;
            }

            if(WiFi_SendPillBoxStatus("BOOT") == WIFI_OK) {
                sprintf(g_WiFiDebugText, "WiFi:OK %u", (unsigned)(j + 1u));
                return 1;
            }

            WiFi_Close(500);
        }

        sprintf(g_WiFiDebugText, "WiFi:FAIL TCP");
    }

    return 0;
}

int main(void)
{
    uint8_t last_min = 255;
    uint8_t last_ir_mask = 0xFF;
    uint8_t last_state = 255;
    uint8_t last_wifi_report_min = 255;
    uint8_t wifi_connected = 0;
    uint8_t wifi_status_pending = 0;
    uint8_t wifi_state_pending = 0;
    uint8_t wifi_missed_pending = 0;
    uint8_t ir_mask;
    uint32_t next_wifi_retry_tick = 1500;
    uint32_t next_wifi_heartbeat_tick = 0;

    SystemClock_Config();
    SysTick_Init();

    LCD_Init();
    Touch_Init();
    RTC_Init();
    LED_Init();
    BEEP_Init();
    Key_Init();
    IR_Init();
    ExtFont_Init();
    WiFi_Init(WIFI_DEFAULT_BAUDRATE);

    GUI_Init();
    PillBox_Init();

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
    PWR_BackupAccessCmd(ENABLE);

    if(FORCE_START_TIME_ON_BOOT || BKP_ReadBackupRegister(BKP_DR1) != 0x5050) {
        g_CurTime.year = 2026;
        g_CurTime.month = 6;
        g_CurTime.day = 6;
        g_CurTime.hour = 7;
        g_CurTime.minute = 28;
        g_CurTime.second = 55;
        RTC_SetTime(&g_CurTime);
        BKP_WriteBackupRegister(BKP_DR1, 0x5050);
    }

    g_PageDirty = 1;
    RenderUI();

    while(1) {
        RTC_GetTime(&g_CurTime);

        if(g_CurTime.minute != last_min) {
            last_min = g_CurTime.minute;
            g_PageDirty = 1;
        }

        if(g_CurTime.minute != last_wifi_report_min) {
            last_wifi_report_min = g_CurTime.minute;
            wifi_status_pending = 1;
        }

        PillBox_Process();
        ProcessTouch();
        ProcessKey();
        ProcessInfrared();

        ir_mask = IR_ReadMask();
        if(ir_mask != last_ir_mask) {
            last_ir_mask = ir_mask;
            g_PageDirty = 1;
        }

        if(PillBox_GetState() != last_state) {
            last_state = PillBox_GetState();
            g_PageDirty = 1;
            if(last_state == STATE_MISSED) {
                wifi_missed_pending = 1;
                wifi_state_pending = 0;
            } else {
                wifi_state_pending = 1;
            }
        }

        if(g_PageDirty) {
            RenderUI();
        }

        if(wifi_connected) {
            if(ProcessServerCommand(wifi_connected)) {
                RenderUI();
            }
        }

        if(wifi_connected && g_SysTickCount >= next_wifi_heartbeat_tick) {
            wifi_status_pending = 1;
            next_wifi_heartbeat_tick = g_SysTickCount + 10000u;
        }

        if(wifi_connected && (wifi_missed_pending || !WiFi_ShouldPauseForDose())) {
            if(wifi_missed_pending) {
                if(WiFi_SendPillBoxStatus("MISSED") == WIFI_OK) {
                    wifi_missed_pending = 0;
                    sprintf(g_WiFiDebugText, "WiFi:OK MISSED");
                } else {
                    wifi_connected = 0;
                    sprintf(g_WiFiDebugText, "WiFi:FAIL MISS");
                    next_wifi_retry_tick = g_SysTickCount + 5000u;
                }
            } else if(wifi_state_pending) {
                if(WiFi_SendPillBoxStatus("STATE") == WIFI_OK) {
                    wifi_state_pending = 0;
                    sprintf(g_WiFiDebugText, "WiFi:OK STATE");
                } else {
                    wifi_connected = 0;
                    sprintf(g_WiFiDebugText, "WiFi:FAIL STATE");
                    next_wifi_retry_tick = g_SysTickCount + 5000u;
                }
            } else if(wifi_status_pending) {
                if(WiFi_SendPillBoxStatus("STATUS") == WIFI_OK) {
                    wifi_status_pending = 0;
                    next_wifi_heartbeat_tick = g_SysTickCount + 10000u;
                    sprintf(g_WiFiDebugText, "WiFi:OK STATUS");
                } else {
                    wifi_connected = 0;
                    sprintf(g_WiFiDebugText, "WiFi:FAIL STATUS");
                    next_wifi_retry_tick = g_SysTickCount + 5000u;
                }
            }
        }

        if(!wifi_connected &&
           g_SysTickCount >= next_wifi_retry_tick &&
           !WiFi_ShouldPauseForDose()) {
            wifi_connected = WiFi_TryConnectAndReport();
            if(wifi_connected) {
                next_wifi_heartbeat_tick = g_SysTickCount + 10000u;
            }
            next_wifi_retry_tick = g_SysTickCount + (wifi_connected ? 60000u : 30000u);
            g_PageDirty = 1;
        }
    }
}
