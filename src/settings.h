#ifndef __EACSMB_settings_h__
#define __EACSMB_settings_h__




#define SETTING_LIST \
	SETTING(int, GUIManager_maxInstances, 8192) \
	




typedef struct GlobalSettings {
#define string char*
#define SETTING(type, name, val) type name;
	SETTING_LIST
#undef SETTING
#undef string
} GlobalSettings;


void GlobalSettings_loadDefaults(GlobalSettings* s);
void GlobalSettings_loadFromFile(GlobalSettings* s, char* path);



#endif // __EACSMB_settings_h__
