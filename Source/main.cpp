#include "main.h"
#include "gui.h"
#include "log.h"
#include "lang.h"

#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

#define MUTEX_NAME_IS_STARTED L"SC2MentatStarted"
HANDLE g_StartedMutex = NULL;

int startApp();

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	return startApp();
}

int main()
{
	setlocale(LC_CTYPE, "rus");

	//testOcr();
	//getchar();
	//return 0;

	return startApp();
}

bool isAppActive()
{
	g_StartedMutex = CreateMutex(NULL, TRUE, MUTEX_NAME_IS_STARTED);
	if (INVALID_HANDLE_VALUE == g_StartedMutex)
		return true;
	DWORD result = WaitForSingleObject(g_StartedMutex, 0);
	if (result != WAIT_OBJECT_0)
		return true;

	return false;

	// Этот вариант не понравился, т.к. он может не работать при одновременном запуске двух копий.
	//HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MUTEX_NAME_IS_STARTED);
	//if (!hMutex)
	//{
	//	g_StartedMutex = CreateMutex(NULL, TRUE, MUTEX_NAME_IS_STARTED);
	//	if (INVALID_HANDLE_VALUE == g_StartedMutex)
	//		return true;

	//	return false;
	//}

	//ReleaseMutex(hMutex);
	//CloseHandle(hMutex);
	//return true;
}

void closeApp()
{
	if (!g_StartedMutex || INVALID_HANDLE_VALUE == g_StartedMutex)
		return;

	ReleaseMutex(g_StartedMutex);
	CloseHandle(g_StartedMutex);
	g_StartedMutex = NULL;
}

int startApp()
{
	//testOcr();
	//return 0;

	try
	{
		lng::initLocale();
		if (isAppActive())
		{
			MessageBoxA(0, lng::string(lng::LS_APP_RUNNED_CAPTION), APP_NAME_ANSI, MB_ICONINFORMATION);
		}
		else
		{
			gui::App* app = new gui::App();
			app->run();
			delete app;
		}
	}
	catch (std::exception& e)
	{
		MessageBoxA(0, e.what(), lng::string(lng::LS_APP_EXCEPTION_TITLE), MB_ICONERROR);
	}
	closeApp();


#ifdef CHECK_MEMLEAK
	// Ручная очистка массивов, чтобы легче было разбираться в утечках.
	cfg::finalization();
	gui::finalization();
	hk::finalization();
	lg::finalization();
	test::finalization();

	_CrtDumpMemoryLeaks();
#endif

	return 0;
}
