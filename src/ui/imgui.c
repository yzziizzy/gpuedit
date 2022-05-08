#include <ctype.h>

#include "gui.h"
#include "gui_internal.h"





#define C(r,g,b)  (&((Color4){r,g,b,1.0}))
#define C4(r,g,b,a)  (&((Color4){r,g,b,a}))
#define V(_x,_y) ((Vector2){(_x),(_y)})

#define HOT(id) GUI_SetHot_(gm, id, NULL, NULL)
#define ACTIVE(id) GUI_SetActive_(gm, id, NULL, NULL)

#define CLAMP(min, val, max) val = MIN(MAX(min, val), max)

// returns true if clicked
int GUI_Button_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, char* text) {
	
	int result = 0;
	
	if(GUI_MouseInside(tl, sz)) {
		HOT(id);
	}
	
	if(gm->activeID == id) {
		if(GUI_MouseWentUp(1)) {
			if(gm->hotID == id) result = 1;
			ACTIVE(NULL);
		}
	}
	else if(gm->hotID == id) {
		if(GUI_MouseWentDown(1)) ACTIVE(id);
	}

	
	if(gm->hotID == id) {
		GUI_BoxFilled_(gm, tl, sz, 2, C(.8,.6,.3), C(.7,.4,.2));
	}
	else {
		GUI_BoxFilled_(gm, tl, sz, 2, C(.5,.6,.6), C(.4,.4,.4));
	}
	
	float fontSz = gm->fontSize;
	if(sz.y - (2*2) < fontSz) fontSz = sz.y - (2*2);
	
	gm->curZ += 0.01;
	GUI_TextLineCentered_(gm, text, strlen(text), (Vector2){tl.x, tl.y - fontSz*.25}, sz, "Arial", fontSz, C(.8,.8,.8));
	gm->curZ -= 0.01;
	
	return result;
}


// returns true if toggled on 
int GUI_ToggleButton_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, char* text, int* state) {
	
	if(GUI_MouseInside(tl, sz)) {
		HOT(id);
	}
	
	if(gm->activeID == id) {
		if(GUI_MouseWentUp(1)) {
			if(gm->hotID == id) {
				*state = !*state;
			}
			ACTIVE(NULL);
		}
	}
	else if(gm->hotID == id) {
		if(GUI_MouseWentDown(1)) ACTIVE(id);
	}

	
	if(gm->hotID == id || *state) {
		GUI_BoxFilled_(gm, tl, sz, 2, C(.8,.6,.3), C(.7,.4,.2));
	}
	else {
		GUI_BoxFilled_(gm, tl, sz, 2, C(.5,.6,.6), C(.4,.4,.4));
	}
	
	float fontSz = gm->fontSize;
	if(sz.y - (2*2) < fontSz) fontSz = sz.y - (2*2);
	
	gm->curZ += 0.01;
	GUI_TextLineCentered_(gm, text, strlen(text), (Vector2){tl.x, tl.y - fontSz*.25}, sz, "Arial", fontSz, C(.8,.8,.8));
	gm->curZ -= 0.01;
	
	return *state;
}


// returns true if checked
int GUI_Checkbox_(GUIManager* gm, void* id, Vector2 tl, char* label, int* state) {
	
	float bs = gm->defaults.checkboxBoxSize;
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
		}
	}
	else if(gm->hotID == id) {
		if(GUI_MouseWentDown(1)) ACTIVE(id);
	}

	
	if(gm->hotID == id || *state) {
		GUI_BoxFilled_(gm, tl, boxSz, 2, C(.8,.6,.3), C(.7,.4,.2));
	}
	else {
		GUI_BoxFilled_(gm, tl, boxSz, 2, C(.5,.6,.6), C(.4,.4,.4));
	}
	
	float fontSz = gm->fontSize;
//	if(sz.y - (2*2) < fontSz) fontSz = sz.y - (2*2);
	
	GUI_TextLine_(gm, label, strlen(label), (Vector2){tl.x + bs + 4, tl.y + fontSz*.75}, "Arial", fontSz, C(.8,.8,.8));
	
	return *state;
}

