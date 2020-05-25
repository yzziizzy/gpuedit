

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../window.h"
#include "../gui.h"
#include "../gui_internal.h"

#include "../utilities.h"



static void render(GUIButton* w, PassFrameParams* pfp);
static void delete(GUIButton* w);
static GUIObject* hitTest(GUIObject* go, Vector2 testPos);


static void writeCharacterGeom(GUIUnifiedVertex* v, struct charInfo* ci, Vector2 off, float sz, float adv, float line);



static void mEnter(GUIObject* w_, GUIEvent* gev) {
	((GUIButton*)w_)->isHovered = 1;
}
static void mLeave(GUIObject* w_, GUIEvent* gev) {
	((GUIButton*)w_)->isHovered = 0;
}

GUIText* GUIButton_New(GUIManager* gm, char* str) {
	
	static struct gui_vtbl static_vt = {
		.Render = render,
		.Delete = delete,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.MouseEnter = mEnter,
		.MouseLeave = mLeave,
	};
	
	GUIButton* w;
	pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	
	if(str) {
		w->label = strdup(str);
	}
	
	return w;
}


/* standard for text
static void updatePos(GUIText* gt, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &gt->header; 
	Vector2 tl = gui_calcPosGrav(h, grp);
	h->absTopLeft = tl;
	h->absClip = grp->clip;
	h->absZ = grp->baseZ + h->z;
}*/

static void render(GUIButton* w, PassFrameParams* pfp) {
	char* txt = w->label;
	GUIManager* gm = w->header.gm;
	
	Vector2 tl = w->header.absTopLeft;
	
	struct Color4* bg = &gm->defaults.buttonBgColor;
	struct Color4* bd = &gm->defaults.buttonBorderColor;
	struct Color4* tx = &gm->defaults.buttonTextColor;
	
	if(w->isHovered) {
		bg = &gm->defaults.buttonHoverBgColor;
		bd = &gm->defaults.buttonHoverBorderColor;
		tx = &gm->defaults.buttonHoverTextColor;
	}
	
	// draw background and border
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	*v = (GUIUnifiedVertex){
		.pos = {tl.x, tl.y, tl.x + w->header.size.x, tl.y + w->header.size.y},
		.clip = {0, 0, 800, 800},
		.texIndex1 = 1, // border width
		.guiType = 4, // bordered window 
		.fg = *bd, // border color
		.bg = *bg,
		.z = 1.75,
		.alpha = 1.0,
	};
	
	// draw label
	float textw = gui_getDefaultUITextWidth(w->header.gm, w->label, strlen(w->label));
	float bw = w->header.size.x;
	float bh = w->header.size.y;
	
	AABB2 box;
	box.min.x = tl.x + ((bw - textw) / 2);
	box.min.y = tl.y + ((bh - 16) / 2);
	box.max.x = tl.x + w->header.size.x - ((bw - textw) / 2);
	box.max.y = tl.y + w->header.size.y - ((bh - 16) / 2);
	
	gui_drawDefaultUITextLine(w->header.gm, &box, tx, 10000000, w->label, strlen(w->label));
}


void delete(GUIButton* w) { // TODO: implement reap functions 
	if(w->label) free(w->label);
	w->label = NULL;
}



