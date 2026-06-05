/**
 * @file    bsp_touch.c
 * @brief   XPT2046 触摸驱动 (SPI2)
 * @note    野火指南者: PB12(CS), PB13(SCK), PB14(MISO), PB15(MOSI), PG7(IRQ)
 */

#include "bsp_touch.h"

/* ==================== 引脚定义 ==================== */
#define TOUCH_CS_PORT   GPIOB
#define TOUCH_CS_PIN    GPIO_Pin_12
#define TOUCH_IRQ_PORT  GPIOG
#define TOUCH_IRQ_PIN   GPIO_Pin_7

#define TOUCH_CS_LOW()   GPIO_ResetBits(TOUCH_CS_PORT, TOUCH_CS_PIN)
#define TOUCH_CS_HIGH()  GPIO_SetBits(TOUCH_CS_PORT, TOUCH_CS_PIN)
#define TOUCH_IRQ_READ() GPIO_ReadInputDataBit(TOUCH_IRQ_PORT, TOUCH_IRQ_PIN)

/* XPT2046 命令 */
#define CMD_X_POS       0xD0   /* 读取X坐标, 12位模式 */
#define CMD_Y_POS       0x90   /* 读取Y坐标 */

/* ==================== SPI2 初始化 ==================== */
static void Touch_SPI_Init(void)
{
    SPI_InitTypeDef  SPI_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOG, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

    /* SPI2: PB13=SCK, PB14=MISO, PB15=MOSI (AF_PP) */
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* MISO: PB14 (Input floating) */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_14;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* CS: PB12 (推挽输出) */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    TOUCH_CS_HIGH();

    /* IRQ: PG7 (上拉输入) */
    GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOG, &GPIO_InitStructure);

    /* SPI2 配置: 主机模式, 8位, CPOL=0, CPHA=0, 2.25MHz */
    SPI_InitStructure.SPI_Direction         = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode              = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize          = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL              = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA              = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS               = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; /* 36MHz/32=1.125MHz */
    SPI_InitStructure.SPI_FirstBit          = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial     = 7;
    SPI_Init(SPI2, &SPI_InitStructure);

    SPI_Cmd(SPI2, ENABLE);
}

/* ==================== SPI 读写 ==================== */
static uint8_t SPI2_ReadWrite(uint8_t data)
{
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI2, data);
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    return SPI_I2S_ReceiveData(SPI2);
}

/* ==================== 读取触摸ADC值 ==================== */
static uint16_t Touch_ReadADC(uint8_t cmd)
{
    uint16_t value;

    TOUCH_CS_LOW();
    SPI2_ReadWrite(cmd);
    /* 读取12位结果 */
    value  = (uint16_t)SPI2_ReadWrite(0x00) << 8;
    value |= SPI2_ReadWrite(0x00);
    value >>= 3;
    TOUCH_CS_HIGH();

    return value;
}

/* ==================== 滤波读取 (多次采样取平均) ==================== */
static uint16_t Touch_ReadFiltered(uint8_t cmd)
{
    uint32_t sum = 0;
    uint8_t  count = 5;
    uint16_t values[5];
    uint8_t  i, j;

    for (i = 0; i < count; i++) {
        values[i] = Touch_ReadADC(cmd);
    }

    /* 冒泡排序后去掉最大最小值 */
    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            if (values[i] > values[j]) {
                uint16_t tmp = values[i];
                values[i] = values[j];
                values[j] = tmp;
            }
        }
    }

    for (i = 1; i < count - 1; i++) {
        sum += values[i];
    }
    return (uint16_t)(sum / (count - 2));
}

/* ==================== 初始化 ==================== */
void Touch_Init(void)
{
    Touch_SPI_Init();
}

/* ==================== 读取触摸状态 ==================== */
TouchState Touch_Read(void)
{
    TouchState state;
    state.x = 0;
    state.y = 0;

    if (TOUCH_IRQ_READ() == 0) {  /* 低电平表示有触摸 */
        state.pressed = 1;

        uint16_t rawX = Touch_ReadFiltered(CMD_X_POS);
        uint16_t rawY = Touch_ReadFiltered(CMD_Y_POS);

        /* 将原始ADC值映射到LCD坐标 */
        if (rawX <= TOUCH_X_MIN) rawX = TOUCH_X_MIN;
        if (rawX >= TOUCH_X_MAX) rawX = TOUCH_X_MAX;
        if (rawY <= TOUCH_Y_MIN) rawY = TOUCH_Y_MIN;
        if (rawY >= TOUCH_Y_MAX) rawY = TOUCH_Y_MAX;

        /* 横屏映射 */
        state.x = (uint16_t)((uint32_t)(rawY - TOUCH_Y_MIN) * LCD_X_MAX /
                             (TOUCH_Y_MAX - TOUCH_Y_MIN));
        state.y = (uint16_t)((uint32_t)(rawX - TOUCH_X_MIN) * LCD_Y_MAX /
                             (TOUCH_X_MAX - TOUCH_X_MIN));
        state.x = LCD_X_MAX - state.x;
    } else {
        state.pressed = 0;
    }

    return state;
}

uint8_t Touch_IsPressed(void)
{
    return (TOUCH_IRQ_READ() == 0) ? 1 : 0;
}

void Touch_GetPos(uint16_t *x, uint16_t *y)
{
    TouchState state = Touch_Read();
    *x = state.x;
    *y = state.y;
}
