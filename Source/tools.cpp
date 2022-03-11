#include "tools.h"
#include <codecvt>
#include <locale>
#include <vector>

void changeFileExt(LPWSTR lpFilename, SIZE_T size, LPCWSTR newExt)
{
	// Уберем расширение
	LPWSTR exc = wcsrchr(lpFilename, '.');
	if (exc)
		*exc = '\0';

	wcscat_s(lpFilename, size - wcslen(lpFilename), newExt);
}

void extractPath(LPWSTR fullFileName, SIZE_T size)
{
	// Уберем имя файла
	LPWSTR exc = wcsrchr(fullFileName, '\\');
	if (exc)
		*(++exc) = '\0';
}

bool isTimePart(LPCSTR part)
{
	int len = strlen(part);
	if (len < 1 || len > 2)
		return false;

	for (int i = 0; i < len; i++)
	{
		if (part[i] < '0' || part[i] > '9')
			return false;
	}

	return true;
}

char* secondsToTime(int sec, char* buff)
{
	if (sec > TIME_MAX_SECONDS)
		sec = TIME_MAX_SECONDS;

	char* b = buff;
	int h = sec / 3600;
	int m = (sec % 3600) / 60;
	int s = sec % 60;
	if (h > 0)
	{
		sprintf(b, "%02d:", h);
		b += 3;
	}
	sprintf(b, "%02d:%02d", m, s);
	return buff;
}

int timeToSecond(LPCSTR time)
{
	if (!time)
		return -1;

	int second = 0;

	char t[TIME_MAX_LEN + 1];
	int len = strlen(time);
	if (len > TIME_MAX_LEN)
		return -1;
	strcpy(t, time);

	int muls[3]{ 1, 60, 3600 };

	// BUG: время "0" будет корректным.
	for (int i = 0; i < 3; i++)
	{
		char* s = strrchr(t, ':');
		if (s)
		{
			*s = 0;
			s++;
		}
		else
			s = t;

		if (!isTimePart(s))
			return -1;

		second += (atoi(s) * muls[i]);
		if (s == t)
			break;
	}

	return second;
}

char* removeSpaces(char* str)
{
	int len = strlen(str);
	// Не самый оптимальный алгоритм, но сойдет.
	int j = 0;
	for (int i = 0; i < len; i++)
	{
		if (' ' == str[i] || '\t' == str[i])
			continue;

		str[j++] = str[i];
	}
	str[j] = 0;
	return str;
}

char* strTrim(char* str)
{
	if (!str)
		return NULL;

	// Trim
	char* s = str;
	while (' ' == *s || '\t' == *s)
		s++;
	if (s != str)
	{
		memcpy(str, s, strlen(s) + 1);
	}

	int len = strlen(str);
	while (' ' == str[len - 1] || '\t' == str[len - 1])
		len--;
	str[len] = 0;

	return str;
}

char* createTrimString(LPCSTR str)
{
	if (!str)
		return NULL;

	// Trim
	while (' ' == *str || '\t' == *str)
		str++;
	int len = strlen(str);
	while (' ' == str[len - 1] || '\t' == str[len - 1])
		len--;

	char* newStr = (char*)malloc(len + 1);
	memcpy(newStr, str, len);
	newStr[len] = 0;
	return newStr;
}

#define KEYS_SPECIAL_COUNT 22
const char KEYS_SPECIAL_STR[KEYS_SPECIAL_COUNT + 1] = "`~-_=+\\|[{]};:'\",<.>/?";
const byte KEYS_SPECIAL[KEYS_SPECIAL_COUNT] = { VK_OEM_3, VK_OEM_3, VK_OEM_MINUS , VK_OEM_MINUS, VK_OEM_PLUS, VK_OEM_PLUS, VK_OEM_5, VK_OEM_5, VK_OEM_4, VK_OEM_4, VK_OEM_6, VK_OEM_6, VK_OEM_1, VK_OEM_1, VK_OEM_7, VK_OEM_7, VK_OEM_COMMA, VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_PERIOD, VK_OEM_2, VK_OEM_2 };
#define KEYS_COUNT 42
const char* KEYS_STR[KEYS_COUNT] = {
	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
	"SPACE", "ENTER", "DELETE", "INSERT", "HOME", "END", "PAGEUP", "PAGEDOWN", "LEFT", "RIGHT", "UP", "DOWN", "ESC", "TAB", "BACK",
	"NUM0", "NUM1", "NUM2", "NUM3", "NUM4", "NUM5", "NUM6", "NUM7", "NUM8", "NUM9",
	"NUM/", "NUM*", "NUM-", "NUMPLUS", "NUM."
};
const byte KEYS[KEYS_COUNT] = {
	VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12,
	VK_SPACE, VK_RETURN, VK_DELETE, VK_INSERT, VK_HOME, VK_END, VK_PRIOR, VK_NEXT, VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_ESCAPE, VK_TAB, VK_BACK,
	VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9,
	VK_DIVIDE, VK_MULTIPLY, VK_SUBTRACT, VK_ADD, VK_DECIMAL
};

