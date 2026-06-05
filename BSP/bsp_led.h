/**
 * @file    bsp_led.h
 * @brief   LED驱动 - 野火指南者 PB5
 */

#ifndef __BSP_LED_H
#define __BSP_LED_H

#include "stm32f10x.h"

#define LED_PORT    GPIOB
#define LED_PIN     GPIO_Pin_5

#define LED_ON()    GPIO_ResetBits(LED_PORT, LED_PIN)
#define LED_OFF()   GPIO_SetBits(LED_PORT, LED_PIN)
#define LED_TOGGLE() (GPIO_ReadOutputDataBit(LED_PORT, LED_PIN) ? LED_ON() : LED_OFF())

void LED_Init(void);

#endif /* __BSP_LED_H */
