#ifndef __SYSTEM_TIME_H
#define __SYSTEM_TIME_H

#include "stm32f10x.h"

void SystemTime_Init(void);
void SystemTime_TickISR(void);
uint32_t SystemTime_GetMillis(void);
uint8_t SystemTime_IsDue(uint32_t *last_time, uint32_t period_ms);

#endif
