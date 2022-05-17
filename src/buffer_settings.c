
#include "buffer.h"



static void grab_Color4(Color4* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(v->type == JSON_TYPE_STRING && v->s) {
			decodeHexColorNorm(v->s, (float*)out);
		}
	}
}

static void grab_charp(char** out, json_value_t* obj, char* prop) {
	json_value_t* v;
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(v->type == JSON_TYPE_STRING && v->s) {
			if(*out) free(*out);
			*out = strdup(v->s);
		}
	}
}

static void grab_bool(char* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	int64_t i;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		*out = json_as_int(v);
	}
}

static void grab_int(int* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		*out = json_as_int(v);
	}
}

static void grab_float(float* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		*out = json_as_float(v);
	}
}





#include "settings_macros_on.h"



ThemeSettings* ThemeSettings_Alloc(void* useless) {
	return calloc(1, sizeof(ThemeSettings));
}

ThemeSettings* ThemeSettings_Copy(void* useless, ThemeSettings* orig) {
	ThemeSettings* new = calloc(1, sizeof(*new));

	#define SETTING(type, name, val ,min,max) new->name = copy_##type(orig->name);
		THEME_SETTING_LIST
	#undef SETTING
	
	return new;
}

void ThemeSettings_Free(void* useless, ThemeSettings* s) {
	#define SETTING(type, name, val ,min,max) free_##type(s->name);
		THEME_SETTING_LIST
	#undef SETTING
}

void ThemeSettings_LoadDefaults(void* useless, ThemeSettings* s) {
	#define SETTING(type, name, val ,min,max) s->name = set_##type(val);
		THEME_SETTING_LIST
	#undef SETTING
}

void ThemeSettings_LoadJSON(void* useless, ThemeSettings* s, struct json_value* jsv) {
	#define SETTING(type, name, val ,min,max) grab_##type(&s->name, jsv, #name);
		THEME_SETTING_LIST
	#undef SETTING
}





BufferSettings* BufferSettings_Alloc(void* useless) {
	return calloc(1, sizeof(BufferSettings));
}

BufferSettings* BufferSettings_Copy(void* useless, BufferSettings* orig) {
	BufferSettings* new = calloc(1, sizeof(*new));

	#define SETTING(type, name, val ,min,max) new->name = copy_##type(orig->name);
		BUFFER_SETTING_LIST
	#undef SETTING
	
	return new;
}

void BufferSettings_Free(void* useless, BufferSettings* s) {
	#define SETTING(type, name, val ,min,max) free_##type(s->name);
		BUFFER_SETTING_LIST
	#undef SETTING
}

void BufferSettings_LoadDefaults(void* useless, BufferSettings* s) {
	#define SETTING(type, name, val ,min,max) s->name = set_##type(val);
		BUFFER_SETTING_LIST
	#undef SETTING
}

void BufferSettings_LoadJSON(void* useless, BufferSettings* s, struct json_value* jsv) {
	#define SETTING(type, name, val ,min,max) grab_##type(&s->name, jsv, #name);
		BUFFER_SETTING_LIST
	#undef SETTING
}



#include "settings_macros_off.h"
