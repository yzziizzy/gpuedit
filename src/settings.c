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

	new[i].type = MCTAB_NONE;
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



#define true 1
#define false 0

#define set_int(x) x;
#define set_bool(x) x;
#define set_float(x) x;
#define set_double(x) x;
#define set_charp(x) strdup(x);
#define set_charpp(x) strlistdup(x);
#define set_tabsp(x) tabspeclistdup(x);
#define set_scrollfn(x) x;
#define set_widsp(x) widgetspeclistdup(x);
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





#define copy_int(x) x;
#define copy_bool(x) x;
#define copy_float(x) x;
#define copy_double(x) x;
#define copy_charp(x) strdup(x);
#define copy_charpp(x) strlistdup(x);
#define copy_tabsp(x) tabspeclistdup(x);
#define copy_scrollfn(x) x;
#define copy_widsp(x) widgetspeclistdup(x);
#define copy_themep(x) ThemeSettings_Copy(x);
#define copy_guisettingsp(x) GUI_GlobalSettings_Copy(x);
GlobalSettings* GlobalSettings_Copy(GlobalSettings* orig) {
	GlobalSettings* new = calloc(1, sizeof(*new));
	
	#define SETTING(type, name, val ,min,max) new->name = copy_##type(orig->name);
		GLOBAL_SETTING_LIST
	#undef SETTING
	
	return new;
}

ThemeSettings* ThemeSettings_Copy(ThemeSettings* orig) {
	ThemeSettings* new = calloc(1, sizeof(*new));

	#define SETTING(type, name, val ,min,max) new->name = copy_##type(orig->name);
		THEME_SETTING_LIST
	#undef SETTING
	
	return new;
}


#define free_int(x)
#define free_bool(x)
#define free_float(x)
#define free_double(x)
#define free_charp(x) free(x);
#define free_charpp(x) freeptrlist(x);
#define free_tabsp(x) freetabspeclist(x);
#define free_scrollfn(x)
#define free_widsp(x) freewidgetspeclist(x);
#define free_themep(x) ThemeSettings_Free(x);
#define free_guisettingsp(x) GUI_GlobalSettings_Free(x);
void GlobalSettings_Free(GlobalSettings* s) {
	#define SETTING(type, name, val ,min,max) free_##type(s->name);
		GLOBAL_SETTING_LIST
	#undef SETTING
}

void ThemeSettings_Free(ThemeSettings* s) {
	#define SETTING(type, name, val ,min,max) free_##type(s->name);
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
					} else if(!strcasecmp(type_str, "GrepOpen")) {
						tmp[i].type = MCTAB_GREPOPEN;
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



// reads path/.gpuedit.json and path/.gpuedit/*.json
// returns the number of files read
int GlobalSettings_ReadDefaultsAt(GlobalSettings* gs, char* path) {
	int files_read = 0;
	
	char* path_2 = resolve_path(path);
	
	char* single_path = path_join(path_2, ".gpuedit.json");
	files_read += !GlobalSettings_LoadFromFile(gs, single_path);
	free(single_path);
	
	char* dir_path = path_join(path, ".gpuedit");
	files_read += GlobalSettings_ReadAllJSONAt(gs, dir_path);
	free(dir_path);
	
	free(path_2);

	return files_read;
}


// reads path/*.json
// returns the number of files read
int GlobalSettings_ReadAllJSONAt(GlobalSettings* gs, char* path) {
	int files_read = 0;
	
	char* path_2 = resolve_path(path);
	if(is_path_a_dir(path_2)) {
		size_t num_files = 0;
		char* conf_glob = path_join(path_2, "*.json");
		char** files;
		
		files = multi_wordexp_dup(conf_glob, &num_files);
		
		for(size_t i = 0; i < num_files; i++) {
			files_read += !GlobalSettings_LoadFromFile(gs, files[i]);
			free(files[i]);
		}
		
		free(conf_glob);
		free(files);
	}
	free(path_2);
	
	return files_read;
}
