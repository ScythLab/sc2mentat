#include "gui.h"
#include "resource.h"
#include "tools.h"
#include "log.h"
#include <windows.h>
#include <wchar.h>
#include <string>

#define WM_USER_SHELLICON (WM_USER + 1)
#define WINDOW_CLASS_NAME L"SC2Mentat.MsgOnly"
#define TIMER_ID_CHECK_GAME 1
#define SC2_CLASS_NAME L"StarCraft II"
#define SC2_WINDOW_NAME L"StarCraft II"

namespace gui
{
#ifdef CHECK_MEMLEAK
	void finalization()
	{
		//g_tipList.~list();
	}
#endif

	//---------------------------------------------------------------------------//
	//------------------------- Вспомогательные функции -------------------------//
	//---------------------------------------------------------------------------//

	// Прокрутим текст в конец
	void scrollEditToEnd(HWND hWnd, int dlgItem = 0)
	{
		if (dlgItem > 0)
			hWnd = GetDlgItem(hWnd, dlgItem);
		if (!hWnd)
			return;

		int lines = SendMessage(hWnd, EM_GETLINECOUNT, 0, 0);
		SendMessage(hWnd, EM_LINESCROLL, 0, lines);
	}

	void showDlgLogText(HWND hwnd)
	{
		char* buff = lg::getLog(0, lg::getCount() - 1);
		SetDlgItemTextA(hwnd, IDC_LOG, buff);
		if (buff)
		{
			free(buff);

			// Прокрутим лог в конец
			scrollEditToEnd(hwnd, IDC_LOG);
		}
	}

	//---------------------------------------------------------------------------//
	//----------------------- Обработчики горячих клавиш ------------------------//
	//---------------------------------------------------------------------------//

	void __thiscall App::handlerOpponentRace(int race)
	{
		g_opponent = (cfg::Race)race;
		g_newRace = true;
		g_showBuilds = true; // Покажем список билдов
	}

	void __thiscall App::handlerOpponentChangeRace(int direction)
	{
		changeRace(&g_opponent, direction);
	}
	
	void __thiscall App::handlerPlayerRace(int race)
	{
		g_player = (cfg::Race)race;
		g_newRace = true;
		g_showBuilds = true; // Покажем список билдов
	}

	void __thiscall App::handlerPlayerChangeRace(int direction)
	{
		changeRace(&g_player, direction);
	}

	void __thiscall App::handlerBuildReset(int dummy)
	{
		g_newBuild = true;
	}

	void __thiscall App::handlerBuildStop(int dummy)
	{
		g_build = NULL;
		g_buildsFilter = NULL;
		g_buildIndex = -1;
	}

	void __thiscall App::handlerBuildPrevNext(int direct)
	{
		if (!g_bm)
			return;

		g_build = (-1 == direct)
			? g_bm->getPrevBuild(g_player, g_opponent, &g_buildIndex)
			: g_bm->getNextBuild(g_player, g_opponent, &g_buildIndex);
		g_newBuild = true;
		g_showBuilds = false; // Спрячем список билдов
	}

	void __thiscall App::handlerBuildByNumber(int number)
	{
		// HARDCORE: Код повторяет drawBuildList.
		int index = -1;
		cfg::Build* firstBuild = NULL;
		cfg::Build* build = g_bm->getNextBuild(g_player, g_opponent, &index);
		while (build && build != firstBuild)
		{
			if (!firstBuild)
				firstBuild = build;

			number--;
			if (0 == number)
			{
				g_build = build;
				g_buildIndex = index;
				g_newBuild = true;
				g_showBuilds = false; // Спрячем список билдов

				return;
			}

			build = g_bm->getNextBuild(g_player, g_opponent, &index);
		}
	}

	void __thiscall App::handlerBuildPrevNextStep(int direct)
	{
		if (!g_bm || !g_build || !g_build->getCount())
			return;

		// При движении по билду (особенно назад) нужна небольшая задержка на срабатывание шага,
		// т.к. алгоритм прохождение по билду может конфликтовать с действиями пользователя.
		g_pause = 1000;
		g_buildStep = min(max(0, g_buildStep + direct), g_build->getCount() - 1);
	}

	void __thiscall App::handlerChangeBool(bool* value)
	{
		*value = !*value;
	}

	void __thiscall App::loadBuilds(int dummy)
	{
		g_newRace = true;
		g_showBuilds = true;
		g_buildsFilter = NULL;
		BOOL success = g_bm->load();
		if (!success)
			showLogDialog();
	}

	//---------------------------------------------------------------------------//

	void App::changeRace(cfg::Race* race, int direction)
	{
		if (-1 == direction)
		{
			if (cfg::Race::Terran == *race)
				*race = cfg::Race::Random;
			else
				*race = (cfg::Race)(*race >> 1);
		}
		else
		{
			if (cfg::Race::Random == *race)
				*race = cfg::Race::Terran;
			else
				*race = (cfg::Race)(*race << 1);
		}

		g_newRace = true;
		g_showBuilds = true; // Покажем список билдов
	}

