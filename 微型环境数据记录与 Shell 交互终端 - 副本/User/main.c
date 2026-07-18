#include "stm32f10x.h"
#include "SystemTime.h"
#include "Watchdog.h"
#include "Serial.h"
#include "OLED.h"
#include "LED.h"
#include "AD.h"
#include "MPU6050.h"
#include "MyRTC.h"
#include "Logger.h"
#include "Shell.h"

#define SHELL_PERIOD_MS             50UL
#define SENSOR_PERIOD_MS            100UL
#define DISPLAY_PERIOD_MS           500UL
#define SLEEP_TIMEOUT_MS            (5UL * 60UL * 1000UL)
#define ALARM_RECOVERY_MS           2000UL

/* MPU6050 uses the +/-16 g range: 2048 LSB/g. */
#define TILT_THRESHOLD_RAW          700U
#define SHOCK_THRESHOLD_RAW         5120U

static int16_t App_AccX;
static int16_t App_AccY;
static int16_t App_AccZ;
static int16_t App_GyroX;
static int16_t App_GyroY;
static int16_t App_GyroZ;
static uint8_t App_MPUReady;
static uint8_t App_AlarmActive;
static uint8_t App_Sleeping;
static uint32_t App_NormalSince;

static uint16_t App_AbsI16(int16_t value)
{
	int32_t wide_value = value;
	if (wide_value < 0)
	{
		wide_value = -wide_value;
	}
	return (uint16_t)wide_value;
}

static uint32_t App_LightPercent(void)
{
	return (4095UL - (uint32_t)AD_Value) * 100UL / 4095UL;
}

static void App_PrintCurrentTime(void)
{
	MyRTC_ReadTime();
	Serial_SendNumber(MyRTC_Time[0], 4);
	Serial_SendByte('-');
	Serial_SendNumber(MyRTC_Time[1], 2);
	Serial_SendByte('-');
	Serial_SendNumber(MyRTC_Time[2], 2);
	Serial_SendByte(' ');
	Serial_SendNumber(MyRTC_Time[3], 2);
	Serial_SendByte(':');
	Serial_SendNumber(MyRTC_Time[4], 2);
	Serial_SendByte(':');
	Serial_SendNumber(MyRTC_Time[5], 2);
}

static void App_ReportAlarm(uint8_t event, uint8_t stored)
{
	Serial_SendString("[WARN] ");
	App_PrintCurrentTime();
	Serial_SendString(" - ");
	if ((event & (LOGGER_EVENT_SHOCK | LOGGER_EVENT_TILT)) ==
		(LOGGER_EVENT_SHOCK | LOGGER_EVENT_TILT))
	{
		Serial_SendString("Shock and Tilt Detected");
	}
	else if ((event & LOGGER_EVENT_SHOCK) != 0U)
	{
		Serial_SendString("Shock Detected");
	}
	else
	{
		Serial_SendString("Tilt Detected");
	}
	Serial_SendString(", Light: ");
	Serial_SendNumber(App_LightPercent(), 3);
	Serial_SendString("%\r\n");
	if (stored == 0U)
	{
		Serial_SendString("[Error] Alarm log was not stored (Flash absent or full).\r\n");
	}
}

static void App_UpdateMotion(uint32_t now)
{
	uint8_t event = 0;
	uint8_t stored;

	if (App_MPUReady == 0U)
	{
		return;
	}

	MPU6050_GetData(&App_AccX, &App_AccY, &App_AccZ,
					  &App_GyroX, &App_GyroY, &App_GyroZ);
	if ((App_AbsI16(App_AccX) >= SHOCK_THRESHOLD_RAW) ||
		(App_AbsI16(App_AccY) >= SHOCK_THRESHOLD_RAW) ||
		(App_AbsI16(App_AccZ) >= SHOCK_THRESHOLD_RAW))
	{
		event |= LOGGER_EVENT_SHOCK;
	}
	if ((App_AbsI16(App_AccX) >= TILT_THRESHOLD_RAW) ||
		(App_AbsI16(App_AccY) >= TILT_THRESHOLD_RAW))
	{
		event |= LOGGER_EVENT_TILT;
	}

	if (event != 0U)
	{
		App_NormalSince = 0;
		if (App_AlarmActive == 0U)
		{
			App_AlarmActive = 1;
			LED1_ON();
			stored = Logger_Append(MyRTC_GetTimestamp(), AD_Value,
								   App_AccX, App_AccY, App_AccZ, event);
			App_ReportAlarm(event, stored);
		}
	}
	else if (App_AlarmActive != 0U)
	{
		if (App_NormalSince == 0UL)
		{
			App_NormalSince = now;
		}
		else if ((uint32_t)(now - App_NormalSince) >= ALARM_RECOVERY_MS)
		{
			App_AlarmActive = 0;
			App_NormalSince = 0;
			LED1_OFF();
		}
	}
}

