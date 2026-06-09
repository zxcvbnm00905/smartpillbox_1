/**
 * @file    bsp_beep.c
 * @brief   蜂鸣器驱动 - 野火指南者 PA8 有源蜂鸣器
 *          有源蜂鸣器只需持续直流即可发声，不需要PWM
 */

#include "bsp_beep.h"

void BEEP_Init(void)
{
    GPIO_InitTypeDef s;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    s.GPIO_Pin   = BEEP_PIN;
    s.GPIO_Mode  = GPIO_Mode_Out_PP;
    s.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BEEP_PORT, &s);

    BEEP_OFF();
}
