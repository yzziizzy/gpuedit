#include <stdlib.h>
#include <string.h>

#include "settings.h"
#include "sti/sti.h"

#include "c_json/json.h"



#define true 1
#define false 0

#define set_int(x) x;
#define set_bool(x) x;
#define set_float(x) x;
#define set_double(x) x;
#define set_charp(x) strdup(x);
void GlobalSettings_loadDefaults(GlobalSettings* s) {
	#define SETTING(type, name, val ,min,max) s->name = set_##type(val);
		SETTING_LIST
	#undef SETTING
}



static void grab_charp(char** out, json_value_t* obj, char* prop) {
	json_value_t* v;
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(v->type == JSON_TYPE_STRING && v->v.str) {
			if(*out) free(*out);
			*out = strdup(v->v.str);
		}
	}
}

static void grab_bool(char* out, json_value_t* obj, char* prop) {
	json_value_t* v;
	int64_t i;
	
	if(!json_obj_get_key(obj, prop, &v) && v != NULL) {
		if(!json_as_int(v, &i)) {
			*out = i;
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
	if(!jsf) return;
	obj = jsf->root;
	
	#define SETTING(type, name, val ,min,max) grab_##type(&s->name, obj, #name);
		SETTING_LIST
	#undef SETTING
	
	json_file_free(jsf);
}


void GlobalSettings_loadFromSimpleFile(GlobalSettings* s, char* path) {
	size_t len;
	
	char* src = readWholeFile(path, &len);
	
	
	char** lines = strsplit_inplace(src, '\n', NULL);
	
	char** lines2 = lines;
	for(int ln = 1; *lines2; lines2++, ln++) {
		char name[128];
		char value[128];
// 		StyleInfo* style;
		
		if(2 != sscanf(*lines2, " %127[_a-zA-Z0-9] = %127s", name, value)) {
			printf("Invalid config line %s:%d: '%s'\n", path, ln, *lines2);
			continue;
		}
		
// 		printf("line: '%s' = '%s'\n", name, value);
		/*
		style = get_style(h, name);
		if(!style) {
			fprintf(stderr, "Unknown style name '%s' in %s:%d\n", name, path, ln);
			continue;
		}*/
		
		/*
		if(value[0] == '#') { // hex code
			decodeHexColorNorm(value, &style->fgColor);
		}*/
		
		// TODO: rgba()
		// TODO: backgrounds, formats, fonts, etc
		
	}
	
	
	free(lines);
	free(src);
	
	
	
}
