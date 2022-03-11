#include "hotkey.h"
#include "log.h"
#include <vector>
#include <list>

// INFO: Есть два основных метода организации горячих клавиш:
// 1) специальная функция RegisterHotKey
// В целом работает хорошо, в основное окно пресылает сообщение WM_HOTKEY,
// но есть существенный минус:
// - остальные программы перестают реагировать на задействованные комбинации клавиш.
// 2) Ловушка WH_KEYBOARD_LL.
// Не занимает клавиши (остальные программы работают также как и без ловушки), но есть минусы:
// - поток с обработчиком ловушки не должен засыпать или выполнять какую-либо долгую работу, т.к. начнутся тормоза печати во всей винде;
// - поток должен быть подключен к очереди сообщений, например можно вызвать GetMessage.
// В модуле реализованы оба способа.

#define HOTKEY_USE_KEYBOARD_LL_HOOK

namespace hk
{
	std::vector<HotkeyHandlerInf*> g_handlerList;

	// Т.к. обработчик клавиш может работать в другом потоке,
	// то не будем напрямую вызывать обработчики, а поместим их в спец массив,
	// чтобы можно было вызывать обработчик из основного потока.
	std::list<HotKeyHandlerParam*> g_executeHandlerList;

	HWND g_hMainWnd;
	HWND g_hTargetWnd = NULL;
	CRITICAL_SECTION lockCS;

	void lock()
	{
		EnterCriticalSection(&lockCS);
	}
	void unlock()
	{
		LeaveCriticalSection(&lockCS);
	}

	// Отложенный вызов обработчика клавиши.
	void callHandler(HotKeyHandlerParam* handlerParam)
	{
		lock();
		__try
		{
			g_executeHandlerList.push_back(handlerParam);
		}
		__finally
		{
			unlock();
		}
	}

	bool callNextHandler()
	{
		bool filledList = false;
		HotKeyHandlerParam* hp = NULL;

		lock();
		__try
		{
			if (!g_executeHandlerList.size())
				return false;

			hp = g_executeHandlerList.front(); g_executeHandlerList.pop_front();
			filledList = (g_executeHandlerList.size() > 0);
		}
		__finally
		{
			unlock();
		}

		if (hp)
		{
			__try
			{
				if (hp->Handler)
					(hp->ObjThis->*hp->Handler)(hp->Param);
			}
			__finally
			{
				delete hp;
			}
		}

		return filledList;
	}

#ifdef HOTKEY_USE_KEYBOARD_LL_HOOK

	HHOOK hHook = 0;
	DWORD g_threadId = 0;
	HANDLE g_hThread = NULL;
	bool g_needExit = false;

	#define MOD_LSHIFT   (VK_LSHIFT   - VK_LSHIFT)
	#define MOD_RSHIFT   (VK_RSHIFT   - VK_LSHIFT)
	#define MOD_LCONTROL (VK_LCONTROL - VK_LSHIFT)
	#define MOD_RCONTROL (VK_RCONTROL - VK_LSHIFT)
	#define MOD_LMENU    (VK_LMENU    - VK_LSHIFT)
	#define MOD_RMENU    (VK_RMENU    - VK_LSHIFT)
	bool g_keysMods[MOD_RMENU + 1] = { 0 };

	bool setKeyMod(DWORD vkCode, bool isDown)
	{
		if (vkCode >= VK_LSHIFT && vkCode <= VK_RMENU)
		{
			int index = vkCode - VK_LSHIFT;
			g_keysMods[index] = isDown;
			return true;
		}

		return false;
	}

	DWORD getKeyMod()
	{
		DWORD mod = 0;
		if (g_keysMods[MOD_LSHIFT] || g_keysMods[MOD_RSHIFT])
			mod |= MOD_SHIFT;
		if (g_keysMods[MOD_LCONTROL] || g_keysMods[MOD_RCONTROL])
			mod |= MOD_CONTROL;
		if (g_keysMods[MOD_LMENU] || g_keysMods[MOD_RMENU])
			mod |= MOD_ALT;
		return mod;
	}

	LRESULT __stdcall HotKeyHookProc(int nCode, WPARAM wParam, LPKBDLLHOOKSTRUCT lParam)
	{
		//if (0x28 == lParam->vkCode)
		//	log_dbg_printf("nCode: %x; flags: %x; vkKode: %x; scanCode: %x", nCode, lParam->flags, lParam->vkCode, lParam->scanCode);

		if (g_hTargetWnd && nCode >= 0)
		{
			// Учет нажатых Alt, Ctrl, Shift.
			// Можно использовать связку getKeyMod -> getModKeyValue -> isPressed, но этот вариант должен быть надежней.
			// INFO: 7 бит - LLKHF_UP.
			bool isDown = (0 == (lParam->flags & LLKHF_UP));
			if (!setKeyMod(lParam->vkCode, isDown) && isDown)
			{
				DWORD keyMod = getKeyMod();
				HWND hWnd = NULL;
				for (size_t i = 0; i < g_handlerList.size(); i++)
				{
					HotkeyHandlerInf* inf = g_handlerList[i];
					if (inf->KeyCode == lParam->vkCode && inf->KeyMod == keyMod)
					{
						// Применять клавишу будет только если активно окно игры.
						// Хоть GetForegroundWindow и достаточно быстрая функция, но она обращается к ядру,
						// так что будем дергать ее только если клавиша совпала.
						if (!hWnd)
							hWnd = GetForegroundWindow();
						if (hWnd != g_hTargetWnd)
							break;

						callHandler(new HotKeyHandlerParam(inf->ObjThis, inf->Handler, inf->CallParam));
					}
				}
			}
		}

		return CallNextHookEx(hHook, nCode, wParam, (LPARAM)lParam);
	}

