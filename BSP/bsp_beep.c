/**
 * @file    bsp_beep.c
 * @brief   蜂鸣器驱动 - 无源蜂鸣器用PWM驱动，有源蜂鸣器直接用GPIO
 * @note    PB2 (TIM2_CH3) 可输出PWM, 也可直接GPIO高低电平
 */

#include "bsp_beep.h"

void BEEP_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin   = BEEP_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BEEP_PORT, &GPIO_InitStructure);

    BEEP_OFF();
}

/**
 * @brief  蜂鸣器响指定毫秒 (简单延时, 实际项目用定时器)
 */
void BEEP_Beep(uint16_t ms)
{
    BEEP_ON();
    for (volatile uint32_t i = 0; i < ms * 1000; i++) {
        __NOP();
    }
    BEEP_OFF();
}

/**
 * @brief  闹钟提示音: 响times次, 每声onMs毫秒, 间隔offMs毫秒
 */
void BEEP_Alarm(uint8_t times, uint16_t onMs, uint16_t offMs)
{
    for (uint8_t i = 0; i < times; i++) {
        BEEP_ON();
        for (volatile uint32_t j = 0; j < onMs * 1000; j++) __NOP();
        BEEP_OFF();
        if (i < times - 1) {
            for (volatile uint32_t j = 0; j < offMs * 1000; j++) __NOP();
        }
    }
}
