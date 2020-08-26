
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#include "../common_math.h"
#include "../sti/sti.h"

#include "../utilities.h"

#include "../gui.h"
#include "../gui_internal.h"


#include "../c_json/json.h"
#include "../json_gl.h"




typedef GUIObject* (*creator_fn)(GUIManager*, json_value_t*);

static struct Color4 json_get_color(json_value_t* v) {
	struct Color4 c = {0,0,0,1};
	
	if(!v) return c;
	
	if(v->type == JSON_TYPE_STRING) {
		// TODO: lookup table
		decodeHexColorNorm(v->v.str, (float*)&c);
	}
	else if(v->type == JSON_TYPE_ARRAY) {
		switch(v->v.arr->length) {
			case 1:
// 				c.r = v->v.arr->head->value; 
			
			case 3:
			case 4:
// 				c.r = 
				
			default:
				printf("Invalid color array format in GUI config loader.\n");
		}
	}
	else if(v->type == JSON_TYPE_OBJ) {
		
	}
	
	return c;
}


static void read_gravity(GUIHeader* h, json_value_t* cfg) {
	char* s = json_obj_get_string(cfg, "gravity");
	if(!s) return;
	
	int grav = GUI_GRAV_TOP_LEFT;
	
	// TODO: test to see if this is faster than another structure
	     if(0 == strcasecmp(s, "CENTER_LEFT")) grav = GUI_GRAV_CENTER_LEFT;
	else if(0 == strcasecmp(s, "LEFT_CENTER")) grav = GUI_GRAV_CENTER_LEFT;
	else if(0 == strcasecmp(s, "BOTTOM_LEFT")) grav = GUI_GRAV_BOTTOM_LEFT;
	else if(0 == strcasecmp(s, "LEFT_BOTTOM")) grav = GUI_GRAV_BOTTOM_LEFT;
	else if(0 == strcasecmp(s, "BOTTOM_CENTER")) grav = GUI_GRAV_CENTER_BOTTOM;
	else if(0 == strcasecmp(s, "CENTER_BOTTOM")) grav = GUI_GRAV_CENTER_BOTTOM;
	else if(0 == strcasecmp(s, "BOTTOM_RIGHT")) grav = GUI_GRAV_BOTTOM_RIGHT;
	else if(0 == strcasecmp(s, "RIGHT_BOTTOM")) grav = GUI_GRAV_BOTTOM_RIGHT;
	else if(0 == strcasecmp(s, "CENTER_RIGHT")) grav = GUI_GRAV_CENTER_RIGHT;
	else if(0 == strcasecmp(s, "RIGHT_CENTER")) grav = GUI_GRAV_CENTER_RIGHT;
	else if(0 == strcasecmp(s, "TOP_RIGHT")) grav = GUI_GRAV_TOP_RIGHT;
	else if(0 == strcasecmp(s, "RIGHT_TOP")) grav = GUI_GRAV_TOP_RIGHT;
	else if(0 == strcasecmp(s, "TOP_CENTER")) grav = GUI_GRAV_CENTER_TOP;
	else if(0 == strcasecmp(s, "CENTER_TOP")) grav = GUI_GRAV_CENTER_TOP;
	else if(0 == strcasecmp(s, "CENTER")) grav = GUI_GRAV_CENTER;
	else if(0 == strcasecmp(s, "CENTER_CENTER")) grav = GUI_GRAV_CENTER;
	
	h->gravity = grav;
}

static void read_header(GUIHeader* h, json_value_t* cfg) {
	json_value_t* v;
	double d;
	int64_t n;
	
	read_gravity(h, cfg);
	
	if(!json_obj_get_key(cfg, "pos", &v)) {
		json_as_vector(v, 2, &h->topleft);
	}
	if(!json_obj_get_key(cfg, "position", &v)) {
		json_as_vector(v, 2, &h->topleft);
	}
	if(!json_obj_get_key(cfg, "topleft", &v)) {
		json_as_vector(v, 2, &h->topleft);
	}
	
	if(!json_obj_get_key(cfg, "size", &v)) {
		json_as_vector(v, 2, &h->size);
		GUIResize(h, h->size);
	}
	
	if(!json_obj_get_key(cfg, "scale", &v)) {
		json_as_double(v, &d);
		h->scale = d;
	}
	
	if(!json_obj_get_key(cfg, "alpha", &v)) {
		json_as_double(v, &d);
		h->alpha = d;
	}
	
	if(!json_obj_get_key(cfg, "z", &v)
		|| !json_obj_get_key(cfg, "z-index", &v)
		|| !json_obj_get_key(cfg, "zIndex", &v)
	) {
		json_as_double(v, &d);
		h->z = d;
	}
	
	if(!json_obj_get_key(cfg, "hidden", &v)) {
		json_as_int(v, &n);
		h->hidden = n;
	}
	
	// TODO: convert from strings
	if(!json_obj_get_key(cfg, "cursor", &v)) {
		json_as_int(v, &n);
		h->hidden = n;
	}
	
	// TODO: flags
	
}




