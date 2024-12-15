
#include "gui.h"
#include "gui_internal.h"

#include "macros_on.h"



struct scrollbar_data {
	bool isDragging;
	f32 dragOffset;
};


// returns true on a change
int GUI_HScrollbar_(GUIManager* gm, void* id, vec2 tl, vec2 sz, f32 range, f32* pos, GUIScrollbarOpts* o) {
	struct scrollbar_data* d;
	
	if(!(d = GUI_GetData_(gm, id))) {
		d = calloc(1, sizeof(*d));
		GUI_SetData_(gm, id, d, free);
	}
		
	f32 handleMinSize = o->handleMinSize;
	f32 scrollMin = 0;
	f32 scrollMax = range;
	
	
	f32 ratio = *pos / (scrollMax - scrollMin); 
	f32 dynamicRange = sz.x - handleMinSize; 
	f32 sboff = ratio * dynamicRange;

	vec2 handletl = V(tl.x + sboff, tl.y + S(1));
	vec2 handlesz = V(handleMinSize, sz.y - S(2));

	HOVER_HOT(id);
	
	int st = CUR_STATE(id);
	
	if(gm->drawMode) {
		GUI_BoxFilled(tl, sz, o[st].borderWidth, &o[st].border, &o[st].bg);
		Z(1);
//		GUI_Rect(handletl, handlesz, &C4H(00cc00ff));
		GUI_BoxFilled(handletl, handlesz, o[st].handleBorderWidth, &o[st].handleBorder, &o[st].handleBg);
		Z(-1);
	}
	
	
	if(GUI_InputAvailable()) {
		
		// window dragging
//		if(gm->curEvent.type == GUIEVENT_DragStart && GUI_MouseInside(fr->winpos, fr->titlebarsz)) {
		if(gm->curEvent.type == GUIEVENT_DragStart && GUI_PointInBoxV(handletl, handlesz, gm->lastMousePos)) {
			d->isDragging = 1;
			d->dragOffset = gm->lastMousePos.x - handletl.x;
			ACTIVE(id);
		}
		else if(d->isDragging && gm->curEvent.type == GUIEVENT_DragMove) {
			
			sboff = gm->lastMousePos.x - d->dragOffset;
			ratio = sboff / dynamicRange;
			f32 scrollPos = ratio * (scrollMax - scrollMin);
			
			if(scrollPos < scrollMin) scrollPos = scrollMin;
			if(scrollPos > scrollMax) scrollPos = scrollMax;
			
			int ret = *pos != scrollPos;
			*pos = scrollPos;
			
			return ret;
		}
		else if(d->isDragging && gm->curEvent.type == GUIEVENT_DragStop) {
			d->isDragging = 0;
			ACTIVE(NULL);
		}
	}
	
	return 0;
}



// returns true on a change
int GUI_VScrollbar_(GUIManager* gm, void* id, vec2 tl, vec2 sz, f32 range, f32* pos, GUIScrollbarOpts* o) {
	struct scrollbar_data* d;
	
	if(!(d = GUI_GetData_(gm, id))) {
		d = calloc(1, sizeof(*d));
		GUI_SetData_(gm, id, d, free);
	}
		
	f32 handleMinSize = o->handleMinSize;
	f32 scrollMin = 0;
	f32 scrollMax = range;
	
	
	f32 ratio = *pos / (scrollMax - scrollMin); 
	f32 dynamicRange = sz.y - handleMinSize; 
	f32 sboff = ratio * dynamicRange;

	vec2 handletl = V(tl.x + S(1), tl.y + sboff);
	vec2 handlesz = V(sz.x - S(2), handleMinSize);

	HOVER_HOT(id);
	
	int st = CUR_STATE(id);
	
	if(gm->drawMode) {
		GUI_BoxFilled(tl, sz, o[st].borderWidth, &o[st].border, &o[st].bg);
		Z(1);
//		GUI_Rect(handletl, handlesz, &C4H(00cc00ff));
		GUI_BoxFilled(handletl, handlesz, o[st].handleBorderWidth, &o[st].handleBorder, &o[st].handleBg);
		Z(-1);
	}
	
	
	if(GUI_InputAvailable()) {
		
		// window dragging
//		if(gm->curEvent.type == GUIEVENT_DragStart && GUI_MouseInside(fr->winpos, fr->titlebarsz)) {
		if(gm->curEvent.type == GUIEVENT_DragStart && GUI_PointInBoxV(handletl, handlesz, gm->lastMousePos)) {
			d->isDragging = 1;
			d->dragOffset = gm->lastMousePos.y - handletl.y;
			ACTIVE(id);
		}
		else if(d->isDragging && gm->curEvent.type == GUIEVENT_DragMove) {
			
			sboff = gm->lastMousePos.y - d->dragOffset;
			ratio = sboff / dynamicRange;
			f32 scrollPos = ratio * (scrollMax - scrollMin);
			
			if(scrollPos < scrollMin) scrollPos = scrollMin;
			if(scrollPos > scrollMax) scrollPos = scrollMax;
			
			
			int ret = *pos != scrollPos;
			
			*pos = scrollPos;
			
			return ret;
		}
		else if(d->isDragging && gm->curEvent.type == GUIEVENT_DragStop) {
			d->isDragging = 0;
			ACTIVE(NULL);
		}
	}
	
	return 0;
}


// returns true on a change
int GUI_ScrollwheelRegion_(GUIManager* gm, vec2 tl, vec2 sz, f32 range, f32 increment, f32* pos) {
	
	if(!gm->drawMode && GUI_InputAvailable()) {
		if(GUI_MouseInside(tl, sz)) {
			*pos -= gm->scrollDist * increment;
			*pos = CLAMP(0, *pos, range);
			
			GUI_CancelInput();
			
			return gm->scrollDist != 0;
		}
	}
	
	return 0;
}




