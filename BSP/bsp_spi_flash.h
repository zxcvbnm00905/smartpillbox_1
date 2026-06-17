/**
 * @file    bsp_spi_flash.h
 * @brief   Minimal W25Qxx SPI flash reader for external font data
 */
#ifndef __BSP_SPI_FLASH_H
#define __BSP_SPI_FLASH_H

#include "stm32f10x.h"

void SPIFlash_Init(void);
uint32_t SPIFlash_ReadID(void);
void SPIFlash_ReadBuffer(uint8_t *buffer, uint32_t readAddr, uint16_t numByteToRead);

#endif /* __BSP_SPI_FLASH_H */
