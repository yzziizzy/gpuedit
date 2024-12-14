
#include "gui.h"
#include "gui_internal.h"

#include "macros_on.h"

// bare mechanics for self-rendered clickable areas
int GUI_Clickable_(GUIManager* gm, void* id, vec2 tl, vec2 sz, int button) {
	int result = 0;

	HOVER_HOT(id)
	
	if(gm->activeID == id) {
		if(GUI_MouseWentUp(button)) {
			if(gm->hotID == id) result = 1;
			GUI_CancelInput();
		}
	}
	else if(gm->hotID == id) {
		if(GUI_MouseWentDown(button)) {
			ACTIVE(id);
			GUI_CancelInput();
		}
	}
	
	return result;
}


// returns 1 if clicked
int GUI_Button_(GUIManager* gm, void* id, Vector2 tl, char* text, GUIButtonOpts* o) {
	Vector2 sz = o->size;

	int result = GUI_Clickable_(gm, id, tl, sz, 1);
	
	if(result) {
		ACTIVE(NULL);
//		GUI_PlaySound(Click);
	}

	// bail early if not drawing
	if(!gm->drawMode) return result;
	
	int st = CUR_STATE(id);
	
//	if(o[st].cornerRadius) {
//		// guard against render glitches and nonsense values
//		float radius = fclamp(o[st].cornerRadius, o[st].borderWidth, fmin(sz.x, sz.y) / 2);
//		GUI_BoxRounded_(gm, tl, sz, radius, o[st].borderWidth, &o[st].border, &o[st].bg);
//	}
//	else {
		GUI_BoxFilled_(gm, tl, sz, o[st].borderWidth, &o[st].border, &o[st].bg);
//	}

	
	gm->curZ += 0.01;
	GUIFont* font = GUI_FindFont(gm, o->fontName);
	GUI_TextLineAdv(tl, sz, text, -1, GUI_TEXT_ALIGN_CENTER | GUI_TEXT_ALIGN_VCENTER, font, o->fontSize, &o[st].text);
	gm->curZ -= 0.01;
	
	return result;
}




// returns 1 if clicked
int GUI_PrintfButton_(GUIManager* gm, void* id, Vector2 tl, GUIButtonOpts* o, char* fmt, ...) {
	Vector2 sz = o->size;
	
	int result = GUI_Clickable_(gm, id, tl, sz, 1);
	
	if(result) {
		ACTIVE(NULL);
	}

	// bail early if not drawing
	if(!gm->drawMode) return result;
	
	int st = CUR_STATE(id);
	
	GUI_BoxFilled_(gm, tl, sz, o->borderWidth, &o[st].border, &o[st].bg);

	char* d;
	
	if(!(d = GUI_GetData_(gm, id))) {
		va_list ap;
			
		va_start(ap, fmt);
		int sz = vsnprintf(NULL, 0, fmt, ap) + 1;
		va_end(ap);
		
		char* d = malloc(sz);
		va_start(ap, fmt);
		vsnprintf(d, sz, fmt, ap);
		va_end(ap);
	
		GUI_SetData_(gm, id, d, free);
	}
	
	gm->curZ += 0.01;
	GUIFont* font = GUI_FindFont(gm, o->fontName);
	GUI_TextLineAdv(tl, sz, d, -1, GUI_TEXT_ALIGN_CENTER | GUI_TEXT_ALIGN_VCENTER, font, o->fontSize, &o[st].text);
	gm->curZ -= 0.01;
	
	return result;
}



// returns true if toggled on 
int GUI_ToggleButton_(GUIManager* gm, void* id, Vector2 tl, char* text, int* state, GUIToggleButtonOpts* o) {
	Vector2 sz = o->size;
	
	HOVER_HOT(id)
	
	if(gm->activeID == id) {
		if(GUI_MouseWentUp(1)) {
			if(gm->hotID == id) {
				*state = !*state;
			}
			ACTIVE(NULL);
			GUI_CancelInput();
		}
	}
	else CLICK_HOT_TO_ACTIVE(id);

	// bail early if not drawing
	if(!gm->drawMode) return *state;
	
	int st = CUR_STATE(id);
	
	GUI_BoxFilled_(gm, tl, sz, o->borderWidth, &o[st].border, &o[st].bg);
	
	gm->curZ += 0.01;
	GUIFont* font = GUI_FindFont(gm, o->fontName);
	GUI_TextLineAdv(tl, sz, text, -1, GUI_TEXT_ALIGN_CENTER | GUI_TEXT_ALIGN_VCENTER, font, o->fontSize, &o[st].text);
	gm->curZ -= 0.01;
	
	return *state;
}



// returns true if checked
int GUI_Checkbox_(GUIManager* gm, void* id, Vector2 tl, char* label, int* state, GUICheckboxOpts* o) {
	
	float bs = o->boxSize;
	Vector2 boxSz = {bs, bs};
	
	if(GUI_MouseInside(tl, boxSz)) {
		HOT(id);
	}
	
	if(gm->activeID == id) {
		if(GUI_MouseWentUp(1)) {
			if(gm->hotID == id) {
				*state = !*state;
			}
			ACTIVE(NULL);
			GUI_CancelInput();
		}
	}
	else CLICK_HOT_TO_ACTIVE(id);

	// bail early if not drawing
	if(!gm->drawMode) return *state;
	
		
	int st = CUR_STATE(id);
	if(*state) st = STATE_ACTIVE;
	
	GUI_BoxFilled_(gm, tl, boxSz, o->borderWidth, &o[st].border, &o[st].bg);
	
	
	float fontSz = o->fontSize;
//	if(sz.y - (2*2) < fontSz) fontSz = sz.y - (2*2);
	GUIFont* font = GUI_FindFont(gm, o->fontName);
	GUI_TextLineAdv(V(tl.x + bs + 4, tl.y), V(0,0), label, -1, 0, font, fontSz, &o[st].text);
	
	return *state;
}



// returns true if *this* radio button is active
int GUI_RadioBox_(GUIManager* gm, void* id, Vector2 tl, char* label, void** state, int isDefault, GUIRadioBoxOpts* o) {
	
	float bs = o->boxSize;
	Vector2 boxSz = {bs, bs};
	
	if(*state == NULL && isDefault) {
		*state = id;
	}
	
	if(GUI_MouseInside(tl, boxSz)) {
		HOT(id);
	}
	
	if(gm->activeID == id) {
		if(GUI_MouseWentUp(1)) {
			if(gm->hotID == id) {
				*state = id;
			}
			ACTIVE(NULL);
			GUI_CancelInput();
		}
	}
	else CLICK_HOT_TO_ACTIVE(id);

	// bail early if not drawing
	if(!gm->drawMode) return *state == id;
		
	int st = CUR_STATE(id);
	if(*state == id) st = STATE_ACTIVE;
	
	GUI_BoxFilled_(gm, tl, boxSz, o->borderWidth, &o[st].border, &o[st].bg);
	
	
	float fontSz = o->fontSize;
//	if(sz.y - (2*2) < fontSz) fontSz = sz.y - (2*2);
	GUIFont* font = GUI_FindFont(gm, o->fontName);
	
	GUI_TextLineAdv(
		V(tl.x + bs + S(4), tl.y),
		V2(9999999, o->boxSize),
		label, -1,
		GUI_TEXT_ALIGN_VCENTER,
		font,
		fontSz,
		&o[st].text
	);
	
	return *state == id;
}