// returns true if *this* radio button is active
int GUI_RadioButton_(GUIManager* gm, void* id, Vector2 tl, char* label, void** state, int isDefault) {
	
	float bs = gm->defaults.checkboxBoxSize;
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
		}
	}
	else if(gm->hotID == id) {
		if(GUI_MouseWentDown(1)) ACTIVE(id);
	}

	
	if(gm->hotID == id || *state == id) {
		GUI_CircleFilled_(gm, tl, bs, 2, C(.8,.6,.3), C(.7,.4,.2));
	}
	else {
		GUI_CircleFilled_(gm, tl, bs, 2, C(.5,.6,.6), C(.4,.4,.4));
	}
	
	float fontSz = gm->fontSize;
//	if(sz.y - (2*2) < fontSz) fontSz = sz.y - (2*2);
	
	GUI_TextLine_(gm, label, strlen(label), (Vector2){tl.x + bs + 4, tl.y + fontSz*.75}, "Arial", fontSz, C(.8,.8,.8));
	
	return *state == id;
}


struct floatslider_data {
	char buf[64];
	size_t len;
	float last_value;
};

// returns 1 on change
int GUI_FloatSlider_(GUIManager* gm, void* id, Vector2 tl, float width, float min, float max, float incr, int prec, float* value) {
	struct floatslider_data* d;
	int first_run = 0;
	
	if(!(d = GUI_GetData_(gm, id))) {
		d = calloc(1, sizeof(*d));
		d->buf[0] = 0;
		first_run = 1;
		GUI_SetData_(gm, id, d, free);
	}
	
	float h = gm->defaults.sliderHeight;
	Vector2 sz = {width, h};
	float oldV = *value;
	
	if(GUI_MouseInside(tl, sz)) HOT(id);
	
	if(gm->activeID == id) {
		Vector2 mp = GUI_MousePos();
		
		float v = mp.x - tl.x;
		v = (v / width);
		v = v < 0 ? 0 : (v > 1.0 ? 1.0 : v);
		v *= max - min;
		v += min;
		
		*value = v; 

		if(GUI_MouseWentUp(1)) {
			ACTIVE(NULL);
		}
	}
	if(gm->hotID == id) {
		if(GUI_MouseWentDown(1)) ACTIVE(id);
		*value += GUI_GetScrollDist() * incr;
	}
	
	*value = *value > max ? max : (*value < min ? min : *value);
	float bw = (*value / (max - min)) * width;
	
	if(first_run || *value != d->last_value) {
		d->len = snprintf(d->buf, 64, "%.*f", prec, *value);
		d->last_value = *value;
	}
	
	GUI_BoxFilled_(gm, V(tl.x, tl.y), V(bw, h), 0, C(.9,.6,.6), C(.9,.4,.4));
	GUI_BoxFilled_(gm, V(tl.x + bw, tl.y), V(width - bw, h), 0, C(.5,.6,.9), C(.4,.4,.9));

	GUI_TextLineCentered_(gm, d->buf, d->len, tl, sz, "Arial", gm->defaults.sliderFontSz, C(.8,.8,.8));

	return *value != oldV;
}

struct intslider_data {
	char buf[64];
	size_t len;
	long last_value;
};

