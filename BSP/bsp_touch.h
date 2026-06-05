/**
 * @file    bsp_touch.h
 * @brief   XPT2046 触摸屏驱动 (SPI2)
 */

#ifndef __BSP_TOUCH_H
#define __BSP_TOUCH_H

#include "stm32f10x.h"

/* ==================== 触摸屏参数 ==================== */
/* 3.2寸触摸屏校准参数 (根据实际屏幕校准) */
#define TOUCH_X_MIN      200
#define TOUCH_X_MAX      3800
#define TOUCH_Y_MIN      200
#define TOUCH_Y_MAX      3800
#define LCD_X_MAX        320
#define LCD_Y_MAX        240

/* ==================== 触摸状态 ==================== */
typedef struct {
    uint16_t x;       /* LCD X坐标 (0~239) */
    uint16_t y;       /* LCD Y坐标 (0~319) */
    uint8_t  pressed; /* 是否按下: 1=按下, 0=释放 */
} TouchState;

/* ==================== 函数声明 ==================== */
void Touch_Init(void);
TouchState Touch_Read(void);
uint8_t Touch_IsPressed(void);
void Touch_GetPos(uint16_t *x, uint16_t *y);

#endif /* __BSP_TOUCH_H */