	LPCSTR App::addHotkey(LPCSTR name, HotkeyHandler handler, int param)
	{
		cfg::Hotkey* hk = g_settings->getHotKey(name);
		if (!hk)
			throw std::invalid_argument(std::format(lng::string(lng::LS_HOTKEY_MISSED), name));

		if (!hk->virtKey_)
			// TODO: нужен флаг обязательности клавиши и пробрасывать исключение.
			return NULL;

		hk::addHotKeyHandler(this, handler, hk->virtKey_, hk->keyMod_, name, param);
		return hk->keyText_;
	}

	void App::loadHotKeys()
	{
		g_hotkeysEmpty = false;
		hk::clearHotkeys();

		// Счетчики обязательных клавиш
		int buildManage = 0; // Работа с билдом (запуск/остановка)
		int buildChange = 0; // Выбор билда (предыдущий/следующий)
		int buildChooseNumbers = 0; // Выбор билда (по номеру)
		int stepChange = 0; // Изменение шага билда (предыдущий/следующий)
		int opponentRace = 0; // Выбор расы соперника (Теран, Зерг, Протосс)
		int opponentChangeRace = 0; // Выбор расы соперника (предыдущая/следующая)

		addHotkey("HIDE", (HotkeyHandler)&App::handlerChangeBool, (int)&g_hide);
		addHotkey("MUTE", (HotkeyHandler)&App::handlerChangeBool, (int)&g_mute);
		addHotkey("DEBUG", (HotkeyHandler)&App::handlerChangeBool, (int)&g_showDebugInfo);
		addHotkey("BUILDS.SHOW", (HotkeyHandler)&App::handlerChangeBool, (int)&g_showBuilds);
		//addHotkey("SETTINGS.REFRESH", );

		addHotkey("BUILDS.RELOAD", (HotkeyHandler)&App::loadBuilds);
		buildManage += (NULL != addHotkey("BUILD.RESET", (HotkeyHandler)&App::handlerBuildReset));
		buildManage += (NULL != addHotkey("BUILD.STOP", (HotkeyHandler)&App::handlerBuildStop));
		buildChange += (NULL != addHotkey("BUILD.PREV", (HotkeyHandler)&App::handlerBuildPrevNext, -1));
		buildChange += (NULL != addHotkey("BUILD.NEXT", (HotkeyHandler)&App::handlerBuildPrevNext, 1));
		for (int i = 1; i <= BUILD_NUMBERS; i++)
		{
			char name[32];
			sprintf(name, "BUILD.%d", i);
			LPCSTR keyText = addHotkey(name, (HotkeyHandler)&App::handlerBuildByNumber, i);
			HOTKEY_BUILDS_BY_NUMBER[i - 1] = keyText;
			buildChooseNumbers += (NULL != keyText);
		}
		stepChange += (NULL != addHotkey("BUILD.PREVSTEP", (HotkeyHandler)&App::handlerBuildPrevNextStep, -1));
		stepChange += (NULL != addHotkey("BUILD.NEXTSTEP", (HotkeyHandler)&App::handlerBuildPrevNextStep, 1));
		opponentRace += (NULL != addHotkey("OPPONENT.TERRAN", (HotkeyHandler)&App::handlerOpponentRace, cfg::Race::Terran));
		opponentRace += (NULL != addHotkey("OPPONENT.ZERG", (HotkeyHandler)&App::handlerOpponentRace, cfg::Race::Zerg));
		opponentRace += (NULL != addHotkey("OPPONENT.PROTOSS", (HotkeyHandler)&App::handlerOpponentRace, cfg::Race::Protoss));
		addHotkey("OPPONENT.RANDOM", (HotkeyHandler)&App::handlerOpponentRace, cfg::Race::Random);
		opponentChangeRace += (NULL != addHotkey("OPPONENT.PREVRACE", (HotkeyHandler)&App::handlerOpponentChangeRace, -1));
		opponentChangeRace += (NULL != addHotkey("OPPONENT.NEXTRACE", (HotkeyHandler)&App::handlerOpponentRace, 1));
		addHotkey("PLAYER.TERRAN", (HotkeyHandler)&App::handlerPlayerRace, cfg::Race::Terran);
		addHotkey("PLAYER.ZERG", (HotkeyHandler)&App::handlerPlayerRace, cfg::Race::Zerg);
		addHotkey("PLAYER.PROTOSS", (HotkeyHandler)&App::handlerPlayerRace, cfg::Race::Protoss);
		addHotkey("PLAYER.RANDOM", (HotkeyHandler)&App::handlerPlayerRace, cfg::Race::Random);
		addHotkey("PLAYER.PREVRACE", (HotkeyHandler)&App::handlerPlayerChangeRace, -1);
		addHotkey("PLAYER.NEXTRACE", (HotkeyHandler)&App::handlerPlayerChangeRace, 1);

		// Должны быть заданы клавиши для:
		// - выбора расы соперника
		if (opponentRace < 3 && !opponentChangeRace)
			g_hotkeysEmpty = true;
		// - выбора билда
		if (buildChooseNumbers < BUILD_NUMBERS && !buildChange)
			g_hotkeysEmpty = true;
		// - запуск/остановка билда
		if (buildManage != 2)
			g_hotkeysEmpty = true;
		// - прохождение по билду
		if (stepChange != 2)
			g_hotkeysEmpty = true;
	}

