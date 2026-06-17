/**
 * @file    bsp_cfont.h
 * @brief   Fixed Noto Sans SC bitmap font for SmartPillBox UI
 */
#ifndef __BSP_CFONT_H
#define __BSP_CFONT_H

#include "stm32f10x.h"

void CFont_ShowText(uint16_t x, uint16_t y, const char *text, uint16_t fg, uint16_t bg, uint8_t size);
void CFont_ShowTextCenter(uint16_t x1, uint16_t y, uint16_t x2, const char *text, uint16_t fg, uint16_t bg, uint8_t size);
uint16_t CFont_TextWidth(const char *text, uint8_t size);
uint8_t CFont_CanShowText(const char *text);

#endif /* __BSP_CFONT_H */