static void App_UpdateDisplay(void)
{
	uint32_t remaining = Logger_GetRemainingBytes();

	MyRTC_ReadTime();
	OLED_ShowString(1, 1, "TIME 00:00:00   ");
	OLED_ShowNum(1, 6, MyRTC_Time[3], 2);
	OLED_ShowNum(1, 9, MyRTC_Time[4], 2);
	OLED_ShowNum(1, 12, MyRTC_Time[5], 2);

	OLED_ShowString(2, 1, "LIGHT 0000 000% ");
	OLED_ShowNum(2, 7, AD_Value, 4);
	OLED_ShowNum(2, 12, App_LightPercent(), 3);

	if (App_Sleeping != 0U)
	{
		OLED_ShowString(3, 1, "STATE SLEEP     ");
	}
	else if (App_AlarmActive != 0U)
	{
		OLED_ShowString(3, 1, "STATE WARNING   ");
	}
	else if (App_MPUReady == 0U)
	{
		OLED_ShowString(3, 1, "STATE MPU ERROR ");
	}
	else
	{
		OLED_ShowString(3, 1, "STATE ACTIVE    ");
	}

	OLED_ShowString(4, 1, "LOG 000 F0000   ");
	OLED_ShowNum(4, 5, Logger_GetCount(), 3);
	if (remaining > 9999UL)
	{
		remaining = 9999UL;
	}
	OLED_ShowNum(4, 10, remaining, 4);
}

int main(void)
{
	uint32_t last_shell;
	uint32_t last_sensor;
	uint32_t last_display;
	uint32_t last_serial_activity;
	uint32_t now;
	uint8_t mpu_retry;
	uint8_t mpu_id = 0xFFU;

	SystemTime_Init();
	Serial_Init();
	LED_Init();
	OLED_Init();
	AD_Init();
	RTC_Init();
	MPU6050_Init();
	App_MPUReady = 0;
	for (mpu_retry = 0; mpu_retry < 5U; mpu_retry ++)
	{
		mpu_id = MPU6050_GetID();
		if (mpu_id == 0x70U)
		{
			App_MPUReady = 1;
			break;
		}
	}
	Logger_Init();

	App_AlarmActive = 0;
	App_Sleeping = 0;
	App_NormalSince = 0;
	App_UpdateDisplay();

	Watchdog_Init();
	Shell_Init();
	if (App_MPUReady == 0U)
	{
		Serial_Printf("[Error] MPU6050 not detected, WHO_AM_I=0x%02X.\r\n> ", mpu_id);
	}
	else
	{
		Serial_Printf("[System] MPU6050 detected, WHO_AM_I=0x%02X.\r\n> ", mpu_id);
	}
	if (Logger_IsReady() == 0U)
	{
		Serial_SendString("[Error] W25Q64 not detected (expected JEDEC EF4017).\r\n> ");
	}

	now = SystemTime_GetMillis();
	last_shell = now;
	last_sensor = now;
	last_display = now;
	last_serial_activity = now;

	while (1)
	{
		now = SystemTime_GetMillis();
		if (Serial_ConsumeActivity() != 0U)
		{
			last_serial_activity = now;
			if (App_Sleeping != 0U)
			{
				App_Sleeping = 0;
				Serial_SendString("[System] Wakeup by UART.\r\n> ");
			}
		}

		if (App_Sleeping != 0U)
		{
			__WFI();
		}
		else
		{
			if (SystemTime_IsDue(&last_shell, SHELL_PERIOD_MS) != 0U)
			{
				Shell_Poll();
			}
			if (SystemTime_IsDue(&last_sensor, SENSOR_PERIOD_MS) != 0U)
			{
				App_UpdateMotion(now);
			}
			if (SystemTime_IsDue(&last_display, DISPLAY_PERIOD_MS) != 0U)
			{
				App_UpdateDisplay();
			}

			if ((App_AlarmActive == 0U) &&
				((uint32_t)(now - last_serial_activity) >= SLEEP_TIMEOUT_MS))
			{
				Serial_SendString("[System] Entering Sleep Mode...\r\n");
				App_Sleeping = 1;
				App_UpdateDisplay();
			}
		}

		Watchdog_Feed();
	}
}
