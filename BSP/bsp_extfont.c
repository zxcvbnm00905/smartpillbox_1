/**
 * @file    bsp_extfont.c
 * @brief   External GB2312 HZK16 font reader
 */
#include "bsp_extfont.h"
#include "bsp_spi_flash.h"
#include "bsp_lcd.h"

static uint8_t g_ExtFontReady = 0u;

static uint8_t ExtFont_ReadGbk16(uint8_t high, uint8_t low, uint8_t *buf)
{
    uint32_t offset;
    if(buf == 0)return 0;
    if(high < 0xA1u || high > 0xF7u || low < 0xA1u || low > 0xFEu)return 0;

    offset = ((uint32_t)(high - 0xA1u) * 94u + (uint32_t)(low - 0xA1u)) * 32u;
    SPIFlash_ReadBuffer(buf, EXTFONT_HZK16_ADDR + offset, 32u);
    return 1;
}

static void ExtFont_DrawGlyph16(uint16_t x, uint16_t y, const uint8_t *buf,
                                uint16_t fg, uint16_t bg)
{
    uint8_t row;
    int8_t col;
    LCD_SetWindow(x, y, (uint16_t)(x + 15u), (uint16_t)(y + 15u));
    for(row = 0; row < 16u; row++) {
        uint16_t rd = ((uint16_t)buf[row * 2u] << 8) | buf[row * 2u + 1u];
        for(col = 0; col < 16; col++) {
            LCD_WRITE_DATA((rd & (0x8000u >> col)) ? fg : bg);
        }
    }
}

void ExtFont_Init(void)
{
    uint32_t id;
    SPIFlash_Init();
    id = SPIFlash_ReadID();
    g_ExtFontReady = (id != 0u && id != 0xFFFFFFu) ? 1u : 0u;
}

uint8_t ExtFont_IsReady(void)
{
    return g_ExtFontReady;
}

uint8_t ExtFont_ShowGbkText16(uint16_t x, uint16_t y, const uint8_t *gbk,
                              uint8_t len, uint16_t fg, uint16_t bg)
{
    uint8_t i;
    uint8_t buf[32];
    if(!g_ExtFontReady || gbk == 0 || len == 0u)return 0;

    for(i = 0; i + 1u < len; i += 2u) {
        if(!ExtFont_ReadGbk16(gbk[i], gbk[i + 1u], buf))return 0;
        ExtFont_DrawGlyph16(x, y, buf, fg, bg);
        x = (uint16_t)(x + 16u);
    }
    return 1;
}
