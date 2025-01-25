#include <ctype.h>

#include "../clipboard.h"

#include "gui.h"
#include "gui_internal.h"



void GUIString_Set(GUIString* gs, char* s) {
	size_t len = strlen(s);
	if(gs->alloc < len + 1) {
		gs->data = realloc(gs->data, len + 1);
		gs->alloc = len + 1;
	}
	
	memcpy(gs->data, s, len);
	gs->len = len;
}

void GUIString_Setn(GUIString* gs, char* s, ssize_t n) {
	if(gs->alloc < n + 1) {
		gs->data = realloc(gs->data, n + 1);
		gs->alloc = n + 1;
	}
	
	n = strnlen(s, n);
	memcpy(gs->data, s, n);
	gs->len = n;
}



#include "macros_on.h"



void GUI_SetFontName_(GUIManager* gm, char* fontName, float size, Color4* color) {
	GUIFont* font = GUI_FindFont(gm, fontName);
	GUI_SetFont_(gm, font, size, color);
}

void GUI_SetFont_(GUIManager* gm, GUIFont* font, float size, Color4* color) {
	gm->curFont = font;
	gm->curFontSize = size;
	gm->curFontColor = *color;
}


void GUI_PushFontName_(GUIManager* gm, char* fontName, float size, Color4* color) {
	GUIFont* font = GUI_FindFont(gm, fontName);
	GUI_PushFont_(gm, font, size, color);
}

void GUI_PushFont_(GUIManager* gm, GUIFont* font, float size, Color4* color) {
	
	struct gui_font_params* fp = VEC_INC(&gm->fontStack);
	
	fp->font = gm->curFont;
	fp->size = gm->curFontSize;
	fp->color = gm->curFontColor;
	
	gm->curFont = font;
	gm->curFontSize = size;
	gm->curFontColor = *color;
}

void GUI_PopFont_(GUIManager* gm) {
	if(!VEC_LEN(&gm->fontStack)) {
		// set to defaults
		gm->curFontColor = gm->defaults.textColor;
		gm->curFontSize = gm->defaults.fontSize;
		gm->curFont = gm->defaults.font;
		
		return;
	}

	struct gui_font_params* fp = &VEC_TAIL(&gm->fontStack);
	gm->curFont = fp->font;
	gm->curFontSize = fp->size;
	gm->curFontColor = fp->color;
	
	VEC_POP1(&gm->fontStack);
}




#define CLAMP(min, val, max) val = MIN(MAX(min, val), max)


// returns true if clicked

//#define DEFAULTS(type, var) type var = gm->defaults.type;

//
//// returns 1 if clicked
//int GUI_Button_(GUIManager* gm, void* id, Vector2 tl, char* text, GUIButtonOpts* o) {
//	int result = 0;
//	Vector2 sz = o->size;
//	
//	
//	HOVER_HOT(id)
//	
//	if(gm->activeID == id) {
//		if(GUI_MouseWentUp(1)) {
//			if(gm->hotID == id) result = 1;
//			ACTIVE(NULL);
//			GUI_CancelInput();
//		}
//	}
//	else CLICK_HOT_TO_ACTIVE(id)
//
//	// bail early if not drawing
//	if(!gm->drawMode) return result;
//	
//	int st = CUR_STATE(id);
//	
//	GUI_BoxFilled_(gm, tl, sz, o->borderWidth, &o->colors[st].border, &o->colors[st].bg);
//
//	
//	gm->curZ += 0.01;
//	GUI_TextLineCentered_(gm, text, strlen(text), tl, sz, o->fontName, o->fontSize, &o->colors[st].text);
//	gm->curZ -= 0.01;
//	
//	return result;
//}
//
// returns 1 if clicked
int GUI_RectButton_(
	GUIManager* gm, 
	void* id, 
	Vector2 tl, 
	Vector2 sz, 
	char* text, 
	char* fontName, 
	float fontSize, 
	Color4* fontColor, 
	Color4 colors[3]
) {
	int result = 0;
	
	HOVER_HOT(id)
	
	if(gm->activeID == id) {
		if(GUI_MouseWentUp(1)) {
			if(gm->hotID == id) result = 1;
			ACTIVE(NULL);
			GUI_CancelInput();
		}
	}
	else CLICK_HOT_TO_ACTIVE(id);

	// bail early if not drawing
	if(!gm->drawMode) return result;
	
	int st = CUR_STATE(id);
	
	GUI_Rect(tl, sz, &colors[CUR_STATE(id)]);

	
	gm->curZ += 0.01;
//	GUI_TextLineCentered(text, strlen(text), tl, sz, fontName, fontSize, fontColor);
	gm->curZ -= 0.01;
	
	return result;
}

//
//// returns true if toggled on 
//int GUI_ToggleButton_(GUIManager* gm, void* id, Vector2 tl, char* text, int* state, GUIToggleButtonOpts* o) {
//	Vector2 sz = o->size;
//	
//	HOVER_HOT(id)
//	
//	if(gm->activeID == id) {
//		if(GUI_MouseWentUp(1)) {
//			if(gm->hotID == id) {
//				*state = !*state;
//			}
//			ACTIVE(NULL);
//			GUI_CancelInput();
//		}
//	}
//	else CLICK_HOT_TO_ACTIVE(id)
//
//	// bail early if not drawing
//	if(!gm->drawMode) return *state;
//	
//	int st = CUR_STATE(id);
//	
//	GUI_BoxFilled_(gm, tl, sz, o->borderWidth, &o->colors[st].border, &o->colors[st].bg);
//	
//	gm->curZ += 0.01;
//	GUI_TextLineCentered_(gm, text, strlen(text), tl, sz, o->fontName, o->fontSize, &o->colors[st].text);
//	gm->curZ -= 0.01;
//	
//	return *state;
//}
//
//
//// returns true if checked
//int GUI_Checkbox_(GUIManager* gm, void* id, Vector2 tl, char* label, int* state, GUICheckboxOpts* o) {
//	
//	float bs = o->boxSize;
//	Vector2 boxSz = {bs, bs};
//	
//	if(GUI_MouseInside(tl, boxSz)) {
//		HOT(id);
//	}
//	
//	if(gm->activeID == id) {
//		if(GUI_MouseWentUp(1)) {
//			if(gm->hotID == id) {
//				*state = !*state;
//			}
//			ACTIVE(NULL);
//			GUI_CancelInput();
//		}
//	}
//	else CLICK_HOT_TO_ACTIVE(id)
//
//	// bail early if not drawing
//	if(!gm->drawMode) return *state;
//	
//		
//	int st = CUR_STATE(id);
//	if(*state) st = STATE_ACTIVE;
//	
//	GUI_BoxFilled_(gm, tl, boxSz, o->borderWidth, &o->colors[st].border, &o->colors[st].bg);
//	
//	
//	float fontSz = o->fontSize;
////	if(sz.y - (2*2) < fontSz) fontSz = sz.y - (2*2);
//	
//	GUI_TextLine_(gm, label, strlen(label), (Vector2){tl.x + bs + 4, tl.y + fontSz*.75}, o->fontName, fontSz, &o->colors[st].text);
//	
//	return *state;
//}
//
//// returns true if *this* radio button is active
//int GUI_RadioBox_(GUIManager* gm, void* id, Vector2 tl, char* label, void** state, int isDefault, GUIRadioBoxOpts* o) {
//	
//	float bs = o->boxSize;
//	Vector2 boxSz = {bs, bs};
//	
//	if(*state == NULL && isDefault) {
//		*state = id;
//	}
//	
//	if(GUI_MouseInside(tl, boxSz)) {
//		HOT(id);
//	}
//	
//	if(gm->activeID == id) {
//		if(GUI_MouseWentUp(1)) {
//			if(gm->hotID == id) {
//				*state = id;
//			}
//			ACTIVE(NULL);
//			GUI_CancelInput();
//		}
//	}
//	else CLICK_HOT_TO_ACTIVE(id)
//
//	// bail early if not drawing
//	if(!gm->drawMode) return *state == id;
//		
//	int st = CUR_STATE(id);
//	if(*state) st = STATE_ACTIVE;
//	
//	GUI_BoxFilled_(gm, tl, boxSz, o->borderWidth, &o->colors[st].border, &o->colors[st].bg);
//	
//	
//	float fontSz = o->fontSize;
////	if(sz.y - (2*2) < fontSz) fontSz = sz.y - (2*2);
//	
//	GUI_TextLine_(gm, label, strlen(label), (Vector2){tl.x + bs + 4, tl.y + fontSz*.75}, o->fontName, fontSz, &o->colors[st].text);
//	
//	return *state == id;
//}

