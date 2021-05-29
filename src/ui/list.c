
#include <stdio.h>
#include <string.h>



#include "gui.h"
#include "gui_internal.h"




static void render(GUIHeader* w_, PassFrameParams* pfp) {
	GUIStringList* w = (GUIStringList*)w_;
	GUIManager* gm = w->header.gm;
	GUI_GlobalSettings* gs = gm->gs;
	GUIHeader* h = &w->header;
	
	Vector2 tl = h->absTopLeft;
	
	float lh = 20;
	float bottom = tl.y + h->size.y - lh;
	int maxDrawn = ceil(h->size.y / lh);
	
	int i = 0;
	struct GUIStringListLink* l = w->first;

	if(h->flags & GUI_LIST_FROM_BOTTOM) {

		while(l && i < maxDrawn) {
			gui_drawTextLine(gm, 
				(Vector2){tl.x, bottom - (lh * i)}, 
				(Vector2){h->size.x, lh}, 
				&h->absClip, 
				&gm->defaults.selectedItemTextColor, h->absZ + 0.1, 
				l->s, strlen(l->s)
			);
			
			i++;
			l = l->next;
		}
		
	}
	else {
		while(l && i < maxDrawn) {
			gui_drawTextLine(gm, 
				(Vector2){tl.x, tl.y + (lh * i)}, 
				(Vector2){h->size.x, lh}, 
				&h->absClip, 
				&gm->defaults.selectedItemTextColor, h->absZ + 0.1, 
				l->s, strlen(l->s)
			);
			
			i++;
			l = l->next;
		}
	}	
	
}



static void updatePos(GUIHeader* w_, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIStringList* w = (GUIStringList*)w_;
	gui_defaultUpdatePos(&w->header, grp, pfp);
}


static void reap(GUIHeader* w_) {
	GUIStringList* w = (GUIStringList*)w_;

}



GUIStringList* GUIStringList_New(GUIManager* gm) {

	static struct gui_vtbl static_vt = {
		.Render = render,
		.Reap = reap,
		.UpdatePos = updatePos,
//		.HandleCommand = handleCommand,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
//		.GainedFocus = gainedFocus,
//		.User = userEvent,
	};


	GUIStringList* w = pcalloc(w);
	GUIHeader* h = &w->header;
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	
	h->flags |= GUI_LIST_FROM_BOTTOM;

	return w;
}


void GUIStringList_PrependItem(GUIStringList* w, char* label) {
	
	struct GUIStringListLink* l = pcalloc(l);
	
	l->s = strdup(label);


	if(w->itemCnt == 0) {
		w->first = l;
		w->last = l;
	}
	else {
		l->next = w->first;
		w->first->prev = l;
		w->first = l;
		
		if(w->maxItems > 0 && w->itemCnt >= w->maxItems) {
			l = w->last;
			w->last = l->prev;
			w->last->next = NULL;
			
			free(l->s);
			free(l);
			
			w->itemCnt--;	
		}
	}
	
	w->itemCnt++;
}



void GUIStringList_AppendItem(GUIStringList* w, char* label) {
	
	struct GUIStringListLink* l = pcalloc(l);
	
	l->s = strdup(label);
	
	if(w->itemCnt == 0) {
		w->first = l;
		w->last = l;
	}
	else {
		w->last->next = l;
		l->prev = w->last;
		w->last = l;
		
		if(w->maxItems > 0 && w->itemCnt >= w->maxItems) {
			l = w->first;
			w->first = l->next;
			w->first->prev = NULL;
			
			free(l->s);
			free(l);
			
			w->itemCnt--;	
		}
	}
	
	w->itemCnt++;
	
}









