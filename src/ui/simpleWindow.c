
#include "stdlib.h"
#include "string.h"



#include "gui.h"
#include "gui_internal.h"



// TODO:
// close button callback event
// resizing
// dynamic scrollbar sizing
// fn to resize to fit content or parent



static Vector2 setScrollAbs(GUIHeader* w_, Vector2 absPos) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
	w->absScrollPos.x = fmax(0, fmin(absPos.x, w->clientExtent.x - w->clientArea.size.x));
	w->absScrollPos.y = fmax(0, fmin(absPos.y, w->clientExtent.y - w->clientArea.size.y));
	
	return w->absScrollPos;
}

static Vector2 setScrollPct(GUIHeader* w_, Vector2 pct) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
	float mx = w->clientExtent.x - w->clientArea.size.x;
	float my = w->clientExtent.y - w->clientArea.size.y;
	
	w->absScrollPos.x = fmax(0, fmin(mx * pct.x, mx));
	w->absScrollPos.y = fmax(0, fmin(my * pct.y, my));
	
	return w->absScrollPos;
}


static void scrollUp(GUIHeader* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
// 	if(gev->originalTarget == w->bg) {
		if(gev->modifiers & GUIMODKEY_ALT) {
			w->absScrollPos.x = fmax(0, w->absScrollPos.x - 20);
		}
		else {
			w->absScrollPos.y = fmax(0, w->absScrollPos.y - 20);
		}
		
		gev->cancelled = 1;
// 	}
}


static void scrollDown(GUIHeader* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
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
	
}

static void clientScrollUp(GUIHeader* w_, GUIEvent* gev) {
	scrollUp(w_->parent, gev);
}
static void clientScrollDown(GUIHeader* w_, GUIEvent* gev) {
	scrollDown(w_->parent, gev);
}

static void click(GUIHeader* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
	// close
	if(gev->originalTarget == (void*)w->closebutton) {
		w->header.hidden = 1;
		GUIHeader_Delete(w_);
		
		
		gev->cancelled = 1;
	}
	
}

static void dragStart(GUIHeader* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
// 	printf("dragstart %p, %p\n", gev->originalTarget, w->titlebar);
	if(gev->originalTarget == (void*)w->titlebar) {
		w->isDragging = 1;
		
		vSub2p(&gev->dragStartPos, &w->header.topleft, &w->dragOffset);
		
		gev->cancelled = 1;
	}
	
	
}


static void dragStop(GUIHeader* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;

// 	printf("drag stop\n");
	if(w->isDragging) {
		w->isDragging = 0;
		
		gev->cancelled = 1;
	}
	
}

static void dragMove(GUIHeader* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
// 	printf("drag move\n");
	if(w->isDragging) {
		vSub2p(&gev->pos, &w->dragOffset, &w->header.topleft);
		
		gev->cancelled = 1;
	}
}

static GUIHeader* hitTest(GUIHeader* w_, Vector2 testPos) {
	GUIHeader* a = NULL, *b = NULL;
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
	a = gui_defaultHitTest(&w->header, testPos);
// 	printf("a: %p\n", a);
	// TODO: take into account the scroll position and clipping
	
	if(boxContainsPoint2p(&w->clientArea.absClip, &testPos)) {
		b = gui_defaultChildrenHitTest(&w->clientArea, testPos);
	}
	
	if(!a) return b;
// 	printf("not b\n");
	if(!b) return a;
// 	printf("not a\n");
// 	
	return a->absZ > b->absZ ? a : b;
}


static void render(GUIHeader* w_, PassFrameParams* pfp) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	GUIHeader* h = &w->header;
	
	GUIHeader_renderChildren(&w->header, pfp);
	GUIHeader_renderChildren(&w->clientArea, pfp);
	
	// title
	Vector2 tl = h->absTopLeft;

	AABB2 box;
	box.min.x = tl.x + 5;
	box.min.y = tl.y + 1;
	box.max.x = h->size.x - 10;
	box.max.y = tl.y + 20;
	
	if(w->title)
		gui_drawTextLine(h->gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x,0}, &h->absClip, &h->gm->defaults.windowTitleTextColor, h->absZ + 0.3, w->title, strlen(w->title));
}


static void delete(GUIHeader* w_) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
	VEC_EACH(&w->clientArea.children, i, child) {
		GUIHeader_Delete(child);
	}
	
	gui_default_Delete(&w->header);
}




