
#include "stdlib.h"
#include "string.h"



#include "gui.h"
#include "gui_internal.h"



void GUIFormControl_SetString(GUIFormControl* w, char* str) {
	int64_t n;
	double d;
	
	switch(w->type) {
		case GUIFORMCONTROL_STRING:
			GUIEdit_SetText(w->edit, str);
			break;
			
		case GUIFORMCONTROL_INT:
			n = strtol(str, NULL, 10);
			GUIEdit_SetInt(w->edit, n);
			break;
			
		case GUIFORMCONTROL_FLOAT:
			d = strtod(str, NULL);
			GUIEdit_SetDouble(w->edit, d);
			break;
			
		case GUIFORMCONTROL_SELECT:
			break;	
			
		default:
			printf("Unsupported type in GUIFormControl_SetString: %d\n", w->type);
	}
}

// returns a strduped pointer
char* GUIFormControl_GetString(GUIFormControl* w) {
	switch(w->type) {
		case GUIFORMCONTROL_STRING:
		case GUIFORMCONTROL_INT:
		case GUIFORMCONTROL_FLOAT:
			return strdup(GUIEdit_GetText(w->edit));
		
		case GUIFORMCONTROL_SELECT:
			if(w->select->selectedIndex < 0) return NULL;
			return (char*)w->select->options[w->select->selectedIndex].data;
			
		default:
			printf("Unsupported type in GUIFormControl_GetString: %d\n", w->type);
	}
	
	return NULL;
}

double GUIFormControl_GetDouble(GUIFormControl* w) {
	switch(w->type) {
		case GUIFORMCONTROL_INT:
			return strtol(GUIEdit_GetText(w->edit), NULL, 10);
			
		case GUIFORMCONTROL_STRING:
		case GUIFORMCONTROL_FLOAT:
			return strtod(GUIEdit_GetText(w->edit), NULL);
		
		case GUIFORMCONTROL_SELECT:
			if(w->select->selectedIndex < 0) return 0;
			double d =  (double)(int64_t)(w->select->options[w->select->selectedIndex].data);
			return d;
			
		default:
			printf("Unsupported type in GUIFormControl_GetDouble: %d\n", w->type);
	}
	
	return 0;
}

int64_t GUIFormControl_GetInt(GUIFormControl* w) {
	switch(w->type) {
		case GUIFORMCONTROL_STRING:
		case GUIFORMCONTROL_FLOAT:
		case GUIFORMCONTROL_INT:
			return strtol(GUIEdit_GetText(w->edit), NULL, 10);
			
		case GUIFORMCONTROL_SELECT:
			if(w->select->selectedIndex < 0) return 0;
			return (int64_t)w->select->options[w->select->selectedIndex].data;
			
		default:
			printf("Unsupported type in GUIFormControl_GetInt: %d\n", w->type);
	}
	
	return 0;
}


static void render(GUIFormControl* w, PassFrameParams* pfp) {
	
	GUIHeader_renderChildren(&w->header, pfp);
	
	// title
	Vector2 tl = w->header.absTopLeft;

	AABB2 box;
	box.min.x = tl.x + 5;
	box.min.y = tl.y + 1;
	box.max.x = w->header.size.x - 10;
	box.max.y = tl.y + 20;
	
	gui_drawTextLine(w->header.gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x,0}, &w->header.absClip, &w->header.gm->defaults.windowTitleTextColor, w->header.absZ+0.1, w->label, strlen(w->label));

}

static void delete(GUIFormControl* w) {
	
}

static void updatePos(GUIFormControl* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	
// 	if(h->flags & GUIFLAG_MAXIMIZE_X) h->size.x = h->parent->header.size.x;
// 	if(h->flags & GUIFLAG_MAXIMIZE_Y) h->size.y = h->parent->header.size.y;
	
	gui_defaultUpdatePos(h, grp, pfp);
}



static void click(GUIHeader* w_, GUIEvent* gev) {
	GUIFormControl* w = (GUIFormControl*)w_;
	
}


GUIFormControl* GUIFormControl_New(GUIManager* gm, int type, char* label) {
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
// 		.Delete = (void*)delete,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
// 		.Click = click,
	};
	
	GUIFormControl* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	w->type = type;
	w->label = strdup(label);
	
	switch(type) {
		case GUIFORMCONTROL_STRING:
		case GUIFORMCONTROL_INT:
		case GUIFORMCONTROL_FLOAT:
			w->edit = GUIEdit_New(gm, "asdfasdf");
			w->edit->header.topleft = (Vector2){0, 0};
			w->edit->header.size = (Vector2){100, 20};
			w->edit->header.gravity = GUI_GRAV_TOP_RIGHT;
			
			GUI_RegisterObject(w, w->edit);
			break;
		
		case GUIFORMCONTROL_SELECT:
			w->select = GUISelectBox_New(gm);
			w->select->header.topleft = (Vector2){0, 0};
			w->select->header.size = (Vector2){100, 20};
			w->select->header.gravity = GUI_GRAV_TOP_RIGHT;
			
			GUI_RegisterObject(w, w->select);
			break;
	}
	

	
	return w;
}