//
//struct floatslider_data {
//	char buf[64];
//	size_t len;
//	float last_value;
//};
//
//// returns 1 on change
//int GUI_FloatSlider_(GUIManager* gm, void* id, Vector2 tl, float min, float max, float incr, float* value, GUIFloatSliderOpts* o) {
//	struct floatslider_data* d;
//	int first_run = 0;
//	
//	if(!(d = GUI_GetData_(gm, id))) {
//		d = calloc(1, sizeof(*d));
//		d->buf[0] = 0;
//		first_run = 1;
//		GUI_SetData_(gm, id, d, free);
//	}
//	
//	Vector2 sz = o->size;
//	float h = sz.y;
//	float oldV = *value;
//	
//	HOVER_HOT(id);
//	
//	if(gm->activeID == id) {
//		Vector2 mp = GUI_MousePos();
//		
//		float v = mp.x - tl.x;
//		v = (v / sz.x);
//		v = v < 0 ? 0 : (v > 1.0 ? 1.0 : v);
//		v *= max - min;
//		v += min;
//		
//		*value = v; 
//
//		if(GUI_MouseWentUp(1)) {
//			ACTIVE(NULL);
//			GUI_CancelInput();
//		}
//	}
//	if(gm->hotID == id) {
//		MOUSE_DOWN_ACTIVE(id)
//		*value += GUI_GetScrollDist() * incr;
//	}
//	
//	*value = *value > max ? max : (*value < min ? min : *value);
//	float bw = (*value / (max - min)) * sz.x;
//	
//	if(first_run || *value != d->last_value) {
//		d->len = snprintf(d->buf, 64, "%.*f", o->precision, *value);
//		d->last_value = *value;
//	}
//	
//	// bail early if not drawing
//	if(!gm->drawMode) return *value != oldV;
//			
//	int st = CUR_STATE(id);
//		
//	GUI_BoxFilled_(gm, V(tl.x, tl.y), V(bw, h), 0, &o->colors[st].bg, &o->colors[st].bg);
//	GUI_BoxFilled_(gm, V(tl.x + bw, tl.y), V(sz.x - bw, h), 0, &o->colors[st].bar, &o->colors[st].bar);
//	
//	GUI_TextLineCentered_(gm, d->buf, d->len, tl, sz, o->fontName, o->fontSize, &o->colors[st].text);
//
//	return *value != oldV;
//}
//
//struct intslider_data {
//	char buf[64];
//	size_t len;
//	long last_value;
//};
//
//// returns 1 on change
//int GUI_IntSlider_(GUIManager* gm, void* id, Vector2 tl, long min, long max, long* value, GUIIntSliderOpts* o) {
//	struct intslider_data* d;
//	int first_run = 0;
//	
//	if(!(d = GUI_GetData_(gm, id))) {
//		d = calloc(1, sizeof(*d));
//		d->buf[0] = 0;
//		first_run = 1;
//		GUI_SetData_(gm, id, d, free);
//	}
//	
//	
//	Vector2 sz = o->size;
//	float h = sz.y;
//	float width = sz.x;
//	long oldV = *value;
//	
//	HOVER_HOT(id)
//		
//	if(gm->activeID == id) {
//		Vector2 mp = GUI_MousePos();
//		
//		float v = mp.x - tl.x;
//		v = (v / width);
//		v = v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
//		v *= max - min;
//		v += min;
//		*value = floor(v); 
//
//		if(GUI_MouseWentUp(1)) {
//			ACTIVE(NULL);
//			GUI_CancelInput();
//		}
//	}
//	if(gm->hotID == id) {
//		MOUSE_DOWN_ACTIVE(id)
//		*value += GUI_GetScrollDist();
//	}
//
//	*value = *value > max ? max : (*value < min ? min : *value);
//	float bw = ((float)*value / (float)(max - min)) * width;
//	
//	
//	if(first_run || *value != d->last_value) {
//		d->len = snprintf(d->buf, 64, "%ld", *value);
//		d->last_value = *value;		
//	}
//	
//	if(!gm->drawMode) return *value != oldV;
//				
//	int st = CUR_STATE(id);
//		
//	GUI_BoxFilled_(gm, V(tl.x, tl.y), V(bw, h), 0, &o->colors[st].bg, &o->colors[st].bg);
//	GUI_BoxFilled_(gm, V(tl.x + bw, tl.y), V(sz.x - bw, h), 0, &o->colors[st].bar, &o->colors[st].bar);
//
//	GUI_TextLineCentered_(gm, d->buf, d->len, tl, sz, o->fontName, o->fontSize, &o->colors[st].text);
//
//	return *value != oldV;
//}
//
//
//// returns 1 when the value changes _due to this control_
//int GUI_OptionSlider_(GUIManager* gm, void* id, Vector2 tl, char** options, int* selectedOption, GUIOptionSliderOpts* o) {
//	int ret = 0;
//	Vector2 sz = o->size;
//	float h = sz.y;
//	float width = sz.x;
//	int old_opt = *selectedOption; 
//	
//	int cnt = 0;
//	for(char** p = options; *p; p++) cnt++;
//	
//	HOVER_HOT(id)
//	
//	if(gm->activeID == id) {
//		Vector2 mp = GUI_MousePos();
//		
//		float v = mp.x - tl.x;
//		v = (v / width);
//		v = v < 0 ? 0 : (v > 1.0 ? 1.0 : v);
//		*selectedOption = cnt * v;
//
//		if(GUI_MouseWentUp(1)) {
//			ACTIVE(NULL);
//			GUI_CancelInput();
//		}
//	}
//	
//	if(gm->hotID == id) {
//		MOUSE_DOWN_ACTIVE(id)
//		*selectedOption += GUI_GetScrollDist();
//	}
//	
//	CLAMP(0, *selectedOption, cnt - 1);
//	
//	// bail early if not drawing
//	if(!gm->drawMode) return old_opt != *selectedOption;
//	
//	float bw = ((float)(*selectedOption + 1) / (float)cnt) * width;
//					
//	int st = CUR_STATE(id);
//		
//	GUI_BoxFilled_(gm, V(tl.x, tl.y), V(bw, h), 0, &o->colors[st].bg, &o->colors[st].bg);
//	GUI_BoxFilled_(gm, V(tl.x + bw, tl.y), V(sz.x - bw, h), 0, &o->colors[st].bar, &o->colors[st].bar);
//
//	GUI_TextLineCentered_(gm, options[*selectedOption], -1, tl, sz, o->fontName, o->fontSize, &o->colors[st].text);
//
//	return old_opt != *selectedOption;
//}
#if 0

