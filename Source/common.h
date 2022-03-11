#pragma once

// Контроль утечки памяти.
// WARNING: Не использовать в реальной сборке,
// т.к. ручной вызов finalization приводит к ошибкам при завершении программы в common_exit.
//#define CHECK_MEMLEAK

#ifdef CHECK_MEMLEAK
	#define __CRTDBG_MAP_ALLOC
	#include <crtdbg.h>

	// Переопределение оператора new, чтобы _CrtDumpMemoryLeaks показывал файл и строку создания объекта.
	//#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
	//#define new DEBUG_NEW
#endif
