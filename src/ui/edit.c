#include <ctype.h>

#include "gui.h"
#include "gui_internal.h"

// temp hack
#include "../clipboard.h"



static void insertChar(GUIEdit* ed, char c);
static void insertString(GUIEdit* w, int where, char* text, size_t len);
static void updateTextControl(GUIEdit* ed);
static void setText(GUIEdit* ed, char* s);
static void fireOnchange(GUIEdit* ed);
static void fireOnEnter(GUIEdit* ed);


static void setCursor(GUIEdit* w, int pos) {
	w->cursorpos = MIN(w->textlen + 1, MAX(0, pos));
	w->cursorOffset = gui_getDefaultUITextWidth(w->header.gm, w->buf, w->cursorpos);
}

static void moveCursor(GUIEdit* w, int delta) {
	setCursor(w, w->cursorpos + delta);
}



static void render(GUIEdit* w, PassFrameParams* pfp) {
	
	GUIManager* gm = w->header.gm;
	Vector2 tl = w->header.absTopLeft;
	GUIHeader* h = &w->header;
	
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
	
	w->blinkTimer += pfp->timeElapsed;
	
	if(w->hasFocus && w->blinkTimer < 0.7) {
		
		cursorOff = textOffset + tl.x + (w->cursorOffset /** w->textControl->fontSize * .01*/);
		cursorAlpha = 1;
	}
	
	w->blinkTimer = fmodf(w->blinkTimer, 1.4);
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(w->header.gm, 2);
	
// 	printf("tl: %f,%f | %f,%f\n", tl.x, tl.y, w->header.size.x, w->header.size.y);
	
	// bg
	gui_drawBoxBorder(gm, tl, h->size, &h->absClip, h->absZ, &gm->defaults.editBgColor, 1, &gm->defaults.editBorderColor);
	
	// selection background
	if(w->cursorpos != w->selEnd) {
		int mincp = MIN(w->cursorpos, w->selEnd);
		int maxcp = MAX(w->cursorpos, w->selEnd);
		
		// get text pixel offsets
		float mincpx = gui_getDefaultUITextWidth(gm, w->buf, mincp);
		float maxcpx = gui_getDefaultUITextWidth(gm, w->buf, maxcp);
		
		// TODO: find weird problem ith 2x voodoo offset
		gui_drawBox(gm, 
			(Vector2){tl.x + textOffset + mincpx, tl.y + 1},
			(Vector2){maxcpx - mincpx + 2, h->size.y - 2},
			&h->absClip, h->absZ + 0.1, 
			&gm->defaults.editSelBgColor
		);
	
	}
	
	struct Color4 cc = gm->defaults.cursorColor;
	cc.a = 1.0; //cursorAlpha;
	
	// cursor
	if(cursorAlpha > 0) {
		gui_drawBox(gm,
			(Vector2){cursorOff + textOffset, tl.y + 1},
			(Vector2){2, h->size.y - 2},
			&h->absClip, h->absZ + .25, 
			&cc
		);
	}
	
	gui_drawVCenteredTextLine(gm, 
		(Vector2){tl.x + textOffset, tl.y}, 
		h->size, &h->absClip, 
		&gm->defaults.editTextColor, 
		h->absZ + .2, 
		w->buf, w->textlen
	);
	
	
	

}


static void delete(GUIEdit* w) {
	free(w->buf);
}

static void removeChar(GUIEdit* ed, int index) {
	if(index >= ed->textlen || index < 0) return;
	
	char* e = ed->buf + index;
	while(e <= ed->buf + ed->textlen) {
		*e = *(e + 1);
		e++;
	}
	
	ed->textlen--;
}

static void removeRange(GUIEdit* w, int a, int b) {
	int min = MIN(a, b);
	int max = MAX(a, b);
	
	memmove(w->buf + min, w->buf + max, w->textlen + 1 - max);
	
	w->textlen -= max - min;
}



