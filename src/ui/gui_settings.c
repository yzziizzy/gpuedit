#include <stdlib.h>
#include <string.h>

#include "../gui_deps.h"
#include "gui.h"




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
#define copy_Color4(x) x;
#define copy_Font(x) x;
#define copy_charp(x) strdup(x);
#define copy_charpp(x) strlistdup(x);
GUISettings* GUISettings_Copy(GUIManager* gm, GUISettings* orig) {
	GUISettings* new = calloc(1, sizeof(*new));
	
	#define SETTING(type, name, val ,min,max) new->name = copy_##type(orig->name);
		GUI_SETTING_LIST
	#undef SETTING
	
	return new;
}


#define free_int(x)
#define free_bool(x)
#define free_float(x)
#define free_double(x)
#define free_Color4(x)
#define free_Font(x)
#define free_charp(x) free(x);
#define free_charpp(x) freeptrlist(x);
void GUISettings_Free(void* useless, GUISettings* s) {
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
#define set_Color4(x) x;
#define set_Font(x) x;
#define set_charp(x) strdup(x);
#define set_charpp(x) strlistdup(x);
#define set_Vector2(x) x;
#define set_Color4(x) x;




static void grab_Vector2(GUIManager* gm, Vector2* out, json_value_t* obj, char* prop) {
//	json_value_t* v;
//	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
//		if(v->type == JSON_TYPE_STRING && v->s) {
//			decodeHexColorNorm(v->s, (float*)out);
//		}
//	}
}

static void grab_Color4(GUIManager* gm, Color4* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(v->type == JSON_TYPE_STRING && v->s) {
			decodeHexColorNorm(v->s, (float*)out);
		}
	}
}

static void grab_Font(GUIManager* gm, GUIFont** out, json_value_t* obj, char* prop) {
	json_value_t* v;
	*out = NULL;
}

static void grab_charp(GUIManager* gm, char** out, json_value_t* obj, char* prop) {
	json_value_t* v;
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(v->type == JSON_TYPE_STRING && v->s) {
			if(*out) free(*out);
			*out = strdup(v->s);
		}
	}
}


static void grab_charpp(GUIManager* gm, char*** out, json_value_t* obj, char* prop) {
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



static void grab_bool(GUIManager* gm, char* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	int64_t i;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		*out = json_as_int(v);
	}
}

static void grab_int(GUIManager* gm, int* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		*out = json_as_int(v);
	}
}

static void grab_float(GUIManager* gm, float* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		*out = json_as_float(v);
	}
}

static void grab_double(GUIManager* gm, double* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		*out = json_as_double(v);
	}
}




void GUISettings_LoadDefaults(GUIManager* gm, GUISettings* s) {
	#define SETTING(type, name, val ,min,max) s->name = set_##type(val);
		GUI_SETTING_LIST
	#undef SETTING
	
	#define X(a, b, ...) \
		a##_LoadDefaults(&s->raw.a[0], 0); \
		a##_LoadDefaults(&s->raw.a[1], 1); \
		a##_LoadDefaults(&s->raw.a[2], 2); \
		a##_LoadDefaults(&s->opts.a[0], 0); \
		a##_LoadDefaults(&s->opts.a[1], 1); \
		a##_LoadDefaults(&s->opts.a[2], 2);
		
		GUI_CONTROL_OPTS_STRUCT_LIST
	#undef X
	
}


GUISettings* GUISettings_Alloc(GUIManager* gm) {
	return calloc(1, sizeof(GUISettings));
}


void GUISettings_LoadFromFile(GUIManager* gm, GUISettings* s, char* path) {
	json_file_t* jsf;
	json_value_t* obj;
	
	jsf = json_load_path(path);
	if(!jsf) {
		L_ERROR("Failed to open config file '%s'\n", path);
		return;
	}
	else if(jsf->error) {
		L_ERROR("Error loading config file: '%s'\n", path);
		L_ERROR("json error: %s %ld:%ld\n", jsf->error_str, jsf->error_line_num, jsf->error_char_num);
		return;
	}
	GUISettings_LoadJSON(gm, s, jsf->root);
	
	
	json_file_free(jsf);
}
	


void GUISettings_LoadJSON(GUIManager* gm, GUISettings* s, struct json_value* jsv) {
	#define SETTING(type, name, val ,min,max) grab_##type(gm, &s->name, jsv, #name);
		GUI_SETTING_LIST
	#undef SETTING
	

	
//	GUI_CONTROL_OPS_STRUCT_LIST

}