// returns 1 when the value changes
int GUI_SelectBox_(GUIManager* gm, void* id, Vector2 tl, float width, char** options, int* selectedOption, GUISelectBoxOpts* o) {
	int ret = 0;
	Vector2 sz = o->size;
	if(width <= 0) sz.x = width;
	
	int optsLen = 0;
	for(char**p = options; *p != NULL; p++) optsLen++;
	
	HOVER_HOT(id)
	
	if(gm->activeID == id) {
		if(GUI_MouseWentUpAnywhere(1)) {
			if(!GUI_MouseInside(tl, sz)) {
				GUI_BeginWindow(id, V(tl.x + 3, tl.y + sz.y), V(tl.x, sz.y * 3), gm->curZ + 5, 0);
				Vector2 mpos = GUI_MousePos();
				
				int n = mpos.y / sz.y;
				
				if(n >= 0 && n < optsLen && *selectedOption != n) {
					ret = 1;
					*selectedOption = n;
				}
				
				GUI_EndWindow();
				
				ACTIVE(NULL);
			}
		}
		else MOUSE_DOWN_ACTIVE(NULL)
	}
	else if(gm->hotID == id) {
		MOUSE_DOWN_ACTIVE(id)
		*selectedOption -= GUI_GetScrollDist();
	}

	// bail early if not drawing
	if(!gm->drawMode) return ret;
	
	float fontSz = o->fontSize;
	CLAMP(0, *selectedOption, optsLen - 1);
						
	int st = CUR_STATE(id);
		
	GUI_BoxFilled_(gm, tl, sz, 2, &o->colors[st].border, &o->colors[st].bg);
	
	
	if(gm->activeID == id) {
		// draw the dropdown
		
		GUI_BeginWindow(id, V(tl.x + 3, tl.y + sz.y), V(tl.x, sz.y * 3), gm->curZ + 5, 0);
		
		gm->curZ += 0.01;
		int cnt = 0;
		for(char** p = options; *p; p++) {
			
			gm->curZ += 0.02;
			GUI_TextLineCentered_(gm, *p, strlen(*p), V(0, sz.y * cnt + fontSz*.25 - 6), sz, o->fontName, fontSz, &o->colors[st].text);
			gm->curZ -= 0.01;
			
			if(GUI_MouseInside(V(0, sz.y * cnt), sz)) {
				GUI_BoxFilled_(gm, V(0, sz.y * cnt), sz, 0, C(.8,.6,.3), C(.7,.4,.2));
			}
			gm->curZ -= 0.01;

			cnt++;
		}
		
		gm->curZ -= 0.01;
		
		
		Vector2 boxSz = {sz.x, sz.y * cnt};
		
		GUI_BoxFilled_(gm, V(0,0), boxSz, 2, C(.5,.6,.6), C(.4,.4,.4));
		
		GUI_EndWindow();
		
	}
	
	gm->curZ += 0.01;
	GUI_TextLineCentered_(gm, options[*selectedOption], strlen(options[*selectedOption]), (Vector2){tl.x, tl.y - fontSz*.25}, sz, "Arial", fontSz, &o->colors[st].text);
	gm->curZ -= 0.01;
	
	return ret;
} 
#endif

#if 0

struct edit_data {
	GUICursorData cursor;
	float scrollX; // x offset to add to all text rendering code 
	char synthed_change; // indicates that the buffer was changed externally
	
	GUIEditFilterFn filter_fn;
	void* filter_user_data;
};


static void check_string(GUIString* s, int extra) {
	if(!s->data) {
		s->alloc = 64;
		s->data = malloc(s->alloc * sizeof(*s->data));
		s->data[0] = 0;
	}
	else if(s->alloc <= s->len + extra) {
		s->alloc = nextPOT(s->len + extra + 1);
		s->data = realloc(s->data, s->alloc * sizeof(*s->data));
	}
}

static void delete_selection(GUICursorData* cd, GUIString* str) {
	int start = MIN(cd->selectPivot, cd->cursorPos);
	int end = MAX(cd->selectPivot, cd->cursorPos);
	int len = end - start;
	
	memmove(str->data + start, str->data + end, str->len - len + 1);
	str->len -= len;
//	printf("now %ld long\n", str->len);
	cd->cursorPos = start;
	cd->selectPivot = -1;
}

static void select_all(GUICursorData* cd, GUIString* str) {
	cd->cursorPos = str->len;
	cd->selectPivot = 0;
}

static void select_none(GUICursorData* cd) {
	cd->cursorPos = 0;
	cd->selectPivot = -1;
}

static void set_clipboard(int which, GUICursorData* cd, GUIString* str) {
	char* tmp;
	int selLen;
	if(cd->selectPivot > cd->cursorPos) {
		selLen = cd->selectPivot - cd->cursorPos;
		tmp = malloc((selLen+1)*sizeof(*tmp));
		memcpy(tmp, str->data + cd->cursorPos, selLen);
		tmp[selLen] = 0;
//		printf("case a: pushing %d chars '%*s'\n", cd->selectPivot - cd->cursorPos, cd->selectPivot - cd->cursorPos, str->data + cd->cursorPos); 
	} else {
		selLen = cd->cursorPos - cd->selectPivot;
		tmp = malloc((selLen+1)*sizeof(*tmp));
		memcpy(tmp, str->data + cd->selectPivot, selLen);
		tmp[selLen] = 0;
//		printf("case b: pushing %d chars '%*s'\n", cd->cursorPos - cd->selectPivot, cd->cursorPos - cd->selectPivot, str->data + cd->selectPivot);
	}
//	printf("pushing %d chars %s\n", selLen, tmp);
	Clipboard_PushRawText(which, tmp, selLen);
	free(tmp);
}

