/**
 * @file    bsp_lcd.c
 * @brief   3.2寸 ILI9341 TFT LCD 驱动
 * @note    野火指南者 STM32F103VET6, FSMC 16位
 *          恢复原始配置 + 字体反转扫描修复镜像
 */

#include "bsp_lcd.h"
#include "font.h"

#define ILI9341_SWRESET     0x01
#define ILI9341_SLEEPOUT    0x11
#define ILI9341_DISPLAYON   0x29
#define ILI9341_COLADDRSET  0x2A
#define ILI9341_PAGEADDRSET 0x2B
#define ILI9341_MEMORYWRITE 0x2C
#define ILI9341_MEMORYREAD  0x2E
#define ILI9341_PIXELFORMAT 0x3A
#define ILI9341_MADCTL      0x36

#define LCD_BL_ON()   GPIO_ResetBits(LCD_BL_PORT, LCD_BL_PIN)
#define LCD_BL_OFF()  GPIO_SetBits(LCD_BL_PORT, LCD_BL_PIN)

static void LCD_GPIO_Config(void)
{
    GPIO_InitTypeDef G;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD|RCC_APB2Periph_GPIOE|
                           RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO,ENABLE);
    G.GPIO_Speed=GPIO_Speed_50MHz;
    G.GPIO_Mode=GPIO_Mode_AF_PP;
    G.GPIO_Pin=GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_4|GPIO_Pin_5|
               GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|
               GPIO_Pin_11|GPIO_Pin_14|GPIO_Pin_15;
    GPIO_Init(GPIOD,&G);
    G.GPIO_Pin=GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|
               GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
    GPIO_Init(GPIOE,&G);
    G.GPIO_Pin=GPIO_Pin_1; G.GPIO_Mode=GPIO_Mode_Out_PP;
    GPIO_Init(GPIOE,&G); GPIO_SetBits(GPIOE,GPIO_Pin_1);
    G.GPIO_Pin=GPIO_Pin_12; G.GPIO_Mode=GPIO_Mode_Out_PP;
    GPIO_Init(GPIOD,&G); LCD_BL_OFF();
}

static void LCD_FSMC_Config(void)
{
    FSMC_NORSRAMInitTypeDef I;
    FSMC_NORSRAMTimingInitTypeDef R,W;
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC,ENABLE);
    R.FSMC_AddressSetupTime=0x01; R.FSMC_AddressHoldTime=0x00;
    R.FSMC_DataSetupTime=0x0F; R.FSMC_BusTurnAroundDuration=0x00;
    R.FSMC_CLKDivision=0x00; R.FSMC_DataLatency=0x00;
    R.FSMC_AccessMode=FSMC_AccessMode_B;
    W.FSMC_AddressSetupTime=0x00; W.FSMC_AddressHoldTime=0x00;
    W.FSMC_DataSetupTime=0x03; W.FSMC_BusTurnAroundDuration=0x00;
    W.FSMC_CLKDivision=0x00; W.FSMC_DataLatency=0x00;
    W.FSMC_AccessMode=FSMC_AccessMode_B;
    I.FSMC_Bank=FSMC_Bank1_NORSRAM1;
    I.FSMC_DataAddressMux=FSMC_DataAddressMux_Disable;
    I.FSMC_MemoryType=FSMC_MemoryType_NOR;
    I.FSMC_MemoryDataWidth=FSMC_MemoryDataWidth_16b;
    I.FSMC_BurstAccessMode=FSMC_BurstAccessMode_Disable;
    I.FSMC_AsynchronousWait=FSMC_AsynchronousWait_Disable;
    I.FSMC_WaitSignalPolarity=FSMC_WaitSignalPolarity_Low;
    I.FSMC_WrapMode=FSMC_WrapMode_Disable;
    I.FSMC_WaitSignalActive=FSMC_WaitSignalActive_BeforeWaitState;
    I.FSMC_WriteOperation=FSMC_WriteOperation_Enable;
    I.FSMC_WaitSignal=FSMC_WaitSignal_Disable;
    I.FSMC_ExtendedMode=FSMC_ExtendedMode_Enable;
    I.FSMC_WriteBurst=FSMC_WriteBurst_Disable;
    I.FSMC_ReadWriteTimingStruct=&R;
    I.FSMC_WriteTimingStruct=&W;
    FSMC_NORSRAMInit(&I);
    FSMC_NORSRAMCmd(FSMC_Bank1_NORSRAM1,ENABLE);
}

