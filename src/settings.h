#ifndef __EACSMB_settings_h__
#define __EACSMB_settings_h__




#define SETTING_LIST \
	SETTING(int, DynamicMeshManager_maxInstances, 8192) \
	SETTING(int, RiggedMeshManager_maxInstances, 4096) \
	SETTING(int, DecalManager_maxInstances, 8192) \
	SETTING(int, CustomDecalManager_maxInstances, 8192) \
	SETTING(int, EmitterManager_maxInstances, 8192) \
	SETTING(int, MarkerManager_maxInstances, 8192) \
	SETTING(int, GUIManager_maxInstances, 8192) \
	\
	SETTING(int, SunShadow_size, 1024) \
	\
	SETTING(string, worldConfigPath, "assets/config/combined_config.json") \
	\
	SETTING(float, keyRotateSensitivity, 0.0f) \
	SETTING(float, keyScrollSensitivity, 0.0f) \
	SETTING(float, keyZoomSensitivity  , 0.0f) \
	\
	SETTING(float, mouseRotateSensitivity, 0.0f) \
	SETTING(float, mouseScrollSensitivity, 0.0f) \
	SETTING(float, mouseZoomSensitivity  , 0.0f) \
	




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
