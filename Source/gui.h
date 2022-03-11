#pragma once
#include "common.h"
#include "tools.h"
#include "config.h"
#include "pix.h"
#include "hotkey.h"
#include "dx.h"
#include "audio.h"
#include "ocr.h"

namespace gui
{
	// Количество горячих клавиш для быстрого выбора билда
	#define BUILD_NUMBERS 9

	// Задержка между срабатыванием шагов билда.
	// Количество ресурсов плавно уменьшается в течение ~300 мс, возьмем задержку с запасом.
	#define DELAY_STEP 500

	struct TipText
	{
		LPCSTR title;
		DWORD endTime;

		TipText(LPCSTR text, DWORD timeout)
		{
			title = createTrimString(text);
			endTime = GetTickCount() + timeout;
		}
		~TipText()
		{
			free((void*)title);
		}
	};

	class App : Base
	{
	private:
		// Представление горячих клавиш для быстрого выбора билда
		LPCSTR HOTKEY_BUILDS_BY_NUMBER[BUILD_NUMBERS] = { NULL };

		void* m_trampolineWndProc;
		void* m_trampolineDlgProc;
		bool m_needClose = false;
		int m_mineralPrev;
		int m_vespenePrev;
		int m_actionPlayed;
		DWORD m_nextStepDt;
		bool m_isStartOfGame;
		char m_matchUp[32];
		char m_tempText[1024]; // Временный буфер для текста, используется по всему классу (крайне хреновая реализация, но сойдет).

		HINSTANCE g_hInst = NULL;
		TCHAR g_cfgFileName[MAX_PATH];
		TCHAR g_appPath[MAX_PATH];
		TCHAR g_fontsPath[MAX_PATH];
		TCHAR g_buildsPath[MAX_PATH];
		TCHAR g_soundsPath[MAX_PATH];
		HWND g_hWnd = NULL; // Хендл нашего основного окна
		ATOM g_hClass = NULL; // "Идентификатор" класса нашего основного окна
		HWND g_hLogDialog = NULL;
		NOTIFYICONDATA g_trayData;
		BOOL g_trayCreated = false; // Флаг создания иконки в трее
		HMENU g_hTrayMenu = NULL; // Всплывающее меню для иконки трея
		UINT_PTR g_timerCheckGame = 0; // Идентификатор таймера контроля игрового окна
		bool g_isGameRun = false; // Флаг наличия игрового окна
		dx::Overlay* g_overlay = NULL;
		dx::Render* g_render = NULL;
		CAudio* g_audio = NULL;
		cfg::Settings* g_settings = NULL; // Настройки программы
		cfg::Panel* g_panelBO = NULL; // Панель отображения текста билдордера
		cfg::Panel* g_panelTips = NULL; // Панель отображения подсказок
		cfg::Resolution* g_resolution = NULL; // Текущее разрешение игры
		ocr::Engine* g_ocr = NULL; // Движок распознавания текста
		cfg::BuildManager* g_bm = NULL; // Менеджер билдов
		cfg::Build* g_build = NULL; // Текущий билд
		cfg::BuildStep* g_buildsFilter = NULL; // Шаг со списком следующих билдов (работает как фильтр доступных билдов)
		int g_buildStep = 0; // Текущий шаг билда
		DWORD g_pause = 0; // Пауза перед прохождением по следующему шагу.
		bool g_newBuild = false; // Флаг начала билда.
		int g_buildIndex; // Индекс билда в менеджере билдов
		cfg::Race g_player = cfg::Race::None; // Выбранная раса игрока
		cfg::Race g_opponent = cfg::Race::None; // Выбранная раса противника
		bool g_newRace = true; // Флаг изменения расы
		bool g_showBuilds = true; // Показывать список доступных билдов
		bool g_hide = false; // Не отображать интерфейс оверлея.
		bool g_mute = false; // Не проигрывать звуки
		bool g_showDebugInfo = false; // Показывать отладочную информацию
		bool g_hotkeysEmpty = false;
		int m_overlayErrors; // Кол-во подряд идущих ошибок создания оверлея.

		int m_mainTextTop;
		PIX* m_screenShot;
		char m_ocrTextMinerals[10];
		char m_ocrTextVespene[10];
		char m_ocrTextSupply[10];
		char m_ocrTextTime[10];
		bool m_ocrTextRecognized;
		int m_mineral;
		int m_vespene;
		int m_supply;
		int m_time;

		std::list<TipText*> g_tipList;

		void createTray();
		void clear();
		bool sleep(DWORD ms);

		LRESULT __thiscall wndProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
		BOOL __thiscall dlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		void __thiscall handlerOpponentRace(int race);
		void __thiscall handlerOpponentChangeRace(int direction);
		void __thiscall handlerPlayerRace(int race);
		void __thiscall handlerPlayerChangeRace(int direction);
		void __thiscall handlerBuildReset(int dummy = 0);
		void __thiscall handlerBuildStop(int dummy = 0);
		void __thiscall handlerBuildPrevNext(int direct);
		void __thiscall handlerBuildByNumber(int number);
		void __thiscall handlerBuildPrevNextStep(int direct);
		void __thiscall handlerChangeBool(bool* value);
		void __thiscall loadBuilds(int dummy = 0);

		void showLogDialog();
		void closeLogDialog();

		void drawScene();
		void drawBuildSteps();
		void drawBuildList();
		void drawDebugInfo();
		void forceDrawDebugInfo();
		void drawMatchupAndState();
		void drawText(LPCSTR text, cfg::Panel* panel, int offY);
		void drawError(LPCSTR text);
		void drawFrame(PIX* pix, cfg::PFrame frame, int* top);
		void drawTips();
		void getOcrData();
		void flowBuild();

		void changeRace(cfg::Race* race, int direction);
		void getOcrText(PIX* image, cfg::Frame* frame, char* text, int textSize);
		void addTip(LPCSTR text, DWORD timeout);
		void clearTips();
		void loadHotKeys();
		void loadSettings();
		void applyResolution();
		void applySettingsToOverlay();
		void killCheckGameTimer();
		LPCSTR addHotkey(LPCSTR name, HotkeyHandler handler, int param = 0);

	public:
		App();
		~App();

		void run();
		void close();
	};

#ifdef CHECK_MEMLEAK
	void finalization();
#endif
}