// return 1 on changes
int GUI_HandleCursor_(GUIManager* gm, GUICursorData* cd, GUIString* str, GUIEvent* e) {
	int ret = 0;

	switch(e->keycode) {
		case 'c':
			if(cd->selectPivot > -1 && e->modifiers == GUIMODKEY_CTRL) {
//				printf("DO COPY; captured 'c' with shift? %d, with ctrl? %d\n", e->modifiers & GUIMODKEY_SHIFT, e->modifiers & GUIMODKEY_CTRL);
				set_clipboard(CLIP_PRIMARY, cd, str);
			} else break;
			goto BLINK;
		
		case 'v':
			if(e->modifiers == GUIMODKEY_CTRL) {
//				printf("DO PASTE; captured 'v' with shift? %d, with ctrl? %d\n", e->modifiers & GUIMODKEY_SHIFT, e->modifiers & GUIMODKEY_CTRL);
				char* pasteData;
				size_t pasteLen;
				Clipboard_PeekRawText(CLIP_PRIMARY, &pasteData, &pasteLen);
				
//				printf("paste data: %s, len %ld)\n", pasteData, pasteLen);
				
				if(pasteLen > 0) {
					if(cd->selectPivot > -1) delete_selection(cd, str);
					check_string(str, pasteLen);
					memmove(str->data + cd->cursorPos + pasteLen, str->data + cd->cursorPos, str->len - cd->cursorPos + 1);
					memcpy(str->data + cd->cursorPos, pasteData, pasteLen);
					str->len += pasteLen;
					cd->cursorPos += pasteLen;
					cd->blinkTimer = 0;
					
					GUI_CancelInput();
					ret = 1;
				}
			} else break;
			goto BLINK;
		
		case 'x':
			if(e->modifiers == GUIMODKEY_CTRL) {
//				printf("DO CUT; captured 'x' with shift? %d, with ctrl? %d\n", e->modifiers & GUIMODKEY_SHIFT, e->modifiers & GUIMODKEY_CTRL);
				if(cd->selectPivot > -1) {
					set_clipboard(CLIP_PRIMARY, cd, str);
					delete_selection(cd, str);
				}
			} else break;
			goto BLINK;
		
		case XK_Left:
			if(e->modifiers == GUIMODKEY_SHIFT) { // start selection
				if(cd->selectPivot == -1) {
					cd->selectPivot = cd->cursorPos;
				}
				cd->cursorPos = cd->cursorPos - 1 <= 0 ? 0 : cd->cursorPos - 1;
				set_clipboard(CLIP_SELECTION, cd, str);
			}
			// ctrl: move by sequences
			else if(e->modifiers == 0) { // just move the cursor normally
				cd->cursorPos = cd->cursorPos - 1 <= 0 ? 0 : cd->cursorPos - 1;
				cd->selectPivot = -1;
			}
			else break;
			goto BLINK;
							
		case XK_Right: 
			if(e->modifiers == GUIMODKEY_SHIFT) { // start selection
				if(cd->selectPivot == -1) {
					cd->selectPivot = cd->cursorPos;
				}
				cd->cursorPos = cd->cursorPos + 1 > str->len ? str->len : cd->cursorPos + 1;
				set_clipboard(CLIP_SELECTION, cd, str);
			}
			// ctrl: move by sequences
			else if(e->modifiers == 0) { // just move the cursor normally
				cd->cursorPos = cd->cursorPos + 1 > str->len ? str->len : cd->cursorPos + 1;
				cd->selectPivot = -1;
			}
			else break;
			goto BLINK;
			
		case XK_BackSpace: 
			if(cd->selectPivot > -1) { // delete the selection
				delete_selection(cd, str);
			}
			else if(cd->cursorPos > 0) { // just one character
				memmove(str->data + cd->cursorPos - 1, str->data + cd->cursorPos, str->len - cd->cursorPos + 1 + 1); // extra +1 is a blind guess
				cd->cursorPos = cd->cursorPos > 0 ? cd->cursorPos - 1 : 0;
				str->len--;
				cd->selectPivot = -1;
				ret = 1;
			}
			
			goto BLINK;
			
		case XK_Delete: 
			if(cd->selectPivot > -1) { // delete the selection
				delete_selection(cd, str);
			}
			else if(cd->cursorPos < str->len) { // just one character
				memmove(str->data + cd->cursorPos, str->data + cd->cursorPos + 1, str->len - cd->cursorPos + 1); // extra +1 is a blind guess
				str->len--;
				cd->selectPivot = -1;
				ret = 1;
			}
			
			goto BLINK;
							
//			case XK_Return: ACTIVE(NULL); break;
		case XK_Home:
			// shift: create selection to start
			// : move to start
			if(e->modifiers & GUIMODKEY_SHIFT) {
				cd->selectPivot = cd->cursorPos;
				cd->cursorPos = 0;
				set_clipboard(CLIP_SELECTION, cd, str);
			}
			else {
				cd->cursorPos = 0;
				cd->selectPivot = -1;
			}
			goto BLINK;
		
		case XK_End:
			// shift: create selection to end
			// : move to end
			if(e->modifiers & GUIMODKEY_SHIFT) {
				cd->selectPivot = cd->cursorPos;
				cd->cursorPos = str->len;
				set_clipboard(CLIP_SELECTION, cd, str);
			}
			else {
				cd->cursorPos = str->len;
				cd->selectPivot = -1;
			}
			goto BLINK;
	}
	
	return ret;

BLINK:
	cd->blinkTimer = 0;	
	GUI_CancelInput();	
	
	return ret;
}

// returns 1 on changes made to the buffer
int GUI_HandleTextInput_(GUIManager* gm, GUICursorData* cd, GUIString* str, GUIEvent* e) {
	int ret = 0;
	
	cd->cursorPos = MIN(cd->cursorPos, (int)str->len);
	cd->selectPivot = MIN(cd->selectPivot, (int)str->len);
	
	if(isprint(e->character) && (e->modifiers & ~(GUIMODKEY_SHIFT | GUIMODKEY_RSHIFT | GUIMODKEY_LSHIFT)) == 0) {
	
		if(cd->selectPivot > -1) {
			delete_selection(cd, str);
		}
		
		check_string(str, 1);
		memmove(str->data + cd->cursorPos + 1, str->data + cd->cursorPos, str->len - cd->cursorPos + 1);
		str->data[cd->cursorPos] = e->character;
		str->len++;
		cd->cursorPos++;
		cd->blinkTimer = 0;
		
		GUI_CancelInput();
		ret = 1;
	}
	else {
		ret |= GUI_HandleCursor_(gm, cd, str, e);
	}
	
	return ret;
}
#endif

#if 0
// filter all input before accepting it
void GUI_Edit_SetFilter_(GUIManager* gm, void* id, GUIEditFilterFn fn, void* data) {
	struct edit_data* d;
	
	if(!(d = GUI_GetData_(gm, id))) {
		d = calloc(1, sizeof(*d));
		d->cursor.selectPivot = -1;
		GUI_SetData_(gm, id, d, free);
	}
	
	d->filter_fn = fn;
	d->filter_user_data = data;
}


// synthesizes an input event for the given edit control
int GUI_Edit_Trigger_(GUIManager* gm, void* id, GUIString* str, int c) {
	struct edit_data* d;
	
	if(!(d = GUI_GetData_(gm, id))) {
		d = calloc(1, sizeof(*d));
		d->cursor.selectPivot = -1;
		GUI_SetData_(gm, id, d, free);
	}
	
	GUIEvent e = {
		.type = GUIEVENT_KeyDown,
		.character = c,
		.keycode = 0,
		.modifiers = 0,
	};
	
	if(d->filter_fn && isprint(c)) {
		if(!d->filter_fn(str, &e, d->cursor.cursorPos, d->filter_user_data)) return 0;
	}
	
	int ret = GUI_HandleTextInput_(gm, &d->cursor, str, &e);
	d->synthed_change |= ret;
	
	return ret;
}

