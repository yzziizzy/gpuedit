

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gui.h"
#include "gui_internal.h"




static void render(GUITextF* w, PassFrameParams* pfp);
static GUIHeader* hitTest(GUIHeader* go, Vector2 testPos);
static void reap(GUITextF* w);
static void updatePos(GUITextF* w, GUIRenderParams* grp, PassFrameParams* pfp);




GUITextF* GUITextF_new(GUIManager* gm) {
	
	static struct gui_vtbl static_vt = {
		.UpdatePos = (void*)updatePos,
		.Render = (void*)render,
		.Reap = (void*)reap,
	};
	
	
	GUITextF* w;
	pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, NULL);
	
	w->bufferLen = 32;
	w->buffer = calloc(1, 32);
	
	w->autoRefresh = 1;
	
	return w;
}



static void updatePos(GUITextF* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	
	if(w->autoRefresh) GUITextF_refresh(w);
	
	w->header.size.x = gui_getDefaultUITextWidth(w->header.gm, w->buffer, w->bufferLen);
	w->header.size.y = 10; // HACK
	
	gui_defaultUpdatePos(&w->header, grp, pfp);
	
}


static void render(GUITextF* w, PassFrameParams* pfp) {
	GUIManager* gm = w->header.gm;
	Vector2 tl = w->header.absTopLeft;
	
	if(w->bufferLen > 0) {
		gui_drawVCenteredTextLine(gm, tl, (Vector2){1000,10},  &w->header.absClip, 
			 &gm->defaults.tabTextColor , w->header.absZ+0.01, w->buffer, w->bufferLen);
	}
}



static void reap(GUITextF* w) { // TODO: implement reap functions 
// 	if(gt->currentStr) free(gt->currentStr);
// 	gt->currentStr = NULL;
	if(w->buffer) {
		free(w->buffer);
		w->buffer = NULL;
		w->bufferLen = 0;
	}
	
	if(w->fmt) {
		free(w->fmt);
		w->fmt = NULL;
	}
	
}




void GUITextF_setString(GUITextF* w, char* fmt, void* args) {
	if(!w->fmt || 0 != strcmp(fmt, w->fmt)) {
		if(w->fmt) free(w->fmt);
		w->fmt = strdup(fmt);
	}
	
	w->args = args;
	
	GUITextF_refresh(w);
}



void GUITextF_refresh(GUITextF* w) {
	size_t n;
	
	n = isnprintfv(w->buffer, w->bufferLen, w->fmt, (void**)w->args);
	
	if(w->bufferLen < n) {
		w->bufferLen = nextPOT(n);
		w->buffer = realloc(w->buffer, w->bufferLen);
		
		isnprintfv(w->buffer, w->bufferLen, w->fmt, (void**)w->args);
	}
}


