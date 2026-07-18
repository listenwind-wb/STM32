#include "Shell.h"
#include "Serial.h"
#include "SystemTime.h"
#include "MyRTC.h"
#include "Logger.h"
#include "Watchdog.h"
#include <string.h>

static void Shell_SendUInt(uint32_t value)
{
	char buffer[10];
	uint8_t length = 0;

	do
	{
		buffer[length ++] = (char)('0' + value % 10UL);
		value /= 10UL;
	} while ((value != 0UL) && (length < sizeof(buffer)));

	while (length != 0U)
	{
		Serial_SendByte((uint8_t)buffer[-- length]);
	}
}

static void Shell_SendInt(int16_t value)
{
	int32_t signed_value = value;
	if (signed_value < 0)
	{
		Serial_SendByte('-');
		signed_value = -signed_value;
	}
	Shell_SendUInt((uint32_t)signed_value);
}

static void Shell_SendHex(uint32_t value, uint8_t length)
{
	static const char hex[] = "0123456789ABCDEF";
	uint8_t shift;

	while (length != 0U)
	{
		length --;
		shift = (uint8_t)(length * 4U);
		Serial_SendByte((uint8_t)hex[(value >> shift) & 0x0FU]);
	}
}

static void Shell_PrintTime(const uint16_t time_value[6])
{
	Serial_SendNumber(time_value[0], 4);
	Serial_SendByte('-');
	Serial_SendNumber(time_value[1], 2);
	Serial_SendByte('-');
	Serial_SendNumber(time_value[2], 2);
	Serial_SendByte(' ');
	Serial_SendNumber(time_value[3], 2);
	Serial_SendByte(':');
	Serial_SendNumber(time_value[4], 2);
	Serial_SendByte(':');
	Serial_SendNumber(time_value[5], 2);
}

static uint8_t Shell_ParseUInt(const char *text, uint16_t *value)
{
	uint32_t result = 0;
	uint8_t digits = 0;

	while ((*text >= '0') && (*text <= '9'))
	{
		result = result * 10UL + (uint32_t)(*text - '0');
		if (result > 65535UL)
		{
			return 0;
		}
		text ++;
		digits ++;
	}
	if ((digits == 0U) || (*text != '\0'))
	{
		return 0;
	}
	*value = (uint16_t)result;
	return 1;
}

static uint8_t Shell_ParseTime(const char *text, uint8_t *hour, uint8_t *minute, uint8_t *second)
{
	uint8_t bracket = 0;

	if (*text == '[')
	{
		bracket = 1;
		text ++;
	}
	if ((text[0] < '0') || (text[0] > '9') ||
		(text[1] < '0') || (text[1] > '9') || (text[2] != ':') ||
		(text[3] < '0') || (text[3] > '9') ||
		(text[4] < '0') || (text[4] > '9') || (text[5] != ':') ||
		(text[6] < '0') || (text[6] > '9') ||
		(text[7] < '0') || (text[7] > '9'))
	{
		return 0;
	}
	if ((bracket != 0U && ((text[8] != ']') || (text[9] != '\0'))) ||
		(bracket == 0U && text[8] != '\0'))
	{
		return 0;
	}

	*hour = (uint8_t)((text[0] - '0') * 10 + text[1] - '0');
	*minute = (uint8_t)((text[3] - '0') * 10 + text[4] - '0');
	*second = (uint8_t)((text[6] - '0') * 10 + text[7] - '0');
	return ((*hour < 24U) && (*minute < 60U) && (*second < 60U)) ? 1U : 0U;
}

static void Shell_CommandSysInfo(void)
{
	Serial_SendString("uptime_ms: ");
	Shell_SendUInt(SystemTime_GetMillis());
	Serial_SendString("\r\nflash: ");
	Serial_SendString(Logger_IsReady() != 0U ? "READY" : "ERROR");
	Serial_SendString(" (ID 0x");
	Shell_SendHex(Logger_GetManufacturerID(), 2);
	Shell_SendHex(Logger_GetDeviceID(), 4);
	Serial_SendString(")\r\nlog_count: ");
	Shell_SendUInt(Logger_GetCount());
	Serial_SendByte('/');
	Shell_SendUInt(Logger_GetCapacity());
	Serial_SendString("\r\nflash_remaining: ");
	Shell_SendUInt(Logger_GetRemainingBytes());
	Serial_SendString(" bytes\r\nwatchdog: ON, last_reset: ");
	Serial_SendString(Watchdog_WasReset() != 0U ? "IWDG" : "OTHER");
	Serial_SendString("\r\n");
}

