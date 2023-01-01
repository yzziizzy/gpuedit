#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "settings.h"
#include "sti/sti.h"

#include "c_json/json.h"


char* mctab_type_names[] = {
	[MCTAB_None] = "None",
#define X(x,...) [MCTAB_##x] = #x,
	MCTAB_TYPE_LIST
#undef X
};


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

static void freeptrlist(void* _p) {
	void** p = (void**)_p;
	void** q = p;
	while(*q) {
		free(*q);
		q++;
	}
	free(p);
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
		new[i].path = old[i].path ? strdup(old[i].path) : NULL;
	}

	new[i].type = MCTAB_None;
	new[i].path = NULL;
	return new;
}


void freetabspeclist(TabSpec* ts) {
	for(int i = 0; ts[i].type; i++) 
		if(ts[i].path) free(ts[i].path);
		
	free(ts);
}

size_t widgetspeclistlen(WidgetSpec* a) {
	size_t n = 0;
	for(; a[n].type; n++);
	return n;
}


WidgetSpec* widgetspeclistdup(WidgetSpec* old) {
	size_t i;
	WidgetSpec* new;

	if(!old) return NULL;
	
	new = malloc(sizeof(*new) * (widgetspeclistlen(old) + 1));
	
	for(i = 0; old[i].type; i++) {
		new[i].type = old[i].type;
		new[i].size = old[i].size;
		new[i].align = old[i].align;
		new[i].format = old[i].format ? strdup(old[i].format) : NULL;
	}

	new[i].type = MCWID_NONE;
	new[i].size = 0;
	new[i].align = 'l';
	new[i].format = NULL;
	return new;
}

void freewidgetspeclist(WidgetSpec* ws) {
	for(int i = 0; ws[i].type; i++) {
		if(ws[i].format) free(ws[i].format);
	}
	free(ws);
}





#include "settings_macros_on.h"

void GeneralSettings_LoadDefaults(void* useless, GeneralSettings* s) {
	#define SETTING(type, name, val ,min,max) s->name = set_##type(val);
		GENERAL_SETTING_LIST
	#undef SETTING

}


GeneralSettings* GeneralSettings_Alloc(void* useless) {
	return calloc(1, sizeof(GeneralSettings));
}

GeneralSettings* GeneralSettings_Copy(void* useless, GeneralSettings* orig) {
	GeneralSettings* new = calloc(1, sizeof(*new));
	
	#define SETTING(type, name, val ,min,max) new->name = copy_##type(orig->name);
		GENERAL_SETTING_LIST
	#undef SETTING
	
	return new;
}

void GeneralSettings_Free(void* useless, GeneralSettings* s) {
	#define SETTING(type, name, val ,min,max) free_##type(s->name);
		GENERAL_SETTING_LIST
	#undef SETTING
}
#include "settings_macros_off.h"









/*
static void grab_Color4(Color4* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(v->type == JSON_TYPE_STRING && v->s) {
			decodeHexColorNorm(v->s, (float*)out);
		}
	}
}
*/

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
					
					tmp[i].type = MCTAB_Buffer;
					for(int tt = 0; tt < MCTAB_MAX_VALUE; tt++) {
						if(!strcasecmp(type_str, mctab_type_names[tt])) {
							tmp[i].type = tt;
							break;
						}
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


static void grab_widsp(WidgetSpec** out, json_value_t* obj, char* prop) {
	json_value_t* v;
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(v->type == JSON_TYPE_ARRAY) {
			if(*out) free(*out);
			WidgetSpec* tmp = calloc(1, sizeof(*tmp) * (v->len + 1));

			json_link_t* link = v->arr.head;
			json_value_t* type_v;
			json_value_t* size_v;
			json_value_t* align_v;
			json_value_t* format_v;
			char* type_str;
			char* align_str;
			char* format_str;
			for(long i = 0; link; i++) {
				if(
					!json_obj_get_key(link->v, "type", &type_v)
					&& !json_obj_get_key(link->v, "size", &size_v)
					&& !json_obj_get_key(link->v, "align", &align_v)
					&& !json_obj_get_key(link->v, "format", &format_v)
				) {
					type_str = json_as_strdup(type_v);
					align_str = json_as_strdup(align_v);
					format_str = json_as_strdup(format_v);
					if(!strcasecmp(type_str, "hello_world")) {
						tmp[i].type = MCWID_HELLO;
					} else if(!strcasecmp(type_str, "ping")) {
						tmp[i].type = MCWID_PING;
					} else if(!strcasecmp(type_str, "clock")) {
						tmp[i].type = MCWID_CLOCK;
					} else if(!strcasecmp(type_str, "battery")) {
						tmp[i].type = MCWID_BATTERY;
					} else if(!strcasecmp(type_str, "linecol")) {
						tmp[i].type = MCWID_LINECOL;
					} else {
						tmp[i].type = MCWID_NONE;
					}
					tmp[i].size = json_as_int(size_v);
					tmp[i].align = align_str[0];
					tmp[i].format = format_str;
					free(type_str);
					free(align_str);
//					free(format_str);
				}
				link = link->next;
			}

			*out = tmp;
		}
	}
}

static char* tab_scroll_fn_name_list[] = { 
#define X(x) [TABSC_##x] = #x,
	TAB_SCROLL_TYPE_LIST
#undef X
};

