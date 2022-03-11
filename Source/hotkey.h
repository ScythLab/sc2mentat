#pragma once
#include "common.h"
#include "tools.h"
#include <windows.h>

//typedef void (__fastcall* HotkeyHandler)(int param);
typedef void(__thiscall Base::* HotkeyHandler)(int param);
//typedef void(__thiscall object::* HotkeyHandler2)(int param);


struct HotKeyHandlerParam
{
	Base* ObjThis;
	HotkeyHandler Handler;
	int Param;

	HotKeyHandlerParam()
	{
		HotKeyHandlerParam(NULL, NULL, 0);
	}
	HotKeyHandlerParam(Base* objThis, HotkeyHandler handler, int param)
	{
		ObjThis = objThis;
		Handler = handler;
		Param = param;
	}
};

struct HotkeyHandlerInf
{
	Base* ObjThis;
	HotkeyHandler Handler;
	DWORD KeyCode;
	DWORD KeyMod;
	int CallParam;
	char* Name;

	HotkeyHandlerInf(Base* objThis, HotkeyHandler handler, DWORD key, DWORD mod, int callParam, LPCSTR name)
	{
		ObjThis = objThis;
		Handler = handler;
		KeyCode = key;
		KeyMod = mod;
		CallParam = callParam;
		Name = createTrimString(name);
	}
	~HotkeyHandlerInf()
	{
		free(Name);
	}
};

namespace hk
{
	void createHotKeys(HWND hWnd);
	void clearHotkeys();
	void removeHotKeys();
	void setTargetWindow(HWND hWnd);
	void addHotKeyHandler(Base* objThis, HotkeyHandler handler, DWORD key, DWORD mod, LPCSTR name, int callParam);
	void processWmHotkey(WPARAM wparam);
	// Если была нажата какая-либо горячая клавиша, то вызывет обработчик для этой клавиши.
	bool callNextHandler();

#ifdef CHECK_MEMLEAK
	void finalization();
#endif
}
