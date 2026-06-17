/**
 * @file    bsp_touch.c
 * @brief   XPT2046 touch driver, software SPI
 * @note    Wildfire Guide touch pins:
 *          PD13(CS), PE0(CLK), PE2(DIN/MOSI), PE3(DOUT/MISO), PE4(PENIRQ)
 */

#include "bsp_touch.h"

/* ==================== Pin definitions ==================== */
#define TOUCH_CS_PORT       GPIOD
#define TOUCH_CS_PIN        GPIO_Pin_13
#define TOUCH_CLK_PORT      GPIOE
#define TOUCH_CLK_PIN       GPIO_Pin_0
#define TOUCH_MOSI_PORT     GPIOE
#define TOUCH_MOSI_PIN      GPIO_Pin_2
#define TOUCH_MISO_PORT     GPIOE
#define TOUCH_MISO_PIN      GPIO_Pin_3
#define TOUCH_IRQ_PORT      GPIOE
#define TOUCH_IRQ_PIN       GPIO_Pin_4

#define TOUCH_CS_LOW()      GPIO_ResetBits(TOUCH_CS_PORT, TOUCH_CS_PIN)
#define TOUCH_CS_HIGH()     GPIO_SetBits(TOUCH_CS_PORT, TOUCH_CS_PIN)
#define TOUCH_CLK_LOW()     GPIO_ResetBits(TOUCH_CLK_PORT, TOUCH_CLK_PIN)
#define TOUCH_CLK_HIGH()    GPIO_SetBits(TOUCH_CLK_PORT, TOUCH_CLK_PIN)
#define TOUCH_MOSI_LOW()    GPIO_ResetBits(TOUCH_MOSI_PORT, TOUCH_MOSI_PIN)
#define TOUCH_MOSI_HIGH()   GPIO_SetBits(TOUCH_MOSI_PORT, TOUCH_MOSI_PIN)
#define TOUCH_MISO_READ()   GPIO_ReadInputDataBit(TOUCH_MISO_PORT, TOUCH_MISO_PIN)
#define TOUCH_IRQ_READ()    GPIO_ReadInputDataBit(TOUCH_IRQ_PORT, TOUCH_IRQ_PIN)

/* XPT2046 commands: official sample uses 0x90 for X, 0xD0 for Y. */
#define CMD_X_POS           0x90
#define CMD_Y_POS           0xD0

static void Touch_Delay(void)
{
    volatile uint8_t i;
    for(i = 0; i < 60; i++) {
        ;
    }
}

static void Touch_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE, ENABLE);

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Pin = TOUCH_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(TOUCH_CS_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = TOUCH_CLK_PIN | TOUCH_MOSI_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(TOUCH_CLK_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = TOUCH_MISO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(TOUCH_MISO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = TOUCH_IRQ_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(TOUCH_IRQ_PORT, &GPIO_InitStructure);

    TOUCH_CS_HIGH();
    TOUCH_CLK_LOW();
    TOUCH_MOSI_LOW();
}

static void Touch_WriteByte(uint8_t data)
{
    uint8_t i;

    TOUCH_MOSI_LOW();
    TOUCH_CLK_LOW();

    for(i = 0; i < 8; i++) {
        if((data >> (7u - i)) & 0x01u) {
            TOUCH_MOSI_HIGH();
        } else {
            TOUCH_MOSI_LOW();
        }

        Touch_Delay();
        TOUCH_CLK_HIGH();
        Touch_Delay();
        TOUCH_CLK_LOW();
    }
}

static uint16_t Touch_Read12Bits(void)
{
    uint8_t i;
    uint16_t value = 0;
    uint16_t bit;

    TOUCH_MOSI_LOW();
    TOUCH_CLK_HIGH();

    for(i = 0; i < 12u; i++) {
        TOUCH_CLK_LOW();
        Touch_Delay();
        bit = TOUCH_MISO_READ();
        value |= (uint16_t)(bit << (11u - i));
        TOUCH_CLK_HIGH();
        Touch_Delay();
    }

    return value;
}

/* ==================== Read touch ADC ==================== */
static uint16_t Touch_ReadADC(uint8_t cmd)
{
    uint16_t value;

    TOUCH_CS_LOW();

    Touch_WriteByte(cmd);
    value = Touch_Read12Bits();

    TOUCH_CS_HIGH();

    return (uint16_t)(value & 0x0FFF);
}

/* ==================== Filtered read ==================== */
static uint16_t Touch_ReadFiltered(uint8_t cmd)
{
    uint32_t sum = 0;
    uint8_t count = 5;
    uint16_t values[5];
    uint8_t i;
    uint8_t j;

    (void)Touch_ReadADC(cmd);
    for(i = 0; i < count; i++) {
        values[i] = Touch_ReadADC(cmd);
    }

    for(i = 0; i < count - 1; i++) {
        for(j = i + 1; j < count; j++) {
            if(values[i] > values[j]) {
                uint16_t tmp = values[i];
                values[i] = values[j];
                values[j] = tmp;
            }
        }
    }

    for(i = 1; i < count - 1; i++) {
        sum += values[i];
    }
    return (uint16_t)(sum / (count - 2));
}

void Touch_Init(void)
{
    Touch_GPIO_Init();
}

TouchState Touch_Read(void)
{
    TouchState state;
    uint16_t rawX;
    uint16_t rawY;
    uint8_t irqPressed;

    state.x = 0;
    state.y = 0;
    state.pressed = 0;

    irqPressed = (TOUCH_IRQ_READ() == 0u) ? 1u : 0u;
    rawX = Touch_ReadFiltered(CMD_X_POS);
    rawY = Touch_ReadFiltered(CMD_Y_POS);

    /*
     * Some LCD adapters do not route PENIRQ reliably.  Keep PENIRQ as the
     * preferred touch flag, but also accept sane ADC coordinates as fallback.
     */
    if(!irqPressed) {
        if(rawX < TOUCH_X_MIN || rawX > TOUCH_X_MAX ||
           rawY < TOUCH_Y_MIN || rawY > TOUCH_Y_MAX) {
            return state;
        }
    }

    state.pressed = 1;

    if(rawX <= TOUCH_X_MIN) rawX = TOUCH_X_MIN;
    if(rawX >= TOUCH_X_MAX) rawX = TOUCH_X_MAX;
    if(rawY <= TOUCH_Y_MIN) rawY = TOUCH_Y_MIN;
    if(rawY >= TOUCH_Y_MAX) rawY = TOUCH_Y_MAX;

    state.x = (uint16_t)((uint32_t)(rawY - TOUCH_Y_MIN) * LCD_X_MAX /
                         (TOUCH_Y_MAX - TOUCH_Y_MIN));
    state.y = (uint16_t)((uint32_t)(rawX - TOUCH_X_MIN) * LCD_Y_MAX /
                         (TOUCH_X_MAX - TOUCH_X_MIN));
    state.x = LCD_X_MAX - state.x;

    if(state.x >= LCD_X_MAX) state.x = LCD_X_MAX - 1;
    if(state.y >= LCD_Y_MAX) state.y = LCD_Y_MAX - 1;

    return state;
}

uint8_t Touch_IsPressed(void)
{
    return (TOUCH_IRQ_READ() == 0) ? 1 : 0;
}

void Touch_GetPos(uint16_t *x, uint16_t *y)
{
    TouchState state;

    state = Touch_Read();
    *x = state.x;
    *y = state.y;
}