// returns true on a change
int GUI_Edit_(GUIManager* gm, void* id, Vector2 tl, float width, GUIString* str, GUIEditOpts* o) {
	int ret;
	struct edit_data* d;
	
	if(!(d = GUI_GetData_(gm, id))) {
		d = calloc(1, sizeof(*d));
		d->cursor.selectPivot = -1;
		if(o->selectAll) {
			d->cursor.selectPivot = 0;
			d->cursor.cursorPos = str->len;
		}
		
		GUI_SetData_(gm, id, d, free);
	}
	
	check_string(str, 0); // make sure the input string exists
	
	ret = d->synthed_change;
	d->synthed_change = 0;
	
	Vector2 sz = o->size;
	if(width > 0) sz.x = width;
	float fontSz = o->fontSize;
	vec2 pad = o[0].padding;
	
	GUIFont* font = GUI_FindFont(gm, o->fontName);	
	
	HOVER_HOT(id)
	
	if(gm->hotID == id) {
		bool wasActive = gm->activeID == id;
		
		MOUSE_DOWN_ACTIVE(id)
		
		// select all if first click on inactive
		if(!wasActive && gm->activeID == id) {
			// select all
			select_all(&d->cursor, str);
		}
		
		if(GUI_MouseWentDown(1)) {
			// position the cursor
			Vector2 mp = GUI_MousePos();
//			d->cursor.cursorPos = gui_charFromPixel(gm, font, fontSz, str->data, mp.x - tl.x);
//			d->cursor.cursorPos = MIN(d->cursor.cursorPos, str->len);
			d->cursor.selectPivot = -1;
			d->cursor.cursorPos = floor(GUI_CharFromPixelF(font, o[0].fontSize, str->data, str->len, mp.x - (tl.x + pad.x + d->scrollX)));
		}
		else if(GUI_MouseWentUp(1)) {
			if(gm->curEvent.multiClick == 2) {
				// select all
				select_all(&d->cursor, str);
			}
		}
		else if(GUI_MouseWentUp(2)) {
			char* pasteData;
			size_t pasteLen;
			Clipboard_PeekRawText(CLIP_SELECTION, &pasteData, &pasteLen);
			GUICursorData* cd = &d->cursor;
				
//			printf("paste data: %s, len %ld)\n", pasteData, pasteLen);
				
			if(pasteLen > 0) {
				if(cd->selectPivot > -1) delete_selection(cd, str);
				check_string(str, pasteLen);
				memmove(str->data + cd->cursorPos + pasteLen, str->data + cd->cursorPos, str->len - cd->cursorPos + 1);
				memcpy(str->data + cd->cursorPos, pasteData, pasteLen);
				str->len += pasteLen;
				cd->cursorPos += pasteLen;
				cd->blinkTimer = 0;
				
				GUI_CancelInput();
				ret |= 1;
			}
		}
	}
	
		// mouse selection dragging
	if(GUI_InputAvailable()) {
		
		if(gm->curEvent.type == GUIEVENT_DragStart && GUI_PointInBoxV(tl, sz, gm->lastMousePos)) {
			d->cursor.isMouseDragging = 1;
			// set pivot 
			d->cursor.selectPivot = d->cursor.cursorPos;
		}
		else if(d->cursor.isMouseDragging && gm->curEvent.type == GUIEVENT_DragMove) {
		
			// set cursorPos
			vec2 mp = GUI_MousePos();
			d->cursor.cursorPos = floor(GUI_CharFromPixelF(font, o[0].fontSize, str->data, str->len, mp.x - (tl.x + pad.x + d->scrollX)));
		}
		else if(d->cursor.isMouseDragging && gm->curEvent.type == GUIEVENT_DragStop) {
			d->cursor.isMouseDragging = 0;
		}
	}
	
	// handle input
	if(gm->activeID == id) {
		if(gm->curEvent.type == GUIEVENT_KeyDown) {
			if(!isprint(gm->curEvent.character) || !d->filter_fn || d->filter_fn(str, &gm->curEvent, d->cursor.cursorPos, d->filter_user_data)) {
				ret |= GUI_HandleTextInput_(gm, &d->cursor, str, &gm->curEvent);
			}
		}
	}
	
	// bail early if not drawing
	if(!gm->drawMode) return ret;
	
	int st = CUR_STATE(id);
	
	// draw the border
	GUI_BoxFilled_(gm, tl, sz, o->borderWidth, &o->colors[st].border, &o->colors[st].bg);
	
	float cursorOff = gui_getTextLineWidth(gm, font, fontSz, str->data, d->cursor.cursorPos);
	
	// calculate scroll offsets
	if(cursorOff + d->scrollX > sz.x) {
		d->scrollX = sz.x - cursorOff;	
	}
	else if(cursorOff + d->scrollX < 0) {
		d->scrollX = -cursorOff;
	}
	
	GUI_PushClip(tl, sz);
	
	// draw cursor and selection background
	if(gm->activeID == id) {
		gm->curZ += 0.001;
		if(d->cursor.blinkTimer < 0.5) { 
			gm->curZ += 0.001;
			
			GUI_Rect(V(tl.x + cursorOff + d->scrollX, tl.y), V(2,sz.y), &o->cursorColor);
			gm->curZ -= 0.001;
		}
		
		d->cursor.blinkTimer += gm->timeElapsed;
		d->cursor.blinkTimer = fmod(d->cursor.blinkTimer, 1.0);
		
		if(d->cursor.selectPivot > -1) {
			float pivotOff = gui_getTextLineWidth(gm, font, fontSz, str->data, d->cursor.selectPivot);
			
			int min = MIN(cursorOff, pivotOff);
			int max = MAX(cursorOff, pivotOff);
			
			GUI_Rect(V(tl.x + min + d->scrollX, tl.y), V(max - min,sz.y), &o->selectionBgColor);
			
		}
		gm->curZ -= 0.001;
	}
	

	
	gm->curZ += 10.01;
	
	if(str->len) {
		GUI_TextLine_(gm, str->data, str->len, V(tl.x + d->scrollX, tl.y ), o->fontName, fontSz, &o->colors[st].text);
	}
	GUI_PopClip();
	gm->curZ -= 10.01;
	
	return ret;
}
#endif


//
//
//struct intedit_data {
//	struct edit_data ed;
//	int cursorPos;
//	float blinkTimer;
//	
//	long lastValue;
//	GUIString str;
//};
//
//static void intedit_free(struct intedit_data* d) {
//	free(d->str.data);
//	free(d);
//}
//
//// returns true on a change
//int GUI_IntEdit_(GUIManager* gm, void* id, Vector2 tl, float width, long* num, GUIIntEditOpts* o) {
//	int ret = 0;
//	int firstRun = 0;
//	struct intedit_data* d;
//	
//	if(!(d = GUI_GetData_(gm, id))) {
//		d = calloc(1, sizeof(*d));
//		GUI_SetData_(gm, id, d, (void*)intedit_free);
//		
//		d->str.alloc = 64;
//		d->str.data = calloc(1, d->str.alloc * sizeof(*d->str.data));
//		firstRun = 1;
//	}
//	
//	GUIFont* font = GUI_FindFont(gm, o[0].fontName);
//	
//	Vector2 sz = o[0].size;
//	if(width > 0) sz.x = width;
//	vec2 pad = o[0].padding;
//	
//	HOVER_HOT(id)
//
//	if(gm->hotID == id) {
//		MOUSE_DOWN_ACTIVE(id)
//		
//		if(GUI_MouseWentUp(1)) {
//			// position the cursor
//			Vector2 mp = GUI_MousePos();
//			d->ed.cursor.cursorPos = gui_charFromPixel(gm, font, o[0].fontSize, d->str.data, mp.x - tl.x + pad.x);
//		}
//	}
//	
//	// handle input
//	if(gm->activeID == id) {
//		if(gm->curEvent.type == GUIEVENT_KeyDown) {
//			if(!isprint(gm->curEvent.character) || ('0' <= gm->curEvent.character && gm->curEvent.character <= '9')) {
//				ret |= GUI_HandleTextInput_(gm, &d->ed.cursor, &d->str, &gm->curEvent);
//			}
//		}
//	}
//	
//	if(ret) {
//		*num = strtol(d->str.data, NULL, 10);
//	}
//
//	// refresh the buffer, maybe
//	if(*num != d->lastValue || firstRun) {
//		d->str.len = snprintf(d->str.data, 64, "%ld", *num);
//		d->lastValue = *num;
//	}
//	
//	// bail early if not drawing
//	if(!gm->drawMode) return ret;
//	int st = CUR_STATE(id);
//	
//	// draw the border
//	GUI_BoxFilled_(gm, tl, sz, o[st].borderWidth, &o[st].border, &o[st].bg);
//		
//	float fontSz = o[st].fontSize;
//	
//	float cursorOff = GUI_GetTextWidthAdv(d->str.data, d->ed.cursor.cursorPos, font, fontSz);
//	
//	// calculate scroll offsets
//	if(cursorOff + d->ed.scrollX > sz.x - pad.x * 2) {
//		d->ed.scrollX = sz.x - cursorOff;	
//	}
//	else if(cursorOff + d->ed.scrollX < pad.x) {
//		d->ed.scrollX = -cursorOff;
//	}
//	
//	GUI_PushClip(tl, sz);
//	
//	// draw cursor and selection background
//	if(gm->activeID == id) {
//		gm->curZ += 0.001;
//		if(d->ed.cursor.blinkTimer < 0.5) { 
//			gm->curZ += 0.001;
//			
//			// TODO: dpi scaling here
//			GUI_Rect(V(tl.x + cursorOff + d->ed.scrollX + pad.x, tl.y + o[st].borderWidth + 1), V(2,sz.y - o[st].borderWidth * 2 - 2), &o[st].cursorColor);
//			gm->curZ -= 0.001;
//		}
//		
//		d->ed.cursor.blinkTimer += gm->timeElapsed;
//		d->ed.cursor.blinkTimer = fmod(d->ed.cursor.blinkTimer, 1.0);
//		
//		if(d->ed.cursor.selectPivot > -1) {
//			float pivotOff = GUI_GetTextWidthAdv(d->str.data, d->ed.cursor.selectPivot, font, fontSz);
//			
//			int min = MIN(cursorOff, pivotOff);
//			int max = MAX(cursorOff, pivotOff);
//			
//			GUI_Rect(V(tl.x + min + d->ed.scrollX, tl.y), V(max - min,sz.y), &o[st].selectionBgColor);
//			
//		}
//		gm->curZ -= 0.001;
//	}
//	
//
//	
//	gm->curZ += 10.01;
//	
//	if(d->str.len) {
//		GUI_TextLineAdv_(gm, V(tl.x + d->ed.scrollX + pad.x, tl.y), sz, d->str.data, d->str.len, GUI_TEXT_ALIGN_VCENTER, font, fontSz, &o[st].text);
//	}
//	GUI_PopClip();
//	gm->curZ -= 10.01;
//	
//	return ret;
//}
//



