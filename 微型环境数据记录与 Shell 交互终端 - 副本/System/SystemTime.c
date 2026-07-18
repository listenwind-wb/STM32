#include "SystemTime.h"

static volatile uint32_t SystemTime_Millis;

void SystemTime_Init(void)
{
	SystemCoreClockUpdate();
	SystemTime_Millis = 0;
	SysTick_Config(SystemCoreClock / 1000U);
	NVIC_SetPriority(SysTick_IRQn, 3);
}

void SystemTime_TickISR(void)
{
	SystemTime_Millis ++;
}

uint32_t SystemTime_GetMillis(void)
{
	return SystemTime_Millis;
}

uint8_t SystemTime_IsDue(uint32_t *last_time, uint32_t period_ms)
{
	uint32_t now = SystemTime_GetMillis();

	if ((uint32_t)(now - *last_time) < period_ms)
	{
		return 0;
	}

	*last_time += period_ms;
	if ((uint32_t)(now - *last_time) >= period_ms)
	{
		*last_time = now;
	}
	return 1;
}
