#include "Logger.h"
#include "W25Q64.h"

#define LOGGER_FLASH_ADDRESS  0x000000UL
#define LOGGER_RECORD_SIZE    32U
#define LOGGER_CAPACITY       (W25Q64_SECTOR_SIZE / LOGGER_RECORD_SIZE)

static uint16_t Logger_Count;
static uint32_t Logger_NextSequence;
static uint8_t Logger_Ready;
static uint8_t Logger_ManufacturerID;
static uint16_t Logger_DeviceID;

static void Logger_WriteU16(uint8_t *data, uint16_t value)
{
	data[0] = (uint8_t)value;
	data[1] = (uint8_t)(value >> 8);
}

static void Logger_WriteU32(uint8_t *data, uint32_t value)
{
	data[0] = (uint8_t)value;
	data[1] = (uint8_t)(value >> 8);
	data[2] = (uint8_t)(value >> 16);
	data[3] = (uint8_t)(value >> 24);
}

static uint16_t Logger_ReadU16(const uint8_t *data)
{
	return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t Logger_ReadU32(const uint8_t *data)
{
	return (uint32_t)data[0] |
		((uint32_t)data[1] << 8) |
		((uint32_t)data[2] << 16) |
		((uint32_t)data[3] << 24);
}

static uint16_t Logger_CRC16(const uint8_t *data, uint16_t length)
{
	uint16_t crc = 0xFFFFU;
	uint16_t i;
	uint8_t bit;

	for (i = 0; i < length; i ++)
	{
		crc ^= (uint16_t)data[i] << 8;
		for (bit = 0; bit < 8U; bit ++)
		{
			if ((crc & 0x8000U) != 0U)
			{
				crc = (uint16_t)((crc << 1) ^ 0x1021U);
			}
			else
			{
				crc <<= 1;
			}
		}
	}
	return crc;
}

static void Logger_Encode(uint8_t *data, const Logger_Record_t *record)
{
	uint8_t i;
	uint16_t crc;

	for (i = 0; i < LOGGER_RECORD_SIZE; i ++)
	{
		data[i] = 0xFFU;
	}
	data[0] = 'E';
	data[1] = 'L';
	data[2] = 'O';
	data[3] = 'G';
	Logger_WriteU32(&data[4], record->sequence);
	Logger_WriteU32(&data[8], record->timestamp);
	Logger_WriteU16(&data[12], record->light_raw);
	Logger_WriteU16(&data[14], (uint16_t)record->acc_x);
	Logger_WriteU16(&data[16], (uint16_t)record->acc_y);
	Logger_WriteU16(&data[18], (uint16_t)record->acc_z);
	data[20] = record->event;
	data[21] = 0;
	crc = Logger_CRC16(data, 22U);
	Logger_WriteU16(&data[22], crc);
}

static uint8_t Logger_Decode(const uint8_t *data, Logger_Record_t *record)
{
	uint16_t stored_crc;

	if ((data[0] != 'E') || (data[1] != 'L') || (data[2] != 'O') || (data[3] != 'G'))
	{
		return 0;
	}
	stored_crc = Logger_ReadU16(&data[22]);
	if (stored_crc != Logger_CRC16(data, 22U))
	{
		return 0;
	}

	record->sequence = Logger_ReadU32(&data[4]);
	record->timestamp = Logger_ReadU32(&data[8]);
	record->light_raw = Logger_ReadU16(&data[12]);
	record->acc_x = (int16_t)Logger_ReadU16(&data[14]);
	record->acc_y = (int16_t)Logger_ReadU16(&data[16]);
	record->acc_z = (int16_t)Logger_ReadU16(&data[18]);
	record->event = data[20];
	return 1;
}

void Logger_Init(void)
{
	uint8_t data[LOGGER_RECORD_SIZE];
	Logger_Record_t record;

	Logger_Count = 0;
	Logger_NextSequence = 1;
	Logger_Ready = 0;

	W25Q64_Init();
	W25Q64_ReadID(&Logger_ManufacturerID, &Logger_DeviceID);
	if ((Logger_ManufacturerID == 0x00U) || (Logger_ManufacturerID == 0xFFU) ||
		(Logger_DeviceID != 0x4017U))
	{
		return;
	}

	Logger_Ready = 1;
	while (Logger_Count < LOGGER_CAPACITY)
	{
		W25Q64_ReadData(LOGGER_FLASH_ADDRESS + (uint32_t)Logger_Count * LOGGER_RECORD_SIZE,
						 data, LOGGER_RECORD_SIZE);
		if (Logger_Decode(data, &record) == 0U)
		{
			break;
		}
		Logger_NextSequence = record.sequence + 1UL;
		Logger_Count ++;
	}
}

uint8_t Logger_IsReady(void)
{
	return Logger_Ready;
}

uint8_t Logger_Append(uint32_t timestamp, uint16_t light_raw,
					  int16_t acc_x, int16_t acc_y, int16_t acc_z, uint8_t event)
{
	uint8_t data[LOGGER_RECORD_SIZE];
	Logger_Record_t record;
	Logger_Record_t verify;

	if ((Logger_Ready == 0U) || (Logger_Count >= LOGGER_CAPACITY))
	{
		return 0;
	}

	record.sequence = Logger_NextSequence;
	record.timestamp = timestamp;
	record.light_raw = light_raw;
	record.acc_x = acc_x;
	record.acc_y = acc_y;
	record.acc_z = acc_z;
	record.event = event;
	Logger_Encode(data, &record);
	W25Q64_PageProgram(LOGGER_FLASH_ADDRESS + (uint32_t)Logger_Count * LOGGER_RECORD_SIZE,
						data, LOGGER_RECORD_SIZE);
	W25Q64_ReadData(LOGGER_FLASH_ADDRESS + (uint32_t)Logger_Count * LOGGER_RECORD_SIZE,
					 data, LOGGER_RECORD_SIZE);
	if ((Logger_Decode(data, &verify) == 0U) || (verify.sequence != record.sequence))
	{
		return 0;
	}

	Logger_Count ++;
	Logger_NextSequence ++;
	return 1;
}

uint8_t Logger_Read(uint16_t index, Logger_Record_t *record)
{
	uint8_t data[LOGGER_RECORD_SIZE];

	if ((Logger_Ready == 0U) || (index >= Logger_Count))
	{
		return 0;
	}
	W25Q64_ReadData(LOGGER_FLASH_ADDRESS + (uint32_t)index * LOGGER_RECORD_SIZE,
					 data, LOGGER_RECORD_SIZE);
	return Logger_Decode(data, record);
}

uint8_t Logger_Format(void)
{
	uint8_t verify[4];

	if (Logger_Ready == 0U)
	{
		return 0;
	}
	W25Q64_SectorErase(LOGGER_FLASH_ADDRESS);
	W25Q64_ReadData(LOGGER_FLASH_ADDRESS, verify, sizeof(verify));
	if ((verify[0] != 0xFFU) || (verify[1] != 0xFFU) ||
		(verify[2] != 0xFFU) || (verify[3] != 0xFFU))
	{
		return 0;
	}
	Logger_Count = 0;
	Logger_NextSequence = 1;
	return 1;
}

uint16_t Logger_GetCount(void)
{
	return Logger_Count;
}

uint16_t Logger_GetCapacity(void)
{
	return LOGGER_CAPACITY;
}

uint32_t Logger_GetRemainingBytes(void)
{
	return (uint32_t)(LOGGER_CAPACITY - Logger_Count) * LOGGER_RECORD_SIZE;
}

uint8_t Logger_GetManufacturerID(void)
{
	return Logger_ManufacturerID;
}

uint16_t Logger_GetDeviceID(void)
{
	return Logger_DeviceID;
}
