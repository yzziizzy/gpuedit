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
	
	#define C4(r,g,b,a) ((Color4){r,g,b,a})
	
	#define G(a,b) s->a.b
	#define I0(a,b) a[0].b
	#define I1(a,b) a[1].b
	#define I2(a,b) a[2].b
	#define F0(a,b0,b1,b2) a = b0;
	#define F1(a,b0,b1,b2) a = b1;
	#define F2(a,b0,b1,b2) a = b2;
	//#define XX(t,n,v) n = set_##t(v);
	#define XX(t,n,v) n = v;
	
	#define H0(a, b) I0(a, O(F0)b)
	#define H1(a, b) I1(a, O(F1)b)
	#define H2(a, b) I2(a, O(F2)b)
	#define X(t,n,v1,v2,v3) (n, v1,v2,v3)
	#define Y(a, b, ...) EX(H0, a, __VA_ARGS__), EX(H1, a, __VA_ARGS__), EX(H2, a, __VA_ARGS__)
	//#define V(m, ...) __VA_ARGS__
	#define V(m, ...) EX2(G, m, __VA_ARGS__)
		GUI_CONTROL_OPS_STRUCT_LIST
	#undef G
	#undef H0
	#undef H1
	#undef H2
	#undef I0
	#undef I1
	#undef I2
	#undef F0
	#undef F1
	#undef F2
	#undef XX
	#undef X
	#undef Y
	#undef V
	
	#undef C4
	
}

GUISettings* GUISettings_Alloc(GUIManager* gm) {
	return calloc(1, sizeof(GUISettings));
}


void GUISettings_LoadFromFile(GUIManager* gm, GUISettings* s, char* path) {
	json_file_t* jsf;
	json_value_t* obj;
	
	jsf = json_load_path(path);
	if(!jsf) return;

	GUISettings_LoadJSON(gm, s, jsf->root);
	
	
	json_file_free(jsf);
}
	

void GUISettings_LoadJSON(GUIManager* gm, GUISettings* s, struct json_value* jsv) {
	#define SETTING(type, name, val ,min,max) grab_##type(gm, &s->name, jsv, #name);
		GUI_SETTING_LIST
	#undef SETTING
	
	
//	#define C4(r,g,b,a) ((Color4){r,g,b,a})
	
	
	#define K(...) P(K_, NARG(__VA_ARGS__))(__VA_ARGS__)
	#define K_5(sn,ssn,ind,mem,t) grab_##t(gm, &s->sn.ssn[ind].mem, jsv, #mem);
	#define K_3(sn,mem,t) grab_##t(gm, &s->sn.mem, jsv, "foo");
	
	#define G(sn,ssn_mem_t) (sn, O(UNWRAP)ssn_mem_t)
	#define UNWRAP(...) __VA_ARGS__
	
	// F adds the array index in
	#define F0(...) 0, __VA_ARGS__
	#define F1(...) 1, __VA_ARGS__
	#define F2(...) 2, __VA_ARGS__
	#define XX(t,mem,v) (mem, t)
	
	// ssn is the sub-struct name
	#define H0(ssn, t_mem) (ssn, O(F0)t_mem)
	#define H1(ssn, t_mem) (ssn, O(F1)t_mem)
	#define H2(ssn, t_mem) (ssn, O(F2)t_mem)
	
	// t is the type, mem is the member name
	#define X(t,mem,v1,v2,v3) (mem, t)
	#define Y(ssn, cnt, ...) EX(H0, ssn, __VA_ARGS__), EX(H1, ssn, __VA_ARGS__), EX(H2, ssn, __VA_ARGS__)
	#define V(sn, ...) EXE(K, EX(G, sn, __VA_ARGS__))
	
	#define EXE(m, ...) P(EXE_, NARG(__VA_ARGS__))(m, __VA_ARGS__)
	#define EXE_1(m,x) O(m)x
	#define EXE_2(m,x, ...) O(m)x EXE_1(m,__VA_ARGS__)
	#define EXE_3(m,x, ...) O(m)x EXE_2(m,__VA_ARGS__)
	#define EXE_4(m,x, ...) O(m)x EXE_3(m,__VA_ARGS__)
	#define EXE_5(m,x, ...) O(m)x EXE_4(m,__VA_ARGS__)
	#define EXE_6(m,x, ...) O(m)x EXE_5(m,__VA_ARGS__)
	#define EXE_7(m,x, ...) O(m)x EXE_6(m,__VA_ARGS__)
	#define EXE_8(m,x, ...) O(m)x EXE_7(m,__VA_ARGS__)
	#define EXE_9(m,x, ...) O(m)x EXE_8(m,__VA_ARGS__)
	#define EXE_10(m,x, ...) O(m)x EXE_9(m,__VA_ARGS__)
	#define EXE_11(m,x, ...) O(m)x EXE_10(m,__VA_ARGS__)
	#define EXE_12(m,x, ...) O(m)x EXE_11(m,__VA_ARGS__)
	#define EXE_13(m,x, ...) O(m)x EXE_12(m,__VA_ARGS__)
	#define EXE_14(m,x, ...) O(m)x EXE_13(m,__VA_ARGS__)
	#define EXE_15(m,x, ...) O(m)x EXE_14(m,__VA_ARGS__)
	#define EXE_16(m,x, ...) O(m)x EXE_15(m,__VA_ARGS__)
	#define EXE_17(m,x, ...) O(m)x EXE_16(m,__VA_ARGS__)
	#define EXE_18(m,x, ...) O(m)x EXE_17(m,__VA_ARGS__)
	#define EXE_19(m,x, ...) O(m)x EXE_18(m,__VA_ARGS__)
	#define EXE_20(m,x, ...) O(m)x EXE_19(m,__VA_ARGS__)
	#define EXE_21(m,x, ...) O(m)x EXE_20(m,__VA_ARGS__)
	#define EXE_22(m,x, ...) O(m)x EXE_21(m,__VA_ARGS__)
	#define EXE_23(m,x, ...) O(m)x EXE_22(m,__VA_ARGS__)
	#define EXE_24(m,x, ...) O(m)x EXE_23(m,__VA_ARGS__)
	#define EXE_25(m,x, ...) O(m)x EXE_24(m,__VA_ARGS__)
	#define EXE_26(m,x, ...) O(m)x EXE_25(m,__VA_ARGS__)
	#define EXE_27(m,x, ...) O(m)x EXE_26(m,__VA_ARGS__)
	#define EXE_28(m,x, ...) O(m)x EXE_27(m,__VA_ARGS__)
	#define EXE_29(m,x, ...) O(m)x EXE_28(m,__VA_ARGS__)
	#define EXE_30(m,x, ...) O(m)x EXE_29(m,__VA_ARGS__)
	
	GUI_CONTROL_OPS_STRUCT_LIST
	
//	#undef C4
	
}



