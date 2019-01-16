
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



void guiSimpleWindowRender(GUISimpleWindow* sw, GameState* gs, PassFrameParams* pfp) {
	
// 	guiRender(sw->bg, gs, pfp);
// 	guiRender(sw->titlebar, gs, pfp);
// 	guiRender(sw->closebutton, gs, pfp);
}

void guiSimpleWindowDelete(GUISimpleWindow* sw) {
	
	
	
	
}


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

void guiSimpleWindowAddClient(GUIObject* parent, GUIObject* child) {
	guiAddClient(parent->simpleWindow.bg, child);
};

void guiSimpleWindowRemoveClient(GUIObject* parent, GUIObject* child) {
	guiRemoveClient(parent->simpleWindow.bg, child);
};



GUISimpleWindow* guiSimpleWindowNew(Vector2 pos, Vector2 size, float zIndex) {
	
	GUISimpleWindow* sw;
	
	float tbh = .03; // titleBarHeight
	
	static struct gui_vtbl static_vt = {
		.Render = guiSimpleWindowRender,
		.Delete = guiSimpleWindowDelete,
		.GetClientSize = guiSimpleWindowGetClientSize,
		.SetClientSize = guiSimpleWindowSetClientSize,
		.RecalcClientSize = guiSimpleWindowRecalcClientSize,
		.AddClient = guiSimpleWindowAddClient,
		.RemoveClient = guiSimpleWindowRemoveClient,
	};
	
	
	sw = calloc(1, sizeof(*sw));
	CHECK_OOM(sw);
	
// 	guiHeaderInit(&sw->header);
	sw->header.vt = &static_vt;
	
	sw->header.hitbox.min.x = pos.x;
	sw->header.hitbox.min.y = pos.y;
	sw->header.hitbox.max.x = pos.x + size.x;
	sw->header.hitbox.max.y = pos.y + size.y;
	
// 	sw->bg = guiWindowNew(pos, size, zIndex);
	sw->bg->color = (Vector){0.1, 0.9, 0.1};
	sw->bg->fadeWidth = 0.0;
	sw->bg->borderWidth = 0.0;
// 	sw->bg->padding.top = .21;
// 	sw->bg->padding.left = .05;
// 	sw->bg->padding.bottom = .05;
// 	sw->bg->padding.right = .05;

// 	guiRegisterObject(sw->bg, &sw->header);
	
// 	sw->titlebar = guiWindowNew(
// 		(Vector2){pos.x, pos.y}, 
// 		(Vector2){size.x, tbh}, 
// 		zIndex + .0001
// 	);
	sw->titlebar->color = (Vector){0.9, 0.1, .9};
	sw->titlebar->fadeWidth = 0.0;
	sw->titlebar->borderWidth = 0.0;
// 	guiRegisterObject(sw->titlebar, &sw->bg->header);
	
// 	sw->closebutton = guiWindowNew(
// 		(Vector2){pos.x + size.x - tbh, pos.y + tbh * .05}, 
// 		(Vector2){tbh * 0.9, tbh * 0.9},
// 		zIndex + .0002
// 	);
	sw->closebutton->color = (Vector){0.9, 0.1, 0.1};
	sw->closebutton->fadeWidth = 0.0;
	sw->closebutton->borderWidth = 0.0;
// 	guiRegisterObject(sw->closebutton, &sw->titlebar->header);
	
	
	sw->header.onClick = closeClick;
	
	
	
	return sw;
}








