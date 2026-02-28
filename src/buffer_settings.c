
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


static void store_Color4(Color4* in, json_value_t* obj, char* prop) {
	char* hexes = "0123456789abcdef";
	char color[10] = {
		'#',
		hexes[(int)(in->r * 255) / 16],
		hexes[(int)(in->r * 255) % 16],
		hexes[(int)(in->g * 255) / 16],
		hexes[(int)(in->g * 255) % 16],
		hexes[(int)(in->b * 255) / 16],
		hexes[(int)(in->b * 255) % 16],
		hexes[(int)(in->a * 255) / 16],
		hexes[(int)(in->a * 255) % 16],
		0
	};
	
	json_obj_set_key(obj, prop, json_new_str(color));
}

static void store_charp(char** in, json_value_t* obj, char* prop) {
	json_obj_set_key(obj, prop, json_new_str(*in));
}


static void store_charpp(char*** in, json_value_t* obj, char* prop) {
	json_value_t* arr = json_new_array();
			
	for(char** s = *in; *s; s++) { 
		json_array_push_tail(arr, json_new_str(*s));
	}
	
	json_obj_set_key(obj, prop, arr);
}



static void store_strvec(strvec* in, json_value_t* obj, char* prop) {
	json_value_t* arr = json_new_array();
			
	VEC_EACH(in, i, s) { 
		json_array_push_tail(arr, json_new_str(s));
	}
	
	json_obj_set_key(obj, prop, arr);
}

static void store_bool(char* in, json_value_t* obj, char* prop) {
	json_obj_set_key(obj, prop, *in ? json_new_true() : json_new_false());
}

static void store_int(int* in, json_value_t* obj, char* prop) {
	json_obj_set_key(obj, prop, json_new_int(*in));
}

static void store_float(float* in, json_value_t* obj, char* prop) {
	json_obj_set_key(obj, prop, json_new_double(*in));
}

static void store_double(double* in, json_value_t* obj, char* prop) {
	json_obj_set_key(obj, prop, json_new_double(*in));
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

void ThemeSettings_SaveJSON(void* useless, ThemeSettings* s, struct json_value* jsv) {
	#define SETTING(type, name, val ,min,max) store_##type(&s->name, jsv, #name);
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

void BufferSettings_SaveJSON(void* useless, BufferSettings* s, struct json_value* jsv) {
	#define SETTING(type, name, val ,min,max) store_##type(&s->name, jsv, #name);
		BUFFER_SETTING_LIST
	#undef SETTING
}



#include "settings_macros_off.h"
