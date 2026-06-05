/**
 * @file    bsp_beep.h
 * @brief   蜂鸣器驱动 - 野火指南者 PB2
 */

#ifndef __BSP_BEEP_H
#define __BSP_BEEP_H

#include "stm32f10x.h"

#define BEEP_PORT   GPIOB
#define BEEP_PIN    GPIO_Pin_2

#define BEEP_ON()   GPIO_SetBits(BEEP_PORT, BEEP_PIN)
#define BEEP_OFF()  GPIO_ResetBits(BEEP_PORT, BEEP_PIN)

void BEEP_Init(void);
void BEEP_Beep(uint16_t ms);
void BEEP_Alarm(uint8_t times, uint16_t onMs, uint16_t offMs);

#endif /* __BSP_BEEP_H */
