#include "W25Q64.h"
#include "MySPI.h"
#include "W25Q64_Ins.h"

#define W25Q64_BUSY_TIMEOUT  5000000UL

static void W25Q64_WriteEnable(void)
{
	MySPI_Start();
	MySPI_SwapByte(W25Q64_WRITE_ENABLE);
	MySPI_Stop();
}

static uint8_t W25Q64_WaitBusy(void)
{
	uint32_t timeout = W25Q64_BUSY_TIMEOUT;
	uint8_t status;

	MySPI_Start();
	MySPI_SwapByte(W25Q64_READ_STATUS_REGISTER_1);
	do
	{
		status = MySPI_SwapByte(W25Q64_DUMMY_BYTE);
		timeout --;
	} while (((status & 0x01U) != 0U) && (timeout != 0U));
	MySPI_Stop();
	return (timeout != 0U) ? 1U : 0U;
}

static void W25Q64_ProgramOnePage(uint32_t address, const uint8_t *data, uint16_t count)
{
	uint16_t i;

	W25Q64_WriteEnable();
	MySPI_Start();
	MySPI_SwapByte(W25Q64_PAGE_PROGRAM);
	MySPI_SwapByte((uint8_t)(address >> 16));
	MySPI_SwapByte((uint8_t)(address >> 8));
	MySPI_SwapByte((uint8_t)address);
	for (i = 0; i < count; i ++)
	{
		MySPI_SwapByte(data[i]);
	}
	MySPI_Stop();
	W25Q64_WaitBusy();
}

void W25Q64_Init(void)
{
	MySPI_Init();
}

void W25Q64_ReadID(uint8_t *manufacturer_id, uint16_t *device_id)
{
	MySPI_Start();
	MySPI_SwapByte(W25Q64_JEDEC_ID);
	*manufacturer_id = MySPI_SwapByte(W25Q64_DUMMY_BYTE);
	*device_id = (uint16_t)MySPI_SwapByte(W25Q64_DUMMY_BYTE) << 8;
	*device_id |= MySPI_SwapByte(W25Q64_DUMMY_BYTE);
	MySPI_Stop();
}

void W25Q64_PageProgram(uint32_t address, const uint8_t *data, uint16_t count)
{
	uint16_t page_space;
	uint16_t chunk;

	while (count != 0U)
	{
		page_space = W25Q64_PAGE_SIZE - (uint16_t)(address % W25Q64_PAGE_SIZE);
		chunk = (count < page_space) ? count : page_space;
		W25Q64_ProgramOnePage(address, data, chunk);
		address += chunk;
		data += chunk;
		count -= chunk;
	}
}

void W25Q64_SectorErase(uint32_t address)
{
	W25Q64_WriteEnable();
	MySPI_Start();
	MySPI_SwapByte(W25Q64_SECTOR_ERASE_4KB);
	MySPI_SwapByte((uint8_t)(address >> 16));
	MySPI_SwapByte((uint8_t)(address >> 8));
	MySPI_SwapByte((uint8_t)address);
	MySPI_Stop();
	W25Q64_WaitBusy();
}

void W25Q64_ReadData(uint32_t address, uint8_t *data, uint32_t count)
{
	uint32_t i;

	MySPI_Start();
	MySPI_SwapByte(W25Q64_READ_DATA);
	MySPI_SwapByte((uint8_t)(address >> 16));
	MySPI_SwapByte((uint8_t)(address >> 8));
	MySPI_SwapByte((uint8_t)address);
	for (i = 0; i < count; i ++)
	{
		data[i] = MySPI_SwapByte(W25Q64_DUMMY_BYTE);
	}
	MySPI_Stop();
}
