#ifndef __gputk_gui_settings_h__
#define __gputk_gui_settings_h__




#define GUI_SETTING_LIST \
	SETTING(int,   maxInstances,                  8192, 64, 9999999999999) \
	SETTING(charp, name,                          "default dark", NULL, NULL) \
	\
	SETTING(charpp,fontList,             ((char*[]){"Arial","Courier New", NULL}), NULL, NULL) \
	\
	SETTING(int,   linesPerScrollWheel,           5, 1, 999) \
	SETTING(bool,  hideScrollbar,                 false,       NULL, NULL) \
	\
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





#define charp char*
#define charpp char**
#define bool char
#define SETTING(type, name, val ,min,max) type name;

typedef struct GUI_GlobalSettings {
	GUI_SETTING_LIST
} GUI_GlobalSettings;

#undef SETTING
#undef charp
#undef charpp
#undef bool


struct json_value;

void GUI_GlobalSettings_LoadDefaults(GUI_GlobalSettings* s);
void GUI_GlobalSettings_LoadFromFile(GUI_GlobalSettings* s, char* path);
void GUI_GlobalSettings_LoadFromJSON(GUI_GlobalSettings* s, struct json_value* jsv);



#endif // __gputk_gui_settings_h__