	DWORD __stdcall ThreadFunc(LPVOID lpParam)
	{
		HINSTANCE hInst = GetModuleHandle(NULL);
		hHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)HotKeyHookProc, hInst, 0);
		//log_dbg_printf("hook: %x", (DWORD)hHook);
		if (!hHook)
			return GetLastError();

		MSG msg;
		if (GetMessage(&msg, g_hMainWnd, 0, 0))
		{
			log_dbg_printf("Hook.GetMessage.msg %d", msg.message);
		}
		else
		{
			log_dbg_printf("Hook.GetMessage.ERROR: %x", GetLastError());
		}

		while (!g_needExit)
		{
			Sleep(1);
		};


		return 0;
	}

#endif

	void createHotKeys(HWND hWnd)
	{
		g_hMainWnd = hWnd;
		InitializeCriticalSection(&lockCS);

	#ifdef HOTKEY_USE_KEYBOARD_LL_HOOK
		g_hThread = CreateThread(NULL, 0, ThreadFunc, NULL, 0, &g_threadId);
		if (!g_hThread)
			throw std::exception(lng::string(lng::LS_HOTKEY_THREAD_CREATE_FAIL));
	#endif
	}

	void setTargetWindow(HWND hWnd)
	{
		g_hTargetWnd = hWnd;
	}

	HotkeyHandlerInf* getHotkey(DWORD key, DWORD mod)
	{
		for (int i = 0; i < g_handlerList.size(); i++)
		{
			HotkeyHandlerInf* inf = g_handlerList[i];
			if (inf->KeyCode == key && inf->KeyMod == mod)
				return inf;
		}

		return NULL;
	}

	void addHotKeyHandler(Base* objThis, HotkeyHandler handler, DWORD key, DWORD mod, LPCSTR name, int callParam)
	{
		HotkeyHandlerInf* inf = getHotkey(key, mod);
		if (inf)
			throw std::invalid_argument(std::format(lng::string(lng::LS_HOTKEY_DUPLICATE), name, inf->Name));


	#ifdef HOTKEY_USE_KEYBOARD_LL_HOOK
	#else
		int id = g_handlerList.size();
		if (!RegisterHotKey(g_hMainWnd, id, mod, key))
			throw std::runtime_error(std::format(lng::string(lng::LS_HOTKEY_REGISTER_FAIL), name, GetLastError()));
	#endif
		inf = new HotkeyHandlerInf(objThis, handler, key, mod, callParam, name);
		g_handlerList.push_back(inf);
	}

	void clearHotkeys()
	{
	#ifdef HOTKEY_USE_KEYBOARD_LL_HOOK
	#else
		for (int i = 0; i < g_handlerList.size(); i++)
		{
			UnregisterHotKey(g_hMainWnd, i);
		}
	#endif
		SafeVectorDelete(g_handlerList);
		ListDelete(g_executeHandlerList);
	}

	void removeHotKeys()
	{
		DeleteCriticalSection(&lockCS);

	#ifdef HOTKEY_USE_KEYBOARD_LL_HOOK
		if (hHook)
		{
			if (g_hThread)
			{
				g_needExit = true;
				DWORD wait = WaitForSingleObject(g_hThread, 500);
				if (wait != WAIT_OBJECT_0)
					TerminateThread(g_hThread, ERROR_TIMEOUT);
				else
					CloseHandle(g_hThread);
				g_hThread = NULL;
			}

			UnhookWindowsHookEx(hHook);
			hHook = NULL;
		}
	#else
	#endif
		clearHotkeys();
	}

	void processWmHotkey(WPARAM wparam)
	{
	#ifndef HOTKEY_USE_KEYBOARD_LL_HOOK
		if (((int)wparam) >= 0 && wparam < g_handlerList.size())
		{
			HotkeyHandlerInf* inf = g_handlerList[wparam];
			// INFO: Здесь можно напрямую вызывать обработчик.
			callHandler(inf->Handler);
		}
	#endif
	}

#ifdef CHECK_MEMLEAK
	void finalization()
	{
		g_handlerList.~vector();
		g_executeHandlerList.~list();
	}
#endif
}
