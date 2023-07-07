#ifndef __gpuedit_settings_h__
#define __gpuedit_settings_h__

#include "c_json/json.h"
#include "sti/vec.h"




// bits for masking the settings sections
enum {
	SETTINGS_ALL     = 0,
	SETTINGS_GUI     = 1 << 0,
	SETTINGS_Theme   = 1 << 1,
	SETTINGS_Buffer  = 1 << 2,
	SETTINGS_General = 1 << 3,
};


#define MCTAB_TYPE_LIST \
	X(Pane) \
	X(Buffer) \
	X(FileOpener) \
	X(FuzzyOpener) \
	X(GrepOpener) \
	X(Hexedit) \
	X(Calculator) \
	X(Empty) \


typedef enum TabType {
	MCTAB_None = 0,
#define X(x,...) MCTAB_##x,
	MCTAB_TYPE_LIST
#undef X
	MCTAB_MAX_VALUE
} TabType_t;

extern char* mctab_type_names[];

typedef struct TabSpec {
	TabType_t type;
	char* path;
} TabSpec;


#define TAB_SCROLL_TYPE_LIST \
	X(None) \
	X(Linear) \
	X(Sinusoidal) \
	X(Swing) \
	X(Loop) \
	

typedef enum TabScrollType {
#define X(x) TABSC_##x,
	TAB_SCROLL_TYPE_LIST
#undef X
} TabScrollType;


#define MCWID_TYPE_LIST \
	X(HELLO) \
	X(PING) \
	X(CLOCK) \
	X(BATTERY) \
	X(LINECOL) \
	X(BUFMODE) \
	X(BUFPATH) \
	X(FINDSTATE) \


typedef enum WidgetType {
	MCWID_NONE = 0,
#define X(x,...) MCWID_##x,
	MCWID_TYPE_LIST
#undef X
	MCWID_MAX_VALUE
} WidgetType_t;

extern char* mcwid_type_names[];

typedef struct WidgetSpec {
	WidgetType_t type;
	size_t size;
	char align;
	char* format;
} WidgetSpec;


//          type   name                default value | min | max    
#define GENERAL_SETTING_LIST \
	SETTING(int,   frameRate,          30,    15,   INT_MAX) \
	SETTING(charp, imagesPath,         "/usr/local/share/gpuedit/images", NULL, NULL) \
	SETTING(charp, commandsPath,       "/usr/local/share/gpuedit/commands.json", NULL, NULL) \
	SETTING(charp, highlightersPath,   "/usr/local/lib/gpuedit/highlighters/", NULL, NULL) \
	SETTING(charp, highlightStylesPath,"/usr/local/share/gpuedit/highlight_styles/", NULL, NULL) \
	SETTING(bool,  enableVSync,        false,   NULL, NULL) \
	SETTING(bool,  enableSessions,     true,    NULL, NULL) \
	SETTING(int,   sessionFileHistory, 100,     0, 1000) \
	SETTING(tabsp, MainControl_startupTabs,     ((TabSpec[]){{.type=MCTAB_FuzzyOpener, .path=NULL}, {.type=MCTAB_None, .path=NULL}}), NULL, NULL) \
	SETTING(widsp, MainControl_statusWidgets,   ((WidgetSpec[]){{.type=MCWID_LINECOL, .size=20, .align='l', .format="line: %L:%C"}, {.type=MCWID_NONE, .size=0}}), NULL, NULL) \
	SETTING(bool,  MainControl_openInPlace,     false, NULL, NULL) \
	SETTING(bool,  MainControl_autoSortTabs,    false, NULL, NULL) \
	SETTING(bool,  MainControl_scrollTabNames,  true,  NULL, NULL) \
	SETTING(scrollfn, MainControl_tabNameScrollFn,          2,    NULL, NULL) \
	SETTING(float,    MainControl_tabNameScrollStartLinger, 2.0,  NULL, NULL) \
	SETTING(float,    MainControl_tabNameScrollEndLinger,   2.0,  NULL, NULL) \
	SETTING(float,    MainControl_tabNameScrollAnimTime,    3.0,  NULL, NULL) \
	SETTING(int,      MainControl_tabHeight,                20,    0,    1920*16) \
	SETTING(charpp,   MainControl_searchPaths,              ((char*[]){"./", NULL}),  NULL, NULL) \
	SETTING(int,   Terminal_lineHeight,         20,    0,    INT_MAX) \
	SETTING(int,   Terminal_colWidth,           10,    0,    INT_MAX) \
	SETTING(charp, Terminal_fontName,           "Courier New", NULL, NULL) \
	SETTING(int,   Terminal_fontSize,           12,    1,    INT_MAX) \
	SETTING(charp, Theme_path,                  "default_dark.json", NULL, NULL) \
	

	




#include "settings_macros_on.h"

typedef struct GeneralSettings {
#define SETTING(type, name, val ,min,max) type name;
	GENERAL_SETTING_LIST
#undef SETTING
} GeneralSettings;

#include "settings_macros_off.h"



typedef void* (*SettingsAllocFn)(void* /*userData*/);
typedef void  (*SettingsFreeFn)(void* /*userData*/, void* /*dataStore*/);
typedef void* (*SettingsCopyFn)(void* /*userData*/, void* /*dataStore*/);
typedef void  (*SettingsDefaultsFn)(void* /*userData*/, void* /*dataStore*/);
typedef void  (*SettingsLoaderFn)(void* /*userData*/, void* /*dataStore*/, struct json_value*);

typedef struct SettingsHandler {
	char* key;

} SettingsHandler;


typedef struct SettingsSection {
	unsigned long bit;
	char* key;
	void* dataStore;
	void* userData;
	SettingsLoaderFn loader;
	SettingsDefaultsFn defaults;
	SettingsAllocFn alloc;
	SettingsCopyFn copy;
	SettingsFreeFn free;
} SettingsSection;

typedef struct Settings {
	VEC(SettingsSection) sections;
	
	struct Settings* parent;
} Settings;


// can only pass in one section bit
// retusn NULL on section not found
void* Settings_GetSection(Settings* s, unsigned long bit);

void Settings_RegisterSection(
	Settings* s, 
	unsigned long bit, 
	char* key, 
	void* userData, 
	SettingsAllocFn a, 
	SettingsCopyFn c, 
	SettingsFreeFn f, 
	SettingsDefaultsFn d, 
	SettingsLoaderFn l
);


Settings* Settings_Copy(Settings* s, unsigned long mask);
void Settings_Free(Settings* s);
void Settings_LoadDefaults(Settings* s, unsigned long mask);
void Settings_LoadJSON(Settings* s, struct json_value* v, unsigned long mask);

// returns 0 if the file was read
int  Settings_LoadFile(Settings* s, char* path, unsigned long mask);

// reads path/.gpuedit.json and path/.gpuedit/*.json
// returns the number of files read
int Settings_ReadDefaultFilesAt(Settings* gs, char* path, unsigned long mask);

// reads path/*.json
// returns the number of files read
int Settings_ReadAllJSONAt(Settings* s, char* path, unsigned long mask);


GeneralSettings* GeneralSettings_Alloc(void* useless);
GeneralSettings* GeneralSettings_Copy(void* useless, GeneralSettings* s);
void GeneralSettings_Free(void* useless, GeneralSettings* s);
void GeneralSettings_LoadDefaults(void* useless, GeneralSettings* s);
void GeneralSettings_LoadJSON(void* useless, GeneralSettings* s, struct json_value* jsv);



#endif // __gpuedit_settings_h__
