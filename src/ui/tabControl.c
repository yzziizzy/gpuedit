


#include "./gui.h"
#include "./gui_internal.h"


static void render(GUITabControl* w, PassFrameParams* pfp);
static void updatePos(GUITabControl* w, GUIRenderParams* grp, PassFrameParams* pfp);


static void render(GUITabControl* w, PassFrameParams* pfp) {
	// only render the active tab
	GUIHeader_render(w->activeTab, pfp);
	
	GUIHeader_renderChildren(&w->header, pfp);
}

static void updatePos(GUITabControl* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	
		
	Vector2 tl = gui_calcPosGrav(h, grp);
	
	h->absTopLeft = tl;
// 	h->absClip = gui_clipTo(grp->clip, (AABB2){tl, { clientArea.x + tl.x, clientArea.y + tl.y}}); // TODO: clip this to grp->clip
// 	h->absClip = (AABB2){tl, { clientArea.x + tl.x, clientArea.y + tl.y}}; // TODO: clip this to grp->clip
	
	h->absZ = grp->baseZ + h->z;
	
	float tabHeight = w->tabHeight;
	if(VEC_LEN(&w->tabs) <= 1) tabHeight = 0;
	
	VEC_EACH(&w->tabs, i, child) { 
		
		GUIRenderParams grp2 = {
			.clip = grp->clip, // TODO: update the clip
			.size = child->size, // sized to the child to eliminate gravity 
			.offset = {
				.x = tl.x,
				.y = tl.y + tabHeight,
			},
			.baseZ = grp->baseZ + w->header.z,
		};
		
		GUIHeader_updatePos((GUIHeader*)child, &grp2, pfp);
	}
}


static GUIHeader* hitTest(GUITabControl* w, Vector2 absTestPos) {
// 	printf("tab tes pos %f,%f %p\n", absTestPos.x, absTestPos.y, w);
	if(w->activeTab) {
		GUIHeader* o = gui_defaultHitTest(w->activeTab, absTestPos);
		if(o) return o;
	}
	
	return gui_defaultHitTest(&w->header, absTestPos);
}


static void gainedFocus(GUIHeader* w_, GUIEvent* gev) {
	GUITabControl* w = (GUITabControl*)w_;
	GUIManager_pushFocusedObject(w->header.gm, (GUIHeader*)w->activeTab);
	
}



GUITabControl* GUITabControl_New(GUIManager* gm) {
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
		.HitTest = (void*)hitTest,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.ParentResize = gui_default_ParentResize,
		.GainedFocus = gainedFocus,
	};
	
	GUITabControl* w = pcalloc(w);
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	w->tabHeight = 20;
	
	// TODO: resize
	w->bar = GUITabBar_New(gm);
	GUI_RegisterObject(w, w->bar);
	GUIResize(&w->bar->header, (Vector2){800, 20});
	
	return w;
}


static void switchtab(int index, void* w_) {
	GUITabControl* w = (GUITabControl*)w_;
	GUITabControl_GoToTab(w, index);
	GUIManager_popFocusedObject(w->header.gm);
	GUIManager_pushFocusedObject(w->header.gm, (GUIHeader*)w->activeTab);
}


int GUITabControl_AddTab(GUITabControl* w, GUIHeader* tab, char* title) {
	if(w->activeTab == NULL) {
		w->activeTab = tab;
		w->currentIndex = 0;
	}
	
	VEC_PUSH(&w->tabs, tab);
	tab->parent = (GUIHeader*)w; // important for event bubbling
	
	GUITabBar_AddTabEx(w->bar, title, w, NULL, (void*)switchtab, NULL, NULL);
	
	return VEC_LEN(&w->tabs) - 1;
}


GUIHeader* GUITabControl_NextTab(GUITabControl* w, char cyclic) {
	int len = VEC_LEN(&w->tabs);
	if(cyclic) {
		w->currentIndex = (w->currentIndex + 1) % len;
	}
	else {
		w->currentIndex = MIN(w->currentIndex + 1, len - 1);
	}
	
	w->activeTab = VEC_ITEM(&w->tabs, w->currentIndex);
	return (GUIHeader*)w->activeTab;
}


GUIHeader* GUITabControl_PrevTab(GUITabControl* w, char cyclic) {
	int len = VEC_LEN(&w->tabs);
	if(cyclic) {
		w->currentIndex = (w->currentIndex - 1 + len) % len;
	}
	else {
		w->currentIndex = MAX(w->currentIndex + 1, 0);
	}
	
	w->activeTab = VEC_ITEM(&w->tabs, w->currentIndex);
	return (GUIHeader*)w->activeTab;
}


GUIHeader* GUITabControl_GoToTab(GUITabControl* w, int i) {
	int len = VEC_LEN(&w->tabs);
	w->currentIndex = MAX(0, MIN(len - 1, i));
	
	w->activeTab = VEC_ITEM(&w->tabs, w->currentIndex);
	return (GUIHeader*)w->activeTab;
}