//
//struct floatedit_data {
//	struct edit_data ed;
//	int cursorPos;
//	float scrollX; // x offset to add to all text rendering code 
//	float blinkTimer;
//	
//	float lastValue;
//	GUIString str;
//};
//
//static void floatedit_free(struct floatedit_data* d) {
//	free(d->str.data);
//	free(d);
//}
//
//// returns true on a change
//int GUI_FloatEdit_(GUIManager* gm, void* id, Vector2 tl, float width, float* num, GUIFloatEditOpts* o) {
//	int ret = 0;
//	int firstRun = 0;
//	struct floatedit_data* d = NULL;
//	
//	if(!(d = GUI_GetData_(gm, id))) {
//		d = calloc(1, sizeof(*d));
//		GUI_SetData_(gm, id, d, (void*)floatedit_free);
//		
//		d->str.alloc = 64;
//		d->str.data = calloc(1, d->str.alloc * sizeof(*d->str.data));
//		firstRun = 1;
//	}
//	
//	GUIFont* font = GUI_FindFont(gm, o[0].fontName);
//	
//	Vector2 sz = o[0].size;
//	if(width > 0) sz.x = width;
//	vec2 pad = o[0].padding;
//	
//	HOVER_HOT(id)
//
//	if(gm->hotID == id) {
//		bool wasActive = gm->activeID == id;
//	
//		MOUSE_DOWN_ACTIVE(id)
//		
//		// select all if first click on inactive
//		if(!wasActive && gm->activeID == id) {
//			// select all
//			select_all(&d->ed.cursor, &d->str);
//		}
//		
//		if(GUI_MouseWentDown(1)) {
//			// kill selection, position the cursor
//			Vector2 mp = GUI_MousePos();
//			d->ed.cursor.selectPivot = -1;
//			d->ed.cursor.cursorPos = floor(GUI_CharFromPixelF(font, o[0].fontSize, d->str.data, d->str.len, mp.x - (tl.x + pad.x + d->ed.scrollX)));
//		}
//		if(GUI_MouseWentUp(1)) {
//			if(gm->curEvent.multiClick == 2) {
//				// select all
//				select_all(&d->ed.cursor, &d->str);
//			}
//		}
//	}
//	
//	// mouse selection dragging
//	if(GUI_InputAvailable()) {
//		
//		if(gm->curEvent.type == GUIEVENT_DragStart && GUI_PointInBoxV(tl, sz, gm->lastMousePos)) {
//			d->ed.cursor.isMouseDragging = 1;
//			// set pivot 
//			d->ed.cursor.selectPivot = d->ed.cursor.cursorPos;
//		}
//		else if(d->ed.cursor.isMouseDragging && gm->curEvent.type == GUIEVENT_DragMove) {
//		
//			// set cursorPos
//			vec2 mp = GUI_MousePos();
//			d->ed.cursor.cursorPos = floor(GUI_CharFromPixelF(font, o[0].fontSize, d->str.data, d->str.len, mp.x - (tl.x + pad.x + d->ed.scrollX)));
//		}
//		else if(d->ed.cursor.isMouseDragging && gm->curEvent.type == GUIEVENT_DragStop) {
//			d->ed.cursor.isMouseDragging = 0;
//		}
//	}
//	
//	
//	// handle input
//	if(gm->activeID == id) {
//		if(gm->curEvent.type == GUIEVENT_KeyDown) {
//			if(
//				!isprint(gm->curEvent.character) // control events (arrow keys)
//				|| strchr("0123456789.-", gm->curEvent.character) // numbers 
//			) {
//				ret |= GUI_HandleTextInput_(gm, &d->ed.cursor, &d->str, &gm->curEvent);
//			}
//		}
//		
//		if(GUI_MouseWentDownAnywhere(1) && !GUI_MouseInside(tl, sz)) {
//			ACTIVE(NULL);
//		}
//	}
//	
//	if(gm->activeID != id) {
//		select_none(&d->ed.cursor);
//	}
//	
//	if(ret) {
//		*num = strtof(d->str.data, NULL);
//		d->lastValue = *num;
//	}
//	
//
//	if(*num != d->lastValue || firstRun) {
//		switch(fpclassify(*num)) {
//			default:
//			case FP_NORMAL:
//			case FP_SUBNORMAL:
//			case FP_ZERO:
//				d->str.len = snprintf(d->str.data, 64, "%f", *num);
//				d->lastValue = *num;
//				break;
//			case FP_NAN: 
//				strcpy(d->str.data, "NaN");
//				d->str.len = 3;
//				break;
//			case FP_INFINITE:
//				strcpy(d->str.data, "Infinity");
//				d->str.len = strlen("Infinity");
//				break;
//		}
//	}
//	
//
//	
//	// bail early if not drawing
//	if(!gm->drawMode) return ret;
//	int st = CUR_STATE(id);
//	
//	// draw the border
//	GUI_BoxFilled_(gm, tl, sz, o[st].borderWidth, &o[st].border, &o[st].bg);
//	
//	// refresh the buffer, maybe
//		
//	float fontSz = o[st].fontSize;
//	
//	float cursorOff = GUI_GetTextWidthAdv(d->str.data, d->ed.cursor.cursorPos, font, fontSz);
//	
//	// calculate scroll offsets
//	if(cursorOff + d->ed.scrollX > sz.x - pad.x * 2) {
//		d->ed.scrollX = sz.x - cursorOff;	
//	}
//	else if(cursorOff + d->ed.scrollX < pad.x) {
//		d->ed.scrollX = -cursorOff;
//	}
//	
//	GUI_PushClip(tl, sz);
//	
//	// draw cursor and selection background
//	if(gm->activeID == id) {
//		gm->curZ += 0.001;
//		if(d->ed.cursor.blinkTimer < 0.5) { 
//			gm->curZ += 0.001;
//			
//			// TODO: dpi scaling here
//			GUI_Rect(V(tl.x + cursorOff + d->ed.scrollX + pad.x, tl.y + o[st].borderWidth + 1), V(2,sz.y - o[st].borderWidth * 2 - 2), &o[st].cursorColor);
//			gm->curZ -= 0.001;
//		}
//		
//		d->ed.cursor.blinkTimer += gm->timeElapsed;
//		d->ed.cursor.blinkTimer = fmod(d->ed.cursor.blinkTimer, 1.0);
//		
//		if(d->ed.cursor.selectPivot > -1) {
//			float pivotOff = GUI_GetTextWidthAdv(d->str.data, d->ed.cursor.selectPivot, font, fontSz);
//			
//			int min = MIN(cursorOff, pivotOff);
//			int max = MAX(cursorOff, pivotOff);
//			
//			GUI_Rect(V(tl.x + min + d->ed.scrollX + pad.x, tl.y + pad.y), V(max - min,sz.y - pad.y*2), &o[st].selectionBgColor);
//			
//		}
//		gm->curZ -= 0.001;
//	}
//	
//
//	
//	gm->curZ += 10.01;
//	
//	if(d->str.len) {
//		GUI_TextLineAdv_(gm, V(tl.x + d->ed.scrollX + pad.x, tl.y), V(9999999,sz.y), d->str.data, d->str.len, GUI_TEXT_ALIGN_VCENTER, font, fontSz, &o[st].text);
//	}
//	GUI_PopClip();
//	gm->curZ -= 10.01;
//	
//	return ret;
//}
//
//