static void updatePos(GUIHeader* w_, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	GUIHeader* h = &w->header;
	GUIHeader* ch = &w->clientArea;
	
	w->bg->header.size = h->size;
	w->titlebar->header.size.x = h->size.x;
	w->titlebar->header.size.y = 20;
	
	gui_defaultUpdatePos(h, grp, pfp);
	
	// the client area is not updated by the parent updatePos function
	// it must be done manually here
	w->clientArea.size = (Vector2){
		h->size.x - w->border.min.x - w->border.max.x,
		h->size.y - w->border.min.y - w->border.max.y - 20,
	};
	
	// look through the children and calculate their extent
	Vector2 internalMax = {0,0};
	VEC_EACH(&w->clientArea.children, i, child) {
		internalMax.x = fmax(internalMax.x, child->topleft.x + child->size.x);
		internalMax.y = fmax(internalMax.y, child->topleft.y + child->size.y);
	}
	
	w->clientExtent = internalMax;
	
	
	
	if(internalMax.y > w->clientArea.size.y) w->yScrollIsShown = 1;
	if(w->alwaysShowYScroll) w->yScrollIsShown = 1;
	if(w->disableYScroll) w->yScrollIsShown = 0;
	
	// adjust client area size first
	if(w->yScrollIsShown) w->clientArea.size.x -= w->yScrollbarThickness;
	if(w->xScrollIsShown) w->clientArea.size.y -= w->xScrollbarThickness;
	
	
	if(w->yScrollIsShown) {
		w->scrollbarY->header.hidden = 0;
		
		float travel = w->clientArea.size.y - 60; // length of the scrollbar
		float m = w->clientExtent.y - w->clientArea.size.y;
		float scrollpct = w->absScrollPos.y / m;
		
		w->scrollbarY->header.size.x = w->yScrollbarThickness;
		w->scrollbarY->header.topleft.y = 20 + w->border.min.y +  ((scrollpct) * travel);
	}
	
	if(internalMax.x > w->clientArea.size.x) w->xScrollIsShown = 1;
	if(w->alwaysShowXScroll) w->xScrollIsShown = 1;
	if(w->disableXScroll) w->xScrollIsShown = 0;
	
	if(w->xScrollIsShown) {
		w->scrollbarX->header.hidden = 0;
		
		float travel = w->clientArea.size.x - 60; // length of the scrollbar
		float m = w->clientExtent.x - w->clientArea.size.x;
		float scrollpct = w->absScrollPos.x / m;
		
		w->scrollbarX->header.size.y = w->xScrollbarThickness;
		w->scrollbarX->header.topleft.x = w->border.min.x  + ((scrollpct) * travel);
	}
	
	
	
	GUIRenderParams grp2 = {
		.offset = {
			h->absTopLeft.x + w->border.min.x - w->absScrollPos.x, 
			h->absTopLeft.y + w->border.min.y + 20 - w->absScrollPos.y,
		},
		.size = w->clientArea.size,
		.baseZ = h->absZ + 100,
	};
	
	
	grp2.clip = gui_clipTo(grp->clip, (AABB2){
		.min = {
			grp2.offset.x + w->absScrollPos.x, 
			grp2.offset.y + w->absScrollPos.y
		},
		.max = {
			grp2.offset.x + grp2.size.x + w->absScrollPos.x, 
			grp2.offset.y + grp2.size.y + w->absScrollPos.y
		},
	});
	
	
	// TODO: hardcoded titlebar height
	 
// 	VEC_EACH(&w->clientArea.children, ind, child) {
// 		GUIHeader_updatePos(&child->header, &grp2, pfp);
// 	}
	gui_defaultUpdatePos(&w->clientArea, &grp2, pfp);
}









/*
Vector2 guiSimpleWindowGetClientSize(GUIHeader* go) {
	return guiGetClientSize(&go->simpleWindow.bg);
}

void guiSimpleWindowSetClientSize(GUIHeader* go, Vector2 cSize) {
	
	// TODO: fix
	GUIHeader* h = &go->header;
	GUIWindow* w = go->simpleWindow.bg;
	w->clientSize = cSize;
// 	h->size.x = cSize.x + w->padding.left + w->padding.right;
// 	h->size.y = cSize.y + w->padding.bottom + w->padding.top;
	
	// TODO: trigger resize event
}

// recalculate client size based on client children sizes and positions
Vector2 guiSimpleWindowRecalcClientSize(GUIHeader* go) {
	Vector2 csz = guiRecalcClientSize(go->simpleWindow.bg);
	// TODO: probably something
	return csz;
}
*/
 
static void addClient(GUIHeader* _parent, GUIHeader* child) {
	GUISimpleWindow* p = (GUISimpleWindow*)_parent;
	GUIHeader_RegisterObject(&p->clientArea, child);
};

static void removeClient(GUIHeader* _parent, GUIHeader* child) {
	GUISimpleWindow* p = (GUISimpleWindow*)_parent;
	
	GUIHeader_UnregisterObject(child);
};

static GUIHeader* findChild(GUIHeader* h, char* name) {
	GUISimpleWindow* w = (GUISimpleWindow*)h;
	
	return gui_defaultFindChild((GUIHeader*)&w->clientArea, name);
};



