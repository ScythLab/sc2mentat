#pragma once

// �������� ������ ������.
// WARNING: �� ������������ � �������� ������,
// �.�. ������ ����� finalization �������� � ������� ��� ���������� ��������� � common_exit.
//#define CHECK_MEMLEAK

#ifdef CHECK_MEMLEAK
	#define __CRTDBG_MAP_ALLOC
	#include <crtdbg.h>

	// ��������������� ��������� new, ����� _CrtDumpMemoryLeaks ��������� ���� � ������ �������� �������.
	//#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
	//#define new DEBUG_NEW
#endif
