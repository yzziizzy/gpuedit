#include <stdlib.h>
#include <string.h>

#include "../gui_deps.h"
#include "gui_settings.h"




static size_t strlistlen(char** a) {
	size_t n = 0;
	for(; a[n]; n++);
	return n;
}


static char** strlistdup(char** old) {
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





#define copy_int(x) x;
#define copy_bool(x) x;
#define copy_float(x) x;
#define copy_double(x) x;
#define copy_charp(x) strdup(x);
#define copy_charpp(x) strlistdup(x);
GUI_GlobalSettings* GUI_GlobalSettings_Copy(GUI_GlobalSettings* orig) {
	GUI_GlobalSettings* new = calloc(1, sizeof(*new));
	
	#define SETTING(type, name, val ,min,max) new->name = copy_##type(orig->name);
		GUI_SETTING_LIST
	#undef SETTING
	
	return new;
}


#define free_int(x)
#define free_bool(x)
#define free_float(x)
#define free_double(x)
#define free_charp(x) free(x);
#define free_charpp(x) freeptrlist(x);
void GUI_GlobalSettings_Free(GUI_GlobalSettings* s) {
	#define SETTING(type, name, val ,min,max) free_##type(s->name);
		GUI_SETTING_LIST
	#undef SETTING
}



#define true 1
#define false 0

#define set_int(x) x;
#define set_bool(x) x;
#define set_float(x) x;
#define set_double(x) x;
#define set_charp(x) strdup(x);
#define set_charpp(x) strlistdup(x);




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



void GUI_GlobalSettings_LoadDefaults(GUI_GlobalSettings* s) {
	#define SETTING(type, name, val ,min,max) s->name = set_##type(val);
		GUI_SETTING_LIST
	#undef SETTING
}


void GUI_GlobalSettings_LoadFromFile(GUI_GlobalSettings* s, char* path) {
	json_file_t* jsf;
	json_value_t* obj;
	
	jsf = json_load_path(path);
	if(!jsf) return;

	GUI_GlobalSettings_LoadFromJSON(s, jsf->root);
	
	
	json_file_free(jsf);
}
	

void GUI_GlobalSettings_LoadFromJSON(GUI_GlobalSettings* s, struct json_value* jsv) {
	#define SETTING(type, name, val ,min,max) grab_##type(&s->name, jsv, #name);
		GUI_SETTING_LIST
	#undef SETTING
}



