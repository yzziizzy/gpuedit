#include <ctype.h>

#include "../clipboard.h"

#include "gui.h"
#include "gui_internal.h"

#include "macros_on.h"




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
		if(o[0].selectAll) {
			d->cursor.selectPivot = 0;
			d->cursor.cursorPos = str->len;
		}
		
		GUI_SetData_(gm, id, d, free);
	}
	
	check_string(str, 0); // make sure the input string exists
	
	ret = d->synthed_change;
	d->synthed_change = 0;
	
	Vector2 sz = o[0].size;
	if(width > 0) sz.x = width;
	float fontSz = o[0].fontSize;
	vec2 pad = o[0].padding;
	
	GUIFont* font = GUI_FindFont(gm, o[0].fontName);//, fontSz);	
	
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
	GUI_BoxFilled_(gm, tl, sz, o[st].borderWidth, &o[st].border, &o[st].bg);
	
	float cursorOff = GUI_GetTextWidthAdv(str->data, d->cursor.cursorPos, font, fontSz);
	
	// calculate scroll offsets
	if(cursorOff + d->scrollX > sz.x - pad.x * 2) {
		d->scrollX = sz.x - cursorOff;	
	}
	else if(cursorOff + d->scrollX < pad.x) {
		d->scrollX = -cursorOff;
	}
	
	GUI_PushClip(tl, sz);
	
	// draw cursor and selection background
	if(gm->activeID == id) {
		gm->curZ += 0.001;
		if(d->cursor.blinkTimer < 0.5) { 
			gm->curZ += 0.001;
			
			// TODO: dpi scaling here
			GUI_Rect(V(tl.x + cursorOff + d->scrollX + pad.x, tl.y + o[st].borderWidth + 1), V(2,sz.y - o[st].borderWidth * 2 - 2), &o[st].cursorColor);
			gm->curZ -= 0.001;
		}
		
		d->cursor.blinkTimer += gm->timeElapsed;
		d->cursor.blinkTimer = fmod(d->cursor.blinkTimer, 1.0);
		
		if(d->cursor.selectPivot > -1) {
			float pivotOff = GUI_GetTextWidthAdv(str->data, d->cursor.selectPivot, font, fontSz);
			
			int min = MIN(cursorOff, pivotOff);
			int max = MAX(cursorOff, pivotOff);
			
			GUI_Rect(V(tl.x + pad.x + min + d->scrollX, tl.y + o[st].borderWidth + 1), V(max - min,sz.y - o[st].borderWidth * 2 - 1), &o[st].selectionBgColor);
			
		}
		gm->curZ -= 0.001;
	}
	

	
	gm->curZ += 10.01;
	
	if(str->len) {
		GUI_TextLineAdv_(gm, V(tl.x + d->scrollX + pad.x, tl.y), sz, str->data, str->len, GUI_TEXT_ALIGN_VCENTER, font, fontSz, &o[st].text);
	}
	GUI_PopClip();
	gm->curZ -= 10.01;
	
	return ret;
}




struct intedit_data {
	struct edit_data ed;
	int cursorPos;
	float blinkTimer;
	
	long lastValue;
	GUIString str;
};

static void intedit_free(struct intedit_data* d) {
	free(d->str.data);
	free(d);
}

