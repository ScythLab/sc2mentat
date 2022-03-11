#pragma once
#include "common.h"
#include "tools.h"
#include <windows.h>
#include <wchar.h>
#include <vector>

namespace cfg
{
	#define CONFIG_NAME_SIZE 32
	#define BUILD_NAME_SIZE 64
	#define RESOLUTION_MIN_WIDTH 640
	#define RESOLUTION_MIN_HEIGHT 480

	struct Hotkey
	{
		char* name_;
		UINT virtKey_;
		UINT keyMod_;
		char* keyText_;

		Hotkey(LPCSTR name, UINT virtKey, UINT keyMod, LPCSTR keyText)
		{
			name_ = _strupr(createTrimString(name));
			virtKey_ = virtKey;
			keyMod_ = keyMod;
			keyText_ = createTrimString(keyText);
		}
		~Hotkey()
		{
			free(name_);
			free(keyText_);
		}
	};

	class Sound
	{
	private:
		char* action_;
		char* fileName_;

	public:
		Sound(LPCSTR action, LPCSTR fileName);
		~Sound();

		LPCSTR getAction() { return action_; }
		LPCSTR getFileName() { return fileName_; }
	};

	class OcrFont
	{
	private:
		char name_[CONFIG_NAME_SIZE + 1];
		std::vector<LPCWSTR> fileNames_;
		char* alphabet_;
		int power_;
		int white_;

	public:
		OcrFont(LPCSTR name, LPCSTR alphabet, int power, int white);
		~OcrFont();

		void addFile(LPCSTR fileName);

		LPCSTR getName() { return name_; }
		std::vector<LPCWSTR> getFiles() { return fileNames_; }
		int getPower() { return power_; }
		char* getAlphabet() { return alphabet_; }
		char getSymbol(int index) { return alphabet_[index]; }
		LPCWSTR getFileName(int index) { return fileNames_[index]; }
		int getWhite() { return white_; }
	};


	typedef struct Frame
	{
		char name_[CONFIG_NAME_SIZE + 1];
		int left_;
		int top_;
		int width_;
		int fontIndex_;
	} *PFrame;

	class Resolution
	{
	private:
		LPWSTR fontSubPath_;
		Frame mineral_;
		Frame vespene_;
		Frame supply_;
		Frame time_;
		int width_;
		int height_;

	public:
		Resolution(int width, int height, LPCSTR fontPath);
		~Resolution();

		void addFrame(LPCSTR name, int left, int top, int width, int fontIndex);

		//LPCSTR getName() { return name_;  }
		PFrame getMineral() { return &mineral_; }
		PFrame getVespene() { return &vespene_; }
		PFrame getSupply() { return &supply_; }
		PFrame getTime() { return &time_; }
		LPCWSTR getFontSubPath() { return fontSubPath_; }
		int getWidth() { return width_; }
		int getHeight() { return height_; }
	};

	struct Panel
	{
		int FontSize;
		int FontColor;
		int AlignH;
		int AlignV;
		int MarginV;
		int MarginH;
		int Outline;
	};

	enum Race
	{
		None = 0,
		Terran = 1,
		Zerg = 2,
		Protoss = 4,
		Random = 8
	};

	class Settings
	{
	private:
		TCHAR fileName_[MAX_PATH];
		std::vector<Resolution*> resList_;
		std::vector<OcrFont*> fontList_;
		std::vector<Sound*> soundList_;
		std::vector<Hotkey*> hotkeyList_;
		bool isLoaded_;
		Race player_;
		Panel buildOrder_;
		Panel tips_;
		int tipsTimeout_;
		bool mute_;
		bool hide_;
		int stepsCount_;

		void clear();
		void readPanel(void* obj, LPCSTR name, Panel* panel);
		int getFontIndex(LPCSTR name);

	public:
		Settings(LPCWSTR configFileName);
		~Settings();

		void load();

