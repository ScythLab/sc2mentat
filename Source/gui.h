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
	// ���������� ������� ������ ��� �������� ������ �����
	#define BUILD_NUMBERS 9

	// �������� ����� ������������� ����� �����.
	// ���������� �������� ������ ����������� � ������� ~300 ��, ������� �������� � �������.
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
		// ������������� ������� ������ ��� �������� ������ �����
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
		char m_tempText[1024]; // ��������� ����� ��� ������, ������������ �� ����� ������ (������ �������� ����������, �� ������).

		HINSTANCE g_hInst = NULL;
		TCHAR g_cfgFileName[MAX_PATH];
		TCHAR g_appPath[MAX_PATH];
		TCHAR g_fontsPath[MAX_PATH];
		TCHAR g_buildsPath[MAX_PATH];
		TCHAR g_soundsPath[MAX_PATH];
		HWND g_hWnd = NULL; // ����� ������ ��������� ����
		ATOM g_hClass = NULL; // "�������������" ������ ������ ��������� ����
		HWND g_hLogDialog = NULL;
		NOTIFYICONDATA g_trayData;
		BOOL g_trayCreated = false; // ���� �������� ������ � ����
		HMENU g_hTrayMenu = NULL; // ����������� ���� ��� ������ ����
		UINT_PTR g_timerCheckGame = 0; // ������������� ������� �������� �������� ����
		bool g_isGameRun = false; // ���� ������� �������� ����
		dx::Overlay* g_overlay = NULL;
		dx::Render* g_render = NULL;
		CAudio* g_audio = NULL;
		cfg::Settings* g_settings = NULL; // ��������� ���������
		cfg::Panel* g_panelBO = NULL; // ������ ����������� ������ ����������
		cfg::Panel* g_panelTips = NULL; // ������ ����������� ���������
		cfg::Resolution* g_resolution = NULL; // ������� ���������� ����
		ocr::Engine* g_ocr = NULL; // ������ ������������� ������
		cfg::BuildManager* g_bm = NULL; // �������� ������
		cfg::Build* g_build = NULL; // ������� ����
		cfg::BuildStep* g_buildsFilter = NULL; // ��� �� ������� ��������� ������ (�������� ��� ������ ��������� ������)
		int g_buildStep = 0; // ������� ��� �����
		DWORD g_pause = 0; // ����� ����� ������������ �� ���������� ����.
		bool g_newBuild = false; // ���� ������ �����.
		int g_buildIndex; // ������ ����� � ��������� ������
		cfg::Race g_player = cfg::Race::None; // ��������� ���� ������
		cfg::Race g_opponent = cfg::Race::None; // ��������� ���� ����������
		bool g_newRace = true; // ���� ��������� ����
		bool g_showBuilds = true; // ���������� ������ ��������� ������
		bool g_hide = false; // �� ���������� ��������� �������.
		bool g_mute = false; // �� ����������� �����
		bool g_showDebugInfo = false; // ���������� ���������� ����������
		bool g_hotkeysEmpty = false;
		int m_overlayErrors; // ���-�� ������ ������ ������ �������� �������.

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
