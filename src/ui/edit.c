

#include "../gui.h"
#include "../gui_internal.h"




static void insertChar(GUIEdit* ed, char c);
static void updateTextControl(GUIEdit* ed);
static void fireOnchange(GUIEdit* ed);


static void moveCursor(GUIEdit* w, int delta) {
	
	w->cursorpos = MIN(w->textlen, MAX(0, w->cursorpos + delta));
	w->cursorOffset = guiTextGetTextWidth(w->textControl, w->cursorpos);
}


static void render(GUIEdit* w, PassFrameParams* pfp) {
	
	Vector2 tl = w->header.absTopLeft;
	
	int cursorAlpha = 0;
	float cursorOff = 0;
	if(w->hasFocus && fmod(pfp->wallTime, 1.0) > .5) {
		cursorOff = tl.x + (w->cursorOffset /** w->textControl->fontSize * .01*/);
		cursorAlpha = 255;
	}
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(w->header.gm, 2);
	
	
	
	// bg
	*v++ = (GUIUnifiedVertex){
		.pos = {tl.x, tl.y, tl.x + w->header.size.x, tl.y + w->header.size.y},
		.clip = {0, 0, 800, 800},
		
		.guiType = 0, // window (just a box)
		
		.texIndex1 = 0, .texIndex2 = 0, .texFade = 0,
		.texOffset1 = 0, .texOffset2 = 0, .texSize1 = 0, .texSize2 = 0,
		
		.fg = {255, 128, 64, 255}, // TODO: border color
		.bg = {28, 28, 64, 255}, // TODO: color
		
		.z = w->header.z,
		.alpha = w->header.alpha,
	};
	// cursor
	*v = (GUIUnifiedVertex){
		.pos = {cursorOff, tl.y, cursorOff + 2, tl.y + w->header.size.y},
		.clip = {0, 0, 800, 800},
		
		.guiType = 0, // window (just a box)
		
		.texIndex1 = 0, .texIndex2 = 0, .texFade = 0,
		.texOffset1 = 0, .texOffset2 = 0, .texSize1 = 0, .texSize2 = 0,
		
		.fg = {255, 128, 64, 255}, // TODO: border color
		.bg = {255, 255, 255, cursorAlpha}, 
		
		.z = w->header.z + 2.5,
		.alpha = w->header.alpha,
	};
	
	
	GUIHeader_renderChildren(&w->header, pfp);
	
	

}

static void delete(GUIEdit* w) {
	free(w->buf);
}

void removeChar(GUIEdit* ed, int index) {
	if(index >= ed->textlen || index < 0) return;
	
	char* e = ed->buf + index;
	while(e <= ed->buf + ed->textlen + 1) {
		*e = *(e + 1);
		e++;
	}
	
	ed->textlen--;
}


void backspace(GUIEdit* ed) {
	if(ed->cursorpos <= 0) return;
	
	
}


static int recieveText(InputEvent* ev, GUIEdit* ed) {
	insertChar(ed, ev->character);
	ed->cursorpos++;
	updateTextControl(ed);
	
	return 0;
}

static int keyDown(InputEvent* ev, GUIEdit* w) {
	if(ev->keysym == XK_Left) moveCursor(w, -1);
	else if(ev->keysym == XK_Right) moveCursor(w, 1);
	else if(ev->keysym == XK_BackSpace) {
		removeChar(w, w->cursorpos - 1);
		moveCursor(w, -1);
	}
	else if(ev->keysym == XK_Delete) removeChar(w, w->cursorpos);
	else if(ev->keysym == XK_Escape) {
// 		GUIObject_revertFocus((GUIObject*)w);
		w->hasFocus = 0;
		return 0;
	};
	
	updateTextControl(w);
	
	return 0;
}

static int click(GUIEdit* w, Vector2 clickPos) {
	
// 	GUIObject_giveFocus(w->header.parent);
	((GUIEdit*)w->header.parent)->hasFocus = 1;
	
	return 0;
}