		bool getIsLoaded() { return isLoaded_; }
		Resolution* getResolution(int width, int height);
		int getFontCount() { return fontList_.size(); }
		OcrFont* getFont(int index) { return fontList_[index]; }
		LPCSTR getSoundFileName(LPCSTR action);
		Hotkey* getHotKey(LPCSTR name);
		Panel* getPanelBuildOrder() { return &buildOrder_; }
		Panel* getPanelTips() { return &tips_; }
		Race getPlayer() { return player_; }
		int getTipsTimeout() { return tipsTimeout_; }
		int getStepsCount() { return stepsCount_; }
		bool getMute() { return mute_; }
		bool getHide() { return hide_; }
	};

	// none, res (с ожиданием), res- (без ожидания), sup, time, use
	// cond = res + sup - default
	// all = res + sup + time
	#define WT_DEFAULT (WaitType)(WaitType::WT_RES | WaitType::WT_SUP | WaitType::WT_RESUSE)
	#define WT_ALL (WaitType)255
	enum WaitType: byte {
		WT_NONE = 0,
		WT_RES = 1,
		WT_SUP = 2,
		WT_TIME = 4,
		WT_RESUSE = 8,
	};

	class Build;

	class BuildStep
	{
	private:
		int time_;
		char* timeStr_;
		int supply_;
		int mineral_;
		int vespene_;
		std::vector<char*> actionList_;
		std::vector<LPWSTR> nextBuildFileNameList_;
		std::vector<Build*> nextBuildList_;
		WaitType wait_;
		char* actionDebug_;
		char* desc_;

	public:
		BuildStep(LPCSTR time, int supply, int mineral, int vespene, WaitType wait, LPCSTR action, LPCSTR desc);
		~BuildStep();


		void setNextBuilds(LPCSTR builds);
		void addNextBuild(Build* build);
		bool existBuild(Build* build);

		int getTime() { return time_; }
		LPCSTR getTimeStr() { return timeStr_; }
		int getSupply() { return supply_; }
		int getMineral() { return mineral_; }
		int getVespene() { return vespene_; }
		WaitType getWait() { return wait_; }
		LPCSTR getActionDebug() { return actionDebug_; }
		int getActionCount() { return actionList_.size(); }
		LPCSTR getAction(int index) { return actionList_[index]; }
		int getBuildFileNameCount() { return nextBuildFileNameList_.size(); }
		LPCWSTR getBuildFileName(int index) { return nextBuildFileNameList_[index]; }
		LPCSTR getDesc() { return desc_; }
	};

	class Build
	{
	private:
		TCHAR fileName_[MAX_PATH];
		char name_[BUILD_NAME_SIZE + 1];
		Race player_; // set?
		Race opponent_; // set?
		std::vector<BuildStep*> stepList_;
		char maxTime[TIME_MAX_LEN + 1];

	public:
		~Build();

		void load(LPCWSTR path, LPCWSTR fileName);
		BuildStep* add(LPCSTR time, int supply, int mineral, int vespene, WaitType wait, LPCSTR action, LPCSTR desc);

		bool available(Race player, Race opponent);
		int getCount() { return stepList_.size(); }
		BuildStep* getStep(int index) { return stepList_[index]; }
		LPCSTR getName() { return name_;  }
		Race getPlayer() { return player_; }
		Race getOpponent() { return opponent_; }
		LPCSTR getMaxTime() { return maxTime; }
		LPCWSTR getFileName() { return fileName_; }
	};

	class BuildManager
	{
	private:
		TCHAR path_[MAX_PATH];
		std::vector<Build*> buildList_;
		TCHAR lastFileName_[MAX_PATH];

		void clear();
		void add(Build* build);
		Build* getBuild(LPCWSTR fileName);

	public:
		BuildManager(LPCWSTR path);
		~BuildManager();

		BOOL load();
		int getBuildCount(Race player, Race opponent);
		Build* getPrevBuild(Race player, Race opponent, int *index);
		Build* getNextBuild(Race player, Race opponent, int *index);
		Build* getBuidlDebug();
		LPCWSTR getLastFileName() { return lastFileName_; }
	};

	LPCSTR raceToStr(Race race);
#ifdef CHECK_MEMLEAK
	void finalization();
#endif
}