// returns 1 on change
int GUI_IntSlider_(GUIManager* gm, void* id, Vector2 tl, float width, long min, long max, long* value) {
	struct intslider_data* d;
	int first_run = 0;
	
	if(!(d = GUI_GetData_(gm, id))) {
		d = calloc(1, sizeof(*d));
		d->buf[0] = 0;
		first_run = 1;
		GUI_SetData_(gm, id, d, free);
	}
	
	
	float h = gm->defaults.sliderHeight;
	Vector2 sz = {width, h};
	long oldV = *value;
	
	if(GUI_MouseInside(tl, sz)) HOT(id);
	
	if(gm->activeID == id) {
		Vector2 mp = GUI_MousePos();
		
		float v = mp.x - tl.x;
		v = (v / width);
		v = v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
		v *= max - min;
		v += min;
		*value = floor(v); 

		if(GUI_MouseWentUp(1)) {
			ACTIVE(NULL);
		}
	}
	if(gm->hotID == id) {
		if(GUI_MouseWentDown(1)) ACTIVE(id);
		*value += GUI_GetScrollDist();
	}

	*value = *value > max ? max : (*value < min ? min : *value);
	float bw = ((float)*value / (float)(max - min)) * width;
	
	
	if(first_run || *value != d->last_value) {
		d->len = snprintf(d->buf, 64, "%ld", *value);
		d->last_value = *value;		
	}
	
	GUI_BoxFilled_(gm, V(tl.x, tl.y), V(bw, h), 0, C(.9,.6,.6), C(.9,.4,.4));
	GUI_BoxFilled_(gm, V(tl.x + bw, tl.y), V(width - bw, h), 0, C(.5,.6,.9), C(.4,.4,.9));

	GUI_TextLineCentered_(gm, d->buf, d->len, tl, sz, "Arial", gm->defaults.sliderFontSz, C(.8,.8,.8));

	return *value != oldV;
}


// returns 1 when the value changes _due to this control_
int GUI_OptionSlider_(GUIManager* gm, void* id, Vector2 tl, float width, char** options, int* selectedOption) {
	int ret = 0;
	float h = gm->defaults.sliderHeight;
	Vector2 sz = {width, h};
	int old_opt = *selectedOption; 
	
	int cnt = 0;
	for(char** p = options; *p; p++) cnt++;
	
	if(GUI_MouseInside(tl, sz)) {
		HOT(id);
	}
	
	if(gm->activeID == id) {
		Vector2 mp = GUI_MousePos();
		
		float v = mp.x - tl.x;
		v = (v / width);
		v = v < 0 ? 0 : (v > 1.0 ? 1.0 : v);
		*selectedOption = cnt * v;

		if(GUI_MouseWentUp(1)) {
			ACTIVE(NULL);
		}
	}
	
	if(gm->hotID == id) {
		if(GUI_MouseWentDown(1)) ACTIVE(id);
		*selectedOption += GUI_GetScrollDist();
	}
	
	CLAMP(0, *selectedOption, cnt - 1);
	
	
	float bw = ((float)(*selectedOption + 1) / (float)cnt) * width;
	
	GUI_BoxFilled_(gm, V(tl.x, tl.y), V(bw, h), 0, C(.9,.6,.6), C(.9,.4,.4));
	GUI_BoxFilled_(gm, V(tl.x + bw, tl.y), V(width - bw, h), 0, C(.5,.6,.9), C(.4,.4,.9));

	GUI_TextLineCentered_(gm, options[*selectedOption], -1, tl, sz, "Arial", gm->defaults.sliderFontSz, C(.8,.8,.8));

	return old_opt != *selectedOption;
}


