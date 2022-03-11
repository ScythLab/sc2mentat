#include "lang.h"
#include "resource.h"
#include <map>
#include <string>

namespace lng
{
	// По умолчанию используем английский язык.
	LANG gLocale = LANG_EN;

	STRING_TABLE STRINGS_RU = {
		{ LS_APP_EXCEPTION_TITLE,                      "SC2Mentat - Необработанное исключение" },
		{ LS_APP_RUNNED_CAPTION,                       "Приложение уже запущено" },
		{ LS_FILE_OPEN_FAIL,                           "Ошибка открытия файла" },
		{ LS_FILE_INVALID,                             "Неверный файл" },
		{ LS_CONFIG_LASTACTION,                        "Последнее действие: %s" },
		{ LS_CONFIG_KEY_MISSED,                        "Нет ключа '%s'" },
		{ LS_CONFIG_KEY_ISNTARRAY,                     "Значение ключа '%s' должно быть массивом" },
		{ LS_CONFIG_BUILDS_LOADED,                     "Загружено билдов: %d" },
		{ LS_CONFIG_BUILDS_FIND_FAIL,                  "Ошибка поиска билдов. Код ошибки: #%d" },
		{ LS_CONFIG_BUILDS_ERROR_FILE,                 "Ошибка в файле билда '%S': %s" },
		{ LS_CONFIG_BUILD_NAME_LONG,                   "Имя слишком длинное" },
		{ LS_CONFIG_BUILD_RACE_EMPTY,                  "Раса игрока/оппонента не задана" },
		{ LS_CONFIG_BUILD_TIME_INVALID,                "Неверный формат времени '%s'" },
		{ LS_CONFIG_BUILD_STEP,                        "Шаг #%d" },
		{ LS_CONFIG_BUILD_STEP2,                       "Шаг #%d (время:%s)" },
		{ LS_CONFIG_BUILD_STEP3,                       "Шаг #%d (время:%s; действие:%s)" },
		{ LS_CONFIG_WAITTYPE_INVALID,                  "Неизвестное значение Wait '%s'" },
		{ LS_CONFIG_WAITTYPE_EXCEPTION,                "Ошибка в обработке Wait '%s'" },
		{ LS_CONFIG_ALIGN_INVALID,                     "Неизвестное значение Align '%s'" },
		{ LS_CONFIG_RACE_INVALID,                      "Неизвестное значение Race '%s'" },
		{ LS_CONFIG_ACTION_NAME_LONG,                  "Название действия '%s' слишком длинное" },
		{ LS_CONFIG_RESOLUTION_TOOSMALL,               "Разрешение должно быть больше %dx%d" },
		{ LS_CONFIG_RESOLUTION_FRAME_MISSED,           "Не найден раздел '%s' в разрешении %dx%d" },
		{ LS_CONFIG_FONT_NAMELONG,                     "Слишком длинное название шрифта" },
		{ LS_CONFIG_GLOBAL_FONT_NOTFOUND,              "Не найден шрифт '%s'" },
		{ LS_CONFIG_GLOBAL_HOTKEY_INVALID,             "У горячей клавиши '%s' некорректное значение '%s'" },
		{ LS_CONFIG_GLOBAL_PANELS_BO_MISSED,           "Не найден ключ 'Panels'.'BuildOrder'" },
		{ LS_CONFIG_GLOBAL_PANELS_TIPS_MISSED,         "Не найден ключ 'Panels'.'Tips'" },
		{ LS_CONFIG_GLOBAL_PANELS_COLOR_INVALID,       "В панели '%s' некорректный цвет '%s'" },
		{ LS_HOTKEY_MISSED,                            "Не задана обязательная горячая клавиша '%s'" },
		{ LS_HOTKEY_THREAD_CREATE_FAIL,                "Ошибка в создании нити обработки горячих клавиши" },
		{ LS_HOTKEY_DUPLICATE,                         "Горячая клавиша '%s' совпадает с '%s'" },
		{ LS_HOTKEY_REGISTER_FAIL,                     "Ошибка в регистрации горячей клавиши '%s'. Код ошибки: #%d" },
		{ LS_MODULEFILE_NAME_FAIL,                     "Ошибка в функции GetModuleFileName: #%d" },
		{ LS_TRAY_REGISTERCLASS_FAIL,                  "Ошибка в регистрации класса окна трея" },
		{ LS_TRAY_CREATEWND_FAIL,                      "Ошибка в создании окна трея" },
		{ LS_TRAY_LOADMENU_FAIL,                       "Ошибка в загрузке меню трея" },
		{ LS_CONFIG_LOAD_FAIL,                         "Ошибка при загрузке конфига: %s" },
		{ LS_CONFIG_HOTKEYS_LOAD_FAIL,                 "Ошибка при загрузке горячих клавиш: %s" },
		{ LS_AUDIO_FAIL,                               "Аудио-интерфейс не создан: %s" },
		{ LS_GAME_RUNNED,                              "Игра запущена" },
		{ LS_GAME_OFF,                                 "Игра закрыта" },
		{ LS_OVERLAY_FAIL,                             "%s. Возможно, игра в полноэкранном режиме? Переключите ее в оконный режим и перезапустите SC2Mentat." },
		{ LS_OVERLAY_MANYERRORS,                       "Слишком много ошибок. Требуется перезапуск SC2Mentat." },
		{ LS_OVERLAY_FINDWND_FAIL,                     "Не найдено игровое окно '%S' с классом '%S'" },
		{ LS_OVERLAY_DEVICE_NULL,                      "Устройство dx3d9 не задано" },
		{ LS_OVERLAY_LINE_CREATE_FAIL,                 "Ошибка при создани 3D-линии" },
		{ LS_OVERLAY_SPRITE_CREATE_FAIL,               "Ошибка при создани 3D-спрайта" },
		{ LS_OVERLAY_FONT_CREATE_FAIL,                 "Ошибка при создани 3D-шрифта" },
		{ LS_OVERLAY_OBJECT_CREATE_FAIL,               "Ошибка при создани 3D-объекта" },
		{ LS_OVERLAY_DEVICE_CREATE_FAIL,               "Ошибка при создани 3D-устройства" },
		{ LS_OVERLAY_TARGETWND_EMPTY,                  "Класс и название окна игры не заданы" },
		{ LS_OVERLAY_CLASS_REGISTER_FAIL,              "Ошибка при регистрации класса окна оверлея" },
		{ LS_OVERLAY_WND_CREATE_FAIL,                  "Ошибка при создании окна оверлея" },
		{ LS_SOUND_MISSED,                             "Не задан звук для действия '%s'" },
		{ LS_SOUND_FILE_OPEN_FAIL,                     "Ошибка открытия звукового файла '%s': %s" },
		{ LS_SOUND_SOURCE_OPEN_FAIL,                   "Ошибка открытия источника звука '%s'. Кош обишки: %#X" },
		{ LS_SOUND_SOURCE_CREATE_FAIL,                 "Ошибка создания источника звука: %#X" },
		{ LS_SOUND_XAUDIO2_CREATE_FAIL,                "Ошибка инициализации XAudio2: %#X" },
		{ LS_SOUND_XAUDIO2_MASTERVOICE_CREATE_FAIL,    "Ошибка создания мастер-звука: %#X" },
		{ LS_FONT_SIZE_INVALID,                        "Неверный размер алфавита '%s': %d" },
		{ LS_FONT_LOAD_FAIL,                           "Ошибка загрузки шаблона шрифта '%S'" },
		{ LS_MSG_MUTE,                                 " БЕЗ ЗВУКА" },
		{ LS_MSG_DEBUG,                                " ОТЛАДКА" },
		{ LS_MSG_SETTINGS_INVALID,                     "Некорректные настройки" },
		{ LS_MSG_RESOLUTION_UND,                       "Не задано разрешение %dx%d" },
		{ LS_MSG_HOTKEY_MISSED,                        "Обязательные горячие клавиши не заданы" },
		{ LS_MSG_BUILD_NULL,                           "Билд не задан" },
	};
	STRING_TABLE STRINGS_EN = {
		{ LS_DEBUG_INFO,                               "[%d] [%s] %s.done (%d->%d, %d->%d); wait: %d" },
		{ LS_APP_EXCEPTION_TITLE,                      "SC2Mentat - Unhandled exception" },
		{ LS_APP_RUNNED_CAPTION,                       "Application is already running" },
		{ LS_FILE_OPEN_FAIL,                           "Failed to open file" },
		{ LS_FILE_INVALID,                             "File is invalid" },
		{ LS_CONFIG_LASTACTION,                        "Last action: %s" },
		{ LS_CONFIG_KEY_MISSED,                        "Key '%s' missed" },
		{ LS_CONFIG_KEY_ISNTARRAY,                     "Key '%s' isn't array" },
		{ LS_CONFIG_BUILDS_LOADED,                     "Loaded builds: %d" },
		{ LS_CONFIG_BUILDS_FIND_FAIL,                  "Failed to find builds. Error code: #%d" },
		{ LS_CONFIG_BUILDS_ERROR_FILE,                 "Error in build-file '%S': %s" },
		{ LS_CONFIG_BUILD_NAME_LONG,                   "Name is too long" },
		{ LS_CONFIG_BUILD_RACE_EMPTY,                  "Race of player/opponent is undefined" },
		{ LS_CONFIG_BUILD_TIME_INVALID,                "Time '%s' is invalid" },
		{ LS_CONFIG_BUILD_STEP,                        "Step #%d" },
		{ LS_CONFIG_BUILD_STEP2,                       "Step #%d (time:%s)" },
		{ LS_CONFIG_BUILD_STEP3,                       "Step #%d (time:%s; action:%s)" },
		{ LS_CONFIG_WAITTYPE_INVALID,                  "Wait '%s' is unknown" },
		{ LS_CONFIG_WAITTYPE_EXCEPTION,                "Failed to process Wait '%s'" },
		{ LS_CONFIG_ALIGN_INVALID,                     "Align '%s' is invalid" },
		{ LS_CONFIG_RACE_INVALID,                      "Race '%s' is invalid" },
		{ LS_CONFIG_ACTION_NAME_LONG,                  "Action name '%s' is too long" },
		{ LS_CONFIG_RESOLUTION_TOOSMALL,               "Resolution must be more than %dx%d" },
		{ LS_CONFIG_RESOLUTION_FRAME_MISSED,           "Frame '%s' in resolution %dx%d missed" },
		{ LS_CONFIG_FONT_NAMELONG,                     "Font name is too long" },
		{ LS_CONFIG_GLOBAL_FONT_NOTFOUND,              "Font '%s' not found" },
		{ LS_CONFIG_GLOBAL_HOTKEY_INVALID,             "Hotkey '%s' has invalid value '%s'" },
		{ LS_CONFIG_GLOBAL_PANELS_BO_MISSED,           "Key 'Panels'.'BuildOrder' missed" },
		{ LS_CONFIG_GLOBAL_PANELS_TIPS_MISSED,         "Key 'Panels'.'Tips' missed" },
		{ LS_CONFIG_GLOBAL_PANELS_COLOR_INVALID,       "Panel '%s' has invalid color '%s'" },
		{ LS_HOTKEY_MISSED,                            "Hotkey '%s' missed" },
		{ LS_HOTKEY_THREAD_CREATE_FAIL,                "Failed to create hotkeys thread" },
		{ LS_HOTKEY_DUPLICATE,                         "Hotkey '%s' duplicates key '%s'" },
		{ LS_HOTKEY_REGISTER_FAIL,                     "Failed to register Hotkey '%s'. Code: #%d" },
		{ LS_MODULEFILE_NAME_FAIL,                     "GetModuleFileName failed: #%d" },
		{ LS_TRAY_REGISTERCLASS_FAIL,                  "Failed to register tray window class" },
		{ LS_TRAY_CREATEWND_FAIL,                      "Failed to create tray window" },
		{ LS_TRAY_LOADMENU_FAIL,                       "Failed to load tray menu" },
		{ LS_CONFIG_LOAD_FAIL,                         "Failed to load settings: %s" },
		{ LS_CONFIG_HOTKEYS_LOAD_FAIL,                 "Failed to load hotkeys: %s" },
		{ LS_AUDIO_FAIL,                               "Failed to create audio interface: %s" },
		{ LS_GAME_RUNNED,                              "Game runned" },
		{ LS_GAME_OFF,                                 "Game closed" },
		{ LS_OVERLAY_FAIL,                             "%s. Is game in fullscreen mode? Switch it to window mode." },
		{ LS_OVERLAY_MANYERRORS,                       "Too many errors. Need restart SC2Mentat." },
		{ LS_OVERLAY_FINDWND_FAIL,                     "Game window '%S' with class '%S' could not be found" },
		{ LS_OVERLAY_DEVICE_NULL,                      "Dx3d9 device is NULL" },
		{ LS_OVERLAY_LINE_CREATE_FAIL,                 "Failed to create 3D line" },
		{ LS_OVERLAY_SPRITE_CREATE_FAIL,               "Failed to create 3D sprite" },
		{ LS_OVERLAY_FONT_CREATE_FAIL,                 "Failed to create 3D font" },
		{ LS_OVERLAY_OBJECT_CREATE_FAIL,               "Failed to create dx3d9 object" },
		{ LS_OVERLAY_DEVICE_CREATE_FAIL,               "Failed to create dx3d9 device" },
		{ LS_OVERLAY_TARGETWND_EMPTY,                  "Game class and window names are empty" },
		{ LS_OVERLAY_CLASS_REGISTER_FAIL,              "Failed to register overlay class" },
		{ LS_OVERLAY_WND_CREATE_FAIL,                  "Failed to create overlay window" },
		{ LS_SOUND_MISSED,                             "No sound for action '%s'" },
		{ LS_SOUND_FILE_OPEN_FAIL,                     "Failed to open audio '%s': %s" },
		{ LS_SOUND_SOURCE_OPEN_FAIL,                   "Failed to opening source audio '%s'. Code: %#X" },
		{ LS_SOUND_SOURCE_CREATE_FAIL,                 "Failed to create source voice: %#X" },
		{ LS_SOUND_XAUDIO2_CREATE_FAIL,                "Failed to init XAudio2 engine: %#X" },
		{ LS_SOUND_XAUDIO2_MASTERVOICE_CREATE_FAIL,    "Failed creating mastering voice: %#X" },
		{ LS_FONT_SIZE_INVALID,                        "Invalid size of alphabet '%s': %d" },
		{ LS_FONT_LOAD_FAIL,                           "Failed to load font image '%S'" },
		{ LS_MSG_MUTE,                                 " MUTE" },
		{ LS_MSG_DEBUG,                                " DEBUG" },
		{ LS_MSG_SETTINGS_INVALID,                     "Settings are invalid" },
		{ LS_MSG_RESOLUTION_UND,                       "Resolution %dx%d is undefined" },
		{ LS_MSG_HOTKEY_MISSED,                        "Required hotkeys not defined" },
		{ LS_MSG_BUILD_NULL,                           "Build order didn't load" },
		{ LS_NONE, "" },
	};

