#include <ctype.h>

#include "../gui.h"
#include "../gui_internal.h"




static void insertChar(GUIEdit* ed, char c);
static void updateTextControl(GUIEdit* ed);
static void fireOnchange(GUIEdit* ed);
static void fireOnEnter(GUIEdit* ed);


static void moveCursor(GUIEdit* w, int delta) {
	w->cursorpos = MIN(w->textlen + 1, MAX(0, w->cursorpos + delta));
	w->cursorOffset = gui_getDefaultUITextWidth(w->header.gm, w->buf, w->cursorpos);
}


static void render(GUIEdit* w, PassFrameParams* pfp) {
	
	GUIManager* gm = w->header.gm;
	Vector2 tl = w->header.absTopLeft;
	
	float textOffset = 0;
	float textWidth = gui_getDefaultUITextWidth(gm, w->buf, w->textlen);
	
	if(w->rightJustify) {
		// HACK hardcoded padding offset
		textOffset = w->header.size.x - textWidth - 4;
	}
	else if(w->centerJustify) {
		textOffset = (w->header.size.x - textWidth) / 2;
	}
	
	
	int cursorAlpha = 0;
	float cursorOff = 0;
	
	if(w->hasFocus && fmod(pfp->wallTime, 1.0) > .5) {
		cursorOff = textOffset + tl.x + (w->cursorOffset /** w->textControl->fontSize * .01*/);
		cursorAlpha = 255;
	}
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(w->header.gm, 2);
	
// 	printf("tl: %f,%f | %f,%f\n", tl.x, tl.y, w->header.size.x, w->header.size.y);
	
	// bg
	*v++ = (GUIUnifiedVertex){
		.pos = {tl.x, tl.y, tl.x + w->header.size.x, tl.y + w->header.size.y},
		.clip = GUI_AABB2_TO_SHADER(w->header.absClip),
		
		.guiType = 4, // bordered window 
		
		.texIndex1 = 1, .texIndex2 = 0, .texFade = 0,
		.texOffset1 = 0, .texOffset2 = 0, .texSize1 = 0, .texSize2 = 0,
		
		.fg = GUI_COLOR4_TO_SHADER(gm->defaults.editBorderColor),
		.bg = GUI_COLOR4_TO_SHADER(gm->defaults.editBgColor),
		
		.z = w->header.absZ + .1,
		.alpha = 1,
	};
	
	
	struct Color4 cc = gm->defaults.cursorColor;
	cc.a = 1.0; //cursorAlpha;
	
	// cursor
	*v = (GUIUnifiedVertex){
		.pos = {cursorOff, tl.y, cursorOff + 2, tl.y + w->header.size.y},
		.clip = GUI_AABB2_TO_SHADER(w->header.absClip),
		
		.guiType = 0, // window 
		
		.texIndex1 = 0, .texIndex2 = 0, .texFade = 0,
		.texOffset1 = 0, .texOffset2 = 0, .texSize1 = 0, .texSize2 = 0,
		
		.fg = GUI_COLOR4_TO_SHADER(cc), 
		.bg = GUI_COLOR4_TO_SHADER(cc), 
		
		.z = w->header.absZ + .25,
		.alpha = w->header.alpha,
	};
	
	AABB2 box;
	box.min.x = tl.x + textOffset;
	box.min.y = tl.y;
	box.max.x = 3000;
	box.max.y = tl.y + 30;
	
	gui_drawTextLine(w->header.gm, (Vector2){box.min.x, box.min.y}, (Vector2){3000,0}, &w->header.absClip, &gm->defaults.tabTextColor , w->header.absZ + .2, w->buf, w->textlen);
// 	printf("%s %f,%f\n", w->buf, tl.x, tl.y);
	
// 	GUIHeader_renderChildren(&w->header, pfp);
	
	

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


static void click(GUIObject* w_, GUIEvent* gev) {
	GUIEdit* w = (GUIEdit*)w_;
	GUIManager_pushFocusedObject(w->header.gm, w);
	
	// TODO: position cursor
}

static void gainedFocus(GUIObject* w_, GUIEvent* gev) {
	GUIEdit* w = (GUIEdit*)w_;
	w->hasFocus = 1;
}
static void lostFocus(GUIObject* w_, GUIEvent* gev) {
	GUIEdit* w = (GUIEdit*)w_;
	w->hasFocus = 0;
}


static void keyDown(GUIObject* w_, GUIEvent* gev) {
	GUIEdit* w = (GUIEdit*)w_;
	
	// NOTE: paste will be an event type
	
	
	if(gev->keycode == XK_Left) {
		moveCursor(w, -1);
		gev->cancelled = 1;
	}
	else if(gev->keycode == XK_Right) {
		moveCursor(w, 1);
		gev->cancelled = 1;
	}
	else if(gev->keycode == XK_Return) {
		fireOnEnter(w);
		gev->cancelled = 1;
	}
	else if(gev->keycode == XK_BackSpace) {
		removeChar(w, w->cursorpos - 1);
		moveCursor(w, -1);
		
		fireOnchange(w);
		
		gev->cancelled = 1;
	}
	else if(gev->keycode == XK_Delete) {
		removeChar(w, w->cursorpos);
		
		fireOnchange(w);
		
		gev->cancelled = 1;
	}
	else if(isprint(gev->character) && (gev->modifiers & (~(GUIMODKEY_SHIFT | GUIMODKEY_LSHIFT | GUIMODKEY_RSHIFT))) == 0) {
		insertChar(w, gev->character);
		w->cursorpos++;
		
		fireOnchange(w);
		
		gev->cancelled = 1;
	}
	
	updateTextControl(w);
	
	return 0;
}



GUIEdit* GUIEdit_New(GUIManager* gm, char* initialValue) {
	
	GUIEdit* w;
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.Delete = (void*)delete,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.KeyDown = keyDown,
		.Click = click,
		.GainedFocus = gainedFocus,
		.LostFocus = lostFocus,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
	};
	
	
	pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	
	w->header.size.x = gm->defaults.editWidth;
	w->header.size.y = gm->defaults.editHeight;
	
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
	
// 	w->textControl = GUIText_new(gm, initialValue, "Arial", 6.0f);
// 	w->textControl->header.z = 10000.5;
// 	GUIRegisterObject(&w->header, w->textControl);

// 	w->cursorOffset = guiTextGetTextWidth(w->textControl, w->cursorpos);
	w->cursorOffset = gui_getDefaultUITextWidth(gm, initialValue, 9999999);
	
	
	
// 	w->textControl->header.onClick = (GUI_OnClickFn)click;
	
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
// 	GUIText_setString(ed->textControl, ed->buf);
	
	// get new cursor pos
// 	ed->cursorOffset = guiTextGetTextWidth(ed->textControl, ed->cursorpos);
	ed->cursorOffset = gui_getDefaultUITextWidth(ed->header.gm, ed->buf, ed->cursorpos);
	
	//printf("cursorpos %f\n", ed->cursorOffset); 
	
	fireOnchange(ed);
}