// sets the current clipping region, respecting the current window
void GUI_PushClip_(GUIManager* gm, Vector2 tl, Vector2 sz) {
	VEC_PUSH(&gm->clipStack, gm->curClip);

	Vector2 abstl = vAdd2(tl, gm->curWin->absClip.min);
	gm->curClip = gui_clipTo(gm->curWin->absClip, (AABB2){min: abstl, max: vAdd2(abstl, sz)});
}

void GUI_PopClip_(GUIManager* gm) {
	if(VEC_LEN(&gm->clipStack) <= 0) {
		fprintf(stderr, "Tried to pop an empty clip stack\n");
		return;
	}
	
	VEC_POP(&gm->clipStack, gm->curClip);
}

//
//struct window_data {
//	Vector2 scroll;
//};
//

// create a new window, push it to the stack, and set it current
void GUI_BeginWindow_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, float z, unsigned long flags) {
	GUIWindow* w, *p;
	struct window_data* d;
	
	
	p = VEC_TAIL(&gm->windowStack);
	w = GUIWindow_new(gm, p);
	
//	d = GUI_GetData_(gm, id))) {
//	Vector2 off = {0};
//	if(d) off = d->scroll;
	
	w->preservedZ = gm->curZ;
	
	w->id = id;
	w->z = z;
	w->flags = flags;
	
	// TODO: calculate the proper visual clipping region
	w->absClip.min.x = p->absClip.min.x + tl.x;
	w->absClip.min.y = p->absClip.min.y + tl.y;
	w->absClip.max.x = w->absClip.min.x + sz.x;
	w->absClip.max.y = w->absClip.min.y + sz.y;
	
	VEC_PUSH(&gm->clipStack, gm->curClip);
	gm->curClip = w->absClip;
	
	VEC_PUSH(&p->children, w);
	
	VEC_PUSH(&gm->windowStack, w);
	gm->curWin = w;
}


// pop the window stack and set the previous window to be current
void GUI_EndWindow_(GUIManager* gm) {
	
	if(VEC_LEN(&gm->windowStack) <= 1) {
		fprintf(stderr, "Tried to pop root window\n");
		return;
	}	
	
	gm->curZ = gm->curWin->preservedZ;
	
	VEC_POP1(&gm->windowStack);
	gm->curWin = VEC_TAIL(&gm->windowStack);
	
	VEC_POP(&gm->clipStack, gm->curClip);	
}



void GUI_Box_(GUIManager* gm, Vector2 tl, Vector2 sz, float width, Color4* borderColor) {
	if(!gm->drawMode) return; // this function is only for drawing mode
	
	Color4 clear = {0,0,0,0};
	GUI_BoxFilled_(gm, tl, sz, width, borderColor, &clear);
}

void GUI_BoxFilled_(
	GUIManager* gm, 
	Vector2 tl, 
	Vector2 sz, 
	float width, 
	Color4* borderColor, 
	Color4* bgColor
) {
	if(!gm->drawMode) return; // this function is only for drawing mode
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	*v++ = (GUIUnifiedVertex){
		.pos = {tl.x, tl.y, tl.x + sz.x, tl.y + sz.y},
		.clip = GUI_AABB2_TO_SHADER(gm->curClip),
		
		.guiType = 4, // bordered box
		
		.texIndex1 = width,
		
		.fg = GUI_COLOR4_TO_SHADER(*borderColor), 
		.bg = GUI_COLOR4_TO_SHADER(*bgColor), 
		.z = gm->curZ,
		.alpha = 1,
	};
}

void GUI_CircleFilled_(
	GUIManager* gm, 
	Vector2 center, 
	float radius, 
	float borderWidth, 
	Color4* borderColor, 
	Color4* bgColor
) {
	if(!gm->drawMode) return; // this function is only for drawing mode
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	
	*v++ = (GUIUnifiedVertex){
		.pos = {center.x, center.y, center.x + radius, center.y + radius},
		.clip = GUI_AABB2_TO_SHADER(gm->curClip),
		
		.guiType = 7, // ellipse
		
		.texIndex1 = borderWidth,
		
		.fg = GUI_COLOR4_TO_SHADER(*borderColor), 
		.bg = GUI_COLOR4_TO_SHADER(*bgColor), 
		.z = gm->curZ,
		.alpha = 1,
		.rot = 0,
	};
}
//
//// draws a single character from its font origin point
//void GUI_Char_(GUIManager* gm, int c, Vector2 origin, char* fontName, float size, Color4* color) {
//	if(!gm->drawMode) return; // this function is only for drawing mode
//	
//	GUIFont* font = GUI_FindFont(gm, fontName);
//	if(!font) font = gm->defaults.font;
//	
//	GUI_CharFont_NoGuard_(gm, c, origin, font, size, color);
//}
//
//// draws a single character from its font origin point
//void GUI_CharFont_(GUIManager* gm, int c, Vector2 origin, GUIFont* font, float size, Color4* color) {
//	if(!gm->drawMode) return; // this function is only for drawing mode
//	
//	GUI_CharFont_NoGuard_(gm, c, origin, font, size, color);
//}
//
//// draws a single character from its font origin point
//void GUI_CharFont_NoGuard_(GUIManager* gm, int c, Vector2 origin, GUIFont* font, float size, Color4* color) {
//	
//	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
//	struct charInfo* ci = &font->regular[c];
//
//	v->pos.t = origin.y + ci->topLeftOffset.y * size;
//	v->pos.l = origin.x + ci->topLeftOffset.x * size;
//	v->pos.b = origin.y + ci->bottomRightOffset.y * size;
//	v->pos.r = origin.x + ci->bottomRightOffset.x * size;
//			
//	v->guiType = font->bitmapSize > 0 ? 20 : 1; // 1 = sdf, 20 = bitmap
//				
//	v->texOffset1.x = ci->texNormOffset.x * 65535.0;
//	v->texOffset1.y = ci->texNormOffset.y * 65535.0;
//	v->texSize1.x =  ci->texNormSize.x *  65535.0;
//	v->texSize1.y =  ci->texNormSize.y * 65535.0;
//	v->texIndex1 = ci->texIndex;
//			
//	v->clip = GUI_AABB2_TO_SHADER(gm->curClip);
//	v->bg = GUI_COLOR4_TO_SHADER(*color);
//	v->fg = GUI_COLOR4_TO_SHADER(*color);
//	v->z = gm->curZ;
//	v->rot = 0;
//}