	// Должно соответствовать перечислению LANG.
	STRING_TABLE STRINGS_TABLES[] = { STRINGS_RU, STRINGS_EN };
	#define STRINGS_DEFAULT STRINGS_EN

	STRING_TABLE STRINGS_UI_RU = {
		{ ID_TRAY_LOG,      "Логи" },
		{ ID_TRAY_REFRESH,  "Обновить настройки" },
		{ ID_TRAY_EXIT,     "Выход" },

		{ DIALOG_LOG,       "Логи" },
		{ IDC_OK,           "ОК" },
		{ IDC_REFRESH,      "Обновить" },
		{ IDC_CLEAR,        "Очистить" },
	};

	void initLocale()
	{
		// Переключимся на русский язык, если:

		// 1) windows на русском языке.
		if (LANGUAGE_ID_RUS == GetSystemDefaultLangID() || LANGUAGE_ID_RUS == GetSystemDefaultUILanguage())
		{
			gLocale = LANG_RU;
			return;
		}

		// 2) в системе присутствует русская расскладка.
		DWORD dwSize = GetKeyboardLayoutList(0, NULL);
		if (!dwSize)
			return;
		HKL* lphkl = (HKL*)malloc(dwSize * sizeof(HKL));
		dwSize = GetKeyboardLayoutList(dwSize, lphkl);
		for (int i = 0; i < dwSize; i++)
		{
			if (LANGUAGE_ID_RUS == (LANGID)lphkl[i])
			{
				gLocale = LANG_RU;
				break;
			}
		}

		free(lphkl);
	}