// returns 1 when the value changes
int GUI_SelectBox_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, char** options, int* selectedOption) {
	int ret = 0;
	
	int optsLen = 0;
	for(char**p = options; *p != NULL; p++) optsLen++;
	
	if(GUI_MouseInside(tl, sz)) {
		HOT(id);
	}
	
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
		else if(GUI_MouseWentDown(1)) {
			ACTIVE(NULL);
		}
	}
	else if(gm->hotID == id) {
		if(GUI_MouseWentDown(1)) ACTIVE(id);
		*selectedOption -= GUI_GetScrollDist();
	}

	
	if(gm->hotID == id) {
		GUI_BoxFilled_(gm, tl, sz, 2, C(.8,.6,.3), C(.7,.4,.2));
	}
	else {
		GUI_BoxFilled_(gm, tl, sz, 2, C(.5,.6,.6), C(.4,.4,.4));
	}
	
	float fontSz = gm->fontSize;
	if(sz.y - (2*2) < fontSz) fontSz = sz.y - (2*2);

	CLAMP(0, *selectedOption, optsLen - 1);
	
	if(gm->activeID == id) {
		// draw the dropdown
		
		GUI_BeginWindow(id, V(tl.x + 3, tl.y + sz.y), V(tl.x, sz.y * 3), gm->curZ + 5, 0);
		
		gm->curZ += 0.01;
		int cnt = 0;
		for(char** p = options; *p; p++) {
			
			gm->curZ += 0.02;
			GUI_TextLineCentered_(gm, *p, strlen(*p), V(0, sz.y * cnt + fontSz*.25 - 6), sz, "Arial", fontSz, C(.8,.8,.8));
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
	GUI_TextLineCentered_(gm, options[*selectedOption], strlen(options[*selectedOption]), (Vector2){tl.x, tl.y - fontSz*.25}, sz, "Arial", fontSz, C(.8,.8,.8));
	gm->curZ -= 0.01;
	
	return ret;
} 



struct cursor_data {
	int cursorPos;
	int selectPivot;
	float blinkTimer;
};

struct edit_data {
	struct cursor_data cursor;
	float scrollX; // x offset to add to all text rendering code 
	char synthed_change; // indicates that the buffer was changed externally
	
	GUIEditFilterFn filter_fn;
	void* filter_user_data;
};


static void check_string(GUIString* s, int extra) {
	if(s->alloc <= s->len + extra) {
		s->alloc = nextPOT(s->len + extra + 1);
		s->data = realloc(s->data, s->alloc * sizeof(*s->data));
	}
}

static void delete_selection(struct cursor_data* cd, GUIString* str) {
	int start = MIN(cd->selectPivot, cd->cursorPos);
	int end = MAX(cd->selectPivot, cd->cursorPos);
	int len = end - start;
	
	memmove(str->data + start, str->data + end, str->len - len + 1);
	str->len -= len;
	cd->cursorPos = start;
	cd->selectPivot = -1;
}

// return 1 on changes
static int handle_cursor(GUIManager* gm, struct cursor_data* cd, GUIString* str, GUIKeyEvent* e) {
	int ret = 0;


	switch(e->keycode) {
		case XK_Left:
			if(e->modifiers & GUIMODKEY_SHIFT && cd->selectPivot == -1) { // start selection
				if(cd->cursorPos > 0) { // cant's start a selection leftward from the left edge
					cd->selectPivot = cd->cursorPos;
					cd->cursorPos--;
				}
			}
			else { // just move the cursor normally
				cd->cursorPos = cd->cursorPos - 1 <= 0 ? 0 : cd->cursorPos - 1;
				if(!(e->modifiers & GUIMODKEY_SHIFT)) cd->selectPivot = -1;
			}
			goto BLINK;
							
		case XK_Right: 
			if(e->modifiers & GUIMODKEY_SHIFT && cd->selectPivot == -1) { // start selection
				if(cd->cursorPos < str->len) { // cant's start a selection leftward from the left edge
					cd->selectPivot = cd->cursorPos;
					cd->cursorPos++;
				}
			}
			else { // just move the cursor normally
				cd->cursorPos = cd->cursorPos + 1 > str->len ? str->len : cd->cursorPos + 1;
				if(!(e->modifiers & GUIMODKEY_SHIFT)) cd->selectPivot = -1;
			}
			goto BLINK;
			
		case XK_BackSpace:
			if(cd->selectPivot > -1) { // delete the selection
				delete_selection(cd, str);
			}
			else if(cd->cursorPos > 0) { // just one character
				memmove(str->data + cd->cursorPos - 1, str->data + cd->cursorPos, str->len - cd->cursorPos + 1);
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
				memmove(str->data + cd->cursorPos, str->data + cd->cursorPos + 1, str->len - cd->cursorPos);
				str->len--;
				cd->selectPivot = -1;
				ret = 1;
			}
			
			goto BLINK;
							
//			case XK_Return: ACTIVE(NULL); break;
	}
	
	return ret;

BLINK:
	cd->blinkTimer = 0;		
	
	return ret;
}

// returns 1 on changes made to the buffer
static int handle_input(GUIManager* gm, struct cursor_data* cd, GUIString* str, GUIKeyEvent* e) {
	int ret = 0;
	
	cd->cursorPos = MIN(cd->cursorPos, str->len);
	cd->selectPivot = MIN(cd->selectPivot, str->len);
	
	if(isprint(e->character)) {
	
		if(cd->selectPivot > -1) {
			delete_selection(cd, str);
		}
		
		check_string(str, 1);
		memmove(str->data + cd->cursorPos + 1, str->data + cd->cursorPos, str->len - cd->cursorPos + 1);
		str->data[cd->cursorPos] = e->character;
		str->len++;
		cd->cursorPos++;
		cd->blinkTimer = 0;
		
		ret = 1;
	}
	else {
		ret |= handle_cursor(gm, cd, str, e);
	}
	
	return ret;
}


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
	
	GUIKeyEvent e = {
		.type = GUIEVENT_KeyUp,
		.character = c,
		.keycode = 0,
		.modifiers = 0,
	};
	
	if(d->filter_fn && isprint(c)) {
		if(!d->filter_fn(str, &e, d->cursor.cursorPos, d->filter_user_data)) return 0;
	}
	
	int ret = handle_input(gm, &d->cursor, str, &e);
	d->synthed_change |= ret;
	
	return ret;
}