static GUIObject* create_GUIButton(GUIManager* gm, json_value_t* cfg) {
	GUIButton* obj;
	
	char* s = json_obj_get_string(cfg, "value");
	
	obj = GUIButton_New(gm, s || "");
	obj->isDisabled = json_obj_get_int(cfg, "disabled", 0);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIColumnLayout(GUIManager* gm, json_value_t* cfg) {
	GUIColumnLayout* obj;
	Vector2 defaultSpacing = {0.01, 0.01};
	
	// TODO: read json for values
	
	obj = GUIColumnLayout_new(gm, defaultSpacing, 0.02, 0);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIDebugAdjuster(GUIManager* gm, json_value_t* cfg) {
	GUIDebugAdjuster* obj;
	
	// TODO: read json for values
	char* fmt = json_obj_get_string(cfg, "format");
	// TODO string type conversion
	int type = 0;
	obj = GUIDebugAdjuster_new(gm, fmt, NULL, type);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIEdit(GUIManager* gm, json_value_t* cfg) {
	GUIEdit* obj;
	
	char* value = json_obj_get_string(cfg, "value");
	// TODO smarter conversion from json
	// TODO fonts, etc
	int type = 0;
	obj = GUIEdit_New(gm, value);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIGridLayout(GUIManager* gm, json_value_t* cfg) {
	GUIGridLayout* obj;
	Vector2 defaultSpacing = {0.01, 0.01};
	json_value_t* v;
	float f;
	
	if(!json_obj_get_key(cfg, "cellSize", &v)) {
		json_as_vector(v, 2, &defaultSpacing);
	}
	
	obj = GUIGridLayout_new(gm, (Vector2){0,0}, defaultSpacing);
	
	obj->maxCols = json_obj_get_int(cfg, "maxCols", 10);
	obj->maxRows = json_obj_get_int(cfg, "maxRows", 10);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIImage(GUIManager* gm, json_value_t* cfg) {
	GUIImage* obj;
	char* defaultImgName = "pre/denied";
	
	char* value = json_obj_get_string(cfg, "imgName");
	// TODO smarter conversion from json
	
	obj = GUIImage_new(gm, value || defaultImgName);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIImageButton(GUIManager* gm, json_value_t* cfg) {
	GUIImageButton* obj;
	float border;
	char* defaultImgName = "pre/denied";
	
	border = json_obj_get_double(cfg, "border", 2);
	char* s = json_obj_get_string(cfg, "image");
	if(!s) s = defaultImgName;
	
	obj = GUIImageButton_New(gm, border, s);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIPerformanceGraph(GUIManager* gm, json_value_t* cfg) {
	GUIPerformanceGraph* obj;
	Vector2 defaultSpacing = {0.01, 0.01};
	
	// TODO: read json for values
	
	obj = guiPerformanceGraphNew(gm, defaultSpacing, 0.02, 120);
	
	return (GUIObject*)obj;
}


static GUIObject* create_GUISimpleWindow(GUIManager* gm, json_value_t* cfg) {
	GUISimpleWindow* obj;
	Vector2 defaultSpacing = {0.01, 0.01};
	
	// TODO: read json for values
	
	obj = GUISimpleWindow_New(gm);
	
	char* s = json_obj_get_string(cfg, "value");
	if(s) obj->title = s; // TODO broken
	
	return (GUIObject*)obj;
}


static GUIObject* create_GUIText(GUIManager* gm, json_value_t* cfg) {
	GUIText* obj;
	char* defaultText = "";
	char* defaultFont = "Arial";
	float defaultSize = 3.0f;
	
	// TODO: read json for values
	char* s = json_obj_get_string(cfg, "value");
	if(s) defaultText = s;
	
	s = json_obj_get_string(cfg, "font");
	if(s) defaultFont = s;
	
	defaultSize = json_obj_get_double(cfg, "size", defaultSize);
	
	obj = GUIText_new(gm, defaultText, defaultFont, defaultSize);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUISelectBox(GUIManager* gm, json_value_t* cfg) {
	GUIGridLayout* obj;
	json_value_t* opts_v;
	int optCnt = 0;
	GUISelectBoxOption* sbOpts;
	
	if(!json_obj_get_key(cfg, "options", &opts_v)) {
		if(opts_v->type == JSON_TYPE_ARRAY) {
			
			
			optCnt = json_array_length(opts_v) / 2;
			sbOpts = calloc(1, sizeof(*sbOpts) * optCnt);
			
			// TODO: [{label: "foo", value: 3.5}, ...]
			
			
			// ["foo", 3.5, ...]
			json_array_node_t* link = opts_v->v.arr->head;
			int i = 0;
			while(link) {
				char* label = NULL;
				char* value = NULL;
				
				// label first
				json_as_string(link->value, &label);
				
				link = link->next;
				if(!link) {
					if(label) free(label);
					break;
				}
				
				// value second
				json_as_string(link->value, &value);
				
				// add the option
				sbOpts[i].label = strdup(label);
				sbOpts[i].data = strdup(value);
				i++;
				
				link = link->next;
			}
			
		}
		else if(opts_v->type == JSON_TYPE_OBJ) {
			json_value_t* v;
			void* iter = NULL;
			char* key;
			int i = 0;
			
			optCnt = json_obj_length(opts_v);
			sbOpts = calloc(1, sizeof(*sbOpts) * optCnt);
			
			while(json_obj_next(opts_v, &iter, &key, &v)) {
				sbOpts[i].label = strdup(key);
				if(v->type == JSON_TYPE_DOUBLE) {
					json_as_double(v, (double*)&sbOpts[i].data);
				}
				else if(v->type == JSON_TYPE_INT) {
					json_as_int(v, (int64_t*)&sbOpts[i].data);
				}
				else if(v->type == JSON_TYPE_STRING) {
					sbOpts[i].data = strdup(v->v.str);
				}
				else {
					printf("Invalid SelectBox value type in GUI config loader.\n");
					sbOpts[i].data = NULL;
				}
				
				i++;
			}
			
			
		}
		else {
			printf("Invalid SelectBox options format in GUI config loader.\n");
		}
	}
	
	obj = GUISelectBox_New(gm);
	if(optCnt > 0) {
		GUISelectBox_SetOptions(obj, sbOpts, optCnt);
		if(sbOpts) free(sbOpts);
	}
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIWindow(GUIManager* gm, json_value_t* cfg) {
	GUIWindow* obj;
	
	// TODO: read json for values

	obj = GUIWindow_New(gm);
	
	char* s = json_obj_key_as_string(cfg, "color");
	if(s) {
		decodeHexColorNorm(s, (float*)&obj->color);
	}
	
	return (GUIObject*)obj;
}



// ------------------------------------------------------------------------

static HashTable(creator_fn) creator_lookup;

static void checkInitLookup() {
	static initialized = 0;
	if(initialized) return;
	
	HT_init(&creator_lookup, 4);
	
	struct {
		char* name;
		creator_fn fn;
	} fns[] = {
		{"text", create_GUIText},
		{"columnlayout", create_GUIColumnLayout},
		{"window", create_GUIWindow},
		{"image", create_GUIImage},
		{"performancegraph", create_GUIPerformanceGraph},
		{"imagebutton", create_GUIImageButton},
		{"gridlayout", create_GUIGridLayout},
		{"selectbox", create_GUISelectBox},
		{"simplewindow", create_GUISimpleWindow},
	};
	
	for(int i = 0; i < sizeof(fns) / sizeof(fns[0]); i++) {
		HT_set(&creator_lookup, fns[i].name, fns[i].fn);
	}
	
}



GUIObject* GUICL_CreateFromConfig(GUIManager* gm, json_value_t* cfg) {
	char* elemType;
	GUIObject* obj = NULL;
	creator_fn fn;
	json_value_t* j_kids;
	
	checkInitLookup();
	
	elemType = json_obj_key_as_string(cfg, "type");
	if(!elemType) {
		printf("Missing element type in config loader\n");
	}
	else{
		for(int i = 0; elemType[i]; i++) elemType[i] = tolower(elemType[i]);
		
		if(!HT_get(&creator_lookup, elemType, &fn) && fn) {
			obj = (*fn)(gm, cfg);
			obj->h.name = json_obj_key_as_string(cfg, "name");
		}
		
		free(elemType);
		
		read_header(&obj->header, cfg);
		
		if(!json_obj_get_key(cfg, "children", &j_kids)) {
			GUICL_LoadChildren(gm, obj, j_kids);
		}
		
	}
	
	
	
	return obj;
}



void GUICL_LoadChildren(GUIManager* gm, GUIHeader* parent, json_value_t* cfg) {
	struct json_array_node* link;
	
	checkInitLookup();
	
	link = cfg->v.arr->head;
	while(link) {
		json_value_t* j_child;
		GUIObject* child;
		json_value_t* j_kids;
		
		if(link->value->type != JSON_TYPE_OBJ) {
			printf("invalid gui element format\n");
			
			link = link->next;
			continue;
		}
		j_child = link->value;
		
		
		child = GUICL_CreateFromConfig(gm, j_child);
		if(!child) {
			printf("failed to create gui element\n");
			
			link = link->next;
			continue;
		}
		
		
		GUIAddClient_(parent, child);
		
		
		link = link->next;
	}
	
}

























