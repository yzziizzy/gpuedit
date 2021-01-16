

#include "gui.h"
#include "gui_internal.h"




static void render(GUITabBar* w, PassFrameParams* pfp) {
	GUIManager* gm = w->header.gm;
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, VEC_LEN(&w->tabs) + 1);
	
	Vector2 tl = w->header.absTopLeft;
	
	// bg
	*v++ = (GUIUnifiedVertex){
		.pos = {tl.x, tl.y, tl.x + w->header.size.x, tl.y + w->header.size.y},
		.clip = GUI_AABB2_TO_SHADER(w->header.absClip),
		
		.guiType = 0, // window (just a box)
		
		.fg = GUI_COLOR4_TO_SHADER(gm->defaults.tabBorderColor), // TODO: border color
		.bg = GUI_COLOR4_TO_SHADER(gm->defaults.tabBorderColor), // TODO: color
		
		.z = w->header.absZ,
		.alpha = 1,
	};
	
	float tabw = (w->header.size.x - (1 + VEC_LEN(&w->tabs))) / (VEC_LEN(&w->tabs));
	
	
	// tab backgrounds
	VEC_EACH(&w->tabs, i, tab) {
		struct Color4* color = tab->isActive ? &gm->defaults.tabActiveBgColor : &gm->defaults.tabBgColor; 
		*v++ = (GUIUnifiedVertex){
			.pos = {tl.x + tabw * i + i + 1, tl.y + 1, tl.x + tabw * (i + 1) + i + 1, tl.y + w->header.size.y - 1},
			.clip = GUI_AABB2_TO_SHADER(w->header.absClip),
			
			.guiType = 0, // window (just a box)
			
			.fg = GUI_COLOR4_TO_SHADER(*color), 
			.bg = GUI_COLOR4_TO_SHADER(*color), 
			
			.z = w->header.absZ,
			.alpha = 1,
		};
		
	}
	
	// tab titles
	VEC_EACH(&w->tabs, i, tab) {
		AABB2 box;
		box.min.x = tl.x + tabw * i + i + 1;
		box.min.y = tl.y + 1;
		box.max.x = tabw * (i + 1) + i + 1;
		box.max.y = tl.y + w->header.size.y - 1;
		
		gui_drawTextLine(gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x,0}, &w->header.absClip, &gm->defaults.tabTextColor , 10000000, tab->title, strlen(tab->title));
	}
}




static void delete(GUITabBar* w) {
}



static void click(GUIHeader* w_, GUIEvent* gev) {
	GUITabBar* w = (GUITabBar*)w_;
	
	Vector2 tl = w->header.absTopLeft;
	float tabw = (w->header.size.x - (1 + VEC_LEN(&w->tabs))) / (VEC_LEN(&w->tabs));
	
	VEC_EACH(&w->tabs, i, tab) {
		AABB2 box;
		box.min.x = tl.x + tabw * i + i + 1;
		box.min.y = tl.y + 1;
		box.max.x = tl.x + tabw * (i + 1) + i + 1;
		box.max.y = tl.y + w->header.size.y - 1;
		
		if(boxContainsPoint2p(&box, &gev->pos)) {
			if(tab->onClick) tab->onClick(i, gev->button, tab);
			if(tab->onActivate) tab->onActivate(i, tab);
			
			return;
		}
		
	}
}



GUITabBar* GUITabBar_New(GUIManager* gm) {
	
	GUITabBar* w;
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.Delete = (void*)delete,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
// 		.Click = click,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
		.MouseUp = click,
	};
	
	
	pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	return w;
}



int GUITabBar_AddTab(GUITabBar* w, char* title) {
	GUITabBarTab* tbt = calloc(1, sizeof(*tbt));
	tbt->title = title;
	
	VEC_PUSH(&w->tabs, tbt);
	
	return VEC_LEN(&w->tabs) - 1;
}


int GUITabBar_AddTabEx(
	GUITabBar* w, 
	char* title, 
	void* userData1,
	void* userData2,
	void (*onClick)(int, int, GUITabBarTab*), 
	void (*onActivate)(int, GUITabBarTab*), 
	void (*onRemove)(int, GUITabBarTab*)
) {
	int index = GUITabBar_AddTab(w, title);
	
	GUITabBarTab* tbt = VEC_ITEM(&w->tabs, index);
	
	tbt->userData1 = userData1;
	tbt->userData2 = userData2;
	
	tbt->onClick = onClick;
	tbt->onActivate = onActivate;
	tbt->onRemove = onRemove;
	
	return index;
}



void GUITabBar_RemoveTab(GUITabBar* w, int index) {
	GUITabBarTab* t = VEC_ITEM(&w->tabs, index);
	
	if(t->onRemove) t->onRemove(index, t);
	
	VEC_RM_SAFE(&w->tabs, (size_t)index);
	
	if(t->title) free(t->title);
	free(t);
}


void GUITabBar_SetActive(GUITabBar* w, int index) {
	if((size_t)index >= VEC_LEN(&w->tabs)) return;
	
	// clear all active tabs
	VEC_EACH(&w->tabs, i, tab) {
		tab->isActive = 0;
	}
	
	// set the one we want
	VEC_ITEM(&w->tabs, index)->isActive = 1;
}
