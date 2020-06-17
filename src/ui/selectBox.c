
#include "stdlib.h"
#include "string.h"



#include "../gui.h"
#include "../gui_internal.h"



// TODO:
// 






static void scrollUp(GUIObject* w_, GUIEvent* gev) {
/*	GUISelectBox* w = (GUISelectBox*)w_;
	
// 	if(gev->originalTarget == w->bg) {
		if(gev->modifiers & GUIMODKEY_ALT) {
			w->absScrollPos.x = fmax(0, w->absScrollPos.x - 20);
		}
		else {
			w->absScrollPos.y = fmax(0, w->absScrollPos.y - 20);
		}
		
		gev->cancelled = 1;
// 	}*/
}


static void scrollDown(GUIObject* w_, GUIEvent* gev) {
/*	GUISelectBox* w = (GUISelectBox*)w_;
	
	float mx = w->clientExtent.x - w->clientArea.size.x;
	float my = w->clientExtent.y - w->clientArea.size.y;
	
// 	if(gev->originalTarget == w->bg) {
		if(gev->modifiers & GUIMODKEY_ALT) {
			w->absScrollPos.x = fmin(mx, w->absScrollPos.x + 20);
		}
		else {
			w->absScrollPos.y = fmin(my, w->absScrollPos.y + 20);
		}
		
		gev->cancelled = 1;
// 	}
	*/
}



static void click(GUIObject* w_, GUIEvent* gev) {
	GUISelectBox* w = (GUISelectBox*)w_;
	
	
}




static void render(GUISelectBox* w, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	GUIManager* gm = h->gm;
	Vector2 tl = h->absTopLeft;
	
#define D(x) &(gm->defaults.x)
	
	// main border
	gui_drawBoxBorder(gm, tl, h->size, &h->absClip, h->absZ + 0.2, D(selectBgColor), 1, D(selectBorderColor));
	
	// arrow button
// 	gui_drawBorderBox(gm, tl, h->size, &h->absClip, h->absZ + 0.2, D(selectColor), 1, D(selectBorderColor));
	
	if(w->selectedIndex > -1) {
		GUISelectBoxOption* opt = &w->options[w->selectedIndex];
		gui_drawVCenteredTextLine(h->gm, tl, h->size, &h->absClip, D(selectTextColor), h->absZ + 100.3, opt->label, strlen(opt->label));
	}
	
	
	GUIHeader_renderChildren(&w->header, pfp);
}


static void dd_render(GUIHeader* ddh, PassFrameParams* pfp) {
	GUISelectBox* w = (GUISelectBox*)&ddh->parent;
	GUIHeader* wh = &w->header;
	GUIManager* gm = ddh->gm;
	Vector2 tl = ddh->absTopLeft;
	
	
	// main box
	gui_drawBoxBorder(gm, tl, ddh->size, &ddh->absClip, ddh->absZ + 0.2, D(selectBgColor), 1, D(selectBorderColor));
	
	// arrow button
// 	gui_drawBorderBox(gm, tl, h->size, &h->absClip, h->absZ + 0.2, D(selectColor), 1, D(selectBorderColor));
	
	for(int i = 0; i < w->optionCnt; i++) {
		float y = tl.y + (i * 35);
		if(i == w->selectedIndex) {
			GUISelectBoxOption* opt = &w->options[w->selectedIndex];
			gui_drawTextLine(gm, (Vector2){tl.x, y}, ddh->size, &ddh->absClip, D(selectTextColor), ddh->absZ + 0.3, opt->label, strlen(opt->label));
		}
	}
	
	GUIHeader_renderChildren(&w->header, pfp);
}


static void reap(GUISelectBox* w) {
	
	free(w->options);
	
}




static void updatePos(GUISelectBox* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
// 	
// 	w->bg->header.size = h->size;
// 	w->titlebar->header.size.x = h->size.x;
// 	w->titlebar->header.size.y = 20;
	
	gui_defaultUpdatePos(h, grp, pfp);
	
	

}











GUISelectBox* GUISelectBox_New(GUIManager* gm) {
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.Reap = (void*)reap,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
// 		.KeyUp = keyUp,
		.Click = click,
// 		.DoubleClick = click,
		.ScrollUp = scrollUp,
		.ScrollDown = scrollDown,
	};
	
	
	static struct gui_vtbl dd_static_vt = {
// 		.Render = render,
// 		.Reap = reap,
// 		.UpdatePos = updatePos,
	};
	
	static struct GUIEventHandler_vtbl dd_event_vt = {
// 		.ScrollUp = clientScrollUp,
// 		.ScrollDown = clientScrollDown,
	};
	
	GUISelectBox* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	// general options
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	
	
	
	// dropdown
	w->dropdownBg = GUIHeader_New(gm, &dd_static_vt, &dd_event_vt);
	GUIRegisterObject_(&w->header, w->dropdownBg);
	
	w->dropdownScrollbar = GUIHeader_New(gm, NULL, NULL);
	GUIRegisterObject_(&w->header, w->dropdownScrollbar);
	
	
	
	
	return w;
}




void GUISelectBox_SetOptions(GUISelectBox* w, GUISelectBoxOption* opts, int cnt) {
	
	if(w->options) {
		free(w->options);
	}
	
	w->options = calloc(1, sizeof(w->options) * cnt);
	w->optionCnt = cnt;
	w->selectedIndex = 0;
	
	memcpy(w->options, opts, sizeof(w->options) * cnt);
}