static void grab_scrollfn(int* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL && v->type == JSON_TYPE_STRING) {
		
		int len = sizeof(tab_scroll_fn_name_list) / sizeof(tab_scroll_fn_name_list[0]);
		for(int i = 0; i < len; i++) {
			
			if(0 == strcasecmp(tab_scroll_fn_name_list[i], v->s)) {
				*out = i;
				return;
			}
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


void GeneralSettings_LoadJSON(void* useless, GeneralSettings* s, struct json_value* jsv) {
	#define SETTING(type, name, val ,min,max) grab_##type(&s->name, jsv, #name);
		GENERAL_SETTING_LIST
	#undef SETTING

}




static SettingsSection* find_section(Settings* s, unsigned long bit) {
	VEC_EACHP(&s->sections, i, sec) {
		if(sec->bit == bit) return sec;
	}
	
	return NULL;
}

static SettingsSection* assert_section(Settings* s, unsigned long bit) {
	SettingsSection* sec = find_section(s, bit);
	
	if(!sec) {
		VEC_PUSH(&s->sections, ((SettingsSection){0}));
		sec = &VEC_TAIL(&s->sections);
		sec->bit = bit;
	}
	
	return sec;
}

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
) {
	SettingsSection* sec = assert_section(s, bit);
	*sec = (SettingsSection){
		.key = key,
		.bit = bit,
		.dataStore = NULL,
		.userData = userData,
		.alloc = a,
		.copy = c,
		.free = f,
		.defaults = d,
		.loader = l,
	};
}


Settings* Settings_Copy(Settings* s, unsigned long mask) {
	Settings* new = calloc(1, sizeof(*new));
	new->parent = s;
	VEC_COPY(&new->sections, &s->sections);
	
	VEC_EACHP(&new->sections, i, sec) {
		if(mask && mask & sec->bit == 0) {
			sec->dataStore = NULL;
			continue;
		}
	
		if(sec->dataStore && sec->copy) {
			sec->dataStore = sec->copy(sec->userData, sec->dataStore);
		}
	}
	
	return new;
}

void Settings_Free(Settings* s) {
	
	VEC_EACHP(&s->sections, i, sec) {	
		if(sec->dataStore && sec->free) {
			sec->free(sec->userData, sec->dataStore);
		}
	}

	VEC_FREE(&s->sections);
}

void Settings_LoadDefaults(Settings* s, unsigned long mask) {
	
	VEC_EACHP(&s->sections, i, sec) {
		if(mask && ((mask & sec->bit) == 0)) continue;
		
		if(!sec->dataStore) {
			sec->dataStore = sec->alloc(sec->userData); 
		}
		
		if(sec->defaults) {
			sec->defaults(sec->userData, sec->dataStore);
		}
	}
}

void Settings_LoadJSON(Settings* s, struct json_value* v, unsigned long mask) {
	json_value_t* v2;
	
	VEC_EACHP(&s->sections, i, sec) {
		if(mask && ((mask & sec->bit) == 0)) continue;
	
		
		if(!json_obj_get_key(v, sec->key, &v2) && v2 != NULL) {
			if(!sec->dataStore) {
				sec->dataStore = sec->alloc(sec->userData); 
				
				if(sec->defaults) {
					sec->defaults(sec->userData, sec->dataStore);
				}
			}
			
			if(sec->loader) {
				sec->loader(sec->userData, sec->dataStore, v2);
			}
		}
	}
}

int Settings_LoadFile(Settings* s, char* path, unsigned long mask) {
	json_file_t* jsf;
	
	jsf = json_load_path(path);
	if(!jsf) {
		L4("Failed to open config file '%s'\n", path);
		return 1;
	}
	
	L5("Reading config file '%s'\n", path);
		
	Settings_LoadJSON(s, jsf->root, mask);
	
	json_file_free(jsf);
	
	return 0;
}

void* Settings_GetSection(Settings* s, unsigned long bit) {
	SettingsSection* sec = find_section(s, bit);
	return sec ? sec->dataStore : NULL;
}


/*
// returns 0 if the file was read
int GlobalSettings_LoadFromFile(GlobalSettings* s, char* path) {
	json_file_t* jsf;
	json_value_t* obj;
	
	jsf = json_load_path(path);
	if(!jsf) return 1;
	obj = jsf->root;
	
	#define SETTING(type, name, val ,min,max) grab_##type(&s->name, obj, #name);
		GLOBAL_SETTING_LIST
	#undef SETTING
	
	json_file_free(jsf);
	
	return 0;
}

void ThemeSettings_LoadFromJSON(ThemeSettings* s, struct json_value* jsv) {
	#define SETTING(type, name, val ,min,max) grab_##type(&s->name, jsv, #name);
		THEME_SETTING_LIST
	#undef SETTING
}
*/


// reads path/.gpuedit.json and path/.gpuedit/*.json
// returns the number of files read
int Settings_ReadDefaultFilesAt(Settings* s, char* path, unsigned long mask) {
	int files_read = 0;
	
	char* path_2 = resolve_path(path);
	
	char* single_path = path_join(path_2, ".gpuedit.json");
	files_read += !Settings_LoadFile(s, single_path, mask);
	free(single_path);
	
	char* dir_path = path_join(path, ".gpuedit");
	files_read += Settings_ReadAllJSONAt(s, dir_path, mask);
	free(dir_path);
	
	free(path_2);

	return files_read;
}


// reads path/*.json
// returns the number of files read
int Settings_ReadAllJSONAt(Settings* s, char* path, unsigned long mask) {
	int files_read = 0;
	
	char* path_2 = resolve_path(path);
	if(is_path_a_dir(path_2)) {
		size_t num_files = 0;
		char* conf_glob = path_join(path_2, "*.json");
		char** files;
		
		files = multi_wordexp_dup(conf_glob, &num_files);
		
		for(size_t i = 0; i < num_files; i++) {
			files_read += !Settings_LoadFile(s, files[i], mask);
			free(files[i]);
		}
		
		free(conf_glob);
		free(files);
	}
	free(path_2);
	
	return files_read;
}

