#ifndef __gpuedit_settings_h__
#define __gpuedit_settings_h__

typedef enum TabType {
	MCTAB_NONE,
	MCTAB_EDIT,
	MCTAB_FILEOPEN,
	MCTAB_FUZZYOPEN,
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
	SETTING(int,   Buffer_linesPerScrollWheel,  3,     1,    100) \
	SETTING(bool,  Buffer_cursorBlinkEnable,    true,  NULL, NULL) \
	SETTING(float, Buffer_cursorBlinkOffTime,   0.600, 0,    300000) \
	SETTING(float, Buffer_cursorBlinkOnTime,    0.600, 0,    300000) \
	SETTING(bool,  Buffer_hideScrollbar,        false, NULL, NULL) \
	SETTING(bool,  Buffer_highlightCurrentLine, true,  NULL, NULL) \
	SETTING(bool,  Buffer_outlineCurrentLine,   true,  NULL, NULL) \
	SETTING(float, Buffer_outlineCurrentLineYOffset,   0,     -999, 999) \
	SETTING(float, Buffer_lineNumExtraWidth,    10,    0,    1920*16) \
	SETTING(bool,  Buffer_showLineNums,         true,  NULL, NULL) \
	SETTING(float, Buffer_charWidth,            10,    1,    1920*16) \
	SETTING(float, Buffer_lineHeight,           20,    1,    1920*16) \
	SETTING(int,   Buffer_tabWidth,             4,     0,    INT_MAX) \
	SETTING(charpp,Buffer_fontList,             ((char*[]){"Arial","Courier New", NULL}), NULL, NULL) \
	SETTING(charp, Buffer_font,                 "Courier New", NULL, NULL) \
	SETTING(float, Buffer_fontSize,             12,    1,    1920*16) \
	SETTING(bool,  Buffer_invertSelection,      true,  NULL,  NULL) \
	SETTING(int,   Buffer_maxUndo,              4096,  0,    INT_MAX) \
	SETTING(int,   Buffer_statusBarHeight,      20,    0,    INT_MAX) \
	SETTING(int,   MainControl_tabHeight,       20,    0,    1920*16) \
	SETTING(charpp,MainControl_searchPaths,     ((char*[]){"./", NULL}),  NULL, NULL) \
	SETTING(charp, Theme_path,                  "default_dark.json", NULL, NULL) \
	SETTING(themep,Theme,                       NULL,  NULL, NULL) \
	

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
	\
	SETTING(charp, textColor,                     "#c8c8c8ff", NULL, NULL) \
	SETTING(charp, buttonTextColor,               "#c8c8e1ff", NULL, NULL) \
	SETTING(charp, buttonHoverTextColor,          "#c80202ff", NULL, NULL) \
	SETTING(charp, buttonDisTextColor,            "#141414ff", NULL, NULL) \
	SETTING(charp, buttonBgColor,                 "#0202e1ff", NULL, NULL) \
	SETTING(charp, buttonHoverBgColor,            "#c8c802ff", NULL, NULL) \
	SETTING(charp, buttonDisBgColor,              "#646464ff", NULL, NULL) \
	SETTING(charp, buttonBorderColor,             "#c802e1ff", NULL, NULL) \
	SETTING(charp, buttonHoverBorderColor,        "#02c8e1ff", NULL, NULL) \
	SETTING(charp, buttonDisBorderColor,          "#14147dff", NULL, NULL) \
	SETTING(charp, editBorderColor,               "#19f519ff", NULL, NULL) \
	SETTING(charp, editBgColor,                   "#143219ff", NULL, NULL) \
	SETTING(charp, cursorColor,                   "#f0f0f0ff", NULL, NULL) \
	SETTING(charp, tabTextColor,                  "#c8c8c8ff", NULL, NULL) \
	SETTING(charp, tabBorderColor,                "#787878ff", NULL, NULL) \
	SETTING(charp, tabActiveBgColor,              "#505050ff", NULL, NULL) \
	SETTING(charp, tabHoverBgColor,               "#282828ff", NULL, NULL) \
	SETTING(charp, tabBgColor,                    "#0a0a0aff", NULL, NULL) \
	SETTING(charp, windowBgBorderColor,           "#b4b400ff", NULL, NULL) \
	SETTING(charp, windowBgColor,                 "#141414ff", NULL, NULL) \
	SETTING(charp, windowTitleBorderColor,        "#b4b400ff", NULL, NULL) \
	SETTING(charp, windowTitleColor,              "#282828ff", NULL, NULL) \
	SETTING(charp, windowTitleTextColor,          "#d2d200ff", NULL, NULL) \
	SETTING(charp, windowCloseBtnBorderColor,     "#d22800ff", NULL, NULL) \
	SETTING(charp, windowCloseBtnColor,           "#b43c00ff", NULL, NULL) \
	SETTING(charp, windowScrollbarColor,          "#969600ff", NULL, NULL) \
	SETTING(charp, windowScrollbarBorderColor,    "#969600ff", NULL, NULL) \
	SETTING(charp, selectBgColor,                 "#140f03ff", NULL, NULL) \
	SETTING(charp, selectBorderColor,             "#64961eff", NULL, NULL) \
	SETTING(charp, selectTextColor,               "#96be3cff", NULL, NULL) \


struct ThemeSettings;
typedef struct ThemeSettings ThemeSettings;

#define charp char*
#define charpp char**
#define bool char
#define tabsp TabSpec*
#define themep ThemeSettings*
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



void GlobalSettings_loadDefaults(GlobalSettings* s);
void GlobalSettings_loadFromFile(GlobalSettings* s, char* path);

void ThemeSettings_loadDefaults(ThemeSettings* s);
void ThemeSettings_loadFromFile(ThemeSettings* s, char* path);


#endif // __gpuedit_settings_h__
