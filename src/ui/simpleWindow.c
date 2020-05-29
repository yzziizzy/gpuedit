
#include "stdlib.h"
#include "string.h"



#include "../gui.h"
#include "../gui_internal.h"





static int closeClick(GUIEvent* e) {
	
	GUISimpleWindow* sw;
	

	sw = (GUISimpleWindow*)e->currentTarget;
	
	if(e->originalTarget == sw->closebutton) {
		
		sw->header.hidden = 1;
		guiDelete(sw);
	}
	
	//TODO no further bubbling
	return 0;
}



void render(GUISimpleWindow* sw, PassFrameParams* pfp) {
	
	GUIHeader_renderChildren(&sw->header, pfp);
}

void delete(GUISimpleWindow* sw) {
	
	
	
	
}


static void updatePos(GUISimpleWindow* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	
	w->bg->header.size = h->size;
	w->titlebar->header.size.x = h->size.x;
	w->titlebar->header.size.y = 20;
	
	
	gui_defaultUpdatePos(h, grp, pfp);
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
	
	GUIRegisterObject_(&child->header, &p->clientArea);
};

void removeClient(GUIObject* _parent, GUIObject* child) {
	GUISimpleWindow* p = (GUISimpleWindow*)_parent;
	
	printf("TODO: fix me; GUISimpleWindow.removeClient\n.");
// 	guiRegisterObject(&w->clientArea, child)
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
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
// 		.KeyUp = keyUp,
// 		.Click = click,
// 		.DoubleClick = click,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
// 		.ParentResize = parentResize,
	};
	
	GUISimpleWindow* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	gui_headerInit(&w->clientArea, gm, NULL, NULL);
	
	
	w->header.z = 99999;
	
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	
	
	w->bg = GUIWindow_New(gm);
	w->bg->header.gravity = GUI_GRAV_TOP_LEFT;
	w->bg->color = (Vector){0.1, 0.9, 0.1};
	w->bg->fadeWidth = 0.0;
	w->bg->borderWidth = 0.0;
// 	sw->bg->padding.top = .21;
// 	sw->bg->padding.left = .05;
// 	sw->bg->padding.bottom = .05;
// 	sw->bg->padding.right = .05;

	GUIRegisterObject(w->bg, w);
	
	w->titlebar = GUIWindow_New(gm);
// 		(Vector2){pos.x, pos.y}, 
// 		(Vector2){size.x, tbh}, 
// 		zIndex + .0001
// 	);
	w->titlebar->header.gravity = GUI_GRAV_TOP_LEFT;
	w->titlebar->color = (Vector){0.9, 0.1, .9};
	w->titlebar->fadeWidth = 0.0;
	w->titlebar->borderWidth = 0.0;
	GUIRegisterObject(w->titlebar, w);
// 	GUIRegisterObject(sw->titlebar, &sw->bg->header);
	
	w->closebutton = GUIWindow_New(gm);
// 		(Vector2){pos.x + size.x - tbh, pos.y + tbh * .05}, 
// 		(Vector2){tbh * 0.9, tbh * 0.9},
// 		zIndex + .0002
// 	);
	w->closebutton->header.gravity = GUI_GRAV_TOP_RIGHT;
	w->closebutton->header.size = (Vector2){16,16};
	w->closebutton->color = (Vector){0.9, 0.1, 0.1};
	w->closebutton->fadeWidth = 0.0;
	w->closebutton->borderWidth = 0.0;
	GUIRegisterObject(w->closebutton, w);
// 	GUIRegisterObject(sw->closebutton, &sw->titlebar->header);
	
	
// 	w->header.onClick = closeClick;
	
	
	
	return w;
}








