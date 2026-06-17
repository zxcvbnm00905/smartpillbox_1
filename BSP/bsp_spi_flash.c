/**
 * @file    bsp_spi_flash.c
 * @brief   Minimal W25Qxx SPI flash reader for external font data
 */
#include "bsp_spi_flash.h"

#define FLASH_SPI                  SPI1
#define FLASH_SPI_CLK              RCC_APB2Periph_SPI1
#define FLASH_GPIO_CLK             RCC_APB2Periph_GPIOA
#define FLASH_GPIO_PORT            GPIOA
#define FLASH_SCK_PIN              GPIO_Pin_5
#define FLASH_MISO_PIN             GPIO_Pin_6
#define FLASH_MOSI_PIN             GPIO_Pin_7
#define FLASH_CS_PORT              GPIOA
#define FLASH_CS_PIN               GPIO_Pin_4

#define W25X_ReadData              0x03u
#define W25X_JedecDeviceID         0x9Fu
#define Dummy_Byte                 0xFFu

#define FLASH_CS_LOW()             GPIO_ResetBits(FLASH_CS_PORT, FLASH_CS_PIN)
#define FLASH_CS_HIGH()            GPIO_SetBits(FLASH_CS_PORT, FLASH_CS_PIN)

static uint8_t SPIFlash_SendByte(uint8_t byte)
{
    while(SPI_I2S_GetFlagStatus(FLASH_SPI, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(FLASH_SPI, byte);
    while(SPI_I2S_GetFlagStatus(FLASH_SPI, SPI_I2S_FLAG_RXNE) == RESET);
    return (uint8_t)SPI_I2S_ReceiveData(FLASH_SPI);
}

void SPIFlash_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    RCC_APB2PeriphClockCmd(FLASH_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(FLASH_SPI_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = FLASH_SCK_PIN | FLASH_MOSI_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(FLASH_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = FLASH_MISO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(FLASH_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = FLASH_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(FLASH_CS_PORT, &GPIO_InitStructure);
    FLASH_CS_HIGH();

    SPI_I2S_DeInit(FLASH_SPI);
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(FLASH_SPI, &SPI_InitStructure);
    SPI_Cmd(FLASH_SPI, ENABLE);
}

uint32_t SPIFlash_ReadID(void)
{
    uint32_t id;
    FLASH_CS_LOW();
    SPIFlash_SendByte(W25X_JedecDeviceID);
    id  = (uint32_t)SPIFlash_SendByte(Dummy_Byte) << 16;
    id |= (uint32_t)SPIFlash_SendByte(Dummy_Byte) << 8;
    id |= SPIFlash_SendByte(Dummy_Byte);
    FLASH_CS_HIGH();
    return id;
}

void SPIFlash_ReadBuffer(uint8_t *buffer, uint32_t readAddr, uint16_t numByteToRead)
{
    uint16_t i;
    if(buffer == 0 || numByteToRead == 0u)return;

    FLASH_CS_LOW();
    SPIFlash_SendByte(W25X_ReadData);
    SPIFlash_SendByte((uint8_t)((readAddr & 0xFF0000u) >> 16));
    SPIFlash_SendByte((uint8_t)((readAddr & 0x00FF00u) >> 8));
    SPIFlash_SendByte((uint8_t)(readAddr & 0x0000FFu));
    for(i = 0; i < numByteToRead; i++) {
        buffer[i] = SPIFlash_SendByte(Dummy_Byte);
    }
    FLASH_CS_HIGH();
}
