#ifndef __MY_RTC_H
#define __MY_RTC_H

#include "stm32f10x.h"

extern uint16_t MyRTC_Time[6];

void RTC_Init(void);
void MyRTC_SetTime(void);
void MyRTC_ReadTime(void);
uint8_t MyRTC_SetHMS(uint8_t hour, uint8_t minute, uint8_t second);
uint32_t MyRTC_GetTimestamp(void);
void MyRTC_TimestampToTime(uint32_t timestamp, uint16_t time_value[6]);

#endif
