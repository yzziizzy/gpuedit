#include <stdlib.h>
#include <string.h>

#include "settings.h"
#include "sti/sti.h"

#include "c_json/json.h"

size_t strlistlen(char** a) {
	size_t n = 0;
	for(; a[n]; n++);
	return n;
}


char** strlistdup(char** old) {
	size_t i;
	char** new;

	if(!old) return NULL;
	
	new = malloc(sizeof(*new) * (strlistlen(old) + 1));
	
	for(i = 0; old[i]; i++) {
		new[i] = strdup(old[i]);
	}

	new[i] = NULL;
	return new;
}


size_t tabspeclistlen(TabSpec* a) {
	size_t n = 0;
	for(; a[n].type; n++);
	return n;
}


TabSpec* tabspeclistdup(TabSpec* old) {
	size_t i;
	TabSpec* new;

	if(!old) return NULL;
	
	new = malloc(sizeof(*new) * (tabspeclistlen(old) + 1));
	
	for(i = 0; old[i].type; i++) {
		new[i].type = old[i].type;
		new[i].path = old[i].path;
	}

	new[i].type = MCTAB_NONE;
	new[i].path = NULL;
	return new;
}


void freeptrlist(void* _p) {
	void** p = (void**)_p;
	void** q = p;
	while(*q) {
		free(*q);
		q++;
	}
	free(p);
}

#define true 1
#define false 0

#define set_int(x) x;
#define set_bool(x) x;
#define set_float(x) x;
#define set_double(x) x;
#define set_charp(x) strdup(x);
#define set_charpp(x) strlistdup(x);
#define set_tabsp(x) tabspeclistdup(x);
#define set_themep(x) x;
#define set_guisettingsp(x) x;
void GlobalSettings_LoadDefaults(GlobalSettings* s) {
	#define SETTING(type, name, val ,min,max) s->name = set_##type(val);
		GLOBAL_SETTING_LIST
	#undef SETTING
}

void ThemeSettings_LoadDefaults(ThemeSettings* s) {
	#define SETTING(type, name, val ,min,max) s->name = set_##type(val);
		THEME_SETTING_LIST
	#undef SETTING
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


static void grab_charpp(char*** out, json_value_t* obj, char* prop) {
	json_value_t* v;
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(v->type == JSON_TYPE_ARRAY) {
			if(*out) freeptrlist(*out);
			char** tmp = calloc(1, sizeof(*tmp) * (v->len + 1));
			
			json_link_t* link = v->arr.head;
			for(long i = 0; link; i++) {
				
				tmp[i] = json_as_strdup(link->v);
				
				link = link->next;
			}
			
			*out = tmp;
		}
	}
}


static void grab_tabsp(TabSpec** out, json_value_t* obj, char* prop) {
	json_value_t* v;
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(v->type == JSON_TYPE_ARRAY) {
			if(*out) free(*out);
			TabSpec* tmp = calloc(1, sizeof(*tmp) * (v->len + 1));

			json_link_t* link = v->arr.head;
			json_value_t* type_v;
			json_value_t* path_v;
			char* type_str;
			for(long i = 0; link; i++) {
				if(
					!json_obj_get_key(link->v, "type", &type_v)
					&& !json_obj_get_key(link->v, "path", &path_v)
				) {
					type_str = json_as_strdup(type_v);
					if(!strcasecmp(type_str, "FuzzyOpen")) {
						tmp[i].type = MCTAB_FUZZYOPEN;
					} else if(!strcasecmp(type_str, "FileOpen")) {
						tmp[i].type = MCTAB_FILEOPEN;
					} else {
						tmp[i].type = MCTAB_EDIT;
					}
					tmp[i].path = json_as_strdup(path_v);
					free(type_str);
				}
				link = link->next;
			}

			*out = tmp;
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

static void grab_double(double* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		*out = json_as_double(v);
	}
}

static void grab_themep(ThemeSettings** out, json_value_t* obj, char* prop) {
	json_value_t* v;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		ThemeSettings_LoadFromJSON(*out, v);
	}
}

static void grab_guisettingsp(GUI_GlobalSettings** out, json_value_t* obj, char* prop) {
	json_value_t* v;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		GUI_GlobalSettings_LoadFromJSON(*out, v);
	}
}



void GlobalSettings_LoadFromFile(GlobalSettings* s, char* path) {
	json_file_t* jsf;
	json_value_t* obj;
	
	jsf = json_load_path(path);
	if(!jsf) return;
	obj = jsf->root;
	
	#define SETTING(type, name, val ,min,max) grab_##type(&s->name, obj, #name);
		GLOBAL_SETTING_LIST
	#undef SETTING
	
	json_file_free(jsf);
}

void ThemeSettings_LoadFromJSON(ThemeSettings* s, struct json_value* jsv) {
	#define SETTING(type, name, val ,min,max) grab_##type(&s->name, jsv, #name);
		THEME_SETTING_LIST
	#undef SETTING
}
