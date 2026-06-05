/**
 * @file    bsp_key.h
 * @brief   按键驱动 - PA0(WK_UP), PC13(TAMPER)
 */

#ifndef __BSP_KEY_H
#define __BSP_KEY_H

#include "stm32f10x.h"

#define KEY1_PORT   GPIOA
#define KEY1_PIN    GPIO_Pin_0     /* WK_UP, 按下为高电平 */
#define KEY2_PORT   GPIOC
#define KEY2_PIN    GPIO_Pin_13    /* TAMPER, 按下为低电平 */

#define KEY1_PRESSED()  (GPIO_ReadInputDataBit(KEY1_PORT, KEY1_PIN) == Bit_SET)
#define KEY2_PRESSED()  (GPIO_ReadInputDataBit(KEY2_PORT, KEY2_PIN) == Bit_RESET)

#define KEY_NONE    0
#define KEY1_DOWN   1
#define KEY2_DOWN   2

void Key_Init(void);
uint8_t Key_Scan(void);     /* 扫描按键, 返回 KEY1_DOWN / KEY2_DOWN / KEY_NONE */

#endif /* __BSP_KEY_H */