// returns true on a change
int GUI_IntEdit_(GUIManager* gm, void* id, Vector2 tl, float width, long* num, GUIIntEditOpts* o) {
	int ret = 0;
	int firstRun = 0;
	struct intedit_data* d;
	
	if(!(d = GUI_GetData_(gm, id))) {
		d = calloc(1, sizeof(*d));

		
		GUI_SetData_(gm, id, d, (void*)intedit_free);
		
		d->str.alloc = 64;
		d->str.data = calloc(1, d->str.alloc * sizeof(*d->str.data));
		
		d->str.len = snprintf(d->str.data, 64, "%ld", *num);
		d->lastValue = *num;
		
		if(o[0].selectAll) {
			d->ed.cursor.selectPivot = 0;
			d->ed.cursor.cursorPos = d->str.len;
		}
		firstRun = 1;
	}
	
	GUIFont* font = GUI_FindFont(gm, o[0].fontName);
	
	Vector2 sz = o[0].size;
	if(width > 0) sz.x = width;
	vec2 pad = o[0].padding;
	
	HOVER_HOT(id)

	if(gm->hotID == id) {
		bool wasActive = gm->activeID == id;
	
		MOUSE_DOWN_ACTIVE(id)
		
		// select all if first click on inactive
		if(!wasActive && gm->activeID == id) {
			// select all
			select_all(&d->ed.cursor, &d->str);
		}
		
		
		if(GUI_MouseWentDown(1)) {
			// position the cursor
			Vector2 mp = GUI_MousePos();
//			d->cursor.cursorPos = gui_charFromPixel(gm, font, fontSz, str->data, mp.x - tl.x);
//			d->cursor.cursorPos = MIN(d->cursor.cursorPos, str->len);
			d->ed.cursor.selectPivot = -1;
			d->ed.cursor.cursorPos = floor(GUI_CharFromPixelF(font, o[0].fontSize, d->str.data, d->str.len, mp.x - (tl.x + pad.x + d->ed.scrollX)));
		}
		else if(GUI_MouseWentUp(1)) {
			if(gm->curEvent.multiClick == 2) {
				// select all
				select_all(&d->ed.cursor, &d->str);
			}
		}
		else if(GUI_MouseWentUp(2)) {
			char* pasteData;
			size_t pasteLen;
			Clipboard_PeekRawText(CLIP_SELECTION, &pasteData, &pasteLen);
			GUICursorData* cd = &d->ed.cursor;
				
//			printf("paste data: %s, len %ld)\n", pasteData, pasteLen);
				
			if(pasteLen > 0) {
				if(cd->selectPivot > -1) delete_selection(cd, &d->str);
				check_string(&d->str, pasteLen);
				memmove(d->str.data + cd->cursorPos + pasteLen, d->str.data + cd->cursorPos, d->str.len - cd->cursorPos + 1);
				memcpy(d->str.data + cd->cursorPos, pasteData, pasteLen);
				d->str.len += pasteLen;
				cd->cursorPos += pasteLen;
				cd->blinkTimer = 0;
				
				GUI_CancelInput();
				ret |= 1;
			}
		}
//		if(GUI_MouseWentUp(1)) {
//			if(gm->curEvent.multiClick == 2) {
//				// select all
//				select_all(&d->ed.cursor, str);
//			}
//			// position the cursor
//			Vector2 mp = GUI_MousePos();
//			d->ed.cursor.cursorPos = gui_charFromPixel(gm, font, o[0].fontSize, d->str.data, mp.x - tl.x + pad.x);
//		}
	}
	
	// handle input
	if(gm->activeID == id) {
		if(gm->curEvent.type == GUIEVENT_KeyDown) {
			if(!isprint(gm->curEvent.character) || ('0' <= gm->curEvent.character && gm->curEvent.character <= '9')) {
				ret |= GUI_HandleTextInput_(gm, &d->ed.cursor, &d->str, &gm->curEvent);
			}
		}
	}
	
	if(ret) {
		*num = strtol(d->str.data, NULL, 10);
	}

	// refresh the buffer, maybe
	if(*num != d->lastValue || firstRun) {
		d->str.len = snprintf(d->str.data, 64, "%ld", *num);
		d->lastValue = *num;
	}
	
	// bail early if not drawing
	if(!gm->drawMode) return ret;
	int st = CUR_STATE(id);
	
	// draw the border
	GUI_BoxFilled_(gm, tl, sz, o[st].borderWidth, &o[st].border, &o[st].bg);
		
	float fontSz = o[st].fontSize;
	
	float cursorOff = GUI_GetTextWidthAdv(d->str.data, d->ed.cursor.cursorPos, font, fontSz);
	
	// calculate scroll offsets
	if(cursorOff + d->ed.scrollX > sz.x - pad.x * 2) {
		d->ed.scrollX = sz.x - cursorOff;	
	}
	else if(cursorOff + d->ed.scrollX < pad.x) {
		d->ed.scrollX = -cursorOff;
	}
	
	GUI_PushClip(tl, sz);
	
	// draw cursor and selection background
	if(gm->activeID == id) {
		gm->curZ += 0.001;
		if(d->ed.cursor.blinkTimer < 0.5) { 
			gm->curZ += 0.001;
			
			// TODO: dpi scaling here
			GUI_Rect(V(tl.x + cursorOff + d->ed.scrollX + pad.x, tl.y + o[st].borderWidth + 1), V(2,sz.y - o[st].borderWidth * 2 - 2), &o[st].cursorColor);
			gm->curZ -= 0.001;
		}
		
		d->ed.cursor.blinkTimer += gm->timeElapsed;
		d->ed.cursor.blinkTimer = fmod(d->ed.cursor.blinkTimer, 1.0);
		
		if(d->ed.cursor.selectPivot > -1) {
			float pivotOff = GUI_GetTextWidthAdv(d->str.data, d->ed.cursor.selectPivot, font, fontSz);
			
			int min = MIN(cursorOff, pivotOff);
			int max = MAX(cursorOff, pivotOff);
			
			GUI_Rect(V(tl.x + pad.x + min + d->ed.scrollX, tl.y), V(max - min,sz.y), &o[st].selectionBgColor);
		}
		gm->curZ -= 0.001;
	}
	

	
	gm->curZ += 10.01;
	
	if(d->str.len) {
		GUI_TextLineAdv_(gm, V(tl.x + d->ed.scrollX + pad.x, tl.y), sz, d->str.data, d->str.len, GUI_TEXT_ALIGN_VCENTER, font, fontSz, &o[st].text);
	}
	GUI_PopClip();
	gm->curZ -= 10.01;
	
	return ret;
}





