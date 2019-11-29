


#include "./gui.h"
#include "./gui_internal.h"


static void render(GUITabControl* w, PassFrameParams* pfp);
static void updatePos(GUITabControl* w, GUIRenderParams* grp, PassFrameParams* pfp);


static void render(GUITabControl* w, PassFrameParams* pfp) {
	// only render the active tab
	GUIHeader_render(VEC_ITEM(&w->header.children, w->currentIndex), pfp);
}

static void updatePos(GUITabControl* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	
		
	Vector2 tl = gui_calcPosGrav(h, grp);
	
	h->absTopLeft = tl;
// 	h->absClip = gui_clipTo(grp->clip, (AABB2){tl, { clientArea.x + tl.x, clientArea.y + tl.y}}); // TODO: clip this to grp->clip
// 	h->absClip = (AABB2){tl, { clientArea.x + tl.x, clientArea.y + tl.y}}; // TODO: clip this to grp->clip
	
	h->absZ = grp->baseZ + h->z;
	
	
	
	VEC_EACH(&w->header.children, i, child) { 
		
		GUIRenderParams grp2 = {
			.clip = grp->clip, // TODO: update the clip
			.size = child->h.size, // sized to the child to eliminate gravity 
			.offset = {
				.x = tl.x,
				.y = tl.y + w->tabHeight,
			},
			.baseZ = grp->baseZ + w->header.z,
		};
		
		GUIHeader_updatePos(child, &grp2, pfp);
	}
}



GUITabControl* GUITabControl_New(GUIManager* gm) {
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
	};
	
	
	GUITabControl* w = pcalloc(w);
	gui_headerInit(&w->header, gm, &static_vt, NULL);
	
	w->tabHeight = 20;
	
	return w;
}



GUIObject* GUITabControl_NextTab(GUITabControl* w, char cyclic) {
	int len = VEC_LEN(&w->header.children);
	if(cyclic) {
		w->currentIndex = (w->currentIndex + 1) % len;
	}
	else {
		w->currentIndex = MIN(w->currentIndex + 1, len - 1);
	}
	
	return VEC_ITEM(&w->header.children, w->currentIndex);
}


GUIObject* GUITabControl_PrevTab(GUITabControl* w, char cyclic) {
	int len = VEC_LEN(&w->header.children);
	if(cyclic) {
		w->currentIndex = (w->currentIndex - 1 + len) % len;
	}
	else {
		w->currentIndex = MAX(w->currentIndex + 1, 0);
	}
	
	return VEC_ITEM(&w->header.children, w->currentIndex);
}


GUIObject* GUITabControl_GoToTab(GUITabControl* w, int i) {
	int len = VEC_LEN(&w->header.children);
	w->currentIndex = MAX(0, MIN(len - 1, i));
	
	return VEC_ITEM(&w->header.children, w->currentIndex);
}

