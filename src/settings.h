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
#define SETTING_LIST \
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
	SETTING(int,   Buffer_maxUndo,              4096,  0,    INT_MAX) \
	SETTING(int,   Buffer_statusBarHeight,      20,    0,    INT_MAX) \
	SETTING(int,   MainControl_tabHeight,       20,    0,    1920*16) \
	SETTING(charpp,MainControl_searchPaths,     ((char*[]){"./", NULL}),  NULL, NULL) \
	\
	SETTING(charp, Theme_name,                  "default dark", NULL, NULL) \
	SETTING(bool,  Theme_is_dark,               true,        NULL, NULL) \
	SETTING(charp, Theme_bgColor,               "#0f0f0fff", NULL, NULL) \
	SETTING(charp, Theme_lineNumColor,          "#ffffffff", NULL, NULL) \
	SETTING(charp, Theme_lineNumBgColor,        "#141414ff", NULL, NULL) \
	SETTING(charp, Theme_lineNumBookmarkColor,  "#32ff32ff", NULL, NULL) \
	SETTING(charp, Theme_hl_bgColor,            "#00c8c8ff", NULL, NULL) \
	SETTING(charp, Theme_hl_textColor,          "#fa0032ff", NULL, NULL) \
	SETTING(charp, Theme_find_bgColor,          "#115511ff", NULL, NULL) \
	SETTING(charp, Theme_find_textColor,        "#660022ff", NULL, NULL) \
	SETTING(charp, Theme_outlineCurrentLineBorderColor, "#323232ff", NULL, NULL) \
	SETTING(charp, Theme_selectedItemTextColor, "#c8c8c8ff", NULL, NULL) \
	SETTING(charp, Theme_selectedItemBgColor,   "#505050ff", NULL, NULL) \
	\
	SETTING(charp, Theme_textColor,             "#c8c8c8ff", NULL, NULL) \
	SETTING(charp, Theme_buttonTextColor,       "#c8c8e1ff", NULL, NULL) \
	SETTING(charp, Theme_buttonHoverTextColor,  "#c80202ff", NULL, NULL) \
	SETTING(charp, Theme_buttonDisTextColor,    "#141414ff", NULL, NULL) \
	SETTING(charp, Theme_buttonBgColor,         "#0202e1ff", NULL, NULL) \
	SETTING(charp, Theme_buttonHoverBgColor,    "#c8c802ff", NULL, NULL) \
	SETTING(charp, Theme_buttonDisBgColor,      "#646464ff", NULL, NULL) \
	SETTING(charp, Theme_buttonBorderColor,     "#c802e1ff", NULL, NULL) \
	SETTING(charp, Theme_buttonHoverBorderColor, "#02c8e1ff", NULL, NULL) \
	SETTING(charp, Theme_buttonDisBorderColor,  "#14147dff", NULL, NULL) \
	SETTING(charp, Theme_editBorderColor,       "#19f519ff", NULL, NULL) \
	SETTING(charp, Theme_editBgColor,           "#143219ff", NULL, NULL) \
	SETTING(charp, Theme_cursorColor,           "#f0f0f0ff", NULL, NULL) \
	SETTING(charp, Theme_tabTextColor,          "#c8c8c8ff", NULL, NULL) \
	SETTING(charp, Theme_tabBorderColor,        "#787878ff", NULL, NULL) \
	SETTING(charp, Theme_tabActiveBgColor,      "#505050ff", NULL, NULL) \
	SETTING(charp, Theme_tabHoverBgColor,       "#282828ff", NULL, NULL) \
	SETTING(charp, Theme_tabBgColor,            "#0a0a0aff", NULL, NULL) \
	SETTING(charp, Theme_windowBgBorderColor,   "#b4b400ff", NULL, NULL) \
	SETTING(charp, Theme_windowBgColor,         "#141414ff", NULL, NULL) \
	SETTING(charp, Theme_windowTitleBorderColor, "#b4b400ff", NULL, NULL) \
	SETTING(charp, Theme_windowTitleColor,      "#282828ff", NULL, NULL) \
	SETTING(charp, Theme_windowTitleTextColor,  "#d2d200ff", NULL, NULL) \
	SETTING(charp, Theme_windowCloseBtnBorderColor, "#d22800ff", NULL, NULL) \
	SETTING(charp, Theme_windowCloseBtnColor,   "#b43c00ff", NULL, NULL) \
	SETTING(charp, Theme_windowScrollbarColor,  "#969600ff", NULL, NULL) \
	SETTING(charp, Theme_windowScrollbarBorderColor, "#969600ff", NULL, NULL) \
	SETTING(charp, Theme_selectBgColor,         "#140f03ff", NULL, NULL) \
	SETTING(charp, Theme_selectBorderColor,     "#64961eff", NULL, NULL) \
	SETTING(charp, Theme_selectTextColor,       "#96be3cff", NULL, NULL) \
	



typedef struct GlobalSettings {
#define charp char*
#define charpp char**
#define bool char
#define tabsp TabSpec*
#define SETTING(type, name, val ,min,max) type name;
	SETTING_LIST
#undef SETTING
#undef charp
#undef charpp
#undef bool
#undef tabsp
} GlobalSettings;


void GlobalSettings_loadDefaults(GlobalSettings* s);
void GlobalSettings_loadFromFile(GlobalSettings* s, char* path);



#endif // __gpuedit_settings_h__
