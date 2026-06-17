/**
 * @file    bsp_sntp.h
 * @brief   SNTP network time sync for SmartPillBox
 */
#ifndef __BSP_SNTP_H
#define __BSP_SNTP_H

#include "stm32f10x.h"
#include "bsp_rtc.h"
#include "bsp_wifi.h"

#define SNTP_SERVER_ALIYUN        "ntp.aliyun.com"
#define SNTP_SERVER_TENCENT       "time1.cloud.tencent.com"
#define SNTP_PORT                 123u
#define SNTP_LOCAL_PORT           123u
#define SNTP_BEIJING_OFFSET_SEC   (8ul * 3600ul)

WiFi_Status SNTP_SyncRtc(RTC_Time *syncedTime);

#endif /* __BSP_SNTP_H */