	void App::applyResolution()
	{
		POINT p = g_overlay->getResolution();
		g_resolution = g_settings->getResolution(p.x, p.y);
		if (g_resolution)
		{
			LPCWSTR fontSubPath = g_resolution->getFontSubPath();
			g_ocr->clear();
			for (int i = 0; i < g_settings->getFontCount(); i++)
			{
				cfg::OcrFont* font = g_settings->getFont(i);
				g_ocr->addFont(fontSubPath, font->getFiles(), font->getAlphabet(), font->getPower(), font->getWhite());
			}
		}
	}

	void App::applySettingsToOverlay()
	{
		if (!g_overlay)
			return;

		applyResolution();
		g_render->clearFonts();
		if (g_panelBO)
			g_render->addFont(g_panelBO->FontSize);
		if (g_panelTips)
			g_render->addFont(g_panelTips->FontSize);
		hk::setTargetWindow(g_overlay->getTargetWnd());
	}

	void App::loadSettings()
	{
		if (!g_bm)
			g_bm = new cfg::BuildManager(g_buildsPath);
		loadBuilds();

		// Заранее очистим горячие клавиши, чтобы в случае ошибок в конфиге они не работали.
		hk::clearHotkeys();

		g_panelBO = NULL;
		g_panelTips = NULL;
		try
		{
			if (!g_settings)
				g_settings = new cfg::Settings(g_cfgFileName);
			g_settings->load();

			g_panelBO = g_settings->getPanelBuildOrder();
			g_panelTips = g_settings->getPanelTips();

			if (!g_ocr)
				g_ocr = new ocr::Engine(g_fontsPath);
		}
		catch (std::exception& e)
		{
			log_printidf(lng::LS_CONFIG_LOAD_FAIL, e.what());
			showLogDialog();
			// Если настройки не загружены, то дальше что-то делать (горячие клавиши) нет смысла.
			return;
		}

		try
		{
			loadHotKeys();
		}
		catch (std::exception& e)
		{
			log_printidf(lng::LS_CONFIG_HOTKEYS_LOAD_FAIL, e.what());
			showLogDialog();
		}

		applySettingsToOverlay();
		g_player = g_settings->getPlayer();
		g_newRace = true;
	}

	void App::addTip(LPCSTR text, DWORD timeout)
	{
		if (!text)
			return;

		g_tipList.push_back(new TipText(text, timeout));
	}

	void App::clearTips()
	{
		std::list<TipText*>::iterator itr;
		itr = g_tipList.begin();
		while (itr != g_tipList.end())
		{
			TipText* tip = itr._Ptr->_Myval;
			delete tip;
			itr++;
		}
		g_tipList.clear();
	}

	void App::closeLogDialog()
	{
		//EndDialog(hwnd, 0);
		DestroyWindow(g_hLogDialog);
		g_hLogDialog = NULL;
	}

	BOOL __thiscall App::dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
			case WM_INITDIALOG:
			{
				showDlgLogText(hwnd);
				return FALSE;
			}

			case WM_COMMAND:
			{
				switch (LOWORD(wParam))
				{
				case IDC_OK:
					closeLogDialog();
					break;

				case IDC_REFRESH:
					showDlgLogText(hwnd);
					break;

				case IDC_CLEAR:
					lg::clear();
					showDlgLogText(hwnd);
					break;
				}

				return FALSE;
			}