void LCD_Init(void)
{
    uint32_t d;
    LCD_GPIO_Config();
    LCD_FSMC_Config();

    /* Hardware reset */
    GPIO_ResetBits(LCD_RST_PORT,LCD_RST_PIN);
    for(d=0;d<100000;d++);
    GPIO_SetBits(LCD_RST_PORT,LCD_RST_PIN);
    for(d=0;d<200000;d++);

    /* Software reset */
    LCD_WRITE_CMD(ILI9341_SWRESET);
    for(d=0;d<200000;d++);

    /* Exit sleep */
    LCD_WRITE_CMD(ILI9341_SLEEPOUT);
    for(d=0;d<50000;d++);

    /* Pixel format: 16bit RGB565 */
    LCD_WRITE_CMD(ILI9341_PIXELFORMAT); LCD_WRITE_DATA(0x55);

    /* MADCTL: original value, BGR=1 */
    LCD_WRITE_CMD(ILI9341_MADCTL); LCD_WRITE_DATA(0x08);

    /* Power control */
    LCD_WRITE_CMD(0xCF); LCD_WRITE_DATA(0x00); LCD_WRITE_DATA(0xC1); LCD_WRITE_DATA(0x30);
    LCD_WRITE_CMD(0xED); LCD_WRITE_DATA(0x64); LCD_WRITE_DATA(0x03); LCD_WRITE_DATA(0x12); LCD_WRITE_DATA(0x81);
    LCD_WRITE_CMD(0xE8); LCD_WRITE_DATA(0x85); LCD_WRITE_DATA(0x10); LCD_WRITE_DATA(0x7A);
    LCD_WRITE_CMD(0xCB); LCD_WRITE_DATA(0x39); LCD_WRITE_DATA(0x2C); LCD_WRITE_DATA(0x00);
    LCD_WRITE_DATA(0x34); LCD_WRITE_DATA(0x02);
    LCD_WRITE_CMD(0xF7); LCD_WRITE_DATA(0x20);
    LCD_WRITE_CMD(0xEA); LCD_WRITE_DATA(0x00); LCD_WRITE_DATA(0x00);

    /* Frame rate */
    LCD_WRITE_CMD(0xC0); LCD_WRITE_DATA(0x23);
    LCD_WRITE_CMD(0xC1); LCD_WRITE_DATA(0x10);
    LCD_WRITE_CMD(0xC5); LCD_WRITE_DATA(0x3E); LCD_WRITE_DATA(0x28);
    LCD_WRITE_CMD(0xC7); LCD_WRITE_DATA(0x86);
    LCD_WRITE_CMD(0xB1); LCD_WRITE_DATA(0x00); LCD_WRITE_DATA(0x1B);
    LCD_WRITE_CMD(0xB6); LCD_WRITE_DATA(0x0A); LCD_WRITE_DATA(0xA2);
    LCD_WRITE_CMD(0xF2); LCD_WRITE_DATA(0x00);
    LCD_WRITE_CMD(0x26); LCD_WRITE_DATA(0x01);

    /* Positive gamma */
    LCD_WRITE_CMD(0xE0);
    LCD_WRITE_DATA(0x0F); LCD_WRITE_DATA(0x1D); LCD_WRITE_DATA(0x17);
    LCD_WRITE_DATA(0x0A); LCD_WRITE_DATA(0x11); LCD_WRITE_DATA(0x06);
    LCD_WRITE_DATA(0x37); LCD_WRITE_DATA(0x3B); LCD_WRITE_DATA(0x55);
    LCD_WRITE_DATA(0x08); LCD_WRITE_DATA(0x18); LCD_WRITE_DATA(0x11);
    LCD_WRITE_DATA(0x13); LCD_WRITE_DATA(0x13); LCD_WRITE_DATA(0x3B);

    /* Negative gamma */
    LCD_WRITE_CMD(0xE1);
    LCD_WRITE_DATA(0x14); LCD_WRITE_DATA(0x15); LCD_WRITE_DATA(0x01);
    LCD_WRITE_DATA(0x04); LCD_WRITE_DATA(0x06); LCD_WRITE_DATA(0x03);
    LCD_WRITE_DATA(0x43); LCD_WRITE_DATA(0x48); LCD_WRITE_DATA(0x15);
    LCD_WRITE_DATA(0x05); LCD_WRITE_DATA(0x15); LCD_WRITE_DATA(0x05);
    LCD_WRITE_DATA(0x0C); LCD_WRITE_DATA(0x20); LCD_WRITE_DATA(0x08);

    /* Display on */
    LCD_WRITE_CMD(ILI9341_DISPLAYON);
    LCD_BL_ON();
    LCD_Clear(BLACK);
}

