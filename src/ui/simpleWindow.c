
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
	
// 	guiRender(sw->bg, gs, pfp);
// 	guiRender(sw->titlebar, gs, pfp);
// 	guiRender(sw->closebutton, gs, pfp);
}

void delete(GUISimpleWindow* sw) {
	
	
	
	
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
	
	guiRegisterObject(&w->clientArea, child)
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
// 		.GetClientSize = guiSimpleWindowGetClientSize,
// 		.SetClientSize = guiSimpleWindowSetClientSize,
// 		.RecalcClientSize = guiSimpleWindowRecalcClientSize,
		.AddClient = addClient,
		.RemoveClient = removeClient,
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
	
	GUISimpleWindow* w = pcalloc(sw);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	gui_headerInit(&w->clientArea, gm, NULL, NULL);
	
	
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	
	
	w->bg = GUIWindow_New(gm);
	w->bg->color = (Vector){0.1, 0.9, 0.1};
	w->bg->fadeWidth = 0.0;
	w->bg->borderWidth = 0.0;
// 	sw->bg->padding.top = .21;
// 	sw->bg->padding.left = .05;
// 	sw->bg->padding.bottom = .05;
// 	sw->bg->padding.right = .05;

	GUIRegisterObject(w, &w->bg);
	
	w->titlebar = GUIWindow_New(gm);
// 		(Vector2){pos.x, pos.y}, 
// 		(Vector2){size.x, tbh}, 
// 		zIndex + .0001
// 	);
	w->titlebar->color = (Vector){0.9, 0.1, .9};
	w->titlebar->fadeWidth = 0.0;
	w->titlebar->borderWidth = 0.0;
	GUIRegisterObject(w, &w->titlebar);
// 	GUIRegisterObject(sw->titlebar, &sw->bg->header);
	
	w->closebutton = GUIWindow_New(gm);
// 		(Vector2){pos.x + size.x - tbh, pos.y + tbh * .05}, 
// 		(Vector2){tbh * 0.9, tbh * 0.9},
// 		zIndex + .0002
// 	);
	w->closebutton->color = (Vector){0.9, 0.1, 0.1};
	w->closebutton->fadeWidth = 0.0;
	w->closebutton->borderWidth = 0.0;
	GUIRegisterObject(w, &w->closebutton);
// 	GUIRegisterObject(sw->closebutton, &sw->titlebar->header);
	
	
// 	w->header.onClick = closeClick;
	
	
	
	return w;
}








