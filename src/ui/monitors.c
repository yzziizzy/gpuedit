#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "gui.h"
#include "gui_internal.h"





static void render(GUIValueMonitor* gfm, PassFrameParams* pfp);

static void updateText(GUIValueMonitor* gfm);



GUIValueMonitor* GUIValueMonitor_new(GUIManager* gm, char* format, void* target, char type) {
	GUIValueMonitor* gfm;
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
	};
	
	
	pcalloc(gfm);
	gui_headerInit(&gfm->header, gm, &static_vt, NULL);
	
	gfm->target = target;
	gfm->type = type;
	gfm->format = format;
	gfm->bufferLen = 0;
	
	
	gfm->text = GUIText_new(gm, "", "Arial", 3.0f);
	updateText(gfm);
	
	GUI_RegisterObject(gfm, gfm->text);
	
	return gfm;
}

/*
static void updatePos(GUIWindow* gw, GUIRenderParams* grp, PassFrameParams* pfp) {
	Vector2 tl = gui_calcPosGrav(&gw->header, grp);
}*/

static void render(GUIValueMonitor* gfm, PassFrameParams* pfp) {
	
	if(gfm->header.hidden || gfm->header.deleted) return;
	
	updateText(gfm);
	
	GUIHeader_renderChildren(&gfm->header, pfp);
}




static void updateText(GUIValueMonitor* gfm) {
	size_t len;
	union {
		double d;
		int64_t l;
		uint64_t u;
		char* a; // "ascii", if you wish?
	} u;
	
	switch(gfm->type) {
		case 'f': u.d = *(float*)gfm->target; break;
		case 'd': u.d = *(double*)gfm->target; break;
		
		case 'c': u.l = *(char*)gfm->target; break;
		case 's': u.l = *(short*)gfm->target; break;
		case 'i': u.l = *(int*)gfm->target; break;
		case 'l': u.l = *(long*)gfm->target; break;
		
		case '1': u.u = *(uint8_t*)gfm->target; break;
		case '2': u.u = *(uint16_t*)gfm->target; break;
		case '4': u.u = *(uint32_t*)gfm->target; break;
		case '8': u.u = *(uint64_t*)gfm->target; break;
		
		case 'a': u.a = *(char**)gfm->target; break;
	}
	
	
	switch(gfm->type) {
		case 'f':
		case 'd':
			len = snprintf(NULL, 0, gfm->format, u.d) + 1; 
			break;
		case 'c':
		case 's':
		case 'i':
		case 'l':
			len = snprintf(NULL, 0, gfm->format, u.l) + 1; 
			break;
		case '1':
		case '2':
		case '4':
		case '8':
			len = snprintf(NULL, 0, gfm->format, u.u) + 1; 
			break;
		case 'a':
			len = snprintf(NULL, 0, gfm->format, u.a) + 1;
			break;
	}
	
	if(gfm->bufferLen < len) {
		gfm->bufferLen = len + 32;
		gfm->buffer = realloc(gfm->buffer, gfm->bufferLen);
	}
	
	switch(gfm->type) {
		case 'f':
		case 'd':
			snprintf(gfm->buffer, gfm->bufferLen, gfm->format, u.d);
			break;
		case 'c':
		case 's':
		case 'i':
		case 'l':
			snprintf(gfm->buffer, gfm->bufferLen, gfm->format, u.l);
			break;
		case '1':
		case '2':
		case '4':
		case '8':
			snprintf(gfm->buffer, gfm->bufferLen, gfm->format, u.u);
			break;
		case 'a':
			snprintf(gfm->buffer, gfm->bufferLen, gfm->format, u.a);
			break;
	}
	
	GUIText_setString(gfm->text, gfm->buffer);
}