// just changes the text value. triggers nothing.
static void setText(GUIEdit* ed, char* s) {
	int len = strlen(s);
	checkBuffer(ed, len);
	
	strcpy(ed->buf, s);
	ed->textlen = len;
	
	if(len < ed->cursorpos) {
		ed->cursorpos = len;
		ed->cursorOffset = gui_getDefaultUITextWidth(ed->header.gm, ed->buf, ed->cursorpos);
	}
	
}

static void fireOnchange(GUIEdit* ed) {
	
	GUIEvent gev = {};
	gev.type = GUIEVENT_User;
	gev.eventTime = 0;
	gev.originalTarget = ed;
	gev.currentTarget = ed;
	gev.cancelled = 0;
	gev.userType = "change";
	
	gev.userData = ed->buf;
	gev.userSize = ed->textlen;
	
	GUIManager_BubbleEvent(ed->header.gm, ed, &gev);
}


static void fireOnEnter(GUIEdit* ed) {
	
	GUIEvent gev = {};
	gev.type = GUIEVENT_User;
	gev.eventTime = 0;
	gev.originalTarget = ed;
	gev.currentTarget = ed;
	gev.cancelled = 0;
	gev.userType = "enter";
	
	gev.userData = ed->buf;
	gev.userSize = ed->textlen;
	
	GUIManager_BubbleEvent(ed->header.gm, ed, &gev);
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


char* GUIEdit_GetText(GUIEdit* ed) {
	return ed->buf;
}