// returns true on a change
int GUI_Edit_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, GUIString* str) {
	int ret;
	struct edit_data* d;
	
	if(!(d = GUI_GetData_(gm, id))) {
		d = calloc(1, sizeof(*d));
		d->cursor.selectPivot = -1;
		GUI_SetData_(gm, id, d, free);
	}
	
	ret = d->synthed_change;
	d->synthed_change = 0;
	
	
	float fontSz = gm->fontSize;
	if(sz.y - (2*2) < fontSz) fontSz = sz.y - (2*2);
	
	GUIFont* font = GUI_FindFont(gm, "Arial");
	if(!font) font = gm->defaults.font;
	
	
	if(GUI_MouseInside(tl, sz)) {
		HOT(id);
	}
	if(gm->hotID == id) {
		if(GUI_MouseWentDown(1)) ACTIVE(id);
		
		if(GUI_MouseWentUp(1)) {
			// position the cursor
			Vector2 mp = GUI_MousePos();
			d->cursor.cursorPos = gui_charFromPixel(gm, font, fontSz, str->data, mp.x - tl.x);
		}
	}
	
	
	// draw the border
	if(gm->activeID == id) {
		GUI_BoxFilled_(gm, tl, sz, 2, C(.8,.6,.3), C(.7,.4,.2));
	}
	else {
		GUI_BoxFilled_(gm, tl, sz, 2, C(.5,.6,.6), C(.4,.4,.4));
	}
	


	// handle input
	if(gm->activeID == id) {
		VEC_EACHP(&gm->keysReleased, i, e) {
			if(d->filter_fn && isprint(e->character)) {
				if(!d->filter_fn(str, e, d->cursor.cursorPos, d->filter_user_data)) continue;
			}
			ret |= handle_input(gm, &d->cursor, str, e);
		}
	}
	
	
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
			
			GUI_BoxFilled_(gm, V(tl.x + cursorOff + d->scrollX, tl.y), V(2,sz.y), 0, C(.8,1,.3), C(.8,1,.3));
			gm->curZ -= 0.001;
		}
		
		d->cursor.blinkTimer += gm->timeElapsed;
		d->cursor.blinkTimer = fmod(d->cursor.blinkTimer, 1.0);
		
		if(d->cursor.selectPivot > -1) {
			float pivotOff = gui_getTextLineWidth(gm, font, fontSz, str->data, d->cursor.selectPivot);
			
			int min = MIN(cursorOff, pivotOff);
			int max = MAX(cursorOff, pivotOff);
			
			
			GUI_BoxFilled_(gm, V(tl.x + min + d->scrollX, tl.y), V(max - min,sz.y), 0, C(.3,1,.7), C(.3,1,.7));
			
		}
		gm->curZ -= 0.001;
	}
	

	
	gm->curZ += 0.01;
	
	GUI_TextLine_(gm, str->data, str->len, V(tl.x + d->scrollX, tl.y +5+ fontSz * .75), "Arial", fontSz, &((Color4){.9,.9,.9,1}));
	GUI_PopClip();
	gm->curZ -= 0.01;
	
	return ret;
}




