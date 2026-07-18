#include "Serial.h"
#include <stdarg.h>
#include <stdio.h>

char Serial_RxPacket[SERIAL_RX_PACKET_SIZE];
volatile uint8_t Serial_RxFlag;

static volatile uint8_t Serial_RxActivity;
static volatile uint8_t Serial_RxOverflow;

void Serial_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1, &USART_InitStructure);

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);

	Serial_RxFlag = 0;
	Serial_RxActivity = 0;
	Serial_RxOverflow = 0;
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_Cmd(USART1, ENABLE);
}

void Serial_SendByte(uint8_t byte)
{
	USART_SendData(USART1, byte);
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void Serial_SendArray(const uint8_t *array, uint16_t length)
{
	uint16_t i;
	for (i = 0; i < length; i ++)
	{
		Serial_SendByte(array[i]);
	}
}

void Serial_SendString(const char *string)
{
	while (*string != '\0')
	{
		Serial_SendByte((uint8_t)*string);
		string ++;
	}
}

static uint32_t Serial_Pow(uint32_t x, uint32_t y)
{
	uint32_t result = 1;
	while (y --)
	{
		result *= x;
	}
	return result;
}

void Serial_SendNumber(uint32_t number, uint8_t length)
{
	uint8_t i;
	for (i = 0; i < length; i ++)
	{
		Serial_SendByte((uint8_t)(number / Serial_Pow(10U, length - i - 1U) % 10U + '0'));
	}
}

int fputc(int ch, FILE *f)
{
	(void)f;
	Serial_SendByte((uint8_t)ch);
	return ch;
}

void Serial_Printf(const char *format, ...)
{
	char string[128];
	va_list arg;

	va_start(arg, format);
	vsnprintf(string, sizeof(string), format, arg);
	va_end(arg);
	string[sizeof(string) - 1U] = '\0';
	Serial_SendString(string);
}

uint8_t Serial_GetLine(char *line, uint16_t size)
{
	uint16_t i;

	if ((Serial_RxFlag == 0U) || (size == 0U))
	{
		return 0;
	}

	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);
	for (i = 0; (i + 1U < size) && (Serial_RxPacket[i] != '\0'); i ++)
	{
		line[i] = Serial_RxPacket[i];
	}
	line[i] = '\0';
	Serial_RxFlag = 0;
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	return 1;
}

uint8_t Serial_GetRxFlag(void)
{
	if (Serial_RxFlag != 0U)
	{
		Serial_RxFlag = 0;
		return 1;
	}
	return 0;
}

uint8_t Serial_ConsumeActivity(void)
{
	uint8_t active = Serial_RxActivity;
	Serial_RxActivity = 0;
	return active;
}

uint8_t Serial_GetOverflowFlag(void)
{
	uint8_t overflow = Serial_RxOverflow;
	Serial_RxOverflow = 0;
	return overflow;
}

void USART1_IRQHandler(void)
{
	static uint16_t index = 0;
	uint8_t data;

	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		data = (uint8_t)USART_ReceiveData(USART1);
		Serial_RxActivity = 1;

		if (Serial_RxFlag == 0U)
		{
			if ((data == '\r') || (data == '\n'))
			{
				if (index != 0U)
				{
					Serial_RxPacket[index] = '\0';
					Serial_RxFlag = 1;
					index = 0;
				}
			}
			else
			{
				if (index < SERIAL_RX_PACKET_SIZE - 1U)
				{
					Serial_RxPacket[index ++] = (char)data;
				}
				else
				{
					index = 0;
					Serial_RxOverflow = 1;
				}
			}
		}

		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}
}