// no wrapping
//void GUI_TextLine_(
//	GUIManager* gm, 
//	char* text, 
//	size_t textLen, 
//	Vector2 tl, 
//	char* fontName, 
//	float size, 
//	Color4* color
//) {
//	if(!gm->drawMode) return; // this function is only for drawing mode
//	
//	GUIFont* font = GUI_FindFont(gm, fontName);
//	if(!font) font = gm->defaults.font;
//	
//	if(textLen == 0) textLen = strlen(text);
//	
//	gui_drawTextLineAdv(gm, 
//		tl, (Vector2){99999999,99999999},
//		&gm->curClip,
//		color,
//		font, size,
//		GUI_TEXT_ALIGN_LEFT,
//		gm->curZ,
//		text, textLen
//	);
//}


//
//// no wrapping
//void GUI_Printf_(
//	GUIManager* gm,  
//	Vector2 tl, 
//	char* fontName, 
//	float size, 
//	Color4* color,
//	char* fmt,
//	...
//) {
//	va_list ap;
//	
//	if(!gm->drawMode) return; // this function is only for drawing mode
//	
//	GUIFont* font = GUI_FindFont(gm, fontName);
//	if(!font) font = gm->defaults.font;
//	
//	va_start(ap, fmt);
//	int sz = vsnprintf(NULL, 0, fmt, ap) + 1;
//	va_end(ap);
//	
//	char* tmp = malloc(sz);
//	va_start(ap, fmt);
//	vsnprintf(tmp, sz, fmt, ap);
//	va_end(ap);
//	
//	GUI_TextLine_(gm, tmp, sz, tl, fontName, size, color);
//	
//	free(tmp);
//}
//
//// no wrapping
//void GUI_TextLineCentered_(
//	GUIManager* gm, 
//	char* text, 
//	size_t textLen, 
//	Vector2 tl, 
//	Vector2 sz, 
//	char* fontName, 
//	float size, 
//	Color4* color
//) {
//	if(!gm->drawMode) return; // this function is only for drawing mode
//	
//	GUIFont* font = GUI_FindFont(gm, fontName);
//	if(!font) font = gm->defaults.font;
//	
//	if(textLen == 0) textLen = strlen(text);
//	
//	float b = (sz.y - (font->ascender * size)) / 2;
//	
//	gui_drawTextLineAdv(gm, (Vector2){tl.x, tl.y + b}, sz, &gm->curClip, color, font, size, GUI_TEXT_ALIGN_CENTER, gm->curZ, text, textLen);
//}
//
//

//void GUI_Double_(GUIManager* gm, double d, int precision, Vector2 tl, char* fontName, float size, Color4* color) {
//	char buf[64];
//	int n = snprintf(buf, 64, "%.*f", precision, d);
//	GUI_TextLine(buf, n, tl, fontName, size, color);
//}
//
//void GUI_Integer_(GUIManager* gm, int64_t i, Vector2 tl, char* fontName, float size, Color4* color) {
//	char buf[64];
//	int n = snprintf(buf, 64, "%ld", i);
//	GUI_TextLine(buf, n, tl, fontName, size, color);
//}
//
//
Vector2 GUI_MousePos_(GUIManager* gm) {
	return V(gm->lastMousePos.x - gm->curWin->absClip.min.x, gm->lastMousePos.y - gm->curWin->absClip.min.y);
}

// only valid when the mouse is dragging
Vector2 GUI_DragStartPos_(GUIManager* gm, int button) {
	return vSub2(gm->mouseDragStartPos[button], V(gm->curWin->absClip.min.x, gm->curWin->absClip.min.y));
}

// may output garbage on non-mouse events
Vector2 GUI_EventPos_(GUIManager* gm) {
	return vSub2(gm->curEvent.pos, V(gm->curWin->absClip.min.x, gm->curWin->absClip.min.y));
}

int GUI_PointInBoxVABS(Vector2 tl, Vector2 size, Vector2 testPos) {
	
	if(!(testPos.x >= tl.x && 
		testPos.y >= tl.y &&
		testPos.x <= (tl.x + size.x) && 
		testPos.y <= (tl.y + size.y))) {
		
		return 0;
	}
	
	return 1;
}

int GUI_PointInBoxV_(GUIManager* gm, Vector2 tl, Vector2 size, Vector2 testPos) {
	tl.x += gm->curWin->absClip.min.x;
	tl.y += gm->curWin->absClip.min.y;
	
	if(!(testPos.x >= tl.x && 
		testPos.y >= tl.y &&
		testPos.x <= (tl.x + size.x) && 
		testPos.y <= (tl.y + size.y))) {
		
		return 0;
	}
	
	return 1;
}

int GUI_PointInBox_(GUIManager* gm, AABB2 box, Vector2 testPos) {
	if(!boxContainsPoint2p(&box, &testPos)) return 0;
	if(!boxContainsPoint2p(&gm->curClip, &testPos)) return 0;
	
	return 1;
}


int GUI_InputAvailable_(GUIManager*gm) {
	return !gm->curEvent.cancelled;
}

void GUI_CancelInput_(GUIManager*gm) {
	gm->curEvent.cancelled = 1;
}


int GUI_MouseInside_(GUIManager* gm, Vector2 tl, Vector2 sz) {
	return gm->curWin->id == gm->mouseWinID && GUI_PointInBoxV_(gm, tl, sz, gm->lastMousePos);
}

int GUI_MouseWentUp_(GUIManager* gm, int button) {
	return gm->curWin->id == gm->mouseWinID && gm->curEvent.type == GUIEVENT_MouseUp && gm->curEvent.button == button;
}
int GUI_MouseWentDown_(GUIManager* gm, int button) {
	return gm->curWin->id == gm->mouseWinID && gm->curEvent.type == GUIEVENT_MouseDown && gm->curEvent.button == button;
}
int GUI_MouseWentUpAnywhere_(GUIManager* gm, int button) {
	return gm->curEvent.type == GUIEVENT_MouseUp && gm->curEvent.button == button;
}
int GUI_MouseWentDownAnywhere_(GUIManager* gm, int button) {
	return gm->curEvent.type == GUIEVENT_MouseDown && gm->curEvent.button == button;
}

float GUI_GetScrollDist_(GUIManager* gm) {
	return gm->curWin->id == gm->mouseWinID ? gm->scrollDist : 0;
}



