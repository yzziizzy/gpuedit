#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "gui.h"
#include "gui_internal.h"





void guiWindowDelete(GUIWindow* gw);
// Vector2 guiWindowGetClientSize(GUIHeader* go);
// void guiWindowSetClientSize(GUIHeader* go, Vector2 cSize);
// Vector2 guiWindowRecalcClientSize(GUIHeader* go);
void guiWindowAddClient(GUIHeader* parent, GUIHeader* child);
void guiWindowRemoveClient(GUIHeader* parent, GUIHeader* child);


static void render(GUIWindow* gw, PassFrameParams* pfp);
// static int onclick(GUIWindow* gw, Vector2* clickPos);




GUIWindow* GUIWindow_New(GUIManager* gm) {
	
	GUIWindow* gw;
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
// 		.Delete = guiWindowDelete,
// 		.GetClientSize = guiWindowGetClientSize,
// 		.SetClientSize = guiWindowSetClientSize,
// 		.RecalcClientSize = guiWindowRecalcClientSize,
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

	//if(pos) vCopy3p(pos, &gw->header.pos);
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
		
		.z = w->header.absZ,
		.alpha = w->header.alpha,
	};
	
	
	
	GUIHeader_renderChildren(&w->header, pfp);
}

void guiWindowDelete(GUIWindow* gw) {
	
}



/*
static int onclick(GUIWindow* gw, Vector2* clickPos) {
	printf("window clicked \n");
	gw->color = (Vector3){1,0,1};
	return 1;
}*/





Vector2 GUIWindow_getClientSize(GUIHeader* go) {
	GUIWindow* w = (GUIWindow*)go;
	return w->clientSize;
}

void GUIWindow_setClientSize(GUIHeader* go, Vector2 cSize) {
	GUIWindow* w = (GUIWindow*)go;
	w->clientSize = cSize;
// 	h->size.x = cSize.x + w->padding.left + w->padding.right;
// 	h->size.y = cSize.y + w->padding.bottom + w->padding.top;
	
	// TODO: trigger resize event
}

// recalculate client size based on client children sizes and positions
Vector2 guiWindowRecalcClientSize(GUIHeader* go) {
	GUIWindow* w = (GUIWindow*)go;
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









