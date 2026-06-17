/**
 * @file    bsp_extfont.h
 * @brief   External GB2312 HZK16 font reader
 */
#ifndef __BSP_EXTFONT_H
#define __BSP_EXTFONT_H

#include "stm32f10x.h"

#define EXTFONT_HZK16_ADDR  0x000000u

void ExtFont_Init(void);
uint8_t ExtFont_IsReady(void);
uint8_t ExtFont_ShowGbkText16(uint16_t x, uint16_t y, const uint8_t *gbk,
                              uint8_t len, uint16_t fg, uint16_t bg);

#endif /* __BSP_EXTFONT_H */
