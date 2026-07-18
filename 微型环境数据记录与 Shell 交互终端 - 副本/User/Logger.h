#ifndef __LOGGER_H
#define __LOGGER_H

#include "stm32f10x.h"

#define LOGGER_EVENT_TILT   0x01U
#define LOGGER_EVENT_SHOCK  0x02U

typedef struct
{
	uint32_t sequence;
	uint32_t timestamp;
	uint16_t light_raw;
	int16_t acc_x;
	int16_t acc_y;
	int16_t acc_z;
	uint8_t event;
} Logger_Record_t;

void Logger_Init(void);
uint8_t Logger_IsReady(void);
uint8_t Logger_Append(uint32_t timestamp, uint16_t light_raw,
					  int16_t acc_x, int16_t acc_y, int16_t acc_z, uint8_t event);
uint8_t Logger_Read(uint16_t index, Logger_Record_t *record);
uint8_t Logger_Format(void);
uint16_t Logger_GetCount(void);
uint16_t Logger_GetCapacity(void);
uint32_t Logger_GetRemainingBytes(void);
uint8_t Logger_GetManufacturerID(void);
uint16_t Logger_GetDeviceID(void);

#endif