void LCD_Direction(uint8_t dir)
{
    /* 原始 madctl 值 */
    static const uint8_t madctl[4] = {0x00, 0x60, 0xC0, 0x60};
    LCD_WRITE_CMD(ILI9341_MADCTL);
    LCD_WRITE_DATA(madctl[dir & 3]);
}

void LCD_Reset(void)
{
    uint32_t d;
    GPIO_ResetBits(LCD_RST_PORT,LCD_RST_PIN);
    for(d=0;d<10000;d++);
    GPIO_SetBits(LCD_RST_PORT,LCD_RST_PIN);
    for(d=0;d<100000;d++);
}

void LCD_BackLight(uint8_t s) { if(s)LCD_BL_ON(); else LCD_BL_OFF(); }

/* ========== Drawing Functions ========== */

void LCD_SetWindow(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2)
{
    LCD_WRITE_CMD(ILI9341_COLADDRSET);
    LCD_WRITE_DATA(x1>>8); LCD_WRITE_DATA(x1&0xFF);
    LCD_WRITE_DATA(x2>>8); LCD_WRITE_DATA(x2&0xFF);
    LCD_WRITE_CMD(ILI9341_PAGEADDRSET);
    LCD_WRITE_DATA(y1>>8); LCD_WRITE_DATA(y1&0xFF);
    LCD_WRITE_DATA(y2>>8); LCD_WRITE_DATA(y2&0xFF);
    LCD_WRITE_CMD(ILI9341_MEMORYWRITE);
}

void LCD_SetCursor(uint16_t x,uint16_t y) { LCD_SetWindow(x,y,x,y); }

void LCD_Clear(uint16_t color)
{
    uint32_t n=(uint32_t)LCD_WIDTH*LCD_HEIGHT;
    LCD_SetWindow(0,0,LCD_WIDTH-1,LCD_HEIGHT-1);
    while(n--) LCD_WRITE_DATA(color);
}

void LCD_Fill(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color)
{
    uint32_t n=(uint32_t)(x2-x1+1)*(y2-y1+1);
    LCD_SetWindow(x1,y1,x2,y2);
    while(n--) LCD_WRITE_DATA(color);
}

void LCD_DrawPoint(uint16_t x,uint16_t y,uint16_t color)
{ LCD_SetCursor(x,y); LCD_WRITE_DATA(color); }

uint16_t LCD_ReadPoint(uint16_t x,uint16_t y)
{ LCD_SetCursor(x,y); LCD_WRITE_CMD(ILI9341_MEMORYREAD); LCD_READ_DATA(); return LCD_READ_DATA(); }

void LCD_DrawLine(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color)
{
    int16_t dx=(int16_t)x2-x1,dy=(int16_t)y2-y1;
    int16_t ux=dx>0?1:-1,uy=dy>0?1:-1;
    uint16_t adx=dx>0?dx:-dx,ady=dy>0?dy:-dy;
    int16_t x=x1,y=y1,eps;
    if(adx>ady){eps=-adx;for(uint16_t i=0;i<=adx;i++){LCD_DrawPoint(x,y,color);x+=ux;eps+=2*ady;if(eps>0){y+=uy;eps-=2*adx;}}}
    else{eps=-ady;for(uint16_t i=0;i<=ady;i++){LCD_DrawPoint(x,y,color);y+=uy;eps+=2*adx;if(eps>0){x+=ux;eps-=2*ady;}}}
}

void LCD_DrawRect(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color)
{ LCD_DrawLine(x1,y1,x2,y1,color);LCD_DrawLine(x1,y1,x1,y2,color);LCD_DrawLine(x1,y2,x2,y2,color);LCD_DrawLine(x2,y1,x2,y2,color); }