GUISimpleWindow* GUISimpleWindow_New(GUIManager* gm) {
	
	
	static struct gui_vtbl static_vt = {
		.Render = render,
		.Delete = delete,
		.UpdatePos = updatePos,
		.HitTest = hitTest,
// 		.GetClientSize = guiSimpleWindowGetClientSize,
// 		.SetClientSize = guiSimpleWindowSetClientSize,
// 		.RecalcClientSize = guiSimpleWindowRecalcClientSize,
		.AddClient = addClient,
		.RemoveClient = removeClient,
		.FindChild = findChild,
		.SetScrollPct = setScrollPct,
		.SetScrollAbs = setScrollAbs,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
// 		.KeyUp = keyUp,
		.Click = click,
// 		.DoubleClick = click,
		.ScrollUp = scrollUp,
		.ScrollDown = scrollDown,
		.DragStart = dragStart,
		.DragStop = dragStop,
		.DragMove = dragMove,
// 		.ParentResize = parentResize,
	};
	
	static struct GUIEventHandler_vtbl client_event_vt = {
		.ScrollUp = clientScrollUp,
		.ScrollDown = clientScrollDown,
	};
	
	GUISimpleWindow* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	gui_headerInit(&w->clientArea, gm, NULL, &client_event_vt);
	w->clientArea.parent = (GUIHeader*)w; // for event handling
	// general options
	w->xScrollbarThickness = 5;
	w->yScrollbarThickness = 5;
	w->xScrollbarMinLength = 15;
	w->yScrollbarMinLength = 15;
	w->header.z = 0.2;
	
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	
	w->border = (AABB2){{3, 3}, {3, 3}};
	
	
	// background
	w->bg = GUIWindow_New(gm);
	w->bg->header.gravity = GUI_GRAV_TOP_LEFT;
	w->bg->header.z = 0.1;
	w->bg->color = gm->defaults.windowBgColor;
	w->bg->borderColor = gm->defaults.windowBgBorderColor;
	w->bg->borderWidth = gm->defaults.windowBgBorderWidth;
	GUI_RegisterObject(w, w->bg);
	
	
	// title bar and close button
	w->titlebar = GUIWindow_New(gm);
	w->titlebar->header.gravity = GUI_GRAV_TOP_LEFT;
	w->titlebar->header.z = 0.11;
	w->titlebar->color = gm->defaults.windowTitleColor;
	w->titlebar->borderColor = gm->defaults.windowTitleBorderColor;
	w->titlebar->borderWidth = gm->defaults.windowTitleBorderWidth;
	GUI_RegisterObject(w, w->titlebar);
	
	w->closebutton = GUIWindow_New(gm);
	w->closebutton->header.topleft = (Vector2){-gm->defaults.windowTitleBorderWidth, gm->defaults.windowTitleBorderWidth};
	w->closebutton->header.gravity = GUI_GRAV_TOP_RIGHT;
	w->closebutton->header.size = (Vector2){18,18};
	w->closebutton->header.z = 0.12;
	w->closebutton->color = (Color4){0.9, 0.1, 0.1, 1};
	w->closebutton->borderWidth = 0.0;
	GUI_RegisterObject(w, w->closebutton);
	
	// scrollbars
	w->scrollbarX = GUIWindow_New(w->header.gm);
	w->scrollbarX->header.topleft = (Vector2){w->border.min.x, -w->border.max.y};
	w->scrollbarX->header.size = (Vector2){60, 20};
	w->scrollbarX->header.gravity = GUI_GRAV_BOTTOM_LEFT;
	w->scrollbarX->header.hidden = 1;
	w->scrollbarX->header.z = 0.13;
	w->scrollbarX->color = gm->defaults.windowScrollbarColor;;
	w->scrollbarX->borderColor = gm->defaults.windowScrollbarBorderColor;;
	w->scrollbarX->borderWidth = gm->defaults.windowScrollbarBorderWidth;
	GUI_RegisterObject(w, w->scrollbarX);
	
	w->scrollbarY = GUIWindow_New(w->header.gm);
	w->scrollbarY->header.topleft = (Vector2){-w->border.max.y, w->border.min.x + 20};
	w->scrollbarY->header.size = (Vector2){20, 60};
	w->scrollbarY->header.gravity = GUI_GRAV_TOP_RIGHT;
	w->scrollbarY->header.hidden = 1;
	w->scrollbarY->header.z = 0.14;
	w->scrollbarY->color = gm->defaults.windowScrollbarColor;;
	w->scrollbarY->borderColor = gm->defaults.windowScrollbarBorderColor;;
	w->scrollbarY->borderWidth = gm->defaults.windowScrollbarBorderWidth;
	GUI_RegisterObject(w, w->scrollbarY);
	
	return w;
}








