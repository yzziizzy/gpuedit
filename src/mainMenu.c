
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


#include "common_math.h"
#include "common_gl.h"

#include "mainMenu.h"

#include "gui_internal.h"




static void render(GUIMainMenu* w, PassFrameParams* pfp) {
	GUIManager* gm = w->header.gm;
	
	Vector2 tl = w->header.absTopLeft;
	
	
// 	drawTextLine();
	float lh = 30;//w->lineHeight;
	float gutter = 0;//w->leftMargin + 20;
	
	int linesDrawn = 0;
	/*
	for(intptr_t i = w->scrollOffset; i < VEC_LEN(&w->items); i++) {
		if(lh * linesDrawn > w->header.size.y) break; // stop at the bottom of the window
		
		// TODO stop drawing at end of window properly
		
		GUIMainMenuItem* e = &VEC_ITEM(&w->items, i);
		
		AABB2 box;
		box.min.x = tl.x + gutter;
		box.min.y = tl.y + (lh * linesDrawn);
		box.max.x = tl.x + 800;
		box.max.y = tl.y + (lh * (linesDrawn + 1));
		
		
		if(e->isSelected) { // backgrounds for selected items
			struct Color4* color = &gm->defaults.tabActiveBgColor;
			
			GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
			*v = (GUIUnifiedVertex){
				.pos = {box.min.x, box.min.y, box.max.x, box.max.y},
				.clip = {0, 0, 800, 800},
				
				.guiType = 0, // window (just a box)
				
				.fg = *color, // TODO: border color
				.bg = *color, // TODO: color
				
				.z = /*w->header.z +* / 1000,
				.alpha = 1,
			};
		}
		
		
		
		// the file name
		gui_drawDefaultUITextLine(gm, &box, &gm->defaults.tabTextColor , 10000000, e->label, strlen(e->label));
		
		linesDrawn++;
	}
*/
	// cursor
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	*v = (GUIUnifiedVertex){
		.pos = {
			tl.x + gutter, 
			tl.y + 50 + (w->cursorIndex - w->scrollOffset) * lh - 1, 
			tl.x + w->header.size.x - 49,
			tl.y + 50 + (w->cursorIndex - w->scrollOffset + 1) * lh + 1
		},
		.clip = {0, 0, 800, 800},
		.texIndex1 = 1, // order width
		.guiType = 4, // bordered window (just a box)
		.fg = gm->defaults.tabActiveBgColor, // border color
		.bg = {0,0,0,0},
		.z = .75,
		.alpha = 1.0,
	};

	
	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIMainMenu* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	
// 	// maximize
	w->header.size = grp->size;
	w->clientArea->header.size.y = grp->size.y - 50;
	w->clientArea->header.size.x = grp->size.x - 50;

	// expand the width of each item
// 	VEC_EACH(&w->items, i, item) {
// 		item->base->header.size.x = grp->size.x;
// 	}

	gui_defaultUpdatePos(&w->header, grp, pfp);
	
	
// 	w->sbMinHeight = 20;
// 	// scrollbar position calculation
// 	// calculate scrollbar height
// 	float wh = w->header.size.y;
// 	float sbh = fmax(wh / (b->numLines - w->linesOnScreen), w->sbMinHeight);
// 	
// 	// calculate scrollbar offset
// 	float sboff = ((wh - sbh) / b->numLines) * (w->scrollLines);
// 	
// 	GUIResize(w->scrollbar, (Vector2){10, sbh});
// 	w->scrollbar->header.topleft.y = sboff;
}


static void clientUpdatePos(GUIWindow* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	
	GUIMainMenu* mm = (GUIMainMenu*)w->header.parent;
	
	// maximize
	w->header.size = grp->size;

	// expand the width of each item
	VEC_EACH(&mm->items, i, item) {
		item->base->header.size.x = grp->size.x - 50;
	}
	
	gui_selfUpdatePos(&w->header, grp, pfp);
	gui_columnUpdatePos(&w->header, grp, pfp);

// 	w->sbMinHeight = 20;
// 	// scrollbar position calculation
// 	// calculate scrollbar height
// 	float wh = w->header.size.y;
// 	float sbh = fmax(wh / (b->numLines - w->linesOnScreen), w->sbMinHeight);
// 	
// 	// calculate scrollbar offset
// 	float sboff = ((wh - sbh) / b->numLines) * (w->scrollLines);
// 	
// 	GUIResize(w->scrollbar, (Vector2){10, sbh});
// 	w->scrollbar->header.topleft.y = sboff;
}




