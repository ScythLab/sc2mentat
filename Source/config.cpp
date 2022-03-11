//#define NDEBUG 
#include "config.h"
#include "log.h"
#include <map>
#include <string>
#include <iostream>

// Перехват ошибок ассертов в библиотеке rapidjson.
bool assertion_failed(char const * expr, char const * file, long line);
#define RAPIDJSON_ASSERT(expression) (void)(                                \
            (!!(expression)) ||                                             \
            (assertion_failed(#expression, __FILE__, (unsigned)(__LINE__))) \
        )
#define SET_LAST_ACTION(...) sprintf(lastAction, __VA_ARGS__)
char lastAction[1024] = { 0 };

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

// Разделитель действий в билдордере
#define ACTION_SEPARATOR ','
// Разделитель списка следующих билдов в билдордере
#define NEXTBUILD_SEPARATOR '|'

// Расширение файлов-билдов
#define BUILD_FILE_EXT ".txt"
#define BUILD_FILE_MASK L"*.txt"

// Названия ключей конфига.
#define KEY_SECTION_GENERAL "general"
#define KEY_SECTION_PANELS "panels"
#define KEY_SECTION_FONTS "fonts"
#define KEY_SECTION_RESOLUTIONS "resolutions"
#define KEY_SECTION_SOUNDS "sounds"
#define KEY_SECTION_HOTKEYS "hotkeys"
#define KEY_GENERAL_PLAYER_RACE "player.race"
#define KEY_GENERAL_TIPS_TIMEOUT "tips.timeout"
#define KEY_GENERAL_MUTE "mute"
#define KEY_GENERAL_HIDE "hide"
#define KEY_GENERAL_STEP_COUNT "buildorder.steps.count"
#define KEY_PANELS_BO "buildorder"
#define KEY_PANELS_TIPS "tips"
#define KEY_PANEL_TEXTSIZE "text.size"
#define KEY_PANEL_TEXTCOLOR "text.color"
#define KEY_PANEL_OUTLINE "outline"
#define KEY_PANEL_ALIGNH "align.horizontal"
#define KEY_PANEL_ALIGNV "align.vertical"
#define KEY_PANEL_MARGINH "margin.horizontal"
#define KEY_PANEL_MARGINV "margin.vertical"
#define KEY_FONT_ID "id"
#define KEY_FONT_WHITE "white"
#define KEY_FONT_POWER "power"
#define KEY_FONT_ALPHABET "alphabet"
#define KEY_FONT_IMAGES "images"
#define KEY_RES_WIDTH "width"
#define KEY_RES_HEIGHT "height"
#define KEY_RES_FONTPATH "font.path"
#define KEY_FRAMES_MINERAL "mineral"
#define KEY_FRAMES_VESPENE "vespene"
#define KEY_FRAMES_SUPPLY "supply"
#define KEY_FRAMES_TIME "time"
#define KEY_FRAME_LEFT "left"
#define KEY_FRAME_TOP "top"
#define KEY_FRAME_WIDTH "width"
#define KEY_FRAME_FONT "font"
#define KEY_BUILD_NAME "name"
#define KEY_BUILD_PLAYER "player"
#define KEY_BUILD_OPPONENT "opponent"
#define KEY_BUILD_COMMENT "comment"
#define KEY_BUILD_STEPS "build"
#define KEY_STEP_TIME "time"
#define KEY_STEP_SUPPLY "supply"
#define KEY_STEP_MINERAL "mineral"
#define KEY_STEP_VESPENE "vespene"
#define KEY_STEP_WAIT "wait"
#define KEY_STEP_ACTION "action"
#define KEY_STEP_DESC "desc"
#define KEY_STEP_NEXTBUILDS "builds"

#define ACTION_FONT "Font #%d"
#define ACTION_FONT2 "Font %s (#%d)"
#define ACTION_RES "Resolutions #%d"
#define ACTION_RES2 "Resolutions[%dx%d]. Frame: %s"
#define ACTION_SOUND "Sound #%d"
#define ACTION_HOTKEY "Hotkey #%d"
#define ACTION_PANEL "Panel '%s'"

#define FORMAT_HEX "%x"

// Настройки по умолчанию.
#define DEFAULT_TIPS_TIMEOUT 7000
#define DEFAULT_STEPS_COUNT 2

// Обработчик ошибок ассертов в библиотеке rapidjson.
bool assertion_failed(char const * expr, char const * file, long line)
{
	//throw std::runtime_error(std::format("%s:%d", file, line));
	throw std::runtime_error(std::format(lng::string(lng::LS_CONFIG_LASTACTION), lastAction));
}

namespace cfg
{
	std::map<std::string, byte> MAP_WAIT_VALUES = {
		{ "none",     WT_NONE },
		{ "res",      WT_RES | WT_RESUSE },
		{ "resonly",  WT_RES },
		{ "sup",      WT_SUP },
		{ "time",     WT_TIME | WT_RES | WT_SUP },
		{ "timeonly", WT_TIME },
		{ "use",      WT_RESUSE },
		{ "all",      WT_ALL },
	};
	std::map<std::string, int> MAP_ALIGN_VALUES = {
		{ "left",   -1 },
		{ "center",  0 },
		{ "right",   1 },
		{ "top",    -1 },
		{ "center",  0 },
		{ "bottom",  1 },
	};

	#define RES_FRAMES_COUNT 4
	LPCSTR RES_FRAME_NAMES[RES_FRAMES_COUNT] = { KEY_FRAMES_MINERAL, KEY_FRAMES_VESPENE, KEY_FRAMES_SUPPLY, KEY_FRAMES_TIME };

	char* readFile(LPCWSTR fileName)
	{
		HANDLE hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
		if (INVALID_HANDLE_VALUE == hFile)
			return NULL;

		int fileSize = GetFileSize(hFile, NULL);
		char* data = (char*)malloc(fileSize + 1);
		memset(data, 0, fileSize + 1);
		ReadFile(hFile, data, fileSize, NULL, NULL);
		CloseHandle(hFile);

		return data;
	}

	byte getWaitValue(LPCSTR str)
	{
		std::map<std::string, byte>::iterator it = MAP_WAIT_VALUES.find(str);
		if (MAP_WAIT_VALUES.end() == it)
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_WAITTYPE_INVALID), str));
		return it->second;
	}

	int getAlignValue(LPCSTR str)
	{
		char* s = strlwr(createTrimString(str));
		std::map<std::string, int>::iterator it = MAP_ALIGN_VALUES.find(s);
		free(s);
		if (MAP_ALIGN_VALUES.end() == it)
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_ALIGN_INVALID), str));
		return it->second;
	}

	WaitType strToWait(LPCSTR waitStr)
	{
		if (!waitStr || !*waitStr)
			return WT_DEFAULT;

		byte wt = WT_NONE;
		char* ws = strlwr(removeSpaces(createTrimString(waitStr)));
		//try
		//{
			char* w = ws;
			while (w)
			{
				char* sep = strchr(w, ',');
				if (sep)
				{
					*sep = 0;
					sep++;
				}

				byte value = getWaitValue(w);
				wt |= value;
				w = sep;
			}

		//}
		//catch (std::exception e)
		//{
		//	free(ws);
		//	throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_WAITTYPE_EXCEPTION), waitStr));
		//}
		free(ws);

		return (WaitType)wt;
	}

	Race strToRace(LPCSTR str, bool single = false)
	{
		byte race = Race::None;
		for (size_t i = 0; i < strlen(str); i++)
		{
			if ('T' == str[i] || 't' == str[i])
				race |= Race::Terran;
			else if ('Z' == str[i] || 'z' == str[i])
				race |= Race::Zerg;
			else if ('P' == str[i] || 'p' == str[i])
				race |= Race::Protoss;
			else if ('R' == str[i] || 'r' == str[i])
				race |= Race::Random;
			else
				throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_RACE_INVALID), str));

			if (single && Race::None != race)
				break;
		}
		return (Race)race;
	}

	LPCSTR raceToStr(Race race)
	{
		switch (race)
		{
		case Race::Terran:
			return "T";

		case Race::Zerg:
			return "Z";

		case Race::Protoss:
			return "P";

		case Race::Random:
			return "R";

		default:
			return "?";
		};
	}

	LPCSTR getJsonStr(rapidjson::Value& value, LPCSTR name)
	{
		rapidjson::Value::MemberIterator member = value.FindMember(name);
		if (member != value.MemberEnd())
			return member->value.GetString();

		return NULL;
	}

	int getJsonInt(rapidjson::Value& value, LPCSTR name, int defValue = -1)
	{
		rapidjson::Value::MemberIterator member = value.FindMember(name);
		if (member != value.MemberEnd())
			return member->value.GetInt();

		return defValue;
	}

	// DEBUG
	//std::string GetElementValue(const rapidjson::Value& val)
	//{
	//	if (val.GetType() == rapidjson::Type::kNumberType)
	//		return std::to_string(val.GetInt());
	//	else if (val.GetType() == rapidjson::Type::kStringType)
	//		return val.GetString();
	//	else if (val.GetType() == rapidjson::Type::kArrayType)
	//		return "Array";
	//	else if (val.GetType() == rapidjson::Type::kObjectType)
	//		return "Object";
	//	return "Unknown";
	//}

	//---------------------------------------------------------------------------//
	//---------------------------------- Sound ----------------------------------//
	//---------------------------------------------------------------------------//

	Sound::Sound(LPCSTR action, LPCSTR fileName)
	{
		action_ = createTrimString(action);
		fileName_ = strlwr(createTrimString(fileName));
	}

	Sound::~Sound()
	{
		free(action_);
		free(fileName_);
	}

	//---------------------------------------------------------------------------//
	//------------------------------- Resolution --------------------------------//
	//---------------------------------------------------------------------------//

	Resolution::Resolution(int width, int height, LPCSTR fontPath)
	{
		if (width < RESOLUTION_MIN_WIDTH || height <= RESOLUTION_MIN_HEIGHT)
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_RESOLUTION_TOOSMALL), RESOLUTION_MIN_WIDTH, RESOLUTION_MIN_HEIGHT));

		width_ = width;
		height_ = height;
		fontSubPath_ = strToWStr(fontPath);
	}

	Resolution::~Resolution()
	{
		free(fontSubPath_);
	}

	void Resolution::addFrame(LPCSTR name, int left, int top, int width, int fontIndex)
	{
		if (strlen(name) > CONFIG_NAME_SIZE)
			// Этот код не должен выполняться, т.к. сюда должны попадать только заданные фреймы.
			return;

		char frameName[CONFIG_NAME_SIZE + 1];
		strcpy(frameName, name);
		strlwr(frameName);

		Frame* lpFrame = NULL;
		if (!strcmp(frameName, KEY_FRAMES_MINERAL))
			lpFrame = &mineral_;
		else if(!strcmp(frameName, KEY_FRAMES_VESPENE))
			lpFrame = &vespene_;
		else if (!strcmp(frameName, KEY_FRAMES_SUPPLY))
			lpFrame = &supply_;
		else if (!strcmp(frameName, KEY_FRAMES_TIME))
			lpFrame = &time_;

		if (!lpFrame)
			// Этот код не должен выполняться, т.к. сюда должны попадать только заданные фреймы.
			return;
		
		strcpy(lpFrame->name_, frameName);
		lpFrame->left_ = left;
		lpFrame->top_ = top;
		lpFrame->width_ = width;
		lpFrame->fontIndex_ = fontIndex;
	}

	//---------------------------------------------------------------------------//
	//---------------------------------- Font -----------------------------------//
	//---------------------------------------------------------------------------//

	OcrFont::OcrFont(LPCSTR name, LPCSTR alphabet, int power, int white)
	{
		if (strlen(alphabet) > power)
			throw std::invalid_argument(std::format(lng::string(lng::LS_FONT_SIZE_INVALID), alphabet, power));
		if (strlen(name) > CONFIG_NAME_SIZE)
			throw std::invalid_argument(lng::string(lng::LS_CONFIG_FONT_NAMELONG));

		strcpy(name_, name);
		strlwr(name_);
		alphabet_ = (char*)malloc(power + 1);
		memset(alphabet_, 0, power + 1);
		strcpy(alphabet_, alphabet);

		power_ = power;
		white_ = white;
	}

	OcrFont::~OcrFont()
	{
		free(alphabet_); alphabet_ = NULL;
		SafeVectorDelete(fileNames_);
	}

	void OcrFont::addFile(LPCSTR fileName)
	{
		fileNames_.push_back(strToWStr(fileName));
	}

	//---------------------------------------------------------------------------//
	//-------------------------------- Settings ---------------------------------//
	//---------------------------------------------------------------------------//

	Settings::Settings(LPCWSTR configFileName)
	{
		wcscpy(fileName_, configFileName);
		isLoaded_ = false;
	}

	Settings::~Settings()
	{
		clear();
	}

	void Settings::clear()
	{
		isLoaded_ = false;
		SafeVectorDelete(resList_);
		SafeVectorDelete(fontList_);
		SafeVectorDelete(soundList_);
		SafeVectorDelete(hotkeyList_);
	}

	void readBool(rapidjson::Value& obj, LPCSTR name, bool* value, bool defValue = false)
	{
		if (obj.HasMember(name))
			*value = (obj[name].GetInt() > 0);
		else
			*value = defValue;
	}

	void readInt(rapidjson::Value& obj, LPCSTR name, int* value, int defValue = 0)
	{
		if (obj.HasMember(name))
			*value = obj[name].GetInt();
		else
			*value = defValue;
	}

	void Settings::load()
	{
		clear();

		char* configData = readFile(fileName_);
		if (!configData)
			throw std::runtime_error(lng::string(lng::LS_FILE_OPEN_FAIL));

		rapidjson::Document document;
		document.Parse(configData);
		free(configData);

		if (!document.IsObject())
			throw std::runtime_error(lng::string(lng::LS_FILE_INVALID));

		rapidjson::Value::ConstMemberIterator iter;

		// Шрифты
		if (!document.HasMember(KEY_SECTION_FONTS))
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_KEY_MISSED), KEY_SECTION_FONTS));
		rapidjson::Value& fontList = document[KEY_SECTION_FONTS];
		if (!fontList.IsArray())
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_KEY_ISNTARRAY), KEY_SECTION_FONTS));

		for (rapidjson::SizeType iFont = 0; iFont < fontList.Size(); iFont++)
		{
			SET_LAST_ACTION(ACTION_FONT, iFont);

			rapidjson::Value& font = fontList[iFont];

			LPCSTR name = font[KEY_FONT_ID].GetString();
			SET_LAST_ACTION(ACTION_FONT2, name, iFont);

			int white = font[KEY_FONT_WHITE].GetInt();
			int power = font[KEY_FONT_POWER].GetInt();
			LPCSTR alphabet = font[KEY_FONT_ALPHABET].GetString();

			OcrFont* f = new OcrFont(name, alphabet, power, white);
			fontList_.push_back(f);

			rapidjson::Value& imageList = font[KEY_FONT_IMAGES];
			for (rapidjson::SizeType iImg = 0; iImg < imageList.Size(); iImg++)
			{
				f->addFile(imageList[iImg].GetString());
			}
		}

		// Разрешения
		if (!document.HasMember(KEY_SECTION_RESOLUTIONS))
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_KEY_MISSED), KEY_SECTION_RESOLUTIONS));
		SET_LAST_ACTION(KEY_SECTION_RESOLUTIONS);
		rapidjson::Value& resList = document[KEY_SECTION_RESOLUTIONS];
		if (!resList.IsArray())
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_KEY_ISNTARRAY), KEY_SECTION_RESOLUTIONS));
		for (rapidjson::SizeType iRes = 0; iRes < resList.Size(); iRes++)
		{
			SET_LAST_ACTION(ACTION_RES, iRes);

			rapidjson::Value& res = resList[iRes];
			int w = res[KEY_RES_WIDTH].GetInt();
			int h = res[KEY_RES_HEIGHT].GetInt();
			LPCSTR fontPath = res[KEY_RES_FONTPATH].GetString();

			Resolution* resolution = new Resolution(w, h, fontPath);
			resList_.push_back(resolution);

			for (int iFrame = 0; iFrame < RES_FRAMES_COUNT; iFrame++)
			{
				LPCSTR name = RES_FRAME_NAMES[iFrame];
				if (!res.HasMember(name))
					throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_RESOLUTION_FRAME_MISSED), name, w, h));

				SET_LAST_ACTION(ACTION_RES2, w, h, name);
				rapidjson::Value& frame = res[name];
				int left = frame[KEY_FRAME_LEFT].GetInt();
				int top = frame[KEY_FRAME_TOP].GetInt();
				int width = frame[KEY_FRAME_WIDTH].GetInt();
				LPCSTR fontName = frame[KEY_FRAME_FONT].GetString();
				int fontIndex = getFontIndex(fontName);
				if (-1 == fontIndex)
					throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_GLOBAL_FONT_NOTFOUND), fontName));

				resolution->addFrame(name, left, top, width, fontIndex);
			}
		}

		// Звуки.
		SET_LAST_ACTION(KEY_SECTION_SOUNDS);
		if (document.HasMember(KEY_SECTION_SOUNDS))
		{
			rapidjson::Value& soundList = document[KEY_SECTION_SOUNDS];
			if (!soundList.IsArray())
				throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_KEY_ISNTARRAY), KEY_SECTION_SOUNDS));

			for (rapidjson::SizeType iSound = 0; iSound < soundList.Size(); iSound++)
			{
				SET_LAST_ACTION(ACTION_SOUND, iSound);

				rapidjson::Value& sound = soundList[iSound];

				rapidjson::Value::ConstMemberIterator itrSound = sound.MemberBegin();
				for (int soundIndex = 0; itrSound != sound.MemberEnd(); ++itrSound, soundIndex++)
				{
					LPCSTR action = itrSound->name.GetString();
					LPCSTR fileName = itrSound->value.GetString();
					//log_dbg_printf(" name: %s; file: %s", action, fileName);
					Sound* s = new Sound(action, fileName);
					soundList_.push_back(s);
				}
			}
		}

		// Горячие клавиши
		if (!document.HasMember(KEY_SECTION_HOTKEYS))
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_KEY_MISSED), KEY_SECTION_HOTKEYS));
		SET_LAST_ACTION(KEY_SECTION_HOTKEYS);
		rapidjson::Value& hkList = document[KEY_SECTION_HOTKEYS];
		rapidjson::Value::ConstMemberIterator itrHk = hkList.MemberBegin();
		for (int hkIndex = 0; itrHk != hkList.MemberEnd(); ++itrHk, hkIndex++)
		{
			SET_LAST_ACTION(ACTION_HOTKEY, hkIndex);
			LPCSTR name = itrHk->name.GetString();
			LPCSTR value = itrHk->value.GetString();
			//log_dbg_printf("%s = %s", name, value);

			// TODO: Проверить допустимость имени.
			char* keys = removeSpaces(createTrimString(value));
			bool isEmpty = false;
			if (!*keys)
			{
				//// TODO: Клавиша не задана.
				//free(keys);
				//continue;
				isEmpty = true;
			}

			char* k = strupr(keys);
			int len = strlen(keys);
			UINT virtKey = 0;
			UINT keyMod = 0;
			bool isOk = false;
			while (k)
			{
				char* sep = strchr(k, '+');
				if (sep)
				{
					*sep = 0;
					sep++;
				}

				UINT vk, mod;
				isOk = strToKey(k, &vk, &mod);
				if (!isOk)
					break;
				// Попытка два раза указать конкретную клавишу
				if (virtKey && vk)
				{
					isOk = false;
					break;
				}
				if (vk)
					virtKey = vk;
				keyMod |= mod;

				k = sep;
			}
			free(keys);

			// Пустые клавиши пропускаем, клавиши с ошибками - отбраковываем.
			if (!isEmpty && (!isOk || !virtKey))
				throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_GLOBAL_HOTKEY_INVALID), name, value));

			hotkeyList_.push_back(new Hotkey(name, virtKey, keyMod, value));
		}

		// Панели отображения информации
		SET_LAST_ACTION(KEY_SECTION_PANELS);
		if (!document.HasMember(KEY_SECTION_PANELS))
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_KEY_MISSED), KEY_SECTION_PANELS));
		rapidjson::Value& panels = document[KEY_SECTION_PANELS];

		if (!panels.HasMember(KEY_PANELS_BO))
			throw std::runtime_error(lng::string(lng::LS_CONFIG_GLOBAL_PANELS_BO_MISSED));
		if (!panels.HasMember(KEY_PANELS_TIPS))
			throw std::runtime_error(lng::string(lng::LS_CONFIG_GLOBAL_PANELS_TIPS_MISSED));
		readPanel(&panels, KEY_PANELS_BO, &buildOrder_);
		readPanel(&panels, KEY_PANELS_TIPS, &tips_);

		// Базовые настройки
		SET_LAST_ACTION(KEY_SECTION_GENERAL);
		player_ = Race::None;
		tipsTimeout_ = DEFAULT_TIPS_TIMEOUT;
		mute_ = false;
		hide_ = false;
		stepsCount_ = DEFAULT_STEPS_COUNT;
		if (document.HasMember(KEY_SECTION_GENERAL))
		{
			rapidjson::Value& general = document[KEY_SECTION_GENERAL];

			if (general.HasMember(KEY_GENERAL_PLAYER_RACE))
			{
				player_ = strToRace(general[KEY_GENERAL_PLAYER_RACE].GetString(), true);
			}

			readInt(general, KEY_GENERAL_TIPS_TIMEOUT, &tipsTimeout_);
			if (tipsTimeout_ <= 0)
				tipsTimeout_ = DEFAULT_TIPS_TIMEOUT;
			readBool(general, KEY_GENERAL_MUTE, &mute_);
			readBool(general, KEY_GENERAL_HIDE, &hide_);
			readInt(general, KEY_GENERAL_STEP_COUNT, &stepsCount_);
			if (stepsCount_ <= 0)
				stepsCount_ = DEFAULT_STEPS_COUNT;
		}
			
		// TODO: Локализация действий.

		SET_LAST_ACTION("");
		isLoaded_ = true;
	}

	Hotkey* Settings::getHotKey(LPCSTR name)
	{
		if (!name)
			return NULL;

		for (size_t i = 0; i < hotkeyList_.size(); i++)
		{
			Hotkey* hk = hotkeyList_[i];
			if (!strcmp(hk->name_, name))
				return hk;
		}

		return NULL;
	}

	LPCSTR Settings::getSoundFileName(LPCSTR action)
	{
		if (!action)
			return NULL;

		for (size_t i = 0; i < soundList_.size(); i++)
		{
			if (!strcmp(action, soundList_[i]->getAction()))
				return soundList_[i]->getFileName();
		}

		return NULL;
	}

	Resolution* Settings::getResolution(int width, int height)
	{
		for (size_t i = 0; i < resList_.size(); i++)
		{
			Resolution* res = resList_[i];
			if (res->getWidth() == width && res->getHeight() == height)
				return res;
		}

		return NULL;
	}

	int Settings::getFontIndex(LPCSTR name)
	{
		if (strlen(name) > CONFIG_NAME_SIZE)
			return -1;

		char nameLwr[CONFIG_NAME_SIZE + 1];
		strcpy(nameLwr, name);
		strlwr(nameLwr);

		for (size_t i = 0; i < fontList_.size(); i++)
		{
			OcrFont* font = fontList_[i];
			if (!strcmp(font->getName(), nameLwr))
				return i;
		}

		return -1;
	}

	void Settings::readPanel(void* obj, LPCSTR name, Panel* panel)
	{
		SET_LAST_ACTION(ACTION_PANEL, name);
		rapidjson::Value* panels = (rapidjson::Value*)obj;

		rapidjson::Value& pnl = (*panels)[name];
		panel->FontSize = pnl[KEY_PANEL_TEXTSIZE].GetInt();
		LPCSTR color = pnl[KEY_PANEL_TEXTCOLOR].GetString();
		if (!sscanf(color, FORMAT_HEX, &panel->FontColor))
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_GLOBAL_PANELS_COLOR_INVALID), name, color));
		panel->AlignH = getAlignValue(pnl[KEY_PANEL_ALIGNH].GetString());
		panel->AlignV = getAlignValue(pnl[KEY_PANEL_ALIGNV].GetString());
		panel->MarginH = pnl[KEY_PANEL_MARGINH].GetInt();
		panel->MarginV = pnl[KEY_PANEL_MARGINV].GetInt();
		int maxOutline = max(1, panel->FontSize / 8);
		panel->Outline = min(maxOutline, pnl[KEY_PANEL_OUTLINE].GetInt());
	}

	//---------------------------------------------------------------------------//
	//------------------------------ BuildManager -------------------------------//
	//---------------------------------------------------------------------------//

	BuildManager::BuildManager(LPCWSTR path)
	{
		//DirectoryExists()
		wcscpy(path_, path);
		includeTrailingPathDelimiter(path_);
	}

	BuildManager::~BuildManager()
	{
		clear();
	}

	void BuildManager::clear()
	{
		SafeVectorDelete(buildList_);
	}

	void BuildManager::add(Build* build)
	{
		// Сортировка билдов по имени файлов.
		LPCWSTR curName = build->getFileName();
		std::vector<Build*>::iterator it = buildList_.begin();
		while (it != buildList_.end())
		{
			LPCWSTR name = it[0]->getFileName();
			int cmp = wcscmp(curName, name);
			if (cmp < 0)
			{
				it = buildList_.insert(it, build);
				break;
			}

			it++;
		}
		if (buildList_.end() == it)
			buildList_.push_back(build);
	}

	Build* BuildManager::getBuild(LPCWSTR fileName)
	{
		WCHAR lwrFileName[MAX_PATH], lwrBuildName[MAX_PATH];
		wcscpy(lwrFileName, fileName);
		wcslwr(lwrFileName);
		for (size_t iB = 0; iB < buildList_.size(); iB++)
		{
			Build* build = buildList_[iB];
			wcscpy(lwrBuildName, build->getFileName());
			wcslwr(lwrBuildName);

			if (0 == wcscmp(lwrFileName, lwrBuildName))
				return build;
		}

		return NULL;
	}

	BOOL BuildManager::load()
	{
		clear();

		TCHAR fileName[MAX_PATH];
		wcscpy(fileName, path_);
		wcscat(fileName, BUILD_FILE_MASK);
		WIN32_FIND_DATAW data;
		HANDLE hFind = FindFirstFile(fileName, &data);
		if (INVALID_HANDLE_VALUE == hFind)
		{
			log_printidf(lng::LS_CONFIG_BUILDS_FIND_FAIL, GetLastError());
			return FALSE;
		}

		BOOL success = TRUE;
		do
		{
			wcscpy(lastFileName_, data.cFileName);
			Build* build = new Build();
			try
			{
				build->load(path_, data.cFileName);
				add(build);
			}
			catch (std::exception& e)
			{
				success = FALSE;
				log_printidf(lng::LS_CONFIG_BUILDS_ERROR_FILE, data.cFileName, e.what());
				delete build;
			}
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);

		log_printidf(lng::LS_CONFIG_BUILDS_LOADED, buildList_.size());

		// Определим ссылки между билдами
		for (size_t iB = 0; iB < buildList_.size(); iB++)
		{
			Build* build = buildList_[iB];
			for (int iBS = 0; iBS < build->getCount(); iBS++)
			{
				BuildStep* bs = build->getStep(iBS);
				for (int iF = 0; iF < bs->getBuildFileNameCount(); iF++)
				{
					LPCWSTR fileName = bs->getBuildFileName(iF);
					Build* nextBuild = getBuild(fileName);
					bs->addNextBuild(nextBuild);
				}
			}
		}

		return success;
	}

	int BuildManager::getBuildCount(Race player, Race opponent)
	{
		int count = 0;
		for (size_t i = 0; i < buildList_.size(); i++)
		{
			Build* b = buildList_[i];
			if (b->available(player, opponent))
				count++;
		}

		return count;
	}

	Build* BuildManager::getPrevBuild(Race player, Race opponent, int *index)
	{
		int size = buildList_.size();
		if (!size)
			return NULL;

		int i = (*index < 0 || *index >= size) ? size - 1 : *index - 1;
		for (int counter = 0; counter < size; counter++)
		{
			if (i < 0)
				i = size - 1;

			Build* b = buildList_[i];
			if (b->available(player, opponent))
			{
				*index = i;
				return b;
			}

			i--;
		}

		return NULL;
	}

	Build* BuildManager::getNextBuild(Race player, Race opponent, int *index)
	{
		int size = buildList_.size();
		if (!size)
			return NULL;

		int i = (*index < 0 || *index >= size) ? 0 : *index + 1;
		for (int counter = 0; counter < size; counter++)
		{
			if (i >= size)
				i = 0;

			Build* b = buildList_[i];
			if (b->available(player, opponent))
			{
				*index = i;
				return b;
			}

			i++;
		}

		return NULL;
	}

	Build* BuildManager::getBuidlDebug()
	{
		return (buildList_.size()) ? buildList_[0] : NULL;
	}

	//---------------------------------------------------------------------------//
	//---------------------------------- Build ----------------------------------//
	//---------------------------------------------------------------------------//

	Build::~Build()
	{
		SafeVectorDelete(stepList_);
	}

	void Build::load(LPCWSTR path, LPCWSTR fileName)
	{
		SET_LAST_ACTION("");

		wcscpy(fileName_, fileName);
		// Удалим расширение.
		LPWSTR ext = wcsrchr(fileName_, '.');
		if (ext)
			*ext = 0;

		TCHAR fullName[MAX_PATH];
		wcscpy(fullName, path);
		wcscat(fullName, fileName);

		char* configData = readFile(fullName);
		if (!configData)
			throw std::invalid_argument(lng::string(lng::LS_FILE_OPEN_FAIL));

		rapidjson::Document document;
		document.Parse(configData);
		free(configData);

		if (!document.IsObject())
			throw std::invalid_argument(lng::string(lng::LS_FILE_INVALID));

		if (!document.HasMember(KEY_BUILD_NAME))
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_KEY_MISSED), KEY_BUILD_NAME));
		if (!document.HasMember(KEY_BUILD_PLAYER))
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_KEY_MISSED), KEY_BUILD_PLAYER));
		if (!document.HasMember(KEY_BUILD_STEPS))
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_KEY_MISSED), KEY_BUILD_STEPS));

		LPCSTR name = document[KEY_BUILD_NAME].GetString();
		if (strlen(name) > BUILD_NAME_SIZE)
			throw std::invalid_argument(lng::string(lng::LS_CONFIG_BUILD_NAME_LONG));
		strcpy(name_, name);

		LPCSTR player = document[KEY_BUILD_PLAYER].GetString();
		LPCSTR opponent = document[KEY_BUILD_OPPONENT].GetString();
		player_ = strToRace(player);
		opponent_ = strToRace(opponent);
		if (Race::None == player_ || Race::None == opponent_)
			throw std::invalid_argument(lng::string(lng::LS_CONFIG_BUILD_RACE_EMPTY));

		//log_dbg_printf("name: %s; player: %x; opponent: %x", name_, player_, opponent_);

		rapidjson::Value::ConstMemberIterator iter;

		rapidjson::Value& build = document[KEY_BUILD_STEPS];
		if (!build.IsArray())
			throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_KEY_ISNTARRAY), KEY_BUILD_STEPS));
		for (rapidjson::SizeType iBuild = 0; iBuild < build.Size(); iBuild++)
		{
			SET_LAST_ACTION(lng::string(lng::LS_CONFIG_BUILD_STEP), iBuild);

			rapidjson::Value& step = build[iBuild];

			LPCSTR time = getJsonStr(step, KEY_STEP_TIME);
			SET_LAST_ACTION(lng::string(lng::LS_CONFIG_BUILD_STEP2), iBuild, time);
			LPCSTR action = getJsonStr(step, KEY_STEP_ACTION);
			SET_LAST_ACTION(lng::string(lng::LS_CONFIG_BUILD_STEP3), iBuild, time, action);

			int supply = getJsonInt(step, KEY_STEP_SUPPLY);
			int mineral = getJsonInt(step, KEY_STEP_MINERAL);
			int vespene = getJsonInt(step, KEY_STEP_VESPENE);
			LPCSTR waitStr = getJsonStr(step, KEY_STEP_WAIT);
			LPCSTR desc = getJsonStr(step, KEY_STEP_DESC);

			BuildStep* bs = add(time, supply, mineral, vespene, strToWait(waitStr), action, desc);
			LPCSTR nextBuilds = getJsonStr(step, KEY_STEP_NEXTBUILDS);
			if (nextBuilds)
			{
				bs->setNextBuilds(nextBuilds);
			}
		}

		// Запомним максимальное время билда
		secondsToTime(0, maxTime);
		for (int i = stepList_.size() - 1; i >= 0; i--)
		{
			BuildStep* bs = stepList_[i];
			int sec = bs->getTime();
			if (sec >= 0)
			{
				secondsToTime(sec, maxTime);
				break;
			}
		}

		SET_LAST_ACTION("");
	}

	BuildStep* Build::add(LPCSTR time, int supply, int mineral, int vespene, WaitType wait, LPCSTR action, LPCSTR desc)
	{
		//log_dbg_printf("[%6s] %3d; %3d; %3d; %s / %s", time, supply, mineral, vespene, action, desc);
		BuildStep* bs = new BuildStep(time, supply, mineral, vespene, wait, action, desc);
		stepList_.push_back(bs);
		return bs;
	}

	bool Build::available(Race player, Race opponent)
	{
		return (player & player_ && opponent & opponent_);
	}

	//---------------------------------------------------------------------------//
	//-------------------------------- BuildStep --------------------------------//
	//---------------------------------------------------------------------------//

	BuildStep::BuildStep(LPCSTR time, int supply, int mineral, int vespene, WaitType wait, LPCSTR action, LPCSTR desc)
	{
		if (!time)
			time_ = -1;
		else
		{
			time_ = timeToSecond(time);
			if (-1 == time_)
				throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_BUILD_TIME_INVALID), time));
		}

		char part[CONFIG_NAME_SIZE + 1];

		timeStr_ = createTrimString(time);
		supply_ = supply;
		mineral_ = mineral;
		vespene_ = vespene;
		wait_ = wait;
		desc_ = createTrimString(desc);

		// Действия
		actionDebug_ = createTrimString(action);

		if (action)
		{
			LPCSTR lpAct = action;
			LPCSTR lpEnd = action + strlen(action);
			while (lpAct < lpEnd)
			{
				int len = strlen(lpAct);
				LPCSTR end = strchr(lpAct, ACTION_SEPARATOR);
				int partLen = (end) ? end - lpAct : len;
				if (partLen > CONFIG_NAME_SIZE)
					throw std::invalid_argument(std::format(lng::string(lng::LS_CONFIG_ACTION_NAME_LONG), action));
				memcpy(part, lpAct, partLen);
				part[partLen] = 0;

				actionList_.push_back(createTrimString(part));

				lpAct += partLen + 1;
			}
		}
	}

	BuildStep::~BuildStep()
	{
		free(timeStr_);
		free(desc_);
		free(actionDebug_);
		SafeVectorDelete(actionList_);
		SafeVectorDelete(nextBuildFileNameList_);
		nextBuildList_.clear();
	}

	void BuildStep::setNextBuilds(LPCSTR builds)
	{
		if (!builds)
			return;

		char part[MAX_PATH];
		LPCSTR lpBuild = builds;
		LPCSTR lpEnd = builds + strlen(builds);
		while (lpBuild < lpEnd)
		{
			int len = strlen(lpBuild);
			LPCSTR end = strchr(lpBuild, NEXTBUILD_SEPARATOR);
			int partLen = (end) ? end - lpBuild : len;
			memcpy(part, lpBuild, partLen);
			part[partLen] = 0;
			strTrim(part);
			//strcat(, BUILD_FILE_EXT);

			// TODO: Проверить файл?
			nextBuildFileNameList_.push_back(strToWStr(part));

			lpBuild += partLen + 1;
		}
	}

	void  BuildStep::addNextBuild(Build* build)
	{
		nextBuildList_.push_back(build);
	}

	bool BuildStep::existBuild(Build* build)
	{
		for (size_t i = 0; i < nextBuildList_.size(); i++)
		{
			if (build == nextBuildList_[i])
				return true;
		}

		return false;
	}

#ifdef CHECK_MEMLEAK
	void finalization()
	{
		MAP_WAIT_VALUES.~map();
		MAP_ALIGN_VALUES.~map();
	}
#endif
}
