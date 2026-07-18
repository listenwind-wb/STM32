#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f10x.h"

#define W25Q64_PAGE_SIZE    256U
#define W25Q64_SECTOR_SIZE  4096UL
#define W25Q64_TOTAL_SIZE   (8UL * 1024UL * 1024UL)

void W25Q64_Init(void);
void W25Q64_ReadID(uint8_t *manufacturer_id, uint16_t *device_id);
void W25Q64_PageProgram(uint32_t address, const uint8_t *data, uint16_t count);
void W25Q64_SectorErase(uint32_t address);
void W25Q64_ReadData(uint32_t address, uint8_t *data, uint32_t count);

#endif
