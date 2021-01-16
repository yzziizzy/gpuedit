#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>


#include "gui.h"
#include "gui_internal.h"



static int keyUp(InputEvent* ev, GUIDebugAdjuster* bc);

static void render(GUIDebugAdjuster* da, PassFrameParams* pfp);

static void updateText(GUIDebugAdjuster* da);

static void delete(GUIHeader* w_) {
	GUIDebugAdjuster* w = (GUIDebugAdjuster*)w_;
	
	if(w->format)
		free(w->format);
}



GUIDebugAdjuster* GUIDebugAdjuster_new(GUIManager* gm, char* format, void* target, char type) {
	GUIDebugAdjuster* da;
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.Delete = delete,
	};
	
	static InputEventHandler input_vt = {
		.keyUp = (void*)keyUp,
	};
	
	
	pcalloc(da);
	gui_headerInit(&da->header, gm, &static_vt, NULL);
// 	da->header.input_vt = &input_vt;
	
	da->target = target;
	da->type = type;
	da->format = strdup(format);
	da->bufferLen = 0;
	
	
	da->text = GUIText_new(gm, "", "Arial", 3.0f);
	updateText(da);
	
	GUI_RegisterObject(da, da->text);
	
	da->increments[0].f = 1.0f;
	da->increments[0].d = 1.0;
	da->increments[0].i = 1;
	da->increments[1].f = 10.0f;
	da->increments[1].d = 10.0;
	da->increments[1].i = 10;
	
	da->max.d = DBL_MAX;
	da->min.d = DBL_MIN;
	da->max.i = INT64_MAX;
	da->min.i = INT64_MIN;
	da->max.u = UINT64_MAX;
	da->min.u = 0;
	
	return da;
}

/*
static void updatePos(GUIWindow* gw, GUIRenderParams* grp, PassFrameParams* pfp) {
	Vector2 tl = gui_calcPosGrav(&gw->header, grp);
}*/

static void render(GUIDebugAdjuster* da, PassFrameParams* pfp) {
	
	if(da->header.hidden || da->header.deleted) return;
	
	updateText(da);
	
	GUIHeader_renderChildren(&da->header, pfp);
}




static void updateText(GUIDebugAdjuster* da) {
	size_t len;
	union {
		double d;
		int64_t l;
		uint64_t u;
		char* a; // "ascii", if you wish?
	} u;
	
	switch(da->type) {
		case 'f': u.d = *(float*)da->target; break;
		case 'd': u.d = *(double*)da->target; break;
		
		case 'c': u.l = *(char*)da->target; break;
		case 's': u.l = *(short*)da->target; break;
		case 'i': u.l = *(int*)da->target; break;
		case 'l': u.l = *(long*)da->target; break;
		
		case '1': u.u = *(uint8_t*)da->target; break;
		case '2': u.u = *(uint16_t*)da->target; break;
		case '4': u.u = *(uint32_t*)da->target; break;
		case '8': u.u = *(uint64_t*)da->target; break;
		
		case 'a': u.a = *(char**)da->target; break;
	}
	
	
	switch(da->type) {
		case 'f':
		case 'd':
			len = snprintf(NULL, 0, da->format, u.d) + 1; 
			break;
		case 'c':
		case 's':
		case 'i':
		case 'l':
			len = snprintf(NULL, 0, da->format, u.l) + 1; 
			break;
		case '1':
		case '2':
		case '4':
		case '8':
			len = snprintf(NULL, 0, da->format, u.u) + 1; 
			break;
		case 'a':
			len = snprintf(NULL, 0, da->format, u.a) + 1;
			break;
	}
	
	if(da->bufferLen < len) {
		da->bufferLen = len + 64;
		da->buffer = realloc(da->buffer, da->bufferLen);
	}
	
	switch(da->type) {
		case 'f':
		case 'd':
			snprintf(da->buffer, da->bufferLen, da->format, u.d);
			break;
		case 'c':
		case 's':
		case 'i':
		case 'l':
			snprintf(da->buffer, da->bufferLen, da->format, u.l);
			break;
		case '1':
		case '2':
		case '4':
		case '8':
			snprintf(da->buffer, da->bufferLen, da->format, u.u);
			break;
		case 'a':
			snprintf(da->buffer, da->bufferLen, da->format, u.a);
			break;
	}
	
	GUIText_setString(da->text, da->buffer);
}


static void increment(GUIDebugAdjuster* da, int scale) {
	switch(da->type) {
		case 'f': *((float*)da->target) += da->increments[scale].f; break; 
		case 'd': *((double*)da->target) += da->increments[scale].d; break; 
		
		case 'c': *((char*)da->target) += da->increments[scale].i; break; 
		case 's': *((short*)da->target) += da->increments[scale].i; break; 
		case 'i': *((int*)da->target) += da->increments[scale].i; break; 
		case 'l': *((long*)da->target) += da->increments[scale].i; break; 
		
		case '1': *((uint8_t*)da->target) += da->increments[scale].i; break; 
		case '2': *((uint16_t*)da->target) += da->increments[scale].i; break; 
		case '4': *((uint32_t*)da->target) += da->increments[scale].i; break; 
		case '8': *((uint64_t*)da->target) += da->increments[scale].i; break; 
		
		// strings are not incremented
	}
}

static void decrement(GUIDebugAdjuster* da, int scale) {
	switch(da->type) {
		case 'f': *((float*)da->target) -= da->increments[scale].f; break; 
		case 'd': *((double*)da->target) -= da->increments[scale].d; break; 
		
		case 'c': *((char*)da->target) -= da->increments[scale].i; break; 
		case 's': *((short*)da->target) -= da->increments[scale].i; break; 
		case 'i': *((int*)da->target) -= da->increments[scale].i; break; 
		case 'l': *((long*)da->target) -= da->increments[scale].i; break; 
		
		case '1': *((uint8_t*)da->target) -= da->increments[scale].i; break; 
		case '2': *((uint16_t*)da->target) -= da->increments[scale].i; break; 
		case '4': *((uint32_t*)da->target) -= da->increments[scale].i; break; 
		case '8': *((uint64_t*)da->target) -= da->increments[scale].i; break; 
		
		// strings are not incremented
	}
}


static int keyUp(InputEvent* ev, GUIDebugAdjuster* da) {
	if(ev->keysym == XK_Escape) {
		
// 		GUIHeader_revertFocus(da);
		//gbcTest = NULL;
		return 0;
	}
	
	if(ev->keysym == XK_Up) {
		increment(da, 0);
		updateText(da);
		
		return 0;
	} 
	if(ev->keysym == XK_Down) {
		decrement(da, 0);
		updateText(da);
		
		return 0;
	} 
	
	return 1;
}



