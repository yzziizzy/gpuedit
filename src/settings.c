#include <stdlib.h>
#include <string.h>

#include "settings.h"

#include "c_json/json.h"





#define set_int(x) x;
#define set_float(x) x;
#define set_double(x) x;
#define set_string(x) strdup(x);
void GlobalSettings_loadDefaults(GlobalSettings* s) {
	#define SETTING(type, name, val) s->name = set_##type(val);
		SETTING_LIST
	#undef SETTING
}



static void grab_string(char** out, json_value_t* obj, char* prop) {
	json_value_t* v;
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(v->type == JSON_TYPE_STRING && v->v.str) {
			if(*out) free(*out);
			*out = strdup(v->v.str);
		}
	}
}

static void grab_int(int* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	int64_t i;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(!json_as_int(v, &i)) {
			*out = i;
		}
	}
}

static void grab_float(float* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	float i;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(!json_as_float(v, &i)) {
			*out = i;
		}
	}
}
static void grab_double(double* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	double i;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(!json_as_double(v, &i)) {
			*out = i;
		}
	}
}



void GlobalSettings_loadFromFile(GlobalSettings* s, char* path) {
	json_file_t* jsf;
	json_value_t* obj;
	
	jsf = json_load_path(path);
	obj = jsf->root;
	
	#define SETTING(type, name, val) grab_##type(&s->name, obj, #name);
		SETTING_LIST
	#undef SETTING
	
	// TODO: free json file
}