void LCD_FillRect(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2,uint16_t color)
{ LCD_Fill(x1,y1,x2,y2,color); }

void LCD_DrawCircle(uint16_t cx,uint16_t cy,uint16_t r,uint16_t color)
{
    int16_t x=0,y=r,d=1-r;
    while(y>=x){
        LCD_DrawPoint(cx+x,cy+y,color);LCD_DrawPoint(cx-x,cy+y,color);
        LCD_DrawPoint(cx+x,cy-y,color);LCD_DrawPoint(cx-x,cy-y,color);
        LCD_DrawPoint(cx+y,cy+x,color);LCD_DrawPoint(cx-y,cy+x,color);
        LCD_DrawPoint(cx+y,cy-x,color);LCD_DrawPoint(cx-y,cy-x,color);
        x++;if(d<0)d+=2*x+1;else{y--;d+=2*(x-y)+1;}
    }
}

void LCD_FillCircle(uint16_t cx,uint16_t cy,uint16_t r,uint16_t color)
{
    int16_t x=0,y=r,d=1-r;
    while(y>=x){
        LCD_DrawLine(cx-x,cy+y,cx+x,cy+y,color);LCD_DrawLine(cx-x,cy-y,cx+x,cy-y,color);
        LCD_DrawLine(cx-y,cy+x,cx+y,cy+x,color);LCD_DrawLine(cx-y,cy-x,cx+y,cy-x,color);
        x++;if(d<0)d+=2*x+1;else{y--;d+=2*(x-y)+1;}
    }
}

/* ========== Text Drawing ========== */

void LCD_ShowString(uint16_t x,uint16_t y,const char*str,uint16_t fg,uint16_t bg,uint8_t size)
{
    while(*str){
        if(x+6>LCD_WIDTH){x=0;y+=12;}
        if(*str=='\n'){x=0;y+=12;str++;continue;}
        LCD_ShowChar(x,y,*str,fg,bg,size);
        x+=6;str++;
    }
}

void LCD_ShowNum(uint16_t x,uint16_t y,int32_t num,uint8_t len,uint16_t fg,uint16_t bg,uint8_t size)
{
    char buf[12]; int8_t i;
    for(i=len-1;i>=0;i--){buf[i]='0'+(num%10);num/=10;}
    buf[len]='\0';
    LCD_ShowString(x,y,buf,fg,bg,size);
}

void LCD_ShowChar(uint16_t x,uint16_t y,char chr,uint16_t fg,uint16_t bg,uint8_t size)
{
    (void)size;
    uint8_t cw=6,ch=12;
    LCD_SetWindow(x,y,x+cw-1,y+ch-1);
    for(uint8_t row=0;row<ch;row++){
        uint8_t b=Font_6x12[(uint8_t)chr-' '][row];
        /* 硬件面板反向: bit2↔bit7, bit3↔bit6, bit4↔bit5 */
        uint8_t rev=0;
        if(b&0x80)rev|=0x04; if(b&0x40)rev|=0x08;
        if(b&0x20)rev|=0x10; if(b&0x10)rev|=0x20;
        if(b&0x08)rev|=0x40; if(b&0x04)rev|=0x80;

        for(int8_t col=0; col<cw; col++)
            LCD_WRITE_DATA((rev&(0x80>>col))?fg:bg);
    }
}

void LCD_ShowChinese(uint16_t x,uint16_t y,const uint8_t*hz,uint16_t fg,uint16_t bg)
{
    extern const uint8_t HZK16[];
    uint32_t off=((hz[0]-0xA1)*94+(hz[1]-0xA1))*32;
    uint8_t buf[32],i;
    for(i=0;i<32;i++)buf[i]=HZK16[off+i];
    LCD_SetWindow(x,y,x+15,y+15);
    for(uint8_t row=0;row<16;row++){
        uint16_t rd=(buf[row*2]<<8)|buf[row*2+1];
        /* 硬件面板反向: 翻转16bit */
        uint16_t rv=0; uint8_t j;
        for(j=0;j<16;j++) if(rd&(1<<j)) rv|=(0x8000>>j);
        
        for(int8_t col=0; col<16; col++)
            LCD_WRITE_DATA((rv&(0x8000>>col))?fg:bg);
    }
}
