#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"
#include <stdio.h>

#define SERIAL_RX_PACKET_SIZE  96U

extern char Serial_RxPacket[SERIAL_RX_PACKET_SIZE];
extern volatile uint8_t Serial_RxFlag;

void Serial_Init(void);
void Serial_SendByte(uint8_t byte);
void Serial_SendArray(const uint8_t *array, uint16_t length);
void Serial_SendString(const char *string);
void Serial_SendNumber(uint32_t number, uint8_t length);
void Serial_Printf(const char *format, ...);

uint8_t Serial_GetLine(char *line, uint16_t size);
uint8_t Serial_GetRxFlag(void);
uint8_t Serial_ConsumeActivity(void);
uint8_t Serial_GetOverflowFlag(void);

#endif
