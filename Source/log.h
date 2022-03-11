#pragma once
#include "common.h"
#include "lang.h"
#include <windows.h>

//#define log_printf(...) { lg::print(__VA_ARGS__); } 
#define log_printidf(...) { lg::printid(__VA_ARGS__); } 
#define log_dbg_printf(...) {  } 
//#define log_printf(...) { } 

namespace lg
{
	int print(LPCSTR format, ...);
	int printid(lng::StringId id, ...);
	void clear();
	int getCount();
	LPCSTR getLog(int index);
	char* getLog(int start, int end);

#ifdef CHECK_MEMLEAK
	void finalization();
#endif
}
