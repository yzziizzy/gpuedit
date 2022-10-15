#ifndef __gputk_gui_settings_h__
#define __gputk_gui_settings_h__



#include "opts_structs.h"


#define GUI_SETTING_LIST \
	SETTING(int,   maxInstances,                  8192, 64, 9999999999999) \
	SETTING(charp, shaderPath,                    "/usr/local/lib/gpuedit/shaders/guiUnified.glsl", NULL, NULL) \
	SETTING(charp, name,                          "default dark", NULL, NULL) \
	\
	SETTING(charp, fontAtlasFile,                 "~/.gpuedit/fonts.atlas", NULL, NULL) \
	SETTING(charpp,fontList,             ((char*[]){"Arial","Courier New", NULL}), NULL, NULL) \
	\
	SETTING(int,   linesPerScrollWheel,           5, 1, 999) \
	SETTING(bool,  hideScrollbar,                 false,       NULL, NULL) \
	SETTING(bool,  hideStatusBar,                 false,       NULL, NULL) \
	\
	SETTING(bool,  is_dark,                       true,        NULL, NULL) \
	SETTING(Color4, bgColor,                       C4H(0f0f0fff), NULL, NULL) \
	SETTING(Color4, outlineCurrentLineBorderColor, C4H(323232ff), NULL, NULL) \
	SETTING(Color4, selectedItemTextColor,         C4H(c8c8c8ff), NULL, NULL) \
	SETTING(Color4, selectedItemBgColor,           C4H(505050ff), NULL, NULL) \
	\
	SETTING(Color4, textColor,                     C4H(c8c8c8ff), NULL, NULL) \
	SETTING(Color4, cursorColor,                   C4H(f0f0f0ff), NULL, NULL) \
	SETTING(Color4, tabTextColor,                  C4H(c8c8c8ff), NULL, NULL) \
	SETTING(Color4, tabBorderColor,                C4H(787878ff), NULL, NULL) \
	SETTING(Color4, tabActiveBgColor,              C4H(505050ff), NULL, NULL) \
	SETTING(Color4, tabHoverBgColor,               C4H(282828ff), NULL, NULL) \
	SETTING(Color4, tabBgColor,                    C4H(0a0a0aff), NULL, NULL) \
	SETTING(Color4, windowBgColor,                 C4H(141414ff), NULL, NULL) \
	SETTING(Color4,trayBgColor,                   C4H(373b41ff), NULL, NULL) \
	SETTING(float, charWidth_fw,                  10,          1,    99) \
	SETTING(float, lineHeight_fw,                 20,          1,    99) \
	SETTING(Font, font_fw,                       NULL, NULL, NULL) \
	SETTING(charp, fontName_fw,                       "Courier New", NULL, NULL) \
	SETTING(float, fontSize_fw,                   12,          1,    99) \
	SETTING(Font, font,                       NULL, NULL, NULL) \
	SETTING(charp, fontName,                       "Arial", NULL, NULL) \
	SETTING(float, fontSize,                   10,          1,    99) \
	SETTING(Color4, statusBarBgColor,              C4H(282828ff), NULL, NULL) \
	SETTING(Color4, statusBarTextColor,            C4H(c8c8c8ff), NULL, NULL) \
	\
	SETTING(charpp,fileBrowserColumnOrder,        ((char*[]){"icon","name","size","mtime", NULL}), NULL, NULL) \
	SETTING(charp, fileBrowserATimeFmt,           "%Y/%m/%d  %H:%M:%S", NULL, NULL) \
	SETTING(charp, fileBrowserMTimeFmt,           "%Y/%m/%d  %H:%M:%S", NULL, NULL) \
	SETTING(charp, fileBrowserCTimeFmt,           "%Y/%m/%d  %H:%M:%S", NULL, NULL) \
	SETTING(float, fileBrowserHeaderHeight,       25,          10,   9999) \
	SETTING(Color4, fileBrowserHeaderTextColor,    C4H(ff00ffff), NULL, NULL) \
	SETTING(Color4, fileBrowserHeaderBgColor,      C4H(fff00fff), NULL, NULL) \
	SETTING(Color4, fileBrowserHeaderBorderColor,  C4H(ffff00ff), NULL, NULL) \




struct GUIFont;
typedef struct GUIFont GUIFont;

#define Font GUIFont*
#define charp char*
#define charpp char**
#define bool char
#define SETTING(type, name, val ,min,max) type name;

typedef struct GUISettings {
	GUI_SETTING_LIST
	
	#define V(a, ...) a a;
		GUI_CONTROL_OPS_STRUCT_LIST
	#undef V
} GUISettings;

#undef SETTING
#undef Font
#undef charp
#undef charpp
#undef bool


struct GUIManager;
typedef struct GUIManager GUIManager;

struct json_value;

GUISettings* GUISettings_Copy(GUIManager* gm, GUISettings* orig);
void GUISettings_LoadDefaults(GUIManager* gm, GUISettings* s);
void GUISettings_LoadFromFile(GUIManager* gm, GUISettings* s, char* path);
void GUISettings_LoadJSON(GUIManager* gm, GUISettings* s, struct json_value* jsv);
GUISettings* GUISettings_Alloc(GUIManager* gm);
void GUISettings_Free(GUISettings* s);


#endif // __gputk_gui_settings_h__