struct intedit_data {
	int cursorPos;
	float blinkTimer;
	
	long lastValue;
	char buffer[64];
};


// returns true on a change
int GUI_IntEdit_(GUIManager* gm, void* id, Vector2 tl, Vector2 sz, long* num) {
	int ret = 0;
	int firstRun = 0;
	struct intedit_data* d;
	
	if(!(d = GUI_GetData_(gm, id))) {
		d = calloc(1, sizeof(*d));
		GUI_SetData_(gm, id, d, free);
		firstRun = 1;
	}
	
	
	if(GUI_MouseInside(tl, sz)) {
		HOT(id);
	}
	
	if(gm->hotID == id) {
		if(GUI_MouseWentDown(1)) {
			ACTIVE(id);
		}
	}
	
	if(gm->activeID == id) {
		GUI_BoxFilled_(gm, tl, sz, 2, C(.8,.6,.3), C(.7,.4,.2));
	}
	else {
		GUI_BoxFilled_(gm, tl, sz, 2, C(.5,.6,.6), C(.4,.4,.4));
	}
	
	
	float fontSz = gm->fontSize;
	if(sz.y - (2*2) < fontSz) fontSz = sz.y - (2*2);
	
	GUIFont* font = GUI_FindFont(gm, "Arial");
	if(!font) font = gm->defaults.font;
	

	// handle input
	if(gm->activeID == id) {
		VEC_EACHP(&gm->keysReleased, i, e) {
			switch(e->character) {
				case '0': case '1': case '2':
				case '3': case '4': case '5':
				case '6': case '7': case '8': case '9':
					memmove(d->buffer + d->cursorPos + 1, d->buffer + d->cursorPos, strlen(d->buffer) - d->cursorPos + 1);
					d->buffer[d->cursorPos] = e->character;
					d->cursorPos++;
					*num = strtol(d->buffer, NULL, 10);
					d->blinkTimer = 0;
					
					ret = 1;
					break;
			}
			
			switch(e->keycode) {
				case XK_Left: 
					d->cursorPos = d->cursorPos < 1 ? 0 : d->cursorPos - 1; 
					d->blinkTimer = 0;
					break;
				case XK_Right: 
					d->cursorPos = d->cursorPos + 1 > strlen(d->buffer) ? strlen(d->buffer) : d->cursorPos + 1; 
					d->blinkTimer = 0;
					break;
					
				case XK_BackSpace: 
					memmove(d->buffer + d->cursorPos - 1, d->buffer + d->cursorPos, strlen(d->buffer) - d->cursorPos + 1);
					d->cursorPos = d->cursorPos > 0 ? d->cursorPos - 1 : 0;
					*num = strtol(d->buffer, NULL, 10);
					d->blinkTimer = 0;
					
					ret = 1;
					break;
					
				case XK_Delete: 
					memmove(d->buffer + d->cursorPos, d->buffer + d->cursorPos + 1, strlen(d->buffer) - d->cursorPos);
					*num = strtol(d->buffer, NULL, 10);
					d->blinkTimer = 0;
					
					ret = 1;
					break;
					
				case XK_Return: ACTIVE(NULL); break;
			}
		}
	}
	
	

	// refresh the buffer, maybe
	if(*num != d->lastValue || firstRun) {
		snprintf(d->buffer, 64, "%ld", *num);
		d->lastValue = *num;
	}
	
	// draw cursor
	if(gm->activeID == id) {
		if(d->blinkTimer < 0.5) { 
			float cursorOff = gui_getTextLineWidth(gm, font, fontSz, d->buffer, d->cursorPos);
			GUI_BoxFilled_(gm, V(tl.x + cursorOff , tl.y), V(2,sz.y), 0, C(.8,1,.3), C(.8,1,.3));
		}
		
		d->blinkTimer += gm->timeElapsed;
		d->blinkTimer = fmod(d->blinkTimer, 1.0);
	}
	
	gm->curZ += 0.01;
	GUI_TextLine_(gm, d->buffer, strlen(d->buffer), V(tl.x, tl.y +5+ fontSz * .75), "Arial", fontSz, &((Color4){.9,.9,.9,1}));
	gm->curZ -= 0.01;
	
	return ret;
}


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
	
	w->id = id;
	w->z = z;
	w->flags = flags;
	
	// TODO: calculate the proper visual clipping region
	w->absClip.min.x = p->absClip.min.x + tl.x;
	w->absClip.min.y = p->absClip.min.y + tl.y;
	w->absClip.max.x = w->absClip.max.x + sz.x;
	w->absClip.max.y = w->absClip.max.y + sz.y;
	
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
	
	VEC_POP1(&gm->windowStack);
	gm->curWin = VEC_TAIL(&gm->windowStack);
	
	VEC_POP(&gm->clipStack, gm->curClip);	
}