#define KEYS_MOD_COUNT 3
const char* KEYS_MOD_STR[KEYS_MOD_COUNT] = {
	"ALT", "CTRL", "SHIFT"
};
const byte KEYS_MOD[KEYS_MOD_COUNT] = {
	MOD_ALT, MOD_CONTROL, MOD_SHIFT
};

byte getKey(LPCSTR text, const char** presentations, const byte* values, int size)
{
	for (int i = 0; i < size; i++)
	{
		if (!strcmp(presentations[i], text))
			return values[i];
	}

	return 0;
}

bool strToKey(LPCSTR text, UINT* virtKey, UINT* keyMod)
{
	*virtKey = 0;
	*keyMod = 0;
	int len = strlen(text);
	if (!len)
		return false;

	if (1 == len)
	{
		char ch = text[0];
		// Вначале проверим специальные символы
		const char* k = strchr(KEYS_SPECIAL_STR, ch);
		if (k)
		{
			*virtKey = KEYS_SPECIAL[k - KEYS_SPECIAL_STR];
			return true;
		}

		// BUG: ASCII-символы, не все из них будут корректны.
		if (ch >= ' ' && ch <= '~')
		{
			*virtKey = ch;
			return true;
		}

		return false;
	}

	byte k = getKey(text, KEYS_STR, KEYS, KEYS_COUNT);
	if (k)
	{
		*virtKey = k;
		return true;
	}
	byte m = getKey(text, KEYS_MOD_STR, KEYS_MOD, KEYS_MOD_COUNT);
	if (m)
	{
		*keyMod = m;
		return true;
	}

	return false;
}

LPWSTR strToWStr(LPCSTR pInStr)
{
	int length = strlen(pInStr);
	wchar_t * pwstr = new wchar_t[length + 1];
	int result = MultiByteToWideChar(
		CP_ACP, MB_PRECOMPOSED, pInStr, length,
		pwstr, length
	);
	pwstr[length] = L'\0';
	return pwstr;
}

void writeJmpAddress(byte* addr, void* dest)
{
	*(int*)(addr + 1) = (byte*)dest - (addr + 5);
}

// Создает переходник с виндового callback на метод класса.
void* makeTrampoline(void* objThis, void* meth)
{
	byte CODE[] = {
		0xB9, 0x00, 0x00, 0x00, 0x00, // mov ecx, DWORD
		0xE9, 0x00, 0x00, 0x00, 0x00, // jmp DWORD:func
	};

	byte* mem = (byte*)VirtualAlloc(NULL, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (!mem)
		return NULL;

	memcpy(mem, CODE, sizeof(CODE));
	*(void**)(mem + 1) = objThis;
	writeJmpAddress(mem + 5, meth);
	return mem;
}

void freeTrompoline(void** trompoline)
{
	if (!trompoline || *trompoline)
		return;

	VirtualFree(*trompoline, 0, MEM_RELEASE);
	*trompoline = NULL;
}

LPWSTR includeTrailingPathDelimiter(LPWSTR str)
{
	int len = wcslen(str);
	if (str[len - 1] != '\\')
	{
		str[len++] = '\\';
		str[len] = 0;
	}
	return str;
}
LPSTR includeTrailingPathDelimiter(LPSTR str)
{
	int len = strlen(str);
	if (str[len - 1] != '\\')
	{
		str[len++] = '\\';
		str[len] = 0;
	}
	return str;
}

namespace std
{
	#define TEXT_MIN_LEN 32
	#define TEXT_MAX_LEN 1024

	string format(LPCSTR format, ...)
	{

		int res;
		int size = TEXT_MIN_LEN;
		char* text = NULL;
		va_list args;
		va_start(args, format);

		do
		{
			text = (char*)malloc(size);
			res = _vsprintf_p(text, size, format, args);
			if (res < 0)
			{
				free(text);
				text = NULL;
				size *= 2;
			}
		} while (res < 0 && size < TEXT_MAX_LEN);

		va_end(args);

		string result(text ? text : "NULL");
		if (text)
			free(text);
		return result;
	}
}
