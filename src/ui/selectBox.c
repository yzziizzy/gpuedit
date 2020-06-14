
#include "stdlib.h"
#include "string.h"



#include "../gui.h"
#include "../gui_internal.h"



// TODO:
// 


static void scrollUp(GUIObject* w_, GUIEvent* gev) {
	GUISelectBox* w = (GUISelectBox*)w_;
	
// 	if(gev->originalTarget == w->bg) {
		if(gev->modifiers & GUIMODKEY_ALT) {
			w->absScrollPos.x = fmax(0, w->absScrollPos.x - 20);
		}
		else {
			w->absScrollPos.y = fmax(0, w->absScrollPos.y - 20);
		}
		
		gev->cancelled = 1;
// 	}
}


static void scrollDown(GUIObject* w_, GUIEvent* gev) {
	GUISelectBox* w = (GUISelectBox*)w_;
	
	float mx = w->clientExtent.x - w->clientArea.size.x;
	float my = w->clientExtent.y - w->clientArea.size.y;
	
// 	if(gev->originalTarget == w->bg) {
		if(gev->modifiers & GUIMODKEY_ALT) {
			w->absScrollPos.x = fmin(mx, w->absScrollPos.x + 20);
		}
		else {
			w->absScrollPos.y = fmin(my, w->absScrollPos.y + 20);
		}
		
		gev->cancelled = 1;
// 	}
	
}



static void click(GUIObject* w_, GUIEvent* gev) {
	GUISelectBox* w = (GUISelectBox*)w_;
	
	// close
	if(gev->originalTarget == w->closebutton) {
		w->header.hidden = 1;
		GUIObject_Delete(w);
		
		
		gev->cancelled = 1;
	}
	
}




static void render(GUISelectBox* w, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	
	GUIHeader_renderChildren(&w->header, pfp);
	
	// title
	
	
	gui_drawDefaultUITextLine(h->gm, &box, &h->absClip, &h->gm->defaults.windowTitleTextColor, h->absZ + 0.3, w->title, strlen(w->title));
}


static void reap(GUISelectBox* w) {
	
	free(w->options);
	
	gui_default_Reap(w);
}




static void updatePos(GUISelectBox* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	GUIHeader* ch = &w->clientArea;
	
	w->bg->header.size = h->size;
	w->titlebar->header.size.x = h->size.x;
	w->titlebar->header.size.y = 20;
	
	gui_defaultUpdatePos(h, grp, pfp);
	
	

}











GUISelectBox* GUISelectBox_New(GUIManager* gm) {
	
	
	static struct gui_vtbl static_vt = {
		.Render = render,
		.Reap = reap,
		.UpdatePos = updatePos,
// 		.GetClientSize = guiSimpleWindowGetClientSize,
// 		.SetClientSize = guiSimpleWindowSetClientSize,
// 		.RecalcClientSize = guiSimpleWindowRecalcClientSize,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
// 		.KeyUp = keyUp,
		.Click = click,
// 		.DoubleClick = click,
		.ScrollUp = scrollUp,
		.ScrollDown = scrollDown,
// 		.ParentResize = parentResize,
	};
	
	static struct GUIEventHandler_vtbl client_event_vt = {
		.ScrollUp = clientScrollUp,
		.ScrollDown = clientScrollDown,
	};
	
	GUISelectBox* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	// general options
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	
	
	
	// background
	w->boxBg = GUIWindow_New(gm);
	w->boxBg->header.gravity = GUI_GRAV_TOP_LEFT;
	w->boxBg->header.z = 0.1;
	w->boxBg->color = gm->defaults.windowBgColor;
	w->boxBg->borderColor = gm->defaults.windowBgBorderColor;
	w->boxBg->borderWidth = gm->defaults.windowBgBorderWidth;
	GUIRegisterObject(w, w->boxBg);
	
	
	
	
	return w;
}