// make sure the cursor never goes off-screen
static void autoscroll(GUIMainMenu* w) { /*
	float linesOnScreen = floor(w->header.size.y / w->lineHeight);
	
	if(w->cursorIndex < w->scrollOffset) {
		w->scrollOffset = w->cursorIndex;
		return;
	}
	
	if(w->cursorIndex > linesOnScreen + w->scrollOffset - 1) {
		w->scrollOffset = w->cursorIndex - linesOnScreen + 1;
		return;
	}
	*/
}

static void keyUp(GUIObject* w_, GUIEvent* gev) {
	GUIMainMenu* w = (GUIMainMenu*)w_;
	
	if(gev->keycode == XK_Down) {
		w->cursorIndex = (w->cursorIndex + 1) % VEC_LEN(&w->items);
		autoscroll(w);
	}
	else if(gev->keycode == XK_Up) {
		w->cursorIndex = (w->cursorIndex - 1 + VEC_LEN(&w->items)) % (intptr_t)VEC_LEN(&w->items);
		autoscroll(w);
	}

	
}


static void click(GUIObject* w_, GUIEvent* gev) {
	GUIMainMenu* w = (GUIMainMenu*)w_;
	
	if(gev->originalTarget == w->saveBtn) {
		printf("save clicked \n");
	}
	else if(gev->originalTarget == w->clientArea) {
		printf("CA clicked \n");
	}
	else {
		printf("main menu clicked %p %p \n", gev->originalTarget, w);
	}
}




GUIMainMenu* GUIMainMenu_New(GUIManager* gm) {

	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct gui_vtbl client_vt = {
		.UpdatePos = (void*)clientUpdatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.KeyUp = keyUp,
		.Click = click,
		.DoubleClick = click,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
// 		.ParentResize = parentResize,
	};
	
	
	GUIMainMenu* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	
	w->clientArea = GUIWindow_New(gm);
	w->clientArea->header.vt = &client_vt;
	w->clientArea->header.topleft.y = 50;
	GUIRegisterObject(w->clientArea, w);

	w->saveBtn = GUIButton_New(gm, "Save");
	w->saveBtn->header.size = (Vector2){100, 30};
	w->saveBtn->header.topleft = (Vector2){10, 10};
	GUIRegisterObject(w->saveBtn, w);
	
	w->scrollbar = GUIWindow_New(gm);
	GUIResize(w->scrollbar, (Vector2){10, 50});
	w->scrollbar->color = (Vector){.9,.9,.9};
	w->scrollbar->header.z = 100;
	w->scrollbar->header.gravity = GUI_GRAV_TOP_RIGHT;
	
	GUIRegisterObject(w->scrollbar, w);
	
	GUIMainMenu_AddItem(w, "Foo", 1);
 	GUIMainMenu_AddItem(w, "Bar", 1);
	
	return w;
}


GUIMainMenuItem* GUIMainMenu_AddItem(GUIMainMenu* w, char* name, int type) {
	GUIMainMenuItem* it = pcalloc(it);
	it->label = strdup(name);
	it->type = type;
	
	it->base = GUIWindow_New(w->header.gm);
	it->base->header.size.x = 400;
	it->base->header.size.y = 30;
	
	it->gLabel = GUIText_new(w->header.gm, name, NULL, -1);
	it->gControl = GUIEdit_New(w->header.gm, "sadsadf");
// 	it->gControl->header.size.x = 120;
// 	it->gControl->header.size.y = 30;
// 	GUIEdit_SetText(it->gControl, "tttyerty");
	GUIEdit_SetInt(it->gControl, 25);
	
	it->gControl->h.gravity = GUI_GRAV_TOP_RIGHT;
	GUIRegisterObject(it->base, w->clientArea);
	GUIRegisterObject(it->gLabel, it->base);
	GUIRegisterObject(it->gControl, it->base);
	
	VEC_PUSH(&w->items, it);
	
	return it;
}

void GUIMainMenu_Destroy(GUIMainMenu* w) {
	
	
	VEC_FREE(&w->items);
	
	// TODO:free stuff inside entries
	
	
	// TODO: free gui stuff
	
}




