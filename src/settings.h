#ifndef __gpuedit_settings_h__
#define __gpuedit_settings_h__

#include "c_json/json.h"
#include "ui/gui_settings.h"


typedef enum TabType {
	MCTAB_NONE,
	MCTAB_EDIT,
	MCTAB_FILEOPEN,
	MCTAB_FUZZYOPEN,
	MCTAB_GREPOPEN,
} TabType_t;

typedef struct TabSpec {
	TabType_t type;
	char* path;
} TabSpec;

//          type   name                default value | min | max    
#define GLOBAL_SETTING_LIST \
	SETTING(int,   AppState_frameRate,          30,    15,   INT_MAX) \
	SETTING(bool,  AppState_enableVSync,        false,  NULL, NULL) \
	SETTING(int,   GUIManager_maxInstances,     8192,  4096, INT_MAX) \
	SETTING(tabsp, MainControl_startupTabs,     ((TabSpec[]){{.type=MCTAB_FUZZYOPEN, .path=NULL}, {.type=MCTAB_NONE, .path=NULL}}), NULL, NULL) \
	SETTING(bool,  MainControl_openInPlace,     false, NULL, NULL) \
	SETTING(int,   Buffer_linesPerScrollWheel,  3,     1,    100) \
	SETTING(bool,  Buffer_cursorBlinkEnable,    true,  NULL, NULL) \
	SETTING(float, Buffer_cursorBlinkOffTime,   0.600, 0,    300000) \
	SETTING(float, Buffer_cursorBlinkOnTime,    0.600, 0,    300000) \
	SETTING(bool,  Buffer_highlightCurrentLine, true,  NULL, NULL) \
	SETTING(bool,  Buffer_outlineCurrentLine,   true,  NULL, NULL) \
	SETTING(float, Buffer_outlineCurrentLineYOffset,   0,     -999, 999) \
	SETTING(float, Buffer_lineNumExtraWidth,    10,    0,    1920*16) \
	SETTING(bool,  Buffer_showLineNums,         true,  NULL, NULL) \
	SETTING(int,   Buffer_lineNumBase,          10,    2,    36) \
	SETTING(charp, Buffer_lineNumCharset,       "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", NULL, NULL) \
	SETTING(float, Buffer_charWidth,            10,    1,    1920*16) \
	SETTING(float, Buffer_lineHeight,           20,    1,    1920*16) \
	SETTING(int,   Buffer_tabWidth,             4,     0,    INT_MAX) \
	SETTING(charp, Buffer_font,                 "Courier New", NULL, NULL) \
	SETTING(float, Buffer_fontSize,             12,    1,    1920*16) \
	SETTING(bool,  Buffer_invertSelection,      true,  NULL,  NULL) \
	SETTING(int,   Buffer_maxUndo,              4096,  0,    INT_MAX) \
	SETTING(int,   Buffer_statusBarHeight,      20,    0,    INT_MAX) \
	SETTING(int,   MainControl_tabHeight,       20,    0,    1920*16) \
	SETTING(charpp,MainControl_searchPaths,     ((char*[]){"./", NULL}),  NULL, NULL) \
	SETTING(charp, Theme_path,                  "default_dark.json", NULL, NULL) \
	SETTING(themep,Theme,                       NULL,  NULL, NULL) \
	SETTING(guisettingsp, GUI_GlobalSettings,   NULL,  NULL, NULL) \
	

#define THEME_SETTING_LIST \
	SETTING(charp, name,                          "default dark", NULL, NULL) \
	SETTING(bool,  is_dark,                       true,        NULL, NULL) \
	SETTING(charp, bgColor,                       "#0f0f0fff", NULL, NULL) \
	SETTING(charp, lineNumColor,                  "#ffffffff", NULL, NULL) \
	SETTING(charp, lineNumBgColor,                "#141414ff", NULL, NULL) \
	SETTING(charp, lineNumBookmarkColor,          "#32ff32ff", NULL, NULL) \
	SETTING(charp, hl_bgColor,                    "#00c8c8ff", NULL, NULL) \
	SETTING(charp, hl_textColor,                  "#fa0032ff", NULL, NULL) \
	SETTING(charp, find_bgColor,                  "#115511ff", NULL, NULL) \
	SETTING(charp, find_textColor,                "#660022ff", NULL, NULL) \
	SETTING(charp, outlineCurrentLineBorderColor, "#323232ff", NULL, NULL) \
	SETTING(charp, selectedItemTextColor,         "#c8c8c8ff", NULL, NULL) \
	SETTING(charp, selectedItemBgColor,           "#505050ff", NULL, NULL) \
	


struct ThemeSettings;
typedef struct ThemeSettings ThemeSettings;

#define charp char*
#define charpp char**
#define bool char
#define tabsp TabSpec*
#define themep ThemeSettings*
#define guisettingsp GUI_GlobalSettings*
#define SETTING(type, name, val ,min,max) type name;

typedef struct GlobalSettings {
	GLOBAL_SETTING_LIST
} GlobalSettings;

typedef struct ThemeSettings {
	THEME_SETTING_LIST
} ThemeSettings;

#undef SETTING
#undef charp
#undef charpp
#undef bool
#undef tabsp
#undef themep
#undef guisettingsp



void GlobalSettings_LoadDefaults(GlobalSettings* s);
void GlobalSettings_LoadFromFile(GlobalSettings* s, char* path);
void GlobalSettings_LoadFromJSON(GlobalSettings* s, char* path);

void ThemeSettings_LoadDefaults(ThemeSettings* s);
void ThemeSettings_LoadFromJSON(ThemeSettings* s, struct json_value* jsv);


#endif // __gpuedit_settings_h__