	LPCSTR string(STRING_TABLE table, StringId id)
	{
		STRING_TABLE::iterator it = table.find(id);
		if (table.end() == it)
			return NULL;
		return it->second;
	}

	LPCSTR string(StringId id)
	{
		if (gLocale < 0 || gLocale >= LANG_MAXID)
			return NULL;

		STRING_TABLE table = STRINGS_TABLES[gLocale];
		LPCSTR str = string(table, id);
		if (!str && table != STRINGS_DEFAULT)
			str = string(STRINGS_DEFAULT, id);
		return str;
	}

	//---------------------------------------------------------------------------//
	// TRICKY: Локализация диалогов и менюшек.                                   //
	// Особо утонченным натурам не смотреть!                                     //
	//---------------------------------------------------------------------------//

	LPCSTR uiString(WORD id)
	{
		return string(STRINGS_UI_RU, (StringId)id);
	}

	void localizeDialog(HWND hWnd, WORD dlgId)
	{
		if (gLocale != LANG_RU)
			return;

		// Не нашел нормальных способов работы с элементами диалогов.
		STRING_TABLE table = STRINGS_UI_RU;

		// Поэтому заголовок диалога будем обрабатывать отдельно.
		LPCSTR dialogTitle = string(table, (StringId)dlgId);
		if (dialogTitle)
			SetWindowTextA(hWnd, dialogTitle);

		// а дочерние контролы будем искать среди общего списка по таблице локализации.
		STRING_TABLE::iterator it = table.begin();
		while (table.end() != it)
		{
			HWND hChild = GetDlgItem(hWnd, it->first);
			if (hChild)
				SetDlgItemTextA(hWnd, it->first, it->second);

			it++;
		}
	}

	void localizeMenu(HMENU hMenu)
	{
		if (gLocale != LANG_RU)
			return;

		// Рекурсивно пройдемся по всем элементам меню
		// и если в таблице локализации есть найденный пункт, то изменим его название.
		MENUITEMINFOA mii;
		int count = GetMenuItemCount(hMenu);
		for (int i = 0; i < count; i++)
		{
			memset(&mii, 0, sizeof(mii));
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_SUBMENU | MIIM_TYPE | MIIM_ID;
			if (!GetMenuItemInfoA(hMenu, i, TRUE, &mii))
				continue;

			HMENU hSubMenu = mii.hSubMenu;
			LPCSTR localeStr = NULL;
			if (MFT_STRING == mii.fType && NULL != (localeStr = uiString(mii.wID)))
			{
				mii.fMask = MIIM_STRING;
				mii.dwTypeData = (LPSTR)localeStr;
				SetMenuItemInfoA(hMenu, i, TRUE, &mii);
			}

			if (hSubMenu != NULL)
				localizeMenu(hSubMenu);
		}
	}

}