static void Shell_CommandGetRTC(void)
{
	MyRTC_ReadTime();
	Shell_PrintTime(MyRTC_Time);
	Serial_SendString("\r\n");
}

static void Shell_PrintRecord(const Logger_Record_t *record)
{
	uint16_t time_value[6];
	uint32_t light_percent;

	MyRTC_TimestampToTime(record->timestamp, time_value);
	Serial_SendByte('#');
	Shell_SendUInt(record->sequence);
	Serial_SendByte(' ');
	Shell_PrintTime(time_value);
	Serial_SendString(" [");
	if ((record->event & (LOGGER_EVENT_SHOCK | LOGGER_EVENT_TILT)) ==
		(LOGGER_EVENT_SHOCK | LOGGER_EVENT_TILT))
	{
		Serial_SendString("SHOCK+TILT");
	}
	else if ((record->event & LOGGER_EVENT_SHOCK) != 0U)
	{
		Serial_SendString("SHOCK");
	}
	else
	{
		Serial_SendString("TILT");
	}
	light_percent = (4095UL - (uint32_t)record->light_raw) * 100UL / 4095UL;
	Serial_SendString("] light=");
	Shell_SendUInt(record->light_raw);
	Serial_SendByte('(');
	Shell_SendUInt(light_percent);
	Serial_SendString("%) acc=");
	Shell_SendInt(record->acc_x);
	Serial_SendByte(',');
	Shell_SendInt(record->acc_y);
	Serial_SendByte(',');
	Shell_SendInt(record->acc_z);
	Serial_SendString("\r\n");
}

static void Shell_CommandReadLog(uint16_t requested)
{
	uint16_t count = Logger_GetCount();
	uint16_t start;
	uint16_t index;
	Logger_Record_t record;

	if (Logger_IsReady() == 0U)
	{
		Serial_SendString("[Error] W25Q64 not detected.\r\n");
		return;
	}
	if (count == 0U)
	{
		Serial_SendString("[Info] No logs.\r\n");
		return;
	}
	if (requested > count)
	{
		requested = count;
	}
	start = count - requested;
	for (index = start; index < count; index ++)
	{
		if (Logger_Read(index, &record) != 0U)
		{
			Shell_PrintRecord(&record);
		}
	}
}

static void Shell_ProcessLine(const char *line)
{
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint16_t count;

	if (strcmp(line, "sys_info") == 0)
	{
		Shell_CommandSysInfo();
	}
	else if (strcmp(line, "get_rtc") == 0)
	{
		Shell_CommandGetRTC();
	}
	else if (strncmp(line, "set_rtc -t ", 11) == 0)
	{
		if ((Shell_ParseTime(line + 11, &hour, &minute, &second) != 0U) &&
			(MyRTC_SetHMS(hour, minute, second) != 0U))
		{
			Serial_SendString("[Success] RTC Updated.\r\n");
		}
		else
		{
			Serial_SendString("[Error] Usage: set_rtc -t hh:mm:ss\r\n");
		}
	}
	else if (strncmp(line, "read_log -n ", 12) == 0)
	{
		if ((Shell_ParseUInt(line + 12, &count) != 0U) && (count != 0U))
		{
			Shell_CommandReadLog(count);
		}
		else
		{
			Serial_SendString("[Error] Usage: read_log -n <count>\r\n");
		}
	}
	else if (strcmp(line, "format_flash") == 0)
	{
		if (Logger_Format() != 0U)
		{
			Serial_SendString("[Warn] All logs cleared.\r\n");
		}
		else
		{
			Serial_SendString("[Error] Flash erase failed.\r\n");
		}
	}
	else if (strcmp(line, "help") == 0)
	{
		Serial_SendString("sys_info | get_rtc | set_rtc -t hh:mm:ss\r\n");
		Serial_SendString("read_log -n <count> | format_flash\r\n");
	}
	else if (*line != '\0')
	{
		Serial_SendString("[Error] Unknown command. Type help.\r\n");
	}
}

void Shell_Init(void)
{
	Serial_SendString("\r\nMicro Environment Logger Shell\r\n");
	Serial_SendString("Type help, then press Send.\r\n> ");
}

void Shell_Poll(void)
{
	char line[SERIAL_RX_PACKET_SIZE];

	if (Serial_GetOverflowFlag() != 0U)
	{
		Serial_SendString("[Error] Command too long.\r\n> ");
	}
	if (Serial_GetLine(line, sizeof(line)) != 0U)
	{
		Shell_ProcessLine(line);
		Serial_SendString("> ");
	}
}