static int recieveText(InputEvent* ev, GUIEdit* ed) {
	insertChar(ed, ev->character);
	ed->cursorpos++;
	updateTextControl(ed);
	
	return 0;
}


static void click(GUIHeader* w_, GUIEvent* gev) {
//	GUIEdit* w = (GUIEdit*)w_;
	GUIManager_pushFocusedObject(w_->gm, w_);
	
	// TODO: position cursor
}

static void gainedFocus(GUIHeader* w_, GUIEvent* gev) {
	GUIEdit* w = (GUIEdit*)w_;
	w->hasFocus = 1;
}
static void lostFocus(GUIHeader* w_, GUIEvent* gev) {
	GUIEdit* w = (GUIEdit*)w_;
	w->hasFocus = 0;
}


static void keyDown(GUIHeader* w_, GUIEvent* gev) {
	GUIEdit* w = (GUIEdit*)w_;
	
	if(gev->modifiers & (GUIMODKEY_CTRL | GUIMODKEY_ALT | GUIMODKEY_TUX)) return;
	
	if(isprint(gev->character) && (gev->modifiers & (~(GUIMODKEY_SHIFT | GUIMODKEY_LSHIFT | GUIMODKEY_RSHIFT))) == 0) {
		insertChar(w, gev->character);
		w->cursorpos++;
		w->selEnd = w->cursorpos;
		w->blinkTimer = 0.0;
		
		fireOnchange(w);
		
		gev->cancelled = 1;
	}
	
	updateTextControl(w);
	
	return;
}




static void handleCommand(GUIHeader* w_, GUI_Cmd* cmd) {
	GUIEdit* w = (GUIEdit*)w_;
	
	switch(cmd->cmd) {
		case GUICMD_Edit_MoveCursorH:
			moveCursor(w, cmd->amt);
			w->selEnd = w->cursorpos;
			w->blinkTimer = 0.0;
			break;
			
		case GUICMD_Edit_GrowSelectionH:
			moveCursor(w, cmd->amt);
			w->blinkTimer = 0.0;
			break;
			
		case GUICMD_Edit_Backspace:
			if(w->cursorpos != w->selEnd) {
				removeRange(w, w->cursorpos, w->selEnd);
				setCursor(w, MIN(w->cursorpos, w->selEnd));
			}
			else {
				removeChar(w, w->cursorpos - 1);
				moveCursor(w, -1);
			}
			w->selEnd = w->cursorpos;
			w->blinkTimer = 0.0;
			break;
			
		case GUICMD_Edit_Delete:
			if(w->cursorpos != w->selEnd) {
				removeRange(w, w->cursorpos, w->selEnd);
				setCursor(w, MIN(w->cursorpos, w->selEnd));
			}
			else {
				removeChar(w, w->cursorpos);
			}
			w->selEnd = w->cursorpos;
			w->blinkTimer = 0.0;
			break;
			
		case GUICMD_Edit_Copy:
			printf("edit copy nyi\n");
			w->blinkTimer = 0.0;
			break;
			
		case GUICMD_Edit_Paste: {
			char* tmp;
			size_t tmplen;
			
			if(w->cursorpos != w->selEnd) {
				removeRange(w, w->cursorpos, w->selEnd);
				setCursor(w, MIN(w->cursorpos, w->selEnd));
			}
			
			Clipboard_PeekRawText(cmd->amt, &tmp, &tmplen);
			if(tmplen == 0) break;
			
			insertString(w, w->cursorpos, tmp, tmplen);
			
			moveCursor(w, tmplen);
			w->selEnd = w->cursorpos;

			w->blinkTimer = 0.0;
			break;
		}
	}
	
	updateTextControl(w);
}



