#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "../gui.h"
#include "../gui_internal.h"
#include "../utilities.h"





void guiWindowDelete(GUIWindow* gw);
// Vector2 guiWindowGetClientSize(GUIObject* go);
// void guiWindowSetClientSize(GUIObject* go, Vector2 cSize);
// Vector2 guiWindowRecalcClientSize(GUIObject* go);
void guiWindowAddClient(GUIObject* parent, GUIObject* child);
void guiWindowRemoveClient(GUIObject* parent, GUIObject* child);


static void render(GUIWindow* gw, PassFrameParams* pfp);
// static int onclick(GUIWindow* gw, Vector2* clickPos);




GUIWindow* GUIWindow_New(GUIManager* gm) {
	
	GUIWindow* gw;
	
	static struct gui_vtbl static_vt = {
		.Render = render,
// 		.Delete = guiWindowDelete,
// 		.GetClientSize = guiWindowGetClientSize,
// 		.SetClientSize = guiWindowSetClientSize,
// 		.RecalcClientSize = guiWindowRecalcClientSize,
		.AddClient = guiWindowAddClient,
		.RemoveClient = guiWindowRemoveClient,
	};
	
	
	pcalloc(gw);
	gui_headerInit(&gw->header, gm, &static_vt, NULL);
/*	
	gw->header.topleft = pos;
	gw->header.size = size;
	gw->header.z = zIndex;
	
	gw->header.hitbox.min.x = pos.x;
	gw->header.hitbox.min.y = pos.y;
	gw->header.hitbox.max.x = pos.x + size.x;
	gw->header.hitbox.max.y = pos.y + size.y;
	*/

	//if(pos) vCopy(pos, &gw->header.pos);
//	gt->size = size;
	
// 	unsigned int colors[] = {
// 		0x88FF88FF, INT_MAX
// 	};
	
	//VEC_PUSH(&gui_list, gw);
	
// 	gw->header.onClick = onclick;
	
	return gw;
}

/*
static void updatePos(GUIWindow* gw, GUIRenderParams* grp, PassFrameParams* pfp) {
	Vector2 tl = gui_calcPosGrav(&gw->header, grp);
}*/

static void render(GUIWindow* w, PassFrameParams* pfp) {
	
	if(w->header.hidden || w->header.deleted) return;
	
	// TODO: clip calculations
	
	Vector2 tl = w->header.absTopLeft;
	
// 	printf("tl: %f, %f - %f, %f\n", tl.x, tl.y, gw->header.size.x, gw->header.size.y);

	
	GUIUnifiedVertex* v = GUIManager_reserveElements(w->header.gm, 1);
	
	*v = (GUIUnifiedVertex){
// 		.pos = {gw->header.topleft.x, gw->header.topleft.y,
// 			gw->header.topleft.x + gw->header.size.x, gw->header.topleft.y + gw->header.size.y},
		.pos = {tl.x, tl.y,
			tl.x + w->header.size.x, tl.y + w->header.size.y},
		.clip = { 
			.l = w->header.absClip.min.x,
			.t = w->header.absClip.min.y,
			.r = w->header.absClip.max.x,
			.b = w->header.absClip.max.y,
		},
		.guiType = w->borderWidth != 0 ? 4 : 0,
		.texIndex1 = w->borderWidth,
		.texIndex2 = 0,
		.texFade = .5,
		
		.texOffset1 = 0,
		.texOffset2 = 0,
		.texSize1 = 0,
		.texSize2 = 0,
		
		.fg = GUI_COLOR4_TO_SHADER(w->borderColor),
		.bg = GUI_COLOR4_TO_SHADER(w->color),
		
		.z = 9999999,// TODO BUG gw->header.z,
		.alpha = w->header.alpha,
	};
	
	
	
	GUIHeader_renderChildren(&w->header, pfp);
}

void guiWindowDelete(GUIWindow* gw) {
	
}



/*
static int onclick(GUIWindow* gw, Vector2* clickPos) {
	printf("window clicked \n");
	gw->color = (Vector){1,0,1};
	return 1;
}*/





Vector2 GUIWindow_getClientSize(GUIObject* go) {
	GUIWindow* w = &go->window;
	return w->clientSize;
}

void GUIWindow_setClientSize(GUIObject* go, Vector2 cSize) {
	GUIHeader* h = &go->header;
	GUIWindow* w = &go->window;
	w->clientSize = cSize;
// 	h->size.x = cSize.x + w->padding.left + w->padding.right;
// 	h->size.y = cSize.y + w->padding.bottom + w->padding.top;
	
	// TODO: trigger resize event
}

// recalculate client size based on client children sizes and positions
Vector2 guiWindowRecalcClientSize(GUIObject* go) {
	GUIWindow* w = &go->window;
	int i;
	Vector2 max = {0, 0}; 
	
// 	for(i = 0; i < VEC_LEN(&w->clients); i++) {
// 		GUIHeader* h = (GUIHeader*)VEC_ITEM(&w->clients, i);
// 		
// 		max.x = fmax(max.x, h->topleft.x + h->size.x);
// 		max.y = fmax(max.y, h->topleft.y + h->size.y);
// 	}
	
	return max;
}

void guiWindowAddClient(GUIObject* parent, GUIObject* child) {
	GUIWindow* w = &parent->window;
	
// 	int i = VEC_FIND(&w->clients, child);
// 	if(i < 0) VEC_PUSH(&w->clients, child);
};

void guiWindowRemoveClient(GUIObject* parent, GUIObject* child) {
	GUIWindow* w = &parent->window;
	
// 	int i = VEC_FIND(&w->clients, child);
// 	if(i <= 0) VEC_RM(&w->clients, i);
};








