#ifndef __gpuedit_settings_h__
#define __gpuedit_settings_h__



//          type   name                default value | min | max    
#define SETTING_LIST \
	SETTING(int,   GUIManager_maxInstances,     8192,  4096, INT_MAX) \
	SETTING(int,   Buffer_linesPerScrollWheel,  3,     1,    100) \
	SETTING(bool,  Buffer_cursorBlinkEnable,    true,  NULL, NULL) \
	SETTING(float, Buffer_cursorBlinkOffTime,   200,   1,    300000) \
	SETTING(float, Buffer_cursorBlinkOnTime,    300,   1,    300000) \
	SETTING(bool,  Buffer_hideScrollbar,        false, NULL, NULL) \
	SETTING(bool,  Buffer_highlightCurrentLine, true,  NULL, NULL) \
	SETTING(bool,  Buffer_outlineCurrentLine,   true,  NULL, NULL) \
	SETTING(float, Buffer_lineNumExtraWidth,    10,    0,    1920*16) \
	SETTING(bool,  Buffer_showLineNums,         true,  NULL, NULL) \
	SETTING(float, Buffer_charWidth,            25,    1,    1920*16) \
	SETTING(float, Buffer_lineHeight,           30,    1,    1920*16) \
	SETTING(int,   Buffer_tabWidth,             4,     0,    INT_MAX) \
	SETTING(charp, Buffer_font,                 "Courier New", NULL, NULL) \
	SETTING(float, Buffer_fontSize,             12,    1,    1920*16) \
	SETTING(int,   Buffer_maxUndo,              4096,  0,    INT_MAX) \
	




typedef struct GlobalSettings {
#define charp char*
#define bool char
#define SETTING(type, name, val ,min,max) type name;
	SETTING_LIST
#undef SETTING
#undef charp
#undef bool
} GlobalSettings;


void GlobalSettings_loadDefaults(GlobalSettings* s);
void GlobalSettings_loadFromFile(GlobalSettings* s, char* path);



#endif // __gpuedit_settings_h__