struct floatedit_data {
	struct edit_data ed;
	int cursorPos;
	float scrollX; // x offset to add to all text rendering code 
	float blinkTimer;
	
	float lastValue;
	GUIString str;
};

static void floatedit_free(struct floatedit_data* d) {
	free(d->str.data);
	free(d);
}

// returns true on a change
int GUI_FloatEdit_(GUIManager* gm, void* id, Vector2 tl, float width, float* num, GUIFloatEditOpts* o) {
	int ret = 0;
	int firstRun = 0;
	struct floatedit_data* d = NULL;
	
	if(!(d = GUI_GetData_(gm, id))) {
		d = calloc(1, sizeof(*d));
		GUI_SetData_(gm, id, d, (void*)floatedit_free);
		
		d->str.alloc = 64;
		d->str.data = calloc(1, d->str.alloc * sizeof(*d->str.data));
		firstRun = 1;
	}
	
	GUIFont* font = GUI_FindFont(gm, o[0].fontName);
	
	Vector2 sz = o[0].size;
	if(width > 0) sz.x = width;
	vec2 pad = o[0].padding;
	
	HOVER_HOT(id)

	if(gm->hotID == id) {
		bool wasActive = gm->activeID == id;
	
		MOUSE_DOWN_ACTIVE(id)
		
		// select all if first click on inactive
		if(!wasActive && gm->activeID == id) {
			// select all
			select_all(&d->ed.cursor, &d->str);
		}
		
		if(GUI_MouseWentDown(1)) {
			// kill selection, position the cursor
			Vector2 mp = GUI_MousePos();
			d->ed.cursor.selectPivot = -1;
			d->ed.cursor.cursorPos = floor(GUI_CharFromPixelF(font, o[0].fontSize, d->str.data, d->str.len, mp.x - (tl.x + pad.x + d->ed.scrollX)));
		}
		if(GUI_MouseWentUp(1)) {
			if(gm->curEvent.multiClick == 2) {
				// select all
				select_all(&d->ed.cursor, &d->str);
			}
		}
	}
	
	// mouse selection dragging
	if(GUI_InputAvailable()) {
		
		if(gm->curEvent.type == GUIEVENT_DragStart && GUI_PointInBoxV(tl, sz, gm->lastMousePos)) {
			d->ed.cursor.isMouseDragging = 1;
			// set pivot 
			d->ed.cursor.selectPivot = d->ed.cursor.cursorPos;
		}
		else if(d->ed.cursor.isMouseDragging && gm->curEvent.type == GUIEVENT_DragMove) {
		
			// set cursorPos
			vec2 mp = GUI_MousePos();
			d->ed.cursor.cursorPos = floor(GUI_CharFromPixelF(font, o[0].fontSize, d->str.data, d->str.len, mp.x - (tl.x + pad.x + d->ed.scrollX)));
		}
		else if(d->ed.cursor.isMouseDragging && gm->curEvent.type == GUIEVENT_DragStop) {
			d->ed.cursor.isMouseDragging = 0;
		}
	}
	
	
	// handle input
	if(gm->activeID == id) {
		if(gm->curEvent.type == GUIEVENT_KeyDown) {
			if(
				!isprint(gm->curEvent.character) // control events (arrow keys)
				|| strchr("0123456789.-", gm->curEvent.character) // numbers 
			) {
				ret |= GUI_HandleTextInput_(gm, &d->ed.cursor, &d->str, &gm->curEvent);
			}
		}
		
		if(GUI_MouseWentDownAnywhere(1) && !GUI_MouseInside(tl, sz)) {
			ACTIVE(NULL);
		}
	}
	
	if(gm->activeID != id) {
		select_none(&d->ed.cursor);
	}
	
	if(ret) {
		*num = strtof(d->str.data, NULL);
		d->lastValue = *num;
	}
	

	if(*num != d->lastValue || firstRun) {
		switch(fpclassify(*num)) {
			default:
			case FP_NORMAL:
			case FP_SUBNORMAL:
			case FP_ZERO:
				d->str.len = snprintf(d->str.data, 64, "%f", *num);
				d->lastValue = *num;
				break;
			case FP_NAN: 
				strcpy(d->str.data, "NaN");
				d->str.len = 3;
				break;
			case FP_INFINITE:
				strcpy(d->str.data, "Infinity");
				d->str.len = strlen("Infinity");
				break;
		}
	}
	

	
	// bail early if not drawing
	if(!gm->drawMode) return ret;
	int st = CUR_STATE(id);
	
	// draw the border
	GUI_BoxFilled_(gm, tl, sz, o[st].borderWidth, &o[st].border, &o[st].bg);
	
	// refresh the buffer, maybe
		
	float fontSz = o[st].fontSize;
	
	float cursorOff = GUI_GetTextWidthAdv(d->str.data, d->ed.cursor.cursorPos, font, fontSz);
	
	// calculate scroll offsets
	if(cursorOff + d->ed.scrollX > sz.x - pad.x * 2) {
		d->ed.scrollX = sz.x - cursorOff;	
	}
	else if(cursorOff + d->ed.scrollX < pad.x) {
		d->ed.scrollX = -cursorOff;
	}
	
	GUI_PushClip(tl, sz);
	
	// draw cursor and selection background
	if(gm->activeID == id) {
		gm->curZ += 0.001;
		if(d->ed.cursor.blinkTimer < 0.5) { 
			gm->curZ += 0.001;
			
			// TODO: dpi scaling here
			GUI_Rect(V(tl.x + cursorOff + d->ed.scrollX + pad.x, tl.y + o[st].borderWidth + 1), V(2,sz.y - o[st].borderWidth * 2 - 2), &o[st].cursorColor);
			gm->curZ -= 0.001;
		}
		
		d->ed.cursor.blinkTimer += gm->timeElapsed;
		d->ed.cursor.blinkTimer = fmod(d->ed.cursor.blinkTimer, 1.0);
		
		if(d->ed.cursor.selectPivot > -1) {
			float pivotOff = GUI_GetTextWidthAdv(d->str.data, d->ed.cursor.selectPivot, font, fontSz);
			
			int min = MIN(cursorOff, pivotOff);
			int max = MAX(cursorOff, pivotOff);
			
			GUI_Rect(V(tl.x + min + d->ed.scrollX + pad.x, tl.y + pad.y), V(max - min,sz.y - pad.y*2), &o[st].selectionBgColor);
			
		}
		gm->curZ -= 0.001;
	}
	

	
	gm->curZ += 10.01;
	
	if(d->str.len) {
		GUI_TextLineAdv_(gm, V(tl.x + d->ed.scrollX + pad.x, tl.y), V(9999999,sz.y), d->str.data, d->str.len, GUI_TEXT_ALIGN_VCENTER, font, fontSz, &o[st].text);
	}
	GUI_PopClip();
	gm->curZ -= 10.01;
	
	return ret;
}



