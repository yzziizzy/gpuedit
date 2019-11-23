
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "../common_math.h"
#include "../hash.h"

#include "../utilities.h"

#include "../gui.h"
#include "../gui_internal.h"


#include "../c_json/json.h"
#include "../json_gl.h"


typedef GUIObject* (*creator_fn)(GUIManager*, json_value_t*);


static void read_gravity(GUIHeader* h, json_value_t* cfg) {
	char* s = json_obj_get_string(cfg, "gravity");
	if(!s) return;
	
	int grav = GUI_GRAV_TOP_LEFT;
	
// 	if(0 == strcasecmp(s, "TOP_LEFT") grav = GUI_GRAV_TOP_LEFT;
	     if(0 == strcasecmp(s, "CENTER_LEFT")) grav = GUI_GRAV_CENTER_LEFT;
	else if(0 == strcasecmp(s, "BOTTOM_LEFT")) grav = GUI_GRAV_BOTTOM_LEFT;
	else if(0 == strcasecmp(s, "BOTTOM_CENTER")) grav = GUI_GRAV_CENTER_BOTTOM;
	else if(0 == strcasecmp(s, "BOTTOM_RIGHT")) grav = GUI_GRAV_BOTTOM_RIGHT;
	else if(0 == strcasecmp(s, "CENTER_RIGHT")) grav = GUI_GRAV_CENTER_RIGHT;
	else if(0 == strcasecmp(s, "TOP_RIGHT")) grav = GUI_GRAV_TOP_RIGHT;
	else if(0 == strcasecmp(s, "TOP_CENTER")) grav = GUI_GRAV_CENTER_TOP;
	else if(0 == strcasecmp(s, "CENTER")) grav = GUI_GRAV_CENTER;
	
	h->gravity = grav;
}

static void read_header(GUIHeader* h, json_value_t* cfg) {
	json_value_t* v;
	
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
		guiResize(h, h->size);
	}
	
	// TODO: all other other props 
	
	
}




static GUIObject* create_GUIWindow(GUIManager* gm, json_value_t* cfg) {
	GUIWindow* obj;
	
	// TODO: read json for values
	
	obj = GUIWindow_new(gm);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIImage(GUIManager* gm, json_value_t* cfg) {
	GUIImage* obj;
	char* defaultImgName = "pre/denied";
	
	// TODO: read json for values
	
	obj = GUIImage_new(gm, defaultImgName);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIText(GUIManager* gm, json_value_t* cfg) {
	GUIText* obj;
	char* defaultText = "-test-";
	char* defaultFont = "Arial";
	float defaultSize = 3.0f;
	
	// TODO: read json for values
	
	obj = GUIText_new(gm, defaultText, defaultFont, defaultSize);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIPerformanceGraph(GUIManager* gm, json_value_t* cfg) {
	GUIPerformanceGraph* obj;
	Vector2 defaultSpacing = {0.01, 0.01};
	
	// TODO: read json for values
	
	obj = guiPerformanceGraphNew(gm, defaultSpacing, 0.02, 120);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIImageButton(GUIManager* gm, json_value_t* cfg) {
	GUIImageButton* obj;
	float border;
	char* defaultImgName = "pre/denied";
	
	border = json_obj_get_double(cfg, "border", 2);
	char* s = json_obj_get_string(cfg, "image");
	if(!s) s = defaultImgName;
	
	// TODO: read json for values
	
	obj = GUIImageButton_New(gm, border, s);
	
	return (GUIObject*)obj;
}

static GUIObject* create_GUIColumnLayout(GUIManager* gm, json_value_t* cfg) {
	GUIColumnLayout* obj;
	Vector2 defaultSpacing = {0.01, 0.01};
	
	// TODO: read json for values
	
	obj = GUIColumnLayout_new(gm, defaultSpacing, 0.02, 0);
	
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
		{"columnLayout", create_GUIColumnLayout},
		{"window", create_GUIWindow},
		{"image", create_GUIImage},
		{"performanceGraph", create_GUIPerformanceGraph},
		{"imageButton", create_GUIImageButton},
		{"gridLayout", create_GUIGridLayout},
	};
	
	for(int i = 0; i < sizeof(fns) / sizeof(fns[0]); i++) {
		HT_set(&creator_lookup, fns[i].name, fns[i].fn);
	}
	
}



GUIObject* GUICL_CreateFromConfig(GUIManager* gm, json_value_t* cfg) {
	char* elemType;
	GUIObject* obj = NULL;
	creator_fn fn;
	
	checkInitLookup();
	
	elemType = json_obj_key_as_string(cfg, "elemType");
	
	if(!HT_get(&creator_lookup, elemType, &fn) && fn) {
		obj = (*fn)(gm, cfg);
		obj->h.name = json_obj_key_as_string(cfg, "name");
	}
	
	free(elemType);
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
		
		read_header(&child->header, j_child);
		
		GUIRegisterObject(child, parent);
		
		
		if(!json_obj_get_key(j_child, "children", &j_kids)) {
			GUICL_LoadChildren(gm, child, j_kids);
		}
		
		link = link->next;
	}
	
}

























