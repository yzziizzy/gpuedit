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
void GlobalSettings_loadDefaults(GlobalSettings* s) {
	#define SETTING(type, name, val ,min,max) s->name = set_##type(val);
		SETTING_LIST
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


void GlobalSettings_loadFromFile(GlobalSettings* s, char* path) {
	json_file_t* jsf;
	json_value_t* obj;
	
	jsf = json_load_path(path);
	if(!jsf) return;
	obj = jsf->root;
	
	#define SETTING(type, name, val ,min,max) grab_##type(&s->name, obj, #name);
		SETTING_LIST
	#undef SETTING
	
	json_file_free(jsf);
}
