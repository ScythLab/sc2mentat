#include "log.h"
#include "tools.h"
#include <vector>
#include <time.h>

namespace lg
{
	#define LOG_MIN_LEN 128
	#define LOG_MAX_LEN 1024

	std::vector<char*> g_logs;

	void clear()
	{
		SafeVectorDelete(g_logs);
	}

	//void invalid_parameter(
	//	const wchar_t * expression,
	//	const wchar_t * function,
	//	const wchar_t * file,
	//	unsigned int line,
	//	uintptr_t pReserved
	//)
	//{
	//	log_printf("%S", expression);
	//}

	////_invoke_watson
	//_set_invalid_parameter_handler(invalid_parameter);

	int print(LPCSTR format, va_list args)
	{
		int res;
		int size = LOG_MIN_LEN;
		char* text = NULL;

		char time[32] = { '[' };
		strcat(_strtime(time + 1), "]   ");
		int timeLen = strlen(time);

		do
		{
			text = (char*)malloc(size);
			res = _vsprintf_p(text + timeLen, size - timeLen, format, args);
			if (res < 0)
			{
				free(text);
				text = NULL;
				size *= 2;
			}
		} while (res < 0 && size < LOG_MAX_LEN);

		if (!text)
			return 0;

		memcpy(text, time, timeLen);

		g_logs.push_back(text);
		return strlen(text);
	}

	int printid(lng::StringId id, ...)
	{
		LPCSTR format = lng::string(id);
		if (!format)
			return 0;

		va_list args;
		va_start(args, id);
		return print(format, args);
	}

	int print(LPCSTR format, ...)
	{
		va_list args;
		va_start(args, format);
		return print(format, args);
	}

	int getCount()
	{
		return (int)g_logs.size();
	}

	LPCSTR getLog(int index)
	{
		return g_logs[index];
	}

	char* getLog(int start, int end)
	{
		int textSize = 0;
		for (int i = start; i <= end; i++)
		{
			LPCSTR text = lg::getLog(i);
			textSize += strlen(text) + 2;
		}
		if (!textSize)
			return NULL;

		char* buff = (char*)malloc(textSize + 1);
		char* lpBuff = buff;
		for (int i = start; i <= end; i++)
		{
			LPCSTR text = lg::getLog(i);
			strcpy(lpBuff, text);
			strcat(lpBuff, "\r\n");
			lpBuff += strlen(lpBuff);
		}

		return buff;
	}

#ifdef CHECK_MEMLEAK
	void finalization()
	{
		g_logs.~vector();
	}
#endif
}
