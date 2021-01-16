
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#include "gui.h"
#include "gui_internal.h"




// TEMP HACK
#include "../fileBrowser.h"


typedef GUIHeader* (*creator_fn)(GUIManager*, json_value_t*);

static struct Color4 json_get_color(json_value_t* v) {
	struct Color4 c = {0,0,0,1};
	
	if(!v) return c;
	
	if(v->type == JSON_TYPE_STRING) {
		// TODO: lookup table
		decodeHexColorNorm(v->s, (float*)&c);
	}
	else if(v->type == JSON_TYPE_ARRAY) {
		switch(v->len) {
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


static void read_flags(GUIHeader* h, json_value_t* cfg) {
	char* s = json_obj_get_str(cfg, "flags"); // returns internal buffer
	if(!s) return;
	
	s = strdup(s);
	size_t len;
	char** flist = strsplit_inplace(s, '|', &len);
	
	int flags = 0;
	
	for(size_t i = 0; i < len; i++) {
		char* f = flist[i];
		
		     if(0 == strcasecmp(f, "MAXIMIZE_X")) flags |= GUI_MAXIMIZE_X;
		else if(0 == strcasecmp(f, "MAXIMIZE_Y")) flags |= GUI_MAXIMIZE_Y;
		else if(0 == strcasecmp(f, "NO_CLIP"))    flags |= GUI_NO_CLIP;
		else if(0 == strcasecmp(f, "SIZE_TO_CONTENT")) flags |= GUI_SIZE_TO_CONTENT;
		else if(0 == strcasecmp(f, "AUTO_SIZE"))       flags |= GUI_AUTO_SIZE;
		else if(0 == strcasecmp(f, "CHILD_TABBING"))   flags |= GUI_CHILD_TABBING;
	}
	
	h->flags = flags;
	
	free(flist);
	free(s);
}

static void read_gravity(GUIHeader* h, json_value_t* cfg) {
	char* s = json_obj_get_str(cfg, "gravity");
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
	
	read_gravity(h, cfg);
	read_flags(h, cfg);
	
	if(!json_obj_get_key(cfg, "pos", &v)) {
		json_as_vector(v, 2, (float*)&h->topleft);
	}
	if(!json_obj_get_key(cfg, "position", &v)) {
		json_as_vector(v, 2, (float*)&h->topleft);
	}
	if(!json_obj_get_key(cfg, "topleft", &v)) {
		json_as_vector(v, 2, (float*)&h->topleft);
	}
	
	if(!json_obj_get_key(cfg, "size", &v)) {
		json_as_vector(v, 2, (float*)&h->size);
		GUIResize(h, h->size);
	}
	
	if(!json_obj_get_key(cfg, "scale", &v)) {
		h->scale = json_as_double(v);
	}
	
	if(!json_obj_get_key(cfg, "alpha", &v)) {
		h->alpha = json_as_double(v);
	}
	
	if(!json_obj_get_key(cfg, "z", &v)
		|| !json_obj_get_key(cfg, "z-index", &v)
		|| !json_obj_get_key(cfg, "zIndex", &v)
	) {
		h->z = json_as_double(v);
	}
	
	if(!json_obj_get_key(cfg, "hidden", &v)) {
		h->hidden = json_as_int(v);
	}
	
	// TODO: convert from strings
	if(!json_obj_get_key(cfg, "cursor", &v)) {
		h->cursor = json_as_int(v);
	}
	
	if(!json_obj_get_key(cfg, "tabStop", &v)) {
		h->tabStop = json_as_int(v);
	}
	
	
}




static GUIHeader* create_GUIButton(GUIManager* gm, json_value_t* cfg) {
	GUIButton* obj;
	
	char* s = json_obj_get_str(cfg, "value");
	
	obj = GUIButton_New(gm, s ? s : "");
	obj->isDisabled = json_obj_get_int(cfg, "disabled", 0);
	
	return (GUIHeader*)obj;
}

static GUIHeader* create_GUIColumnLayout(GUIManager* gm, json_value_t* cfg) {
	GUIColumnLayout* obj;
	Vector2 defaultSpacing = {0.01, 0.01};
	
	// TODO: read json for values
	
	obj = GUIColumnLayout_new(gm, defaultSpacing, 0.02, 0);
	
	return (GUIHeader*)obj;
}

static GUIHeader* create_GUIDebugAdjuster(GUIManager* gm, json_value_t* cfg) {
	GUIDebugAdjuster* obj;
	
	// TODO: read json for values
	char* fmt = json_obj_get_str(cfg, "format");
	// TODO string type conversion
	int type = 0;
	obj = GUIDebugAdjuster_new(gm, fmt, NULL, type);
	
	return (GUIHeader*)obj;
}

static GUIHeader* create_GUIEdit(GUIManager* gm, json_value_t* cfg) {
	GUIEdit* obj;
	
	char* s = json_obj_get_str(cfg, "value");
	obj = GUIEdit_New(gm, s ? s : "");
	
	return (GUIHeader*)obj;
}

static GUIHeader* create_GUIFileBrowser(GUIManager* gm, json_value_t* cfg) {
	GUIFileBrowser* obj;
	
// 	char* s = json_obj_get_string(cfg, "value");
	obj = GUIFileBrowser_New(gm, ".");
	
	return (GUIHeader*)obj;
}

static GUIHeader* create_GUIFileBrowserControl(GUIManager* gm, json_value_t* cfg) {
	GUIFileBrowserControl* obj;
	
// 	char* s = json_obj_get_string(cfg, "value");
	obj = GUIFileBrowserControl_New(gm, ".");
	
	return (GUIHeader*)obj;
}

static GUIHeader* create_GUIGridLayout(GUIManager* gm, json_value_t* cfg) {
	GUIGridLayout* obj;
	Vector2 defaultSpacing = {0.01, 0.01};
	json_value_t* v;
	float f;
	
	if(!json_obj_get_key(cfg, "cellSize", &v)) {
		json_as_vector(v, 2, (float*)&defaultSpacing);
	}
	
	obj = GUIGridLayout_new(gm, (Vector2){0,0}, defaultSpacing);
	
	obj->maxCols = json_obj_get_int(cfg, "maxCols", 10);
	obj->maxRows = json_obj_get_int(cfg, "maxRows", 10);
	
	return (GUIHeader*)obj;
}

static GUIHeader* create_GUIImage(GUIManager* gm, json_value_t* cfg) {
	GUIImage* obj;
	char* defaultImgName = "pre/denied";
	
	char* value = json_obj_get_str(cfg, "imgName");
	// TODO smarter conversion from json
	
	obj = GUIImage_new(gm, value ? value : defaultImgName);
	
	return (GUIHeader*)obj;
}

static GUIHeader* create_GUIImageButton(GUIManager* gm, json_value_t* cfg) {
	GUIImageButton* obj;
	float border;
	char* defaultImgName = "pre/denied";
	
	border = json_obj_get_double(cfg, "border", 2);
	char* s = json_obj_get_str(cfg, "image");
	if(!s) s = defaultImgName;
	
	obj = GUIImageButton_New(gm, border, s);
	
	return (GUIHeader*)obj;
}

static GUIHeader* create_GUIPerformanceGraph(GUIManager* gm, json_value_t* cfg) {
	GUIPerformanceGraph* obj;
	Vector2 defaultSpacing = {0.01, 0.01};
	
	// TODO: read json for values
	
	obj = guiPerformanceGraphNew(gm, defaultSpacing, 0.02, 120);
	
	return (GUIHeader*)obj;
}


static GUIHeader* create_GUISimpleWindow(GUIManager* gm, json_value_t* cfg) {
	GUISimpleWindow* obj;
	Vector2 defaultSpacing = {0.01, 0.01};
	
	// TODO: read json for values
	
	obj = GUISimpleWindow_New(gm);
	
	char* s = json_obj_get_strdup(cfg, "title");
	if(s) obj->title = s; // TODO broken
	
	return (GUIHeader*)obj;
}


static GUIHeader* create_GUIText(GUIManager* gm, json_value_t* cfg) {
	GUIText* obj;
	char* defaultText = "";
	char* defaultFont = "Arial";
	float defaultSize = 3.0f;
	
	// TODO: read json for values
	char* s = json_obj_get_str(cfg, "value");
	if(s) defaultText = s;
	
	s = json_obj_get_str(cfg, "font");
	if(s) defaultFont = s;
	
	defaultSize = json_obj_get_double(cfg, "size", defaultSize);
	
	obj = GUIText_new(gm, defaultText, defaultFont, defaultSize);
	
	return (GUIHeader*)obj;
}

static GUIHeader* create_GUISelectBox(GUIManager* gm, json_value_t* cfg) {
	GUISelectBox* obj;
	json_value_t* opts_v;
	int optCnt = 0;
	GUISelectBoxOption* sbOpts;
	
	if(!json_obj_get_key(cfg, "options", &opts_v)) {
		if(opts_v->type == JSON_TYPE_ARRAY) {
			
			
			optCnt = opts_v->len / 2;
			sbOpts = calloc(1, sizeof(*sbOpts) * optCnt);
			
			// TODO: [{label: "foo", value: 3.5}, ...]
			
			
			// ["foo", 3.5, ...]
			json_link_t* link = opts_v->arr.head;
			int i = 0;
			while(link) {
				char* label = NULL;
				char* value = NULL;
				
				// label first
				label = link->v->s;
				
				link = link->next;
				if(!link) {
					if(label) free(label);
					break;
				}
				
				// value second
				value = link->v->s;
				
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
			
			optCnt = opts_v->len;
			sbOpts = calloc(1, sizeof(*sbOpts) * optCnt);
			
			while(json_obj_next(opts_v, &iter, &key, &v)) {
				sbOpts[i].label = strdup(key);
				if(v->type == JSON_TYPE_DOUBLE) {
					*(double*)(&sbOpts[i].data) = json_as_double(v);
				}
				else if(v->type == JSON_TYPE_INT) {
					*(int64_t*)(&sbOpts[i].data) = json_as_int(v);
				}
				else if(v->type == JSON_TYPE_STRING) {
					sbOpts[i].data = strdup(v->s);
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
	
	return (GUIHeader*)obj;
}

static GUIHeader* create_GUIWindow(GUIManager* gm, json_value_t* cfg) {
	GUIWindow* obj;
	
	// TODO: read json for values

	obj = GUIWindow_New(gm);
	
	char* s = json_obj_get_str(cfg, "color");
	if(s) {
		decodeHexColorNorm(s, (float*)&obj->color);
	}
	
	return (GUIHeader*)obj;
}



// ------------------------------------------------------------------------

static HT(creator_fn) creator_lookup;

static void checkInitLookup(void) {
	static int initialized = 0;
	if(initialized) return;
	
	HT_init(&creator_lookup, 32);
	
	struct {
		char* name;
		creator_fn fn;
	} fns[] = {
		{"button", create_GUIButton},
		{"columnlayout", create_GUIColumnLayout},
		{"edit", create_GUIEdit},
		{"filebrowser", create_GUIEdit},
		{"filebrowsercontrol", create_GUIEdit},
		{"gridlayout", create_GUIGridLayout},
		{"image", create_GUIImage},
		{"imagebutton", create_GUIImageButton},
		{"performancegraph", create_GUIPerformanceGraph},
		{"selectbox", create_GUISelectBox},
		{"simplewindow", create_GUISimpleWindow},
		{"text", create_GUIText},
		{"window", create_GUIWindow},
	};
	
	for(size_t i = 0; i < sizeof(fns) / sizeof(fns[0]); i++) {
		HT_set(&creator_lookup, fns[i].name, fns[i].fn);
	}
	
}



GUIHeader* GUICL_CreateFromConfig(GUIManager* gm, json_value_t* cfg) {
	char* elemType;
	GUIHeader* obj = NULL;
	creator_fn fn;
	json_value_t* j_kids;
	
	checkInitLookup();
	
	elemType = json_obj_get_str(cfg, "type");
	if(!elemType) {
		printf("Missing element type in config loader\n");
	}
	else{
		for(int i = 0; elemType[i]; i++) elemType[i] = tolower(elemType[i]);
		
		if(!HT_get(&creator_lookup, elemType, &fn) && fn) {
			obj = (*fn)(gm, cfg);
			obj->name = json_obj_get_str(cfg, "name");
		}
		else {
			printf("unknown gui type: '%s'\n", elemType);
			return NULL;
		}
		
		read_header(obj, cfg);
		
		if(!json_obj_get_key(cfg, "children", &j_kids)) {
			GUICL_LoadChildren(gm, obj, j_kids);
		}
		
	}
	
	
	
	return obj;
}



void GUICL_LoadChildren(GUIManager* gm, GUIHeader* parent, json_value_t* cfg) {
	struct json_link* link;
	
	checkInitLookup();
	
	link = cfg->arr.head;
	while(link) {
		json_value_t* j_child;
		GUIHeader* child;
		json_value_t* j_kids;
		
		if(link->v->type != JSON_TYPE_OBJ) {
			printf("invalid gui element format\n");
			
			link = link->next;
			continue;
		}
		j_child = link->v;
		
		
		child = GUICL_CreateFromConfig(gm, j_child);
		if(!child) {
			printf("failed to create gui element\n");
			
			link = link->next;
			continue;
		}
		
		
		GUIHeader_AddClient(parent, child);
		
		
		link = link->next;
	}
	
}

























