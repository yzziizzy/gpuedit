
#include "statusBar.h"
#include "gui_internal.h"




static int sort_items_fn(void* a_, void* b_) {
	GUIStatusBarItem* a = *((GUIStatusBarItem**)a_);
	GUIStatusBarItem* b = *((GUIStatusBarItem**)b_);

	return a->order - b->order;
}



static void render(GUIStatusBar* w, PassFrameParams* pfp) {
	GUIManager* gm = w->header.gm;
	
	GUIHeader* h = &w->header;
	Vector2 tl = h->absTopLeft;
	
	
	gui_drawBox(gm, h->absTopLeft, h->size, &h->absClip, h->absZ + 0.0001, &w->bgColor); 
// 	// title
// 
// 	AABB2 box;
// 	box.min.x = tl.x + 5;
// 	box.min.y = tl.y + 1;
// 	box.max.x = w->header.size.x - 10;
// 	box.max.y = tl.y + 20;
// 	
// 	gui_drawTextLine(w->header.gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x,0}, &w->header.absClip, &w->header.gm->defaults.windowTitleTextColor, w->header.absZ+0.1, w->label, strlen(w->label));
	
	GUIHeader_renderChildren(h, pfp);
}

static void delete(GUIStatusBar* w) {
	
}

static void updatePos(GUIStatusBar* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	
	
	VEC_SORT(&w->left, sort_items_fn);
	VEC_SORT(&w->center, sort_items_fn);
	VEC_SORT(&w->right, sort_items_fn);
	/*
	GUIRenderParams grp2 = {
		.clip = grp->clip,
		.size = h->size, // sized to the child to eliminate gravity 
		.offset = {
			.x = 0,
			.y = 0 
		},
		.baseZ = grp->baseZ + h->z,
	};
		
	
	float total_w = 0.0;
	VEC_EACH(&w->left, i, item) {
// 		float iwidth = gui_getDefaultUITextWidth(h->gm, *item->data[0].str, NULL);
		
		item->offset = total_w;
		
// 		total_w += iwidth + w->spacing;
		
// 		GUIHeader_updatePos(child, &grp2, pfp);
	}
	
	float left_max = total_w;
	
	
	total_w = 0.0;
	VEC_R_EACH(&w->right, i, item) {
// 		float iwidth = gui_getDefaultUITextWidth(h->gm, *item->data[0].str, NULL);
		
		item->offset = total_w;
		
// 		total_w += iwidth + w->spacing;
	}
	*/
	
	gui_defaultUpdatePos(h, grp, pfp);
}




GUIStatusBar* GUIStatusBar_New(GUIManager* gm) {
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
// 		.Delete = (void*)delete,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
// 		.Click = click,
	};
	
	GUIStatusBar* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	
	
	return w;
}




GUIStatusBarItem* GUIStatusBar_AddItem(GUIStatusBar* w, GUIHeader* item, int order, char align, char* name) {
	
	GUIStatusBarItem* it = pcalloc(it);
	
	it->item = item;
	it->order = order;
	it->align = align;
	it->name = strdup(name);
	
	it->offset = 0;
	
	VEC_PUSH(&w->items, it);
	
	if(it->align == 'l') {
		VEC_PUSH(&w->left, it);
		VEC_SORT(&w->left, sort_items_fn);
	}
	else if(it->align == 'c') {
		VEC_PUSH(&w->center, it);
		VEC_SORT(&w->center, sort_items_fn);
	}
	else if(it->align == 'r') {
		VEC_PUSH(&w->right, it);
		VEC_SORT(&w->right, sort_items_fn);
	}
	
	return it;
}






