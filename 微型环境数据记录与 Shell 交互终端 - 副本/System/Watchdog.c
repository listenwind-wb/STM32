#include "Watchdog.h"

static uint8_t Watchdog_ResetFlag;

void Watchdog_Init(void)
{
	Watchdog_ResetFlag = (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET) ? 1U : 0U;
	RCC_ClearFlag();

	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	IWDG_SetPrescaler(IWDG_Prescaler_64);
	IWDG_SetReload(2500U);
	IWDG_ReloadCounter();
	IWDG_Enable();
}

void Watchdog_Feed(void)
{
	IWDG_ReloadCounter();
}

uint8_t Watchdog_WasReset(void)
{
	return Watchdog_ResetFlag;
}
