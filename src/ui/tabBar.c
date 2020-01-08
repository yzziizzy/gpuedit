

#include "../gui.h"
#include "../gui_internal.h"




static void render(GUITabBar* w, PassFrameParams* pfp) {
	GUIManager* gm = w->header.gm;
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, VEC_LEN(&w->tabs) + 1);
	
	Vector2 tl = w->header.absTopLeft;
	
	// bg
	*v++ = (GUIUnifiedVertex){
		.pos = {tl.x, tl.y, tl.x + w->header.size.x, tl.y + w->header.size.y},
		.clip = {0, 0, 800, 800},
		
		.guiType = 0, // window (just a box)
		
		.fg = gm->defaults.tabBorderColor, // TODO: border color
		.bg = gm->defaults.tabBorderColor, // TODO: color
		
		.z = /*w->header.z +*/ 1000,
		.alpha = 1,
	};
	
	float tabw = (w->header.size.x - (1 + VEC_LEN(&w->tabs))) / (VEC_LEN(&w->tabs));
	
	
	// tab backgrounds
	VEC_EACH(&w->tabs, i, tab) {
		struct Color4* color = tab->isActive ? &gm->defaults.tabActiveBgColor : &gm->defaults.tabBgColor; 
		*v++ = (GUIUnifiedVertex){
			.pos = {tl.x + tabw * i + i + 1, tl.y + 1, tl.x + tabw * (i + 1) + i + 1, tl.y + w->header.size.y - 1},
			.clip = {0, 0, 800, 800},
			
			.guiType = 0, // window (just a box)
			
			.fg = *color, // TODO: border color
			.bg = *color, // TODO: color
			
			.z = /*w->header.z +*/ 100000,
			.alpha = 1,
		};
		
	}
	
	// tab titles
	VEC_EACH(&w->tabs, i, tab) {
		AABB2 box;
		box.min.x = tl.x + tabw * i + i + 1;
		box.min.y = tl.y + 1;
		box.max.x = tl.x + tabw * (i + 1) + i + 1;
		box.max.y = tl.y + w->header.size.y - 1;
		
		gui_drawDefaultUITextLine(gm, &box, &gm->defaults.tabTextColor , 10000000, tab->title, strlen(tab->title));
	}
}




static void delete(GUITabBar* w) {
}



static void click(GUIObject* w_, GUIEvent* gev) {
	GUITabBar* w = (GUITabBar*)w_;
	
	Vector2 tl = w->header.absTopLeft;
	float tabw = (w->header.size.x - (1 + VEC_LEN(&w->tabs))) / (VEC_LEN(&w->tabs));
	
	VEC_EACH(&w->tabs, i, tab) {
		AABB2 box;
		box.min.x = tl.x + tabw * i + i + 1;
		box.min.y = tl.y + 1;
		box.max.x = tl.x + tabw * (i + 1) + i + 1;
		box.max.y = tl.y + w->header.size.y - 1;
		
		if(boxContainsPoint2(&box, &gev->pos)) {
			if(tab->onClick) tab->onClick(i, tab->onClickData);
			if(tab->onActivate) tab->onActivate(i, tab->onActivateData);
			
			return NULL;
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
		.Click = click,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
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
	void (*onClick)(int, void*), 
	void* onClickData,
	void (*onActivate)(int, void*), 
	void* onActivateData
) {
	int index = GUITabBar_AddTab(w, title);
	
	GUITabBarTab* tbt = VEC_ITEM(&w->tabs, index);
	
	tbt->onClick = onClick;
	tbt->onClickData = onClickData;
	tbt->onActivate = onActivate;
	tbt->onActivateData = onActivateData;
	
	return index;
}



void GUITabBar_SetActive(GUITabBar* w, int index) {
	if(index >= VEC_LEN(&w->tabs)) return;
	
	// clear all active tabs
	VEC_EACH(&w->tabs, i, tab) {
		tab->isActive = 0;
	}
	
	// set the one we want
	VEC_ITEM(&w->tabs, index)->isActive = 1;
}