void GUI_Box_(GUIManager* gm, Vector2 tl, Vector2 sz, float width, Color4* borderColor) {
	Color4 clear = {0,0,0,0};
	gui_drawBoxBorder(gm, tl, sz, &gm->curClip, gm->curZ, &clear, width, borderColor);
}

void GUI_BoxFilled_(
	GUIManager* gm, 
	Vector2 tl, 
	Vector2 sz, 
	float width, 
	Color4* borderColor, 
	Color4* bgColor
) {
	gui_drawBoxBorder(gm, tl, sz, &gm->curClip, gm->curZ, bgColor, width, borderColor);
}

void GUI_CircleFilled_(
	GUIManager* gm, 
	Vector2 center, 
	float radius, 
	float borderWidth, 
	Color4* borderColor, 
	Color4* bgColor
) {
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

// no wrapping
void GUI_TextLine_(
	GUIManager* gm, 
	char* text, 
	size_t textLen, 
	Vector2 tl, 
	char* fontName, 
	float size, 
	Color4* color
) {
	GUIFont* font = GUI_FindFont(gm, fontName);
	if(!font) font = gm->defaults.font;
	
	gui_drawTextLineAdv(gm, 
		tl, (Vector2){99999999,99999999},
		&gm->curClip,
		color,
		font, size,
		GUI_TEXT_ALIGN_LEFT,
		gm->curZ,
		text, textLen
	);
}

// no wrapping
void GUI_TextLineCentered_(
	GUIManager* gm, 
	char* text, 
	size_t textLen, 
	Vector2 tl, 
	Vector2 sz, 
	char* fontName, 
	float size, 
	Color4* color
) {
	GUIFont* font = GUI_FindFont(gm, fontName);
	if(!font) font = gm->defaults.font;
	
	
	float b = sz.y - ((sz.y - size) / 2);
	
	
	gui_drawTextLineAdv(gm, (Vector2){tl.x, tl.y + b}, sz, &gm->curClip, color, font, size, GUI_TEXT_ALIGN_CENTER, gm->curZ, text, textLen);
}


Vector2 GUI_MousePos_(GUIManager* gm) {
	return V(gm->lastMousePos.x - gm->curWin->absClip.min.x, gm->lastMousePos.y - gm->curWin->absClip.min.y);
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

	
int GUI_MouseInside_(GUIManager* gm, Vector2 tl, Vector2 sz) {
	return gm->curWin->id == gm->mouseWinID && GUI_PointInBoxV_(gm, tl, sz, gm->lastMousePos);
}

int GUI_MouseWentUp_(GUIManager* gm, int button) {
	return gm->curWin->id == gm->mouseWinID && gm->mouseWentUp[button];
}
int GUI_MouseWentDown_(GUIManager* gm, int button) {
	return gm->curWin->id == gm->mouseWinID && gm->mouseWentDown[button];
}
int GUI_MouseWentUpAnywhere_(GUIManager* gm, int button) {
	return gm->mouseWentUp[button];
}
int GUI_MouseWentDownAnywhere_(GUIManager* gm, int button) {
	return gm->mouseWentDown[button];
}

float GUI_GetScrollDist_(GUIManager* gm) {
	return gm->curWin->id == gm->mouseWinID ? gm->scrollDist : 0;
}



