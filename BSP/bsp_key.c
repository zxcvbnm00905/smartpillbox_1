/**
 * @file    bsp_key.c
 * @brief   按键驱动 - 带消抖
 */

#include "bsp_key.h"

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

    /* PA0: WK_UP (下拉输入, 按下接VCC=高电平) */
    GPIO_InitStructure.GPIO_Pin  = KEY1_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(KEY1_PORT, &GPIO_InitStructure);

    /* PC13: TAMPER (上拉输入, 按下接GND=低电平) */
    GPIO_InitStructure.GPIO_Pin  = KEY2_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(KEY2_PORT, &GPIO_InitStructure);
}

/**
 * @brief  按键扫描 (带20ms消抖)
 * @return KEY1_DOWN / KEY2_DOWN / KEY_NONE
 */
uint8_t Key_Scan(void)
{
    static uint8_t key1_state = 0;
    static uint8_t key2_state = 0;

    if (KEY1_PRESSED()) {
        if(key1_state == 0) {
            key1_state = 1;
            return KEY1_DOWN;
        }
    } else {
        key1_state = 0;
    }

    if (KEY2_PRESSED()) {
        if(key2_state == 0) {
            key2_state = 1;
            return KEY2_DOWN;
        }
    } else {
        key2_state = 0;
    }

    return KEY_NONE;
}