GUIEdit* GUIEdit_New(GUIManager* gm, char* initialValue, Vector2 size) {
	
	GUIEdit* w;
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.Delete = (void*)delete
	};
		
	static InputEventHandler input_vt = {
		.keyText = (void*)recieveText,
		.keyUp = (void*)keyDown,
	};
	
	w = calloc(1, sizeof(*w));
	CHECK_OOM(w);
	
	gui_headerInit(&w->header, gm, &static_vt, NULL);
// 	w->header.input_vt = &input_vt;
	w->header.size = size; 
	
	w->blinkRate = 1.5;
	
	if(initialValue) {
		w->textlen = strlen(initialValue);
		w->buflen = nextPOT(w->textlen + 1);
		w->buf = malloc(w->buflen);
		strcpy(w->buf, initialValue);
	}
	else {
		w->textlen = 0;
		w->buflen = 16;
		w->buf = malloc(16);
		w->buf[0] = 0;
	}
	
	w->cursorpos = w->textlen;
	
	w->textControl = GUIText_new(gm, initialValue, "Arial", 6.0f);
	w->textControl->header.size = w->header.size;
	w->textControl->header.z = 100.5;
	GUIRegisterObject(w->textControl, &w->header);

	w->cursorOffset = guiTextGetTextWidth(w->textControl, w->cursorpos);
	
	w->textControl->header.onClick = (GUI_OnClickFn)click;
	
	return w;
}


static void growBuffer(GUIEdit* ed, int extra) {
	ed->buflen = nextPOT(ed->textlen + extra + 1);
	ed->buf = realloc(ed->buf, ed->buflen);
}

static void checkBuffer(GUIEdit* ed, int minlen) {
	if(ed->buflen < minlen + 1) {
		growBuffer(ed, ed->buflen - minlen + 1);
	}
}

// at the cursor. does not move the cursor.
static void insertChar(GUIEdit* ed, char c) { 
	checkBuffer(ed, ed->textlen + 1);
	
	char* e = ed->buf + ed->textlen + 1; // copy the null terminator too
	while(e >= ed->buf + ed->cursorpos) {
		*e = *(e - 1);
		e--;
	}
	
	ed->textlen++;
	*(e+1) = c;
}

static void updateTextControl(GUIEdit* ed) {
	GUIText_setString(ed->textControl, ed->buf);
	
	// get new cursor pos
	ed->cursorOffset = guiTextGetTextWidth(ed->textControl, ed->cursorpos);
	//printf("cursorpos %f\n", ed->cursorOffset); 
	
	fireOnchange(ed);
}

// just changes the text value. triggers nothing.
static void setText(GUIEdit* ed, char* s) {
	int len = strlen(s);
	checkBuffer(ed, len);
	
	strcpy(ed->buf, s);
	
	// TODO: check and adjust cursor pos if text shrank
}

static void fireOnchange(GUIEdit* ed) {
	if(ed->onChange) {
		(*ed->onChange)(ed, ed->onChangeData);
	}
}



void GUIEdit_SetText(GUIEdit* ed, char* text) {
	setText(ed, text);
}




void GUIEdit_SetInt(GUIEdit* ed, int64_t ival) {
	fprintf(stderr, "FIXME: GUIEditSetInt\n");
	GUIEdit_SetDouble(ed, ival);
}

void GUIEdit_SetDouble(GUIEdit* ed, double dval) {
	char txtVal[64]; 
	
	ed->numVal = dval;
	
	gcvt(dval, 6, txtVal);
	GUIEdit_SetText(ed, txtVal);
}


static int updateDval(GUIEdit* ed) {
	char* end = NULL;
	
	double d = strtod(ed->buf, &end);
	if(ed->buf == end) { // conversion failed
		ed->numVal = 0.0;
		return 1;
	}
	
	ed->numVal = d;
	return 0;
}


double GUIEdit_GetDouble(GUIEdit* ed) {
	// TODO: cache this value
	updateDval(ed);
	return ed->numVal;
}



