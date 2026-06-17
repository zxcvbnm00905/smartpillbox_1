/**
 * @file    bsp_sntp.c
 * @brief   SNTP network time sync for SmartPillBox
 */
#include "bsp_sntp.h"
#include <string.h>

#define SNTP_PACKET_SIZE          48u
#define SNTP_UNIX_OFFSET          2208988800ul
#define SNTP_RESPONSE_TIMEOUT_MS  6000ul

extern volatile uint32_t g_SysTickCount;

static uint8_t SNTP_IsLeapYear(uint16_t year)
{
    return (uint8_t)(((year % 4u) == 0u && (year % 100u) != 0u) ||
                     ((year % 400u) == 0u));
}

static void SNTP_UnixToRtcTime(uint32_t unixSeconds, RTC_Time *time)
{
    static const uint8_t daysInMonth[12] = {
        31u, 28u, 31u, 30u, 31u, 30u, 31u, 31u, 30u, 31u, 30u, 31u
    };
    uint32_t days;
    uint32_t secondsOfDay;
    uint16_t year;
    uint8_t month;
    uint8_t dim;

    days = unixSeconds / 86400ul;
    secondsOfDay = unixSeconds % 86400ul;

    time->hour = (uint8_t)(secondsOfDay / 3600ul);
    time->minute = (uint8_t)((secondsOfDay % 3600ul) / 60ul);
    time->second = (uint8_t)(secondsOfDay % 60ul);
    time->week = (uint8_t)((days + 4ul) % 7ul);

    year = 1970u;
    while(1) {
        uint16_t yearDays = SNTP_IsLeapYear(year) ? 366u : 365u;
        if(days < yearDays) {
            break;
        }
        days -= yearDays;
        year++;
    }

    month = 1u;
    while(month <= 12u) {
        dim = daysInMonth[month - 1u];
        if(month == 2u && SNTP_IsLeapYear(year)) {
            dim = 29u;
        }
        if(days < dim) {
            break;
        }
        days -= dim;
        month++;
    }

    time->year = year;
    time->month = month;
    time->day = (uint8_t)(days + 1u);
}

static WiFi_Status SNTP_QueryServer(const char *server, RTC_Time *time)
{
    uint8_t packet[SNTP_PACKET_SIZE];
    uint16_t len = 0;
    uint32_t startTick;
    uint32_t ntpSeconds;
    uint32_t beijingUnixSeconds;
    WiFi_Status status;

    status = WiFi_UDPConnect(server, SNTP_PORT, SNTP_LOCAL_PORT, 5000);
    if(status != WIFI_OK) {
        WiFi_Close(500);
        return status;
    }

    memset(packet, 0, sizeof(packet));
    packet[0] = 0x1Bu;

    status = WiFi_SendData((const char *)packet, SNTP_PACKET_SIZE, 3000);
    if(status != WIFI_OK) {
        WiFi_Close(500);
        return status;
    }

    startTick = g_SysTickCount;
    while((g_SysTickCount - startTick) < SNTP_RESPONSE_TIMEOUT_MS) {
        if(WiFi_ReadIpdPayload(packet, sizeof(packet), &len) && len >= SNTP_PACKET_SIZE) {
            ntpSeconds = ((uint32_t)packet[40] << 24) |
                         ((uint32_t)packet[41] << 16) |
                         ((uint32_t)packet[42] << 8) |
                         ((uint32_t)packet[43]);

            if(ntpSeconds > SNTP_UNIX_OFFSET) {
                beijingUnixSeconds = ntpSeconds - SNTP_UNIX_OFFSET + SNTP_BEIJING_OFFSET_SEC;
                SNTP_UnixToRtcTime(beijingUnixSeconds, time);
                RTC_SetTime(time);
                WiFi_Close(500);
                return WIFI_OK;
            }
            WiFi_Close(500);
            return WIFI_ERROR;
        }
    }

    WiFi_Close(500);
    return WIFI_TIMEOUT;
}

WiFi_Status SNTP_SyncRtc(RTC_Time *syncedTime)
{
    WiFi_Status status;
    RTC_Time time;

    status = SNTP_QueryServer(SNTP_SERVER_ALIYUN, &time);
    if(status != WIFI_OK) {
        status = SNTP_QueryServer(SNTP_SERVER_TENCENT, &time);
    }

    if(status == WIFI_OK && syncedTime != 0) {
        *syncedTime = time;
    }

    return status;
}
