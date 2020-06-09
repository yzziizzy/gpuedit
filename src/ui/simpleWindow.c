
#include "stdlib.h"
#include "string.h"



#include "../gui.h"
#include "../gui_internal.h"



// TODO:
// close button callback event
// resizing
// dynamic scrollbar sizing
// fn to resize to fit content or parent
// scroll events


static void scrollUp(GUIObject* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
}
static void scrollDown(GUIObject* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
	
}

static void click(GUIObject* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
	// close
	if(gev->originalTarget == w->closebutton) {
		w->header.hidden = 1;
		GUIObject_Delete(w);
		GUIObject_Delete_(&w->clientArea);
	}
	
	//TODO no further bubbling
	return 0;
}

static void dragStart(GUIObject* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
// 	printf("dragstart %p, %p\n", gev->originalTarget, w->titlebar);
	if(gev->originalTarget == w->titlebar) {
		w->isDragging = 1;
		
		vSub2(&gev->dragStartPos, &w->header.topleft, &w->dragOffset);
	}
	
	
}


static void dragStop(GUIObject* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;

// 	printf("drag stop\n");
	if(w->isDragging) {
		w->isDragging = 0;
	}
	
}

static void dragMove(GUIObject* w_, GUIEvent* gev) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
// 	printf("drag move\n");
	if(w->isDragging) {
		vSub2(&gev->pos, &w->dragOffset, &w->header.topleft);
	}
}


void render(GUISimpleWindow* w, PassFrameParams* pfp) {
	
	GUIHeader_renderChildren(&w->header, pfp);
	GUIHeader_renderChildren(&w->clientArea, pfp);
	
	// title
	Vector2 tl = w->header.absTopLeft;

	AABB2 box;
	box.min.x = tl.x + 5;
	box.min.y = tl.y + 1;
	box.max.x = tl.x + w->header.size.x - 10;
	box.max.y = tl.y + 20;
	
	gui_drawDefaultUITextLine(w->header.gm, &box, &w->header.gm->defaults.tabTextColor , 10000000, w->title, strlen(w->title));
}


void delete(GUISimpleWindow* sw) {
	
	
	
	
}




static void updatePos(GUISimpleWindow* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	
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
		internalMax.x = fmax(internalMax.x, child->h.topleft.x + child->h.size.x);
		internalMax.y = fmax(internalMax.y, child->h.topleft.y + child->h.size.y);
	}
	
	w->clientExtent = internalMax;
	
// 	float scrollpct = sin(fmod(pfp->appTime, 6.28)) * .5 + .5;
	
	
	if(internalMax.y > w->clientArea.size.y) w->yScrollIsShown = 1;
	if(w->alwaysShowYScroll) w->yScrollIsShown = 1;
	if(w->disableYScroll) w->yScrollIsShown = 0;
	
	// adjust client area size first
	if(w->yScrollIsShown) w->clientArea.size.x -= w->yScrollbarThickness;
	if(w->xScrollIsShown) w->clientArea.size.y -= w->xScrollbarThickness;
	
	if(w->yScrollIsShown) {
		w->scrollbarY->header.hidden = 0;
		// BUG: borders
		float travel = w->clientArea.size.y - 60; // length of the scrollbar
		float scrollpct = w->absScrollPos.y / w->clientArea.size.y;
		
		w->scrollbarY->header.size.x = w->yScrollbarThickness;
		w->scrollbarY->header.topleft.y = 20 + w->border.min.y +  ((scrollpct) * travel);
	}
	
	if(internalMax.x > w->clientArea.size.x) w->xScrollIsShown = 1;
	if(w->alwaysShowXScroll) w->xScrollIsShown = 1;
	if(w->disableXScroll) w->xScrollIsShown = 0;
	
	if(w->xScrollIsShown) {
		w->scrollbarX->header.hidden = 0;
		float travel = w->clientArea.size.x - 60; // length of the scrollbar
		float scrollpct = w->absScrollPos.x / w->clientArea.size.x;
		
		// BUG: borders
		w->scrollbarX->header.size.y = w->xScrollbarThickness;
		w->scrollbarX->header.topleft.x = w->border.min.x  + ((scrollpct) * travel);
	}
	
	
	
	/*
	w->absScrollPos = (Vector2){
		-scrollpct * (internalMax.x - w->clientArea.size.x),
		-scrollpct * (internalMax.y - w->clientArea.size.y),
	};
	*/
// 	printf("%f, %f\n", internalMax.x, internalMax.y);
// 	printf("%f, %f\n", w->absScrollPos.x, w->absScrollPos.y);
	
	GUIRenderParams grp2 = {
		.offset = {
			h->absTopLeft.x + w->border.min.x + w->absScrollPos.x, 
			h->absTopLeft.y + w->border.min.y + 20 + w->absScrollPos.y,
		},
		.size = w->clientArea.size,
		.baseZ = h->absZ,
	};
	
	
	grp2.clip = gui_clipTo(grp->clip, (AABB2){
		.min = {
			grp2.offset.x - w->absScrollPos.x, 
			grp2.offset.y - w->absScrollPos.y
		},
		.max = {
			grp2.offset.x + grp2.size.x - w->absScrollPos.x, 
			grp2.offset.y + grp2.size.y - w->absScrollPos.y
		},
	});
	
	
	// TODO: hardcoded titlebar height
	
	
	gui_defaultUpdatePos(&w->clientArea, &grp2, pfp);
}



