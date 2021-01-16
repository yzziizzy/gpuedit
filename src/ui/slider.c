


#include "gui.h"
#include "gui_internal.h"





static void render(GUISlider* w, PassFrameParams* pfp) {
	
	Vector2 tl = w->header.absTopLeft;
	
	int cursorAlpha = 0;
	float cursorOff = 0;
	if(w->hasFocus) {
// 		cursorOff = tl.x + (w->cursorOffset /** w->textControl->fontSize * .01*/);
		cursorAlpha = 255;
	}
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(w->header.gm, 2);
	
	
	
	// bar
	*v++ = (GUIUnifiedVertex){
		.pos = {tl.x, tl.y + 10, tl.x + w->header.size.x, tl.y + 15},
		.clip = {0, 0, 800, 800},
		
		.guiType = 0, // window (just a box)
		
		.fg = {255, 128, 64, 255}, // TODO: border color
		.bg = {28, 202, 64, 255}, // TODO: color
		
		.z = w->header.z,
		.alpha = w->header.alpha,
	};
	// handle
	*v = (GUIUnifiedVertex){
		.pos = {cursorOff, tl.y + 2, cursorOff + 10, tl.y + w->header.size.y - 2},
		.clip = {0, 0, 800, 800},
		
		.guiType = 0, // window (just a box)
		
		.fg = {255, 128, 64, 255}, // TODO: border color
		.bg = {155, 155, 255, 255}, 
		
		.z = w->header.z + 2.5,
		.alpha = w->header.alpha,
	};
	
	
	GUIHeader_renderChildren(&w->header, pfp);
	
	

}

static void updateTextControl(GUIEdit* ed) {
// 	GUIText_setString(ed->textControl, ed->buf);
	
	// get new cursor pos
// 	ed->cursorOffset = guiTextGetTextWidth(ed->textControl, ed->cursorpos);
	//printf("cursorpos %f\n", ed->cursorOffset); 
	
// 	fireOnchange(ed);
}

static int dragStart(InputEvent* ev, GUISlider* w) {
	
	printf("dragstart\n");
	return 1;
}


static int keyDown(InputEvent* ev, GUISlider* w) {
	if(ev->keysym == XK_Left) return 0;
	else if(ev->keysym == XK_Right) return 0;

	else if(ev->keysym == XK_Escape) {
// 		GUIHeader_revertFocus((GUIHeader*)w);
		w->hasFocus = 0;
		return 0;
	};
	
	return 1;
// 	updateTextControl(w);
}
static int click(GUISlider* w, Vector2 clickPos) {
	
// 	GUIHeader_giveFocus(w->header.parent);
// 	((GUISlider*)w->header.parent)->hasFocus = 1;
	
	return 0;
}



GUISlider* GUISlider_New(GUIManager* gm, double min, double max, double initialValue) {
	
	GUISlider* w;
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
	};
		
	static InputEventHandler input_vt = {
		.keyUp = (void*)keyDown,
		.dragStart = (void*)dragStart,
	};
	
	w = calloc(1, sizeof(*w));
	CHECK_OOM(w);
	
	gui_headerInit(&w->header, gm, &static_vt, NULL);
// 	w->header.input_vt = &input_vt;
	w->header.size = (Vector2){150, 25}; 
	
	
	w->textControl = GUIText_new(gm, "", "Arial", 6.0f);
	w->textControl->header.size = w->header.size;
	w->textControl->header.z = 100.5;
	GUI_RegisterObject(w, w->textControl);
	
// 	w->textControl->header.onClick = (GUI_OnClickFn)click;
	
	return w;
}



void GUISlider_SetDouble(GUISlider* ed, double dval) {
	char txtVal[64]; 
	
	ed->value = dval;
	
	gcvt(dval, 6, txtVal);
// 	GUISlider_SetText(ed, txtVal);
}

double GUISlider_GetDouble(GUISlider* ed) {
	// TODO: cache this value
	return ed->value;
}
