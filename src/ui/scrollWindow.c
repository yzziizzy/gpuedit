#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "../gui.h"
#include "../gui_internal.h"
#include "../utilities.h"





// Vector2 guiWindowGetClientSize(GUIObject* go);
// void guiWindowSetClientSize(GUIObject* go, Vector2 cSize);
// Vector2 guiWindowRecalcClientSize(GUIObject* go);
void guiWindowAddClient(GUIObject* parent, GUIObject* child);
void guiWindowRemoveClient(GUIObject* parent, GUIObject* child);


static void delete(GUIScrollWindow* gw);
static void render(GUIScrollWindow* gw, PassFrameParams* pfp);
static void updatePos(GUIScrollWindow* gw, GUIRenderParams* grp, PassFrameParams* pfp);
// static int onclick(GUIWindow* gw, Vector2* clickPos);




GUIScrollWindow* GUIScrollWindow_new(GUIManager* gm) {
	
	GUIScrollWindow* gw;
	
	static struct gui_vtbl static_vt = {
		.Render = render,
		.Delete = delete,
		.UpdatePos = updatePos,
// 		.GetClientSize = guiWindowGetClientSize,
// 		.SetClientSize = guiWindowSetClientSize,
// 		.RecalcClientSize = guiWindowRecalcClientSize,
// 		.AddClient = addClient,
// 		.RemoveClient = guiWindowRemoveClient,
	};
	
	
	pcalloc(gw);
	gui_headerInit(&gw->header, gm, &static_vt);
	
	gw->vscrollbar = GUIWindow_new(gm); 
	gw->vscrollbar->color = (Vector){0,1,.7};
	
	gw->hscrollbar = GUIWindow_new(gm);
	
	return gw;
}

/*
static void updatePos(GUIWindow* gw, GUIRenderParams* grp, PassFrameParams* pfp) {
	Vector2 tl = gui_calcPosGrav(&gw->header, grp);
}*/

static void render(GUIScrollWindow* gw, PassFrameParams* pfp) {
	
	if(gw->header.hidden || gw->header.deleted) return;
	
	GUIHeader_render(&gw->vscrollbar->header, pfp);
	GUIHeader_render(&gw->hscrollbar->header, pfp);
	
	GUIHeader_renderChildren(&gw->header, pfp);
}

static void delete(GUIScrollWindow* gw) {
	
}



static void updatePos(GUIScrollWindow* gw, GUIRenderParams* grp, PassFrameParams* pfp) {
	
	GUIHeader* h = &gw->header;
	
	
	gw->scrollPos.x = 200 * fabs(sin(1 * pfp->appTime));
	gw->scrollPos.y = 200 * fabs(sin(1 * pfp->appTime));
	
	
	float ymargin = gw->internalSize.x <= h->size.x ? 0.0 : gw->sbWidth;
	float xmargin = gw->internalSize.y <= h->size.y ? 0.0 : gw->sbWidth;
	
	Vector2 clientArea = {
		h->size.x,// - xmargin,
		h->size.y,// - ymargin,
	};
	
	
	Vector2 tl = gui_calcPosGrav(h, grp);
	h->absTopLeft = tl;
// 	h->absClip = gui_clipTo(grp->clip, (AABB2){tl, { clientArea.x + tl.x, clientArea.y + tl.y}}); // TODO: clip this to grp->clip
	h->absClip = (AABB2){tl, { clientArea.x + tl.x, clientArea.y + tl.y}}; // TODO: clip this to grp->clip

// 		printf("%f %f, \n", h->absClip.min.x, h->absClip.min.y);
// 	printf(" - %f %f, \n", h->absClip.max.x, h->absClip.max.y);

	
	//h->absClip = (AABB2){{300,300}, {400,400}}; // TODO: clip this to grp->clip
	h->absZ = grp->baseZ + h->z;
	
	
	
	
		GUIUnifiedVertex* v = GUIManager_reserveElements(gw->header.gm, 1);
	
	*v = (GUIUnifiedVertex){
// 		.pos = {gw->header.topleft.x, gw->header.topleft.y,
// 			gw->header.topleft.x + gw->header.size.x, gw->header.topleft.y + gw->header.size.y},
		.pos = {tl.x , tl.y,
			tl.x + gw->header.size.x, tl.y + gw->header.size.y},
		.clip = {200, 200, 500, 500},
		
		.texIndex1 = 0,
		.texIndex2 = 0,
		.texFade = .5,
		.guiType = 0, // window (just a box)
		
		.texOffset1 = 0,
		.texOffset2 = 0,
		.texSize1 = 0,
		.texSize2 = 0,
		
		.fg = {255, 128, 64, 155}, // TODO: border color
		.bg = {255, 128, 64, 125}, // TODO: border color
		
		.z = gw->header.z,
		.alpha = gw->header.alpha,
	};
	
	
	
	
	
	
	
	// clip to the client area
	GUIRenderParams grp2 = {
		.clip = h->absClip,
		.size = clientArea,
		.offset = (Vector2){
			tl.x - gw->scrollPos.x,
			tl.y - gw->scrollPos.y,
		},
		.baseZ = h->absZ,
	};
	
	// update child positions
	VEC_EACH(&gw->header.children, i, child) {
		GUIHeader_updatePos(child, &grp2, pfp);
	}
	
	
	
	// look through the children and calculate their extent
	Vector2 max;
	VEC_EACH(&gw->header.children, i, child) {
		// TODO: figure out
		max.x = fmax(max.x, child->h.topleft.x + child->h.size.x);
		max.y = fmax(max.y, child->h.topleft.y + child->h.size.y);
	}
	
	gw->internalSize = max;
	
	
	// update scrollbars
	float sratio = .7;
	float smin = 20;
	
	float sh = fmax(smin, sratio * gw->internalSize.y / h->size.y);
	gw->vscrollbar->header.size.y = sh;
	gw->vscrollbar->header.size.x = gw->sbWidth;
	
	
// 	GUIHeader_updatePos(&gw->vscrollbar->header, pfp);
// 	GUIHeader_updatePos(&gw->hscrollbar->header, pfp);
	
}