static Vector2 setScrollAbs(GUIObject* w_, Vector2 absPos) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
	w->absScrollPos.x = absPos.x;
	w->absScrollPos.y = absPos.y;
	
	return w->absScrollPos;
}

static Vector2 setScrollPct(GUIObject* w_, Vector2 pct) {
	GUISimpleWindow* w = (GUISimpleWindow*)w_;
	
	return setScrollAbs(w_, (Vector2){
		pct.x * w->clientExtent.x,
		pct.y * w->clientExtent.y,
	});
}







/*
Vector2 guiSimpleWindowGetClientSize(GUIObject* go) {
	return guiGetClientSize(&go->simpleWindow.bg);
}

void guiSimpleWindowSetClientSize(GUIObject* go, Vector2 cSize) {
	
	// TODO: fix
	GUIHeader* h = &go->header;
	GUIWindow* w = go->simpleWindow.bg;
	w->clientSize = cSize;
// 	h->size.x = cSize.x + w->padding.left + w->padding.right;
// 	h->size.y = cSize.y + w->padding.bottom + w->padding.top;
	
	// TODO: trigger resize event
}

// recalculate client size based on client children sizes and positions
Vector2 guiSimpleWindowRecalcClientSize(GUIObject* go) {
	Vector2 csz = guiRecalcClientSize(go->simpleWindow.bg);
	// TODO: probably something
	return csz;
}
*/

void addClient(GUIObject* _parent, GUIObject* child) {
	GUISimpleWindow* p = (GUISimpleWindow*)_parent;
	
	GUIRegisterObject_(&p->clientArea, &child->header);
};

void removeClient(GUIObject* _parent, GUIObject* child) {
	GUISimpleWindow* p = (GUISimpleWindow*)_parent;
	
	printf("TODO: fix me; GUISimpleWindow.removeClient\n.");
// 	guiRegisterObject(child, &w->clientArea)
};



GUISimpleWindow* GUISimpleWindow_New(GUIManager* gm) {
	
	
	float tbh = .03; // titleBarHeight
	
	static struct gui_vtbl static_vt = {
		.Render = render,
		.Delete = delete,
		.UpdatePos = updatePos,
// 		.GetClientSize = guiSimpleWindowGetClientSize,
// 		.SetClientSize = guiSimpleWindowSetClientSize,
// 		.RecalcClientSize = guiSimpleWindowRecalcClientSize,
		.AddClient = addClient,
		.RemoveClient = removeClient,
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
	
	GUISimpleWindow* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	gui_headerInit(&w->clientArea, gm, NULL, NULL);
	
	// general options
	w->xScrollbarThickness = 5;
	w->yScrollbarThickness = 5;
	w->xScrollbarMinLength = 15;
	w->yScrollbarMinLength = 15;
	w->header.z = 99999;
	
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	
	w->border = (AABB2){{3, 3}, {3, 3}};
	
	
	// background
	w->bg = GUIWindow_New(gm);
	w->bg->header.gravity = GUI_GRAV_TOP_LEFT;
	w->bg->header.z = 999990;
	w->bg->color = (Vector){0.1, 0.9, 0.1};
	w->bg->fadeWidth = 0.0;
	w->bg->borderWidth = 0.0;
	GUIRegisterObject(w, w->bg);
	
	
	// title bar and close button
	w->titlebar = GUIWindow_New(gm);
	w->titlebar->header.gravity = GUI_GRAV_TOP_LEFT;
	w->titlebar->header.z = 999991;
	w->titlebar->color = (Vector){0.9, 0.1, .9};
	w->titlebar->fadeWidth = 0.0;
	w->titlebar->borderWidth = 0.0;
	GUIRegisterObject(w, w->titlebar);
	
	w->closebutton = GUIWindow_New(gm);
	w->closebutton->header.gravity = GUI_GRAV_TOP_RIGHT;
	w->closebutton->header.size = (Vector2){16,16};
	w->closebutton->header.z = 999992;
	w->closebutton->color = (Vector){0.9, 0.1, 0.1};
	w->closebutton->fadeWidth = 0.0;
	w->closebutton->borderWidth = 0.0;
	GUIRegisterObject(w, w->closebutton);
	
	// scrollbars
	w->scrollbarX = GUIWindow_New(w->header.gm);
	w->scrollbarX->header.topleft = (Vector2){w->border.min.x, -w->border.max.y};
	w->scrollbarX->header.size = (Vector2){60, 20};
	w->scrollbarX->header.gravity = GUI_GRAV_BOTTOM_LEFT;
	w->scrollbarX->header.hidden = 1;
	w->scrollbarX->header.z = 9999999999;
	w->scrollbarX->color = (Vector){0.9, 0.7, .9};
	w->scrollbarX->fadeWidth = 0.0;
	w->scrollbarX->borderWidth = 0.0;
	GUIRegisterObject(w, w->scrollbarX);
	
	w->scrollbarY = GUIWindow_New(w->header.gm);
	w->scrollbarY->header.topleft = (Vector2){-w->border.max.y, w->border.min.x + 20};
	w->scrollbarY->header.size = (Vector2){20, 60};
	w->scrollbarY->header.gravity = GUI_GRAV_TOP_RIGHT;
	w->scrollbarY->header.hidden = 1;
	w->scrollbarX->header.z = 9999999999;
	w->scrollbarY->color = (Vector){0.9, 0.7, .9};
	w->scrollbarY->fadeWidth = 0.0;
	w->scrollbarY->borderWidth = 0.0;
	GUIRegisterObject(w, w->scrollbarY);

	
	return w;
}








