
#include "stdlib.h"
#include "string.h"



#include "../gui.h"
#include "../gui_internal.h"



// TODO:
// border issues with hover selection
// theming of dropdown items
// "no selection" option
// sizing of dropdown
// clipping of dropdown







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


static GUIObject* hitTest(GUIObject* w_, Vector2 testPos) {
	GUIObject* a = NULL;
	GUISelectBox* w = (GUISelectBox*)w_;
	
	if(w->isOpen) {
		a = gui_defaultChildrenHitTest(&w->header, testPos);
		if(a) return a;
	}
	
	return gui_defaultHitTest(&w->header, testPos);
}



static void click(GUIObject* w_, GUIEvent* gev) {
	GUISelectBox* w = (GUISelectBox*)w_;
	w->isOpen = !w->isOpen;
}

static void dd_mouseMove(GUIObject* ddh, GUIEvent* gev) {
	GUISelectBox* w = (GUISelectBox*)ddh->header.parent;
	w->hoveredIndex = -1 + (gev->pos.y - w->header.absTopLeft.y) / 35;
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
	GUISelectBox* w = (GUISelectBox*)ddh->parent;
	GUIHeader* wh = &w->header;
	GUIManager* gm = ddh->gm;
	Vector2 tl = ddh->absTopLeft;
	
	if(!w->isOpen) return; 
	
	// main box
	gui_drawBoxBorder(gm, tl, ddh->size, &ddh->absClip, ddh->absZ + 0.2, D(selectBgColor), 1, D(selectBorderColor));
	
	// arrow button
// 	gui_drawBorderBox(gm, tl, h->size, &h->absClip, h->absZ + 0.2, D(selectColor), 1, D(selectBorderColor));
	
	for(int i = 0; i < w->optionCnt; i++) {
		float y = tl.y + (i * 35);
		GUISelectBoxOption* opt = &w->options[i];
		
		if(i == w->hoveredIndex) {
			gui_drawBox(gm, (Vector2){tl.x, y}, (Vector2){ddh->size.x, 35}, &ddh->absClip, ddh->absZ + 0.3, D(selectTextColor));
			gui_drawTextLine(gm, (Vector2){tl.x, y}, ddh->size, &ddh->absClip, D(selectBgColor), ddh->absZ + 0.4, opt->label, strlen(opt->label));
		}
		else {
			gui_drawTextLine(gm, (Vector2){tl.x, y}, ddh->size, &ddh->absClip, D(selectTextColor), ddh->absZ + 0.3, opt->label, strlen(opt->label));
		}
	}
	
	GUIHeader_renderChildren(ddh, pfp);
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

static void dd_updatePos(GUIHeader* ddh, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUISelectBox* w = (GUISelectBox*)ddh->parent;
	GUIHeader* wh = &w->header;
// 	
// 	w->bg->header.size = h->size;
// 	w->titlebar->header.size.x = h->size.x;
// 	w->titlebar->header.size.y = 20;
	
	ddh->topleft = (Vector2){5, wh->size.y};
	// TODO calculate height 
	ddh->size = (Vector2){wh->size.x - 5, 35*MIN(4, w->optionCnt)};
	
	Vector2 o = grp->offset;
	
	GUIRenderParams grp2 = {
		.offset = grp->offset,
		.size = ddh->size,
		.baseZ = grp->baseZ + 100,
		.clip = gui_clipTo(grp->clip, (AABB2){
			.min = { o.x + ddh->topleft.x, o.y + ddh->topleft.y },
			.max = { o.x + ddh->topleft.x + ddh->size.x, o.y + ddh->topleft.y + ddh->size.y },
		}),
	};
	
	gui_defaultUpdatePos(ddh, &grp2, pfp);
}











GUISelectBox* GUISelectBox_New(GUIManager* gm) {
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.Reap = (void*)reap,
		.UpdatePos = (void*)updatePos,
		.HitTest = hitTest,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
// 		.KeyUp = keyUp,
		.Click = click,
// 		.DoubleClick = click,
		.ScrollUp = scrollUp,
		.ScrollDown = scrollDown,
	};
	
	
	static struct gui_vtbl dd_static_vt = {
		.Render = dd_render,
// 		.Reap = reap,
		.UpdatePos = dd_updatePos,
	};
	
	static struct GUIEventHandler_vtbl dd_event_vt = {
// 		.ScrollUp = clientScrollUp,
// 		.ScrollDown = clientScrollDown,
		.MouseMove = dd_mouseMove,
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
	
	w->options = calloc(1, sizeof(*w->options) * cnt);
	w->optionCnt = cnt;
	w->selectedIndex = 0;
	
	memcpy(w->options, opts, sizeof(*w->options) * cnt);
}




