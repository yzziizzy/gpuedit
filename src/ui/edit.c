

#include "../gui.h"
#include "../gui_internal.h"




static void insertChar(GUIEdit* ed, char c);
static void updateTextControl(GUIEdit* ed);
static void fireOnchange(GUIEdit* ed);


static void moveCursor(GUIEdit* ed, int delta) {
	ed->cursorpos = MIN(ed->textlen, MAX(0, ed->cursorpos + delta));
}


void guiEditRender(GUIEdit* ed, GameState* gs, PassFrameParams* pfp) {
	
// 	guiRender(ed->bg, gs, pfp);
// 	guiRender(ed->textControl, gs, pfp);
	
	
	if(fmod(pfp->wallTime, 1.0) > .5) {
		ed->cursor->header.topleft.x = 
			ed->bg->header.topleft.x +
			(ed->cursorOffset * ed->textControl->fontSize * .01);
// 		guiRender(ed->cursor, gs, pfp);
	}
}

void guiEditDelete(GUIEdit* sw) {
	
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


static void recieveText(InputEvent* ev, GUIEdit* ed) {
	insertChar(ed, ev->character);
	ed->cursorpos++;
	updateTextControl(ed);
}

static void keyDown(InputEvent* ev, GUIEdit* ed) {
	if(ev->keysym == XK_Left) moveCursor(ed, -1);
	else if(ev->keysym == XK_Right) moveCursor(ed, 1);
	else if(ev->keysym == XK_BackSpace) {
		removeChar(ed, ed->cursorpos - 1);
		moveCursor(ed, -1);
	}
	else if(ev->keysym == XK_Delete) removeChar(ed, ed->cursorpos);
	
	updateTextControl(ed);
}



GUIEdit* GUIEditNew(char* initialValue, Vector2 pos, Vector2 size) {
	
	GUIEdit* ed;
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)guiEditRender,
		.Delete = (void*)guiEditDelete
	};
		
	static InputEventHandler input_vt = {
		.keyText = recieveText,
		.keyDown = keyDown,
	};
	
	ed = calloc(1, sizeof(*ed));
	CHECK_OOM(ed);
	
	gui_headerInit(&ed->header, NULL, &static_vt);
	ed->inputHandlers = &input_vt;
	
	ed->header.hitbox.min.x = pos.x;
	ed->header.hitbox.min.y = pos.y;
	ed->header.hitbox.max.x = pos.x + size.x;
	ed->header.hitbox.max.y = pos.y + size.y;
	
	ed->blinkRate = 1.5;
	
	if(initialValue) {
		ed->textlen = strlen(initialValue);
		ed->buflen = nextPOT(ed->textlen + 1);
		ed->buf = malloc(ed->buflen);
		strcpy(ed->buf, initialValue);
	}
	else {
		ed->textlen = 0;
		ed->buflen = 16;
		ed->buf = malloc(16);
		ed->buf[0] = 0;
	}
	
	ed->cursorpos = ed->textlen;
	
// 	ed->bg = GUIWindow_new(pos, size, 1);
	ed->bg->color = (Vector){0.1, 0.1, 0.1};
	ed->bg->borderColor = (Vector4){1.0, .7, .3, 1.0};
// 	guiRegisterObject(ed->bg, &ed->header);
	
	// TODO: fix size and pos of cursor
// 	ed->cursor = guiWindowNew(pos, (Vector2){.003, size.y}, 1);
	ed->cursor->color = (Vector){1.0, 1.0, 1.0};
// 	guiRegisterObject(ed->cursor, &ed->bg->header);
	
// 	ed->textControl = GUIText_new(initialValue, pos, 6.0f, "Arial");
	ed->textControl->header.size.x = .5;
// 	guiRegisterObject(ed->textControl, &ed->bg->header);

	ed->cursorOffset = guiTextGetTextWidth(ed->textControl, ed->cursorpos);

	
	return ed;
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
// 	ed->cursorOffset = guiTextGetTextWidth(ed->textControl, ed->cursorpos);
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



void guiEditSetText(GUIEdit* ed, char* text) {
	setText(ed, text);
}




void guiEditSetInt(GUIEdit* ed, int64_t ival) {
	fprintf(stderr, "FIXME: GUIEditSetInt\n");
	guiEditSetDouble(ed, ival);
}

void guiEditSetDouble(GUIEdit* ed, double dval) {
	char txtVal[64]; 
	
	ed->numVal = dval;
	
	gcvt(dval, 6, txtVal);
	guiEditSetText(ed, txtVal);
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


double guiEditGetDouble(GUIEdit* ed) {
	// TODO: cache this value
	updateDval(ed);
	return ed->numVal;
}