GUIEdit* GUIEdit_New(GUIManager* gm, char* initialValue) {
	
	GUIEdit* w;
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.Delete = (void*)delete,
		.HandleCommand = (void*)handleCommand,
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
	w->header.cmdElementType = GUIELEMENT_Edit;
	
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
// 	GUI_RegisterObject(&w->header, w->textControl);

// 	w->cursorOffset = guiTextGetTextWidth(w->textControl, w->cursorpos);
	w->cursorOffset = gui_getDefaultUITextWidth(gm, initialValue, 9999999);
	
	
	
// 	w->textControl->header.onClick = (GUI_OnClickFn)click;
	
	return w;
}


static void growBuffer(GUIEdit* ed, int extra) {
	ed->buflen = nextPOT(ed->textlen + extra + 1);
	if(ed->buf) {
		ed->buf = realloc(ed->buf, ed->buflen);
	}
	else {
		ed->buf = calloc(1, ed->buflen);
	}
}

static void checkBuffer(GUIEdit* ed, int minlen) {
	if(ed->buflen < minlen + 1) {
		growBuffer(ed, minlen - ed->buflen + 1);
	}
}

// at the cursor. does not move the cursor.
static void insertChar(GUIEdit* ed, char c) { 
	checkBuffer(ed, ed->textlen + 1);
	
	char* e = ed->buf + ed->textlen + 1; // copy the null terminator too
	while(e >= ed->buf + ed->cursorpos + 1) {
		*e = *(e - 1);
		e--;
	}
	
	ed->textlen++;
	*(e) = c;
}

static void insertString(GUIEdit* w, int where, char* text, size_t len) {
	if(where > w->textlen) where = w->textlen;
	
	checkBuffer(w, len + w->textlen);
	
	if(where != w->textlen) {
		memmove(w->buf + where + len, w->buf + where, w->textlen - where);
	}
	
	// move the latter chunk down
	memcpy(w->buf + where, text, len);
	w->textlen += len;
	w->buf[w->textlen] = 0;
}

static void updateTextControl(GUIEdit* ed) {
// 	GUIText_setString(ed->textControl, ed->buf);
	
	// get new cursor pos
// 	ed->cursorOffset = guiTextGetTextWidth(ed->textControl, ed->cursorpos);
	ed->cursorOffset = gui_getDefaultUITextWidth(ed->header.gm, ed->buf, ed->cursorpos);
	
	//printf("cursorpos %f\n", ed->cursorOffset); 
	
//	fireOnchange(ed);
}

// just changes the text value. triggers nothing.
static void setText(GUIEdit* ed, char* s) {
	int len = strlen(s);
	checkBuffer(ed, len);
	
	strcpy(ed->buf, s);
	ed->textlen = len;
	
	if(len < ed->cursorpos || ed->cursorpos == 0) {
		ed->cursorpos = len;
		ed->cursorOffset = gui_getDefaultUITextWidth(ed->header.gm, ed->buf, ed->cursorpos);
	}
	
}

static void fireOnchange(GUIEdit* ed) {
	
	GUIEvent gev = {};
	gev.type = GUIEVENT_User;
	gev.eventTime = 0;
	gev.originalTarget = (GUIHeader*)ed;
	gev.currentTarget = (GUIHeader*)ed;
	gev.cancelled = 0;
	gev.userType = "change";
	
	gev.userData = ed->buf;
	gev.userSize = ed->textlen;
	
	GUIManager_BubbleEvent(ed->header.gm, (GUIHeader*)ed, &gev);
}


static void fireOnEnter(GUIEdit* ed) {
	
	GUIEvent gev = {};
	gev.type = GUIEVENT_User;
	gev.eventTime = 0;
	gev.originalTarget = (GUIHeader*)ed;
	gev.currentTarget = (GUIHeader*)ed;
	gev.cancelled = 0;
	gev.userType = "enter";
	
	gev.userData = ed->buf;
	gev.userSize = ed->textlen;
	
	GUIManager_BubbleEvent(ed->header.gm, (GUIHeader*)ed, &gev);
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



