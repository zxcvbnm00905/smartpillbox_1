#include <stdio.h>
#include <string.h>

static int beep_on_count;
static int led_on_count;
static int beep_is_on;
static int led_is_on;

#include "bsp_rtc.h"

static RTC_Time fake_now = {
    2026, 6, 6, 6, 7, 29, 50
};

void RTC_GetTime(RTC_Time *time)
{
    *time = fake_now;
}

void BSP_RTC_Alarm(uint8_t index, uint8_t hour, uint8_t minute, uint8_t enabled)
{
    (void)index;
    (void)hour;
    (void)minute;
    (void)enabled;
}

void Fake_BeepOn(void)
{
    beep_is_on = 1;
    beep_on_count++;
}

void Fake_BeepOff(void)
{
    beep_is_on = 0;
}

void Fake_LedOn(void)
{
    led_is_on = 1;
    led_on_count++;
}

void Fake_LedOff(void)
{
    led_is_on = 0;
}

#include "../APP/pillbox.c"

static void tick_one_second(void)
{
    fake_now.second++;
    if (fake_now.second >= 60) {
        fake_now.second = 0;
        fake_now.minute++;
    }
    if (fake_now.minute >= 60) {
        fake_now.minute = 0;
        fake_now.hour++;
    }
}

int main(void)
{
    PillBox_Init();

    for (int i = 0; i < 12; i++) {
        PillBox_Process();
        tick_one_second();
    }

    if (PillBox_GetState() != STATE_ALARM) {
        printf("FAIL: state=%d, expected STATE_ALARM\n", PillBox_GetState());
        return 1;
    }

    if (beep_on_count == 0 || led_on_count == 0) {
        printf("FAIL: beep_on_count=%d led_on_count=%d\n",
               beep_on_count, led_on_count);
        return 1;
    }

    printf("PASS: alarm triggered at 07:30, beep_on_count=%d led_on_count=%d, beep_is_on=%d led_is_on=%d\n",
           beep_on_count, led_on_count, beep_is_on, led_is_on);
    return 0;
}
