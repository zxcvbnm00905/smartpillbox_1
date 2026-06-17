/**
 * Smart Pill Box - Application Layer Header
 */
#ifndef __PILLBOX_H
#define __PILLBOX_H

#include "stm32f10x.h"
#include "bsp_rtc.h"

#define MED_NAME_LEN    16
#define MED_GBK_NAME_LEN  24
#define MED_DOSE_LEN    12
#define MAX_MEDICINES   4

typedef struct {
    char     name[MED_NAME_LEN];
    uint8_t  nameGbk[MED_GBK_NAME_LEN];
    uint8_t  nameGbkLen;
    uint8_t  alarmHour;
    uint8_t  alarmMinute;
    uint8_t  enabled;
    uint8_t  taken;
    char     dose[MED_DOSE_LEN];
} Medicine;

typedef enum {
    STATE_NORMAL = 0,
    STATE_ALARM,
    STATE_MISSED,
    STATE_SETTING
} PillBox_State;

void PillBox_Init(void);
PillBox_State PillBox_GetState(void);
uint8_t PillBox_GetTriggeredIndex(void);
void PillBox_Process(void);

Medicine* PillBox_GetMedicines(void);
uint8_t   PillBox_GetMedicineCount(void);
void      PillBox_SetMedicine(uint8_t idx,const char*name,uint8_t hour,uint8_t minute,uint8_t enabled);
void      PillBox_SetMedicineGbkName(uint8_t idx,const uint8_t *gbk,uint8_t len);

void PillBox_ConfirmTaken(void);
void PillBox_Snooze(void);
void PillBox_GetNextDoseString(char*buf);
uint8_t PillBox_GetSnoozeRemaining(uint8_t *idx, uint16_t *remainMin);
uint8_t PillBox_GetTakenCount(void);
uint8_t PillBox_GetTotalEnabled(void);
void PillBox_ResetDaily(void);

#endif
