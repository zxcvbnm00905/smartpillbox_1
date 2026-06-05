/**
 * @file    bsp_lcd.h
 * @brief   3.2�� ILI9341 TFT LCD ����ͷ�ļ�
 * @note    Ұ��ָ���� STM32F103VET6, FSMC 16λ
 */

#ifndef __BSP_LCD_H
#define __BSP_LCD_H

#include "stm32f10x.h"
#include <stdio.h>

/* ==================== LCD �ֱ��� ==================== */
#define LCD_WIDTH    320
#define LCD_HEIGHT   240

/* ==================== FSMC ��ַӳ�� ==================== */
/* FSMC Bank1 NE1(PD7), A16(PD11)=RS, 16λ: HADDR[17]=>A16 */
#define LCD_CMD_ADDR    ((uint32_t)0x60000000)
#define LCD_DATA_ADDR   ((uint32_t)(0x60000000 | (1 << 17)))

#define LCD_WRITE_CMD(cmd)    (*(__IO uint16_t *)LCD_CMD_ADDR = (cmd))
#define LCD_WRITE_DATA(data)  (*(__IO uint16_t *)LCD_DATA_ADDR = (data))
#define LCD_READ_DATA()       (*(__IO uint16_t *)LCD_DATA_ADDR)

/* ==================== �������� ==================== */
#define LCD_RST_PORT    GPIOE
#define LCD_RST_PIN     GPIO_Pin_1
#define LCD_BL_PORT     GPIOD
#define LCD_BL_PIN      GPIO_Pin_12

/* ==================== ��ɫ (RGB565) ==================== */
#define WHITE           0xFFFF
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define YELLOW          0xFFE0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define GRAY            0x8430
#define LIGHTGRAY       0xC618
#define DARKBLUE        0x01CF
#define LIGHTBLUE       0x7D7C
#define ORANGE          0xFD20
#define PINK            0xF81F

/* ==================== �������� ==================== */
void LCD_Init(void);
void LCD_Reset(void);
void LCD_BackLight(uint8_t state);
void LCD_Direction(uint8_t dir);  /* 0/1/2/3 = portrait/landscape/portrait-flip/landscape-flip */

void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_SetCursor(uint16_t x, uint16_t y);
void LCD_Clear(uint16_t color);
void LCD_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
uint16_t LCD_ReadPoint(uint16_t x, uint16_t y);
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_FillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);
void LCD_FillCircle(uint16_t cx, uint16_t cy, uint16_t r, uint16_t color);

void LCD_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bgColor, uint8_t size);
void LCD_ShowString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bgColor, uint8_t size);
void LCD_ShowNum(uint16_t x, uint16_t y, int32_t num, uint8_t len, uint16_t color, uint16_t bgColor, uint8_t size);
void LCD_ShowChinese(uint16_t x, uint16_t y, const uint8_t *hz, uint16_t color, uint16_t bgColor);

#endif /* __BSP_LCD_H */
