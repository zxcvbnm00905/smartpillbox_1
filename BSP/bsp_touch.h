/**
 * @file    bsp_touch.h
 * @brief   XPT2046 touch driver
 */

#ifndef __BSP_TOUCH_H
#define __BSP_TOUCH_H

#include "stm32f10x.h"

/* ==================== Touch calibration ==================== */
#define TOUCH_X_MIN      200
#define TOUCH_X_MAX      3800
#define TOUCH_Y_MIN      200
#define TOUCH_Y_MAX      3800
#define LCD_X_MAX        320
#define LCD_Y_MAX        240

/* ==================== Touch state ==================== */
typedef struct {
    uint16_t x;       /* LCD X coordinate (0~319) */
    uint16_t y;       /* LCD Y coordinate (0~239) */
    uint8_t  pressed; /* 1=pressed, 0=released */
} TouchState;

void Touch_Init(void);
TouchState Touch_Read(void);
uint8_t Touch_IsPressed(void);
void Touch_GetPos(uint16_t *x, uint16_t *y);

#endif /* __BSP_TOUCH_H */
