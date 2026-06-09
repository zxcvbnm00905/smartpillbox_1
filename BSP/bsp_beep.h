/**
 * @file    bsp_beep.h
 * @brief   蜂鸣器驱动 - 野火指南者 PA8（有源蜂鸣器）
 */

#ifndef __BSP_BEEP_H
#define __BSP_BEEP_H

#include "stm32f10x.h"

#define BEEP_PORT   GPIOA
#define BEEP_PIN    GPIO_Pin_8

#define BEEP_ON()   GPIO_SetBits(BEEP_PORT, BEEP_PIN)
#define BEEP_OFF()  GPIO_ResetBits(BEEP_PORT, BEEP_PIN)

void BEEP_Init(void);

#endif /* __BSP_BEEP_H */