			case WM_CLOSE:
			{
				closeLogDialog();
				return FALSE;
			}
		}

		return FALSE;
	}

	void App::showLogDialog()
	{
		if (g_hLogDialog)
		{
			showDlgLogText(g_hLogDialog);
			ShowWindow(g_hLogDialog, SW_SHOWNORMAL);
			SetForegroundWindow(g_hLogDialog);
		}
		else
		{
			g_hLogDialog = CreateDialog(g_hInst, MAKEINTRESOURCE(DIALOG_LOG), g_hWnd, (DLGPROC)m_trampolineDlgProc);
			if (g_hLogDialog)
			{
				lng::localizeDialog(g_hLogDialog, DIALOG_LOG);
				HICON icon = LoadIcon(g_hInst, MAKEINTRESOURCE(MAINICON));
				if (icon)
					SendMessage(g_hLogDialog, WM_SETICON, ICON_BIG, (LPARAM)icon);
			}
		}
	}

	void App::getOcrText(PIX* image, cfg::Frame* frame, char* text, int textSize)
	{
		g_ocr->getText(image, frame->left_, frame->top_, frame->width_, frame->fontIndex_, text, textSize);
	}

	void App::drawText(LPCSTR text, cfg::Panel* panel, int offY)
	{
		int x = 0;
		int y = 0;
		int w = g_resolution->getWidth();
		int h = g_resolution->getHeight();
		POINT textSize = g_render->getTextDimensions(text, panel->FontSize);
		int offX = panel->MarginH;
		offY += panel->MarginV;

		if (panel->AlignH < 0)
			x = offX;
		else if (panel->AlignH > 0)
			x = w - textSize.x - offX;
		else
			x = (w - textSize.x) / 2 + offX;
		if (panel->AlignV < 0)
			y = offY;
		else if (panel->AlignV > 0)
			y = h - textSize.y - offY;
		else
			y = (h - textSize.y) / 2 + offY;

		g_render->drawText(text, panel->FontSize, x, y, panel->FontColor, false, panel->Outline);
	}

	void App::drawTips()
	{
		if (!g_render || !g_resolution)
			return;

		DWORD dt = GetTickCount();
		std::list<TipText*>::iterator itr;
		itr = g_tipList.begin();
		int i = 0;
		while (itr != g_tipList.end())
		{
			TipText* tip = itr._Ptr->_Myval;
			if (tip->endTime > dt)
			{
				drawText(tip->title, g_panelTips, i * g_panelTips->FontSize);
				i++;
				itr++;
			}
			else
			{
				itr = g_tipList.erase(itr);
				delete tip;
			}
		}
	}

	void App::drawMatchupAndState()
	{
		strcpy(m_tempText, m_matchUp);
		if (g_mute)
			strcat(m_tempText, lng::string(lng::LS_MSG_MUTE));
		if (g_showDebugInfo)
			strcat(m_tempText, lng::string(lng::LS_MSG_DEBUG));
		drawText(m_tempText, g_panelBO, m_mainTextTop += g_panelBO->FontSize);
	}

	void App::drawError(LPCSTR text)
	{
		g_render->drawText(text, DEFAULT_FONT_SIZE, 8, 8, 0xFF0000, false, 1);
	}

	void App::drawFrame(PIX* pix, cfg::PFrame frame, int* top)
	{
		// BUG: Не учитывается ориентация g_panelBO.
		int fontHeight = g_ocr->getFontHeight(frame->fontIndex_);
		PIX* im = pixClipRectangle(pix, frame->left_, frame->top_, frame->width_, fontHeight);
		if (!im)
			return;

		#define ZOOM 4
		#define BORDER_WIDTH 2
		g_render->drawPix(im, g_panelBO->MarginH, g_panelBO->MarginV + *top, ZOOM, BORDER_WIDTH);
		*top += im->h * ZOOM + BORDER_WIDTH * 3;
		pixDestroy(&im);
	}

	App::App()
	{
		m_overlayErrors = 0;

		g_hInst = GetModuleHandle(NULL);
		// Определим базовые директории и имена файлов
		TCHAR exeName[MAX_PATH];
		int length = GetModuleFileName(0, exeName, MAX_PATH);
		if (!length)
			throw std::runtime_error(std::format(lng::string(lng::LS_MODULEFILE_NAME_FAIL), GetLastError()));

		wcscpy(g_appPath, exeName);
		extractPath(g_appPath, MAX_PATH);
		wcscpy(g_cfgFileName, exeName);
		changeFileExt(g_cfgFileName, MAX_PATH, L".cfg");
		wcscpy(g_fontsPath, g_appPath);
		wcscat(g_fontsPath, L"Fonts\\");
		wcscpy(g_buildsPath, g_appPath);
		wcscat(g_buildsPath, L"Builds\\");
		wcscpy(g_soundsPath, g_appPath);
		wcscat(g_soundsPath, L"Sounds\\");

		void* funcAddr;
		LABEL_ADDR(funcAddr, wndProc);
		m_trampolineWndProc = makeTrampoline(this, funcAddr);
		LABEL_ADDR(funcAddr, dlgProc);
		m_trampolineDlgProc = makeTrampoline(this, funcAddr);

		createTray();

		hk::createHotKeys(g_hWnd);
		loadSettings();

		g_opponent = cfg::Race::None;

		try
		{
			g_audio = new CAudio(g_soundsPath);
			g_audio->initialiseAudio();
		}
		catch (std::exception& e)
		{
			SafeDelete(g_audio);
			log_printidf(lng::LS_AUDIO_FAIL, e.what());
			showLogDialog();
		}
	}

	App::~App()
	{
		clear();
	}

	void App::killCheckGameTimer()
	{
		if (g_timerCheckGame && g_hWnd)
		{
			KillTimer(g_hWnd, g_timerCheckGame);
			g_timerCheckGame = 0;
		}
	}

	void App::clear()
	{
		SafeDelete(g_audio);
		SafeDelete(g_render);
		SafeDelete(g_overlay);
		SafeDelete(g_settings);
		SafeDelete(g_ocr);
		SafeDelete(g_bm);
		lg::clear();

		hk::removeHotKeys();
		clearTips();

		killCheckGameTimer();
		if (g_hTrayMenu)
		{
			DestroyMenu(g_hTrayMenu);
			g_hTrayMenu = NULL;
		}
		if (g_trayCreated)
		{
			g_trayCreated = false;
			Shell_NotifyIcon(NIM_DELETE, &g_trayData);
		}

		if (g_hWnd && INVALID_HANDLE_VALUE != g_hWnd)
		{
			DestroyWindow(g_hWnd);
			g_hWnd = NULL;
		}
		if (g_hClass)
		{
			g_hClass = NULL;
			UnregisterClass(WINDOW_CLASS_NAME, NULL);
		}

		freeTrompoline(&m_trampolineDlgProc);
		freeTrompoline(&m_trampolineWndProc);
	}

	void App::createTray()
	{
		WNDCLASSEX wc;
		memset(&wc, 0, sizeof(wc));
		wc.cbSize = sizeof(wc);

		// Create our window class
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC)m_trampolineWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = g_hInst;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = HBRUSH(RGB(0, 0, 0));
		wc.lpszClassName = WINDOW_CLASS_NAME;
		wc.hIconSm = NULL;

		// Register our window class
		g_hClass = RegisterClassEx(&wc);
		if (!g_hClass)
			throw std::exception(lng::string(lng::LS_TRAY_REGISTERCLASS_FAIL));

		g_hWnd = CreateWindowEx(
			/* dwExStyle    */ 0,
			/* lpClassName  */ WINDOW_CLASS_NAME,
			/* lpWindowName */ NULL,
			/* dwStyle      */ 0,
			/* x, y, w, h   */ 0, 0, 0, 0,
			/* hWndParent   */ HWND_MESSAGE,
			/* hMenu        */ NULL,
			/* hInstance    */ g_hInst,
			/* lpParam      */ NULL
		);

		if (INVALID_HANDLE_VALUE == g_hWnd)
			throw std::exception(lng::string(lng::LS_TRAY_CREATEWND_FAIL));

		memset(&g_trayData, 0, sizeof(g_trayData));
		g_trayData.cbSize = sizeof(g_trayData);
		g_trayData.hWnd = g_hWnd;
		g_trayData.uID = 1;          // Можно поставить любой идентификатор, всё равно иконка только одна
		g_trayData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		g_trayData.uCallbackMessage = WM_USER_SHELLICON;
		g_trayData.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(MAINICON));
		wcscpy(g_trayData.szTip, APP_NAME);
		g_trayData.uVersion = NOTIFYICON_VERSION;

		g_trayCreated = Shell_NotifyIcon(NIM_ADD, &g_trayData);
		if (g_trayCreated)
		{
			g_hTrayMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(TRAYMENU));
			if (!g_hTrayMenu)
				throw std::exception(lng::string(lng::LS_TRAY_LOADMENU_FAIL));
			lng::localizeMenu(g_hTrayMenu);
		}

		g_timerCheckGame = SetTimer(g_hWnd, TIMER_ID_CHECK_GAME, 1000, NULL);
	}

	LRESULT __thiscall App::wndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		switch (msg)
		{
			case WM_DESTROY:
				g_hWnd = NULL;
				close();
				return 0;

			case WM_HOTKEY:
				//log_dbg_printf("WM_HOTKEY. w: %#x; l: %#x", wparam, lparam);
				hk::processWmHotkey(wparam);
				return 0;

			case WM_COMMAND:
				switch (wparam)
				{
					case ID_TRAY_LOG:
						showLogDialog();
						break;

					case ID_TRAY_REFRESH:
						loadSettings();
						break;

					case ID_TRAY_EXIT:
						close();
						break;

					default:
						return DefWindowProc(wnd, msg, wparam, lparam);
				} // switch (wparam)
				return 0;

			case WM_USER_SHELLICON:
				switch (lparam)
				{
					case WM_LBUTTONDBLCLK:
						showLogDialog();
						break;

					case WM_RBUTTONUP:
						if (g_hTrayMenu)
						{
							POINT pt;
							GetCursorPos(&pt);
							HMENU hSubMenu = GetSubMenu(g_hTrayMenu, 0);
							SetForegroundWindow(g_hWnd); // Чтобы меню автоматически закрывалось
							TrackPopupMenu(hSubMenu, 0, pt.x, pt.y, 0, g_hWnd, NULL);
						}
						break;
				} // switch (lparam)
				return 0;

			case WM_TIMER:
				switch (wparam)
				{
					case TIMER_ID_CHECK_GAME:
						bool isGameRun = (dx::Overlay::existsGame(SC2_CLASS_NAME, SC2_WINDOW_NAME));
						if (g_isGameRun != isGameRun && g_settings)
						{
							g_isGameRun = isGameRun;
							if (g_isGameRun)
							{
								log_printidf(lng::LS_GAME_RUNNED);

								try
								{
									// При запуске игры в полноэкранном режиме может быть ошибка при создании DirectX устройства.
									g_overlay = new dx::Overlay(SC2_CLASS_NAME, SC2_WINDOW_NAME, false);
									g_render = g_overlay->createRender();
									m_overlayErrors = 0;
								}
								catch(std::exception& e)
								{
									m_overlayErrors++;
									log_printidf(lng::LS_OVERLAY_FAIL, e.what());
									// Если слишком много ошибок подряд, то остановим это безумие.
									if (m_overlayErrors >= 5)
									{
										// TODO: Изменить значок в трее.
										killCheckGameTimer();
										log_printidf(lng::LS_OVERLAY_MANYERRORS);
									}

									if (g_overlay)
										delete g_overlay;
									g_overlay = NULL;
									if (g_render)
										delete g_render;
									g_render = NULL;
								}
								applySettingsToOverlay();
							}
							else
							{
								log_printidf(lng::LS_GAME_OFF);

								// Удаляем окно-оверлей.
								delete g_render;
								g_render = NULL;
								delete g_overlay;
								g_overlay = NULL;
								g_resolution = NULL;
								hk::setTargetWindow(NULL);
								// Сбросим настройки по предыдущему билду.
								g_newRace = true;
								g_showBuilds = true;
							}
						}
						break;
				} // switch (wparam)
				//log_dbg_printf("WM_TIMER. w: %#x; l: %#x", wparam, lparam);
				return 0;

			default:
				return DefWindowProc(wnd, msg, wparam, lparam);
		} // switch (msg)
	}

	void App::close()
	{
		m_needClose = true;
	}

	bool App::sleep(DWORD ms)
	{
		Sleep(ms);
		return !m_needClose && (g_hWnd != NULL);
	}

	void App::run()
	{
		MSG m;
		ZeroMemory(&m, sizeof(m));
		DWORD nextDt = 0, dt;

		while (sleep(1))
		{
			if (PeekMessage(&m, 0, NULL, NULL, PM_REMOVE))
			{
				if (!IsWindow(g_hLogDialog) || !IsDialogMessage(g_hLogDialog, &m))
				{
					TranslateMessage(&m);
					DispatchMessage(&m);
				}
			}

			// Если выбрали пункт "выход" в меню (или другой способ выхода), то дальнейшая работа по циклу приведет к ошибке.
			if (!g_hWnd || m_needClose)
				break;

			while (hk::callNextHandler());

			// Отрисовывать нечего.
			if (!g_overlay || !g_render)
				continue;

			// Ограничим частоту отрисовки сцены и прохода по билду, чтобы не нагружать проц.
			dt = GetTickCount();
			if (nextDt > dt)
				continue;

			if (g_overlay->getGameResized())
			{
				g_overlay->setGameResized(false);
				applyResolution();
			}

			if (g_audio)
				g_audio->update();

			g_render->beginRendering();
			drawScene();
			g_render->endRendering();

			// Если игра не активна, то сильнее снизим FPS.
			DWORD idleTime = (!g_overlay || g_overlay->getTargetWnd() != GetForegroundWindow()) ? 250 : 50;
			nextDt = dt + idleTime;
		}
	}

	void App::drawBuildSteps()
	{
		if (!g_build)
			return;

		for (int i = 0; i < g_settings->getStepsCount() && g_buildStep + i < g_build->getCount(); i++)
		{
			cfg::BuildStep* bs = g_build->getStep(g_buildStep + i);
			LPCSTR t = (bs->getTime() < 0) ? "--:--" : bs->getTimeStr();
			char s[10];
			if (bs->getSupply() <= 0)
				strcpy(s, "--");
			else
				sprintf(s, "%2d", bs->getSupply());
			char m[10];
			if (bs->getMineral() <= 0)
				strcpy(m, "---");
			else
				sprintf(m, "%3d", bs->getMineral());
			char v[10];
			if (bs->getVespene() <= 0)
				strcpy(v, "---");
			else
				sprintf(v, "%3d", bs->getVespene());


			sprintf(m_tempText, "%s %s [%s:%s] %s", t, s, m, v, bs->getActionDebug());
			drawText(m_tempText, g_panelBO, m_mainTextTop += g_panelBO->FontSize);
		}
	}

	void App::forceDrawDebugInfo()
	{
		if (!g_showDebugInfo)
			return;

		// HARDCORE: Частичное дублирование кода из drawScene.
		m_screenShot = dx::getScreenShot(g_overlay->getTargetWnd());
		getOcrData();
		drawDebugInfo();
		pixDestroy(&m_screenShot);
	}

	void App::drawBuildList()
	{
		// HADRCORE: Код частично повторяет handlerBuildByNumber.
		if (!g_showBuilds)
			return;

		int index = -1;
		int number = 1;
		cfg::Build* firstBuild = NULL;
		cfg::Build* build = g_bm->getNextBuild(g_player, g_opponent, &index);
		while (build && build != firstBuild)
		{
			if (!firstBuild)
				firstBuild = build;

			// Фильтр отображаемых билдов.
			// TODO: Если билд указан в списке, но он не доступен, то вывести его красным.
			bool isVisible = true;
			if (g_buildsFilter)
			{
				isVisible = g_buildsFilter->existBuild(build);
			}

			if (isVisible)
			{
				LPCSTR keyText = HOTKEY_BUILDS_BY_NUMBER[number - 1];
				char strNumber[10];
				if (!keyText)
				{
					itoa(number, strNumber, 10);
					keyText = strNumber;
				}
				sprintf(m_tempText, "%s) %s (%s, %d)", keyText, build->getName(), build->getMaxTime(), build->getCount());
				drawText(m_tempText, g_panelBO, m_mainTextTop += g_panelBO->FontSize);
			}

			number++;
			build = g_bm->getNextBuild(g_player, g_opponent, &index);
		}

		// Сделаем небольшой отступ.
		m_mainTextTop += g_panelBO->FontSize;
	}

	void App::drawDebugInfo()
	{
		if (!g_showDebugInfo)
			return;

		m_mainTextTop += g_panelBO->FontSize;

		drawFrame(m_screenShot, g_resolution->getMineral(), &m_mainTextTop);
		drawFrame(m_screenShot, g_resolution->getVespene(), &m_mainTextTop);
		drawFrame(m_screenShot, g_resolution->getSupply(), &m_mainTextTop);
		drawFrame(m_screenShot, g_resolution->getTime(), &m_mainTextTop);

		m_mainTextTop -= g_panelBO->FontSize;
		sprintf(m_tempText, "[%s] '%s'=%d | '%s'=%d | '%s'=%d", m_ocrTextTime, m_ocrTextMinerals, m_mineral, m_ocrTextVespene, m_vespene, m_ocrTextSupply, m_supply);
		drawText(m_tempText, g_panelBO, m_mainTextTop += g_panelBO->FontSize);
	}

	void App::getOcrData()
	{
		ocr::debugEnabled = true;
		getOcrText(m_screenShot, g_resolution->getMineral(), m_ocrTextMinerals, sizeof(m_ocrTextMinerals));
		ocr::debugEnabled = false;

		getOcrText(m_screenShot, g_resolution->getVespene(), m_ocrTextVespene, sizeof(m_ocrTextVespene));
		getOcrText(m_screenShot, g_resolution->getSupply(), m_ocrTextSupply, sizeof(m_ocrTextSupply));
		getOcrText(m_screenShot, g_resolution->getTime(), m_ocrTextTime, sizeof(m_ocrTextTime));
		m_ocrTextRecognized = strlen(m_ocrTextMinerals) && strlen(m_ocrTextVespene) && strlen(m_ocrTextSupply) && strlen(m_ocrTextTime);

		m_mineral = atoi(m_ocrTextMinerals);
		m_vespene = atoi(m_ocrTextVespene);
		m_supply = atoi(m_ocrTextSupply);
		m_time = timeToSecond(m_ocrTextTime);

		//log_dbg_printf("[%s] %s (%d), %s (%d), step:%d", m_ocrTextTime, m_ocrTextMinerals, m_mineral, m_ocrTextVespene, m_vespene, g_buildStep);
	}

	void App::flowBuild()
	{
		cfg::BuildStep* bs = g_build->getStep(g_buildStep);

		// Проверка, что условия шага соблюдены.
		// При стандартном ожидании необходимо соблюдение параметров <минералы, газ, хранилища>.
		// При ожидании времении необходимо соблюдение времени и если заданы, то <минералы, газ, хранилища>.
		// Без ожидания - ничего не нужно.
		bool isResCondOk = ((bs->getMineral() < 0 || m_mineral >= bs->getMineral())
			&& (bs->getVespene() < 0 || m_vespene >= bs->getVespene())
			);
		bool isSupplyCondOk = (bs->getSupply() < 0 || m_supply >= bs->getSupply());
		bool isCondOk = isResCondOk && isSupplyCondOk;
		bool isTimeOk = (bs->getTime() < 0 || m_time >= bs->getTime());

		cfg::WaitType wait = bs->getWait();
		if (
			(!(cfg::WaitType::WT_RES  & wait) || isResCondOk)
			&& (!(cfg::WaitType::WT_SUP  & wait) || isSupplyCondOk)
			&& (!(cfg::WaitType::WT_TIME & wait) || isTimeOk)
			)
		{
			if (m_actionPlayed != g_buildStep)
			{
				for (int i = 0; !g_mute && g_audio && i < bs->getActionCount(); i++)
				{
					LPCSTR action = bs->getAction(i);
					LPCSTR soundFileName = g_settings->getSoundFileName(action);
					if (!soundFileName)
						// TODO: Выводить только 1 раз на действие.
						log_printidf(lng::LS_SOUND_MISSED, action);
					g_audio->addToQueue(soundFileName);
				}
				m_actionPlayed = g_buildStep;
				addTip(bs->getDesc(), g_settings->getTipsTimeout());

				// Если в шаге задан переход к следующим билдам.
				// то покажем список билдов и отфильруем их по указанным.
				if (bs->getBuildFileNameCount())
				{
					handlerBuildStop();
					g_buildsFilter = bs;
					g_showBuilds = true;
					return;
				}
			}
		}

		// Контроль выполнения шага.
		if (m_actionPlayed == g_buildStep)
		{
			// При стандартном ожидании необходимо, чтобы минералы или газ уменьшились.
			// При ожидании времении, если в условиях заданы минерыли или газ, то ждем их уменьшения, в противном случае, шаг считается выполненным.
			// Без ожидания - шаг сразу выполнен.
			if (
				(!(cfg::WaitType::WT_RESUSE  & wait) || (m_mineralPrev > m_mineral || m_vespenePrev > m_vespene))
				)
			{
				if (g_showDebugInfo)
				{
					log_printidf(lng::LS_DEBUG_INFO, GetTickCount(), m_ocrTextTime, bs->getActionDebug(), m_mineralPrev, m_mineral, m_vespenePrev, m_vespene, wait);
				}

				g_buildStep++;
				m_nextStepDt = GetTickCount() + DELAY_STEP;
			}
		}
	}

	void App::drawScene()
	{
		if (!g_settings || !g_settings->getIsLoaded())
		{
			drawError(lng::string(lng::LS_MSG_SETTINGS_INVALID));
			return;
		}
		if (!g_resolution)
		{
			POINT p = g_overlay->getResolution();
			sprintf(m_tempText, lng::string(lng::LS_MSG_RESOLUTION_UND), p.x, p.y);
			drawError(m_tempText);
			return;
		}
		if (g_hotkeysEmpty)
		{
			drawError(lng::string(lng::LS_MSG_HOTKEY_MISSED));
			return;
		}

		m_mainTextTop = -g_panelBO->FontSize;

		if (g_hide)
			return;

		if (!g_bm)
		{
			drawText(lng::string(lng::LS_MSG_BUILD_NULL), g_panelBO, m_mainTextTop += g_panelBO->FontSize);
			return;
		}

		if (g_newRace)
		{
			int buildCount = g_bm->getBuildCount(g_player, g_opponent);
			sprintf(m_matchUp, "%sv%s (%d)", raceToStr(g_player), raceToStr(g_opponent), buildCount);
			g_build = NULL;
			g_newBuild = true;
			g_buildIndex = -1;
			g_newRace = false;
		}

		if (g_newBuild)
		{
			g_newBuild = false;
			g_buildsFilter = NULL;
			g_buildStep = 0;
			m_mineralPrev = 0;
			m_vespenePrev = 0;
			m_actionPlayed = -1;
			m_nextStepDt = 0;
			m_isStartOfGame = true;
		}

		drawTips();
		drawMatchupAndState();

		// Список билдов матчапа и их горячие клавиши.
		drawBuildList();

		if (!g_resolution || !g_ocr)
			return;

		// Даже если билд не выбран, должна быть возможность отображения отладочной информации
		if (!g_build)
		{
			forceDrawDebugInfo();
			return;
		}

		// Инфа по текущему билду
		int buildStepCount = g_build->getCount();
		sprintf(m_tempText, "%s (%s, %d / %d)", g_build->getName(), g_build->getMaxTime(), min(buildStepCount, g_buildStep + 1), buildStepCount);
		drawText(m_tempText, g_panelBO, m_mainTextTop += g_panelBO->FontSize);

		// Билд закончился
		if (g_buildStep >= buildStepCount)
		{
			forceDrawDebugInfo();
			return;
		}

		m_screenShot = dx::getScreenShot(g_overlay->getTargetWnd());
		getOcrData();
		drawBuildSteps();
		drawDebugInfo();
		pixDestroy(&m_screenShot);

		if (!m_ocrTextRecognized)
			return;

		// В начале игры трехсекундный отсчет с затемнением экрана,
		// но ресурсы нормально отрисовываются на доли секунды и иногда программа успевает их определить,
		// сделаем задержку чтобы преждевременно не воспроизводить звук первого действия.
		if (m_isStartOfGame)
		{
			m_isStartOfGame = false;
			m_nextStepDt = max(m_nextStepDt, GetTickCount() + DELAY_STEP);
		}
		if (g_pause)
		{
			m_nextStepDt = max(m_nextStepDt, GetTickCount() + g_pause);
			g_pause = 0;
			m_actionPlayed = -1;
		}

		// Показания реурсов изменяются плавно, введем небольшую задержку, чтобы два шага билда не засчитывались из-за одного действия игрока.
		if (!m_nextStepDt || m_nextStepDt < GetTickCount())
		{
			flowBuild();
		}

		m_mineralPrev = m_mineral;
		m_vespenePrev = m_vespene;
	}
}

