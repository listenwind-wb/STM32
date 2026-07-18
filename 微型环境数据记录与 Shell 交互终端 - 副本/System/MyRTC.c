#include "MyRTC.h"

#define MYRTC_BACKUP_MARKER  0xA5A6U
#define MYRTC_TIMEZONE_SEC   (8UL * 60UL * 60UL)

uint16_t MyRTC_Time[6] = {2026, 6, 11, 14, 59, 0};

static uint8_t MyRTC_IsLeapYear(uint16_t year)
{
	if ((year % 400U) == 0U)
	{
		return 1;
	}
	if ((year % 100U) == 0U)
	{
		return 0;
	}
	return ((year % 4U) == 0U) ? 1U : 0U;
}

static uint8_t MyRTC_DaysInMonth(uint16_t year, uint8_t month)
{
	static const uint8_t days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if ((month == 2U) && (MyRTC_IsLeapYear(year) != 0U))
	{
		return 29;
	}
	return days[month - 1U];
}

static uint32_t MyRTC_CalendarToTimestamp(const uint16_t *calendar)
{
	uint32_t days = 0;
	uint16_t year;
	uint8_t month;
	uint32_t local_seconds;

	for (year = 1970; year < calendar[0]; year ++)
	{
		days += (MyRTC_IsLeapYear(year) != 0U) ? 366U : 365U;
	}
	for (month = 1; month < calendar[1]; month ++)
	{
		days += MyRTC_DaysInMonth(calendar[0], month);
	}
	days += calendar[2] - 1U;

	local_seconds = days * 86400UL +
		(uint32_t)calendar[3] * 3600UL +
		(uint32_t)calendar[4] * 60UL + calendar[5];
	return local_seconds - MYRTC_TIMEZONE_SEC;
}

static void MyRTC_TimestampToCalendar(uint32_t timestamp, uint16_t *calendar)
{
	uint32_t local_seconds = timestamp + MYRTC_TIMEZONE_SEC;
	uint32_t days = local_seconds / 86400UL;
	uint32_t seconds_of_day = local_seconds % 86400UL;
	uint16_t year = 1970;
	uint8_t month = 1;
	uint16_t year_days;
	uint8_t month_days;

	for (;;)
	{
		year_days = (MyRTC_IsLeapYear(year) != 0U) ? 366U : 365U;
		if (days < year_days)
		{
			break;
		}
		days -= year_days;
		year ++;
	}

	for (;;)
	{
		month_days = MyRTC_DaysInMonth(year, month);
		if (days < month_days)
		{
			break;
		}
		days -= month_days;
		month ++;
	}

	calendar[0] = year;
	calendar[1] = month;
	calendar[2] = (uint16_t)days + 1U;
	calendar[3] = (uint16_t)(seconds_of_day / 3600UL);
	calendar[4] = (uint16_t)((seconds_of_day % 3600UL) / 60UL);
	calendar[5] = (uint16_t)(seconds_of_day % 60UL);
}

void RTC_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	PWR_BackupAccessCmd(ENABLE);

	RCC_LSICmd(ENABLE);
	while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);

	if (BKP_ReadBackupRegister(BKP_DR1) != MYRTC_BACKUP_MARKER)
	{
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
		RCC_RTCCLKCmd(ENABLE);
		RTC_WaitForSynchro();
		RTC_WaitForLastTask();
		RTC_SetPrescaler(40000UL - 1UL);
		RTC_WaitForLastTask();
		MyRTC_SetTime();
		BKP_WriteBackupRegister(BKP_DR1, MYRTC_BACKUP_MARKER);
		RTC_WaitForLastTask();
	}
	else
	{
		RCC_RTCCLKCmd(ENABLE);
		RTC_WaitForSynchro();
		RTC_WaitForLastTask();
	}

	MyRTC_ReadTime();
}

void MyRTC_SetTime(void)
{
	RTC_SetCounter(MyRTC_CalendarToTimestamp(MyRTC_Time));
	RTC_WaitForLastTask();
}

void MyRTC_ReadTime(void)
{
	MyRTC_TimestampToCalendar(RTC_GetCounter(), MyRTC_Time);
}

uint8_t MyRTC_SetHMS(uint8_t hour, uint8_t minute, uint8_t second)
{
	if ((hour > 23U) || (minute > 59U) || (second > 59U))
	{
		return 0;
	}

	MyRTC_ReadTime();
	MyRTC_Time[3] = hour;
	MyRTC_Time[4] = minute;
	MyRTC_Time[5] = second;
	MyRTC_SetTime();
	return 1;
}

uint32_t MyRTC_GetTimestamp(void)
{
	return RTC_GetCounter();
}

void MyRTC_TimestampToTime(uint32_t timestamp, uint16_t time_value[6])
{
	MyRTC_TimestampToCalendar(timestamp, time_value);
}
