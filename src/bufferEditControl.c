
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "ui/gui.h"
#include "commands.h"
#include "ui/gui_internal.h"
#include "clipboard.h"







size_t GBEC_lineFromPos(GUIBufferEditControl* w, Vector2 pos) {
	return floor((pos.y - w->tl.y) / w->bs->lineHeight) + 1 + w->scrollLines; // TODO IMGUI
}

size_t GBEC_getColForPos(GUIBufferEditControl* w, BufferLine* bl, float x) {
	if(bl->length == 0) return 0;
	
	// must handle tabs
	float a = (x - w->tl.x - w->textAreaOffsetX) / w->bs->charWidth; // TODO IMGUI
	ptrdiff_t screenCol = floor(a + 0) + w->scrollCols;
	
	if(screenCol <= 0) return 0;
	
	int tabwidth = w->bs->tabWidth;
	ptrdiff_t charCol = 0;
	
	while(screenCol > 0 && charCol < bl->length) {
		if(bl->buf[charCol] == '\t') screenCol -= tabwidth;
		else screenCol--;
		charCol++;
	}
	
	if(screenCol < -2) {
		// wide character blew past the condition
		charCol--;
	}
	
	return MAX(0, MIN(charCol, bl->length));
}



#include "ui/macros_on.h"

static void scrollUp(GUIBufferEditControl* w) {
	w->scrollLines = MAX(0, w->scrollLines - w->linesPerScrollWheel);
}

static void scrollDown(GUIBufferEditControl* w) {
	w->scrollLines = MIN(MAX(0, w->b->numLines - w->linesOnScreen), w->scrollLines + w->linesPerScrollWheel);
}



static void dragStart(GUIBufferEditControl* w, GUIManager* gm) {
	Buffer* b = w->b;
	
	/* scrollbar dragging
	if(gev->originalTarget == (void*)w->scrollbar) {
		printf("scrollbar drag start\n");
		return;
	}
	*/
	
	Vector2 mp = GUI_EventPos();
	
	if(gm->curEvent.button == 1) {
	
		Vector2 tl = w->tl;
		Vector2 sz = w->sz;
		
		w->sbMinHeight = 50;
		float sb_line_range = w->b->numLines - w->linesOnScreen;
		float sb_px_range = sz.y - w->sbMinHeight;
		float sb_pos = sb_px_range * (w->scrollLines / sb_line_range);
		
		// scrollbar dragging
		if(!w->bs->disableScrollbar && GUI_PointInBoxVABS(V(tl.x + sz.x - 10, tl.y + sb_pos), V(10, w->sbMinHeight), mp)) {
			w->isDragScrolling = 1;
			
			w->scrollDragStartOffset = mp.y - tl.y - sb_pos;
		}
		else {
			// normal text traging
			BufferLine* bl = Buffer_raw_GetLineByNum(b, GBEC_lineFromPos(w, mp));
			size_t col = GBEC_getColForPos(w, bl, mp.x);
			
			PIVOT_LINE(w->sel) = bl;
			PIVOT_COL(w->sel) = col;
			w->sel->selecting = 1;
			
			w->isDragSelecting = 1;
		}
	}
}

static void dragStop(GUIBufferEditControl* w, GUIManager* gm) {

	if(gm->curEvent.button == 1 && (w->isDragScrolling || w->isDragScrollCoasting || w->isDragSelecting)) {
		w->isDragSelecting = 0;
		w->isDragScrollCoasting = 0;
		w->isDragScrolling = 0;
		
		w->scrollDragStartOffset = 0;
		
		GBEC_SetSelectionFromPivot(w);
	}
	
// 	w->scrollLines = MIN(w->buffer->numLines - w->linesOnScreen, w->scrollLines + w->linesPerScrollWheel);
}

static void dragMove(GUIBufferEditControl* w, GUIManager* gm) {
	Buffer* b = w->b;
// 	w->header.gm

	Vector2 mp = GUI_EventPos();
	
	
	if(w->isDragScrolling) {
		Vector2 tl = w->tl;
		Vector2 sz = w->sz;
	
		w->sbMinHeight = 50;
		float sb_line_range = w->b->numLines - w->linesOnScreen;
		float sb_px_range = sz.y - w->sbMinHeight;
		
		float y = mp.y - tl.y - w->scrollDragStartOffset;
		
		float sb_line_ratio = (y - tl.y) / sb_px_range;
		int sb_line = sb_line_range * sb_line_ratio;
		
		w->scrollLines = MIN(MAX(0, sb_line), sb_line_range);
		
		return;
	}
	
	
	if(w->isDragSelecting) {
		BufferLine* bl = Buffer_raw_GetLineByNum(b, GBEC_lineFromPos(w, mp));
		
		size_t col = GBEC_getColForPos(w, bl, mp.x);
		
		//GBEC_SetCurrentSelection(w, bl, col, PIVOT_LINE(w->sel), PIVOT_COL(w->sel));
		
		CURSOR_LINE(w->sel) = bl;
		CURSOR_COL(w->sel) = col;
		
		
		BufferRange_Normalize(w->sel);
		
		if(mp.y < w->tl.y) {
			w->isDragScrollCoasting = 1;
			
			float d = w->tl.y - mp.y;
			
			w->scrollCoastStrength = (fclamp(d, 0, w->scrollCoastMax) / w->scrollCoastMax);
			w->scrollCoastDir = -1;
		}
		else if(mp.y > w->tl.y + w->sz.y) {
			w->isDragScrollCoasting = 1;
			
			float d = mp.y - w->tl.y - w->sz.y;
			
			w->scrollCoastStrength = (fclamp(d, 0, w->scrollCoastMax) / w->scrollCoastMax) ;
			w->scrollCoastDir = 1;
		}
		else {
			w->isDragScrollCoasting = 0;
		}
		
		
		GBEC_MoveCursorTo(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel));
		GBEC_scrollToCursor(w);
	}
	/*
	if(bl->lineNum < w->selectPivotLine->lineNum) {
		b->sel->line[0] = bl;
		b->sel->line[1] = w->selectPivotLine;
		b->sel->col[0] = col;
		b->sel->col[1] = w->selectPivotCol - 1;
	}
	else if(bl->lineNum > w->selectPivotLine->lineNum) {
		b->sel->line[0] = w->selectPivotLine;
		b->sel->line[1] = bl;
		b->sel->col[0] = w->selectPivotCol;
		b->sel->col[1] = col;
	}
	else { // same line
		b->sel->line[0] = bl;
		b->sel->line[1] = bl;
		
		if(col < w->selectPivotCol) {
			b->sel->col[0] = col;
			b->sel->col[1] = w->selectPivotCol - 1;
		}
		else {
			b->sel->col[0] = w->selectPivotCol;
			b->sel->col[1] = col;
		}
	}*/
	
}


static void click(GUIBufferEditControl* w, GUIManager* gm) {
	Buffer* b = w->b;
	
	if(!b->first) return; // empty buffer
	
	unsigned int shift = gm->curEvent.modifiers & GUIMODKEY_SHIFT;
	
	Vector2 mp = GUI_EventPos();
	Vector2 tl = w->tl;
	Vector2 sz = w->sz;
	
	if(gm->curEvent.button == 1) { // left click

		if(!shift) GBEC_ClearAllSelections(w);
		
		// TODO: reverse calculate cursor position
		if(mp.x < tl.x + w->textAreaOffsetX || mp.x > tl.x + sz.x) return;
		if(mp.y < tl.y || mp.y > tl.y + sz.y) return;
		
		if(shift && !HAS_SELECTION(w->sel)) {
			PIVOT_LINE(w->sel) = CURSOR_LINE(w->sel);
			PIVOT_COL(w->sel) = CURSOR_COL(w->sel);
		}
		
		size_t line = GBEC_lineFromPos(w, mp);
		CURSOR_LINE(w->sel) = Buffer_raw_GetLineByNum(b, line);
		CURSOR_COL(w->sel) = GBEC_getColForPos(w, CURSOR_LINE(w->sel), mp.x);
		
		w->cursorBlinkTimer = 0;
		
		BufferRange_Normalize(w->sel);
		
		if(shift) {
			GBEC_SetSelectionFromPivot(w);
		}
		
		
		
		// maybe nudge the screen down a tiny bit
		GBEC_MoveCursorTo(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel));
		GBEC_scrollToCursor(w);
			
		if(gm->curEvent.multiClick == 2) {
			GBEC_SelectSequenceUnder(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
		}
		else if(gm->curEvent.multiClick == 3) {
			GBEC_SetCurrentSelection(w, CURSOR_LINE(w->sel), 0, CURSOR_LINE(w->sel), CURSOR_LINE(w->sel)->length);
		}
		
		// kill autocomplete when the cursor is moved via mouse
		GBEC_CancelAutocomplete(w);
		if(w->inputState.mode == 10 || w->inputState.mode == 11) {
			w->inputState.mode = 0;
		}
		
	}
	else if(gm->curEvent.button == 2) { // middle click	
	
		// TODO: reverse calculate cursor position
		if(mp.x < tl.x + w->textAreaOffsetX || mp.x > tl.x + sz.x) return;
		if(mp.y < tl.y || mp.y > tl.y + sz.y) return;
		
		size_t lineNum = GBEC_lineFromPos(w, mp);
		BufferLine* line = Buffer_raw_GetLineByNum(b, lineNum);
		size_t col = GBEC_getColForPos(w, CURSOR_LINE(w->sel), mp.x);
		col = MIN(col, line->length);
		
		Buffer* b2 = Clipboard_PopBuffer(CLIP_SELECTION);
		if(b2) {
			// TODO: undo
			BufferRange pasteRange;
			Buffer_InsertBufferAt(b, b2, line, col, &pasteRange);
		}
	}
	else if(gm->curEvent.button == 4) { // scroll up
		scrollUp(w);
	}
	else if(gm->curEvent.button == 5) { // scroll down	
		scrollDown(w);
	}
	else if(gm->curEvent.button == 6) { // scroll left
	}
	else if(gm->curEvent.button == 7) { // scroll right	
	}
	

}




void GBEC_Render(GUIBufferEditControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	void* id = w;
	
	w->tl = tl;
	w->sz = sz;
	
	HOVER_HOT(id);
	CLICK_HOT_TO_ACTIVE(id);
	
	w->sbMinHeight = 50;
	float sb_line_range = w->b->numLines - w->linesOnScreen;
	float sb_px_range = sz.y - w->sbMinHeight;
	float sb_pos = sb_px_range * (w->scrollLines / sb_line_range);

	if(!gm->drawMode) {
		if(!GUI_InputAvailable()) return;
	
		GBEC_Update(w, tl, sz, pfp);
		
		size_t numCmds;
		
		GUI_Cmd* cmd = Commands_ProbeCommand(gm, GUIELEMENT_Buffer, &gm->curEvent, &w->inputState, &numCmds);
		int needRehighlight = 0;
		for(int j = 0; j < numCmds; j++) { 
			if(!GBEC_ProcessCommand(w, cmd+j, &needRehighlight)) {
		
				if(needRehighlight || cmd[j].flags & GUICMD_FLAG_rehighlight) {
					GUIBufferEditControl_MarkRefreshHighlight(w);
				}
				
				GUI_CancelInput();
			}
		}
		
		Commands_UpdateModes(gm, &w->inputState, cmd, numCmds);
		
		
		if(gm->activeID == w || gm->hotID == w) {
			
			Vector2 mp = GUI_EventPos();
			int inbox = GUI_PointInBoxVABS(tl, sz, mp);
			
			switch(gm->curEvent.type) {
				case GUIEVENT_MouseUp: if(inbox) click(w, gm); break;
				case GUIEVENT_DragStart: if(inbox) dragStart(w, gm); break;
				case GUIEVENT_DragStop: dragStop(w, gm); break;
				case GUIEVENT_DragMove: if(inbox) dragMove(w, gm); break;
					
				// todo scroll
			}
		
		}
		
		
		return;
	}
	
	// ------ drawing commands after here ------
	
	GBEC_Update(w, tl, sz, pfp);
	
	GUIBufferEditControl_Draw(w, gm, tl, sz, w->scrollLines, + w->scrollLines + w->linesOnScreen + 2, 0, 100, pfp);
	
	// scrollbar 
	gm->curZ += 200;
	
	int skipScrollbar = w->bs->disableScrollbar;
	Color4 sbColor = w->ts->scrollbarColor;
	
	if(w->bs->scrollbarFadeDistance > 0 && !w->isDragScrolling) {
		Vector2 mp = GUI_MousePos();
		if(mp.x < tl.x + sz.x - w->bs->scrollbarFadeDistance || mp.x > sz.x) {
			skipScrollbar = 1;
		}
		else {
			sbColor.a *= MAX(0.0, MIN(1.0, (mp.x - tl.x - (sz.x - w->bs->scrollbarFadeDistance)) / w->bs->scrollbarFadeDistance));
		}
	}
	
	if(!skipScrollbar)
		GUI_Rect(V(tl.x + sz.x - 10, tl.y + sb_pos), V(10, w->sbMinHeight), &sbColor);
	
	gm->curZ -= 200;
}
#include "ui/macros_off.h"


void GBEC_Update(GUIBufferEditControl* w, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	
	Buffer* b = w->b;
	
	float lineNumWidth = ceil(LOGB(w->bs->lineNumBase, b->numLines + 0.5)) * w->bs->charWidth + w->bs->lineNumExtraWidth;
	
	w->linesOnScreen = sz.y / w->bs->lineHeight;
	w->colsOnScreen = (sz.x - lineNumWidth) / w->bs->charWidth;

	if(w->wantedScrollLine > -1) {
		GBEC_SetScrollCentered(w, w->wantedScrollLine, w->wantedScrollCol > -1 ? w->wantedScrollCol : 0);
		w->wantedScrollLine = -1;
		w->wantedScrollCol = -1;
	}

	w->scrollCoastMax = 50;
	// scroll coasting while selection dragging
	if(w->isDragScrollCoasting) {
		w->scrollCoastTimer += pfp->timeElapsed;
		
		float n = ((1.0 - w->scrollCoastStrength) * .1) + 0.001;
		
		if(w->scrollCoastTimer > n) {
			
			GBEC_ScrollDir(w, w->scrollCoastDir, 0);
			w->scrollCoastTimer = fmod(w->scrollCoastTimer, n);
		}
		
		// TODO: update selection endpoints too
	}
	
	// cursor blink
	float t = w->cursorBlinkOnTime + w->cursorBlinkOffTime;
	w->cursorBlinkTimer = fmod(w->cursorBlinkTimer + pfp->timeElapsed, t);
}




// called by the buffer when things change
static void bufferChangeNotify(BufferChangeNotification* note, void* _w) {
	GUIBufferEditControl* w = (GUIBufferEditControl*)_w;
	
	if(note->action == BCA_Undo_MoveCursor) {
//		printf("move notify\n");
		GBEC_MoveCursorTo(w, note->sel.line[0], note->sel.col[0]);
	}
	else if(note->action == BCA_Undo_SetSelection) {
		/*printf("selection notify %ld:%ld -> %ld:%ld\n",
			note->sel.line[0]->lineNum, note->sel.col[0],
			note->sel.line[1]->lineNum, note->sel.col[1]
		);*/
		if(note->isReverse) {
//			w->selectPivotLine = note->sel.line[1];
//			w->selectPivotCol = note->sel.col[1];
			GBEC_MoveCursorTo(w, note->sel.line[0], note->sel.col[0]);
			GBEC_SetCurrentSelection(w, 
				note->sel.line[1], note->sel.col[1],
				note->sel.line[0], note->sel.col[0]);
		}
		else {
//			w->selectPivotLine = note->sel.line[0];
//			w->selectPivotCol = note->sel.col[0];
			GBEC_MoveCursorTo(w, note->sel.line[1], note->sel.col[1]);
			GBEC_SetCurrentSelection(w, 
				note->sel.line[0], note->sel.col[0],
				note->sel.line[1], note->sel.col[1]);
		}
	}
	else if(note->action == BCA_DeleteLines) {		
		VEC_EACH(&w->selSet->ranges, i, r) {
			BufferRange_DeleteLineNotify(r, &note->sel);		
		}
		
		if(w->findSet && VEC_LEN(&w->findSet->ranges)) {
			VEC_EACH(&w->findSet->ranges, i, r) {
				BufferRange_DeleteLineNotify(r, &note->sel);		
			}
			// TODO: check deleted chars and re-regex		
		}
		// TODO: check scrollLines and scrollCols
	
		if(w->autocompleteOptions) {
			if(w->autocompleteOptions->matches) free(w->autocompleteOptions->matches);
			free(w->autocompleteOptions);
			w->autocompleteOptions = NULL;
		}
	}
	
}





GUIBufferEditControl* GUIBufferEditControl_New(GUIManager* gm) {
	
	GUIBufferEditControl* w = pcalloc(w);
	
	pcalloc(w->selSet);
	w->sel = BufferRange_New(w);
	
	VEC_PUSH(&w->selSet->ranges, w->sel);
	
	w->wantedScrollLine = -1;
	w->wantedScrollCol = -1;
	
	RING_INIT(&w->macros, 12);
	
	return w;
}

void GUIBufferEditControl_UpdateSettings(GUIBufferEditControl* w, Settings* s) {
	w->s = s;
	w->gs = Settings_GetSection(s, SETTINGS_General);
	w->bs = Settings_GetSection(s, SETTINGS_Buffer);
	w->ts = Settings_GetSection(s, SETTINGS_Theme);
	
	w->linesPerScrollWheel = w->bs->linesPerScrollWheel;
	w->cursorBlinkOnTime = w->bs->cursorBlinkOnTime;
	w->cursorBlinkOffTime = w->bs->cursorBlinkOffTime;
	w->outlineCurLine = w->bs->outlineCurrentLine;
	
}

void GUIBufferEditControl_SetScroll(GUIBufferEditControl* w, linenum_t line, colnum_t col) {
	w->scrollLines = MIN(MAX(0, line), w->b->numLines);
	w->scrollCols = MAX(0, col);
}

void GBEC_SetScrollCentered(GUIBufferEditControl* w, linenum_t line, colnum_t col) {
	if(w->linesOnScreen == 0) {
		w->wantedScrollLine = line;
		w->wantedScrollCol = col;
		return;
	}

	int mid = w->linesOnScreen / 2; // == 0 during init.
	w->scrollLines = MIN(MAX(0, line - mid), w->b->numLines);
	w->scrollCols = col > w->colsOnScreen - 10 ? MAX(0, col - (w->colsOnScreen / 2)) : 0;
}

// move the view by this delta
void GBEC_ScrollDir(GUIBufferEditControl* w, linenum_t lines, colnum_t cols) {
	linenum_t line = w->scrollLines + lines;
	colnum_t col = w->scrollCols + cols;
	if(line < 0) line = 0;
	if(col < 0) col = 0;
	GUIBufferEditControl_SetScroll(w, line, col);
}


// makes sure the cursor is on screen, with minimal necessary movement
void GBEC_scrollToCursor(GUIBufferEditControl* w) {
	GBEC_scrollToCursorOpt(w, 0);
}

void GBEC_scrollToCursorOpt(GUIBufferEditControl* w, int centered) {
	if(!w || !CURSOR_LINE(w->sel)) return;
	
	intptr_t scroll_first = w->scrollLines;
	intptr_t scroll_last = w->scrollLines + w->linesOnScreen;
	
	if(centered) {
		w->scrollLines = CURSOR_LINE(w->sel)->lineNum - w->linesOnScreen / 2;
	}
	else if(CURSOR_LINE(w->sel)->lineNum <= scroll_first) {
		w->scrollLines = CURSOR_LINE(w->sel)->lineNum - 1;
	}
	else if(CURSOR_LINE(w->sel)->lineNum > scroll_last) {
		w->scrollLines = scroll_first + (CURSOR_LINE(w->sel)->lineNum - scroll_last);
	}
	
	intptr_t col_first = w->scrollCols;
	intptr_t col_last = w->scrollCols + w->colsOnScreen;
	
	
	colnum_t curColDisp = GBEC_VisualColFromNominal(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel));
	if(curColDisp <= col_first) {
		w->scrollCols = curColDisp - 1;
	}
	else if(curColDisp >= col_last - 3) {
		w->scrollCols = col_first + 3 + (curColDisp - col_last);
	}
	
	
	GUIBufferEditControl_SetScroll(w, w->scrollLines, w->scrollCols);
}


void GBEC_SetSelectionFromPivot(GUIBufferEditControl* w) {
	if(!w->sel->line[0] || !w->sel->line[1] || (w->sel->line[0] == w->sel->line[1] && w->sel->col[0] == w->sel->col[1])) {
		w->sel->selecting = 0;
		return;
	}
	
	w->sel->selecting = 1;
	
	GBEC_SelectionChanged(w);
}



int getNextLine(HLContextInternal* hl, char** txt, intptr_t* len) {
	BufferLine* l = hl->color.readLine;
	
	if(!hl->color.readLine) return 1;
	
	// TODO: end of file
	hl->color.readLine = hl->color.readLine->next;
	
	*txt = l->buf;
	*len = l->length;
	
	return 0;
}

void writeSection(HLContextInternal* hl, unsigned char style, intptr_t len) {
	if(len == 0) return;
	
//	printf("writeSection, %d\n", style);
	while(len > 0 && hl->color.writeLine) {
		intptr_t maxl = MIN(255, MIN(len, hl->color.writeLine->length - hl->color.writeCol));
		
//	printf(" maxl: %d\n", maxl, len);
				
		VEC_INC(&hl->color.writeLine->style);
		VEC_TAIL(&hl->color.writeLine->style).length = maxl;
		VEC_TAIL(&hl->color.writeLine->style).styleIndex = style;
	
		hl->color.writeCol += maxl;
		len -= maxl;
		
		if(hl->color.writeCol >= hl->color.writeLine->length || hl->color.writeLine->length == 0) {
//			printf(" nextline: writecol: %ld, llen: %ld\n", hl->color.writeCol, hl->color.writeLine->length);
			hl->color.writeCol = 0;
			hl->color.writeLine = hl->color.writeLine->next;
			hl->ctx.dirtyLines--;
		}
		
	}
	
	// TODO: handle overlapping style segments
}

void writeFlags(HLContextInternal* hl, unsigned char style, intptr_t len) {
	if(len == 0) return;
	
	VEC_INC(&hl->flags.writeLine->style);
	VEC_TAIL(&hl->flags.writeLine->style).length = len;
	VEC_TAIL(&hl->flags.writeLine->style).styleIndex = style;
	
	hl->flags.writeCol += len;
	
	// TODO: handle overlapping style segments
	// TODO: handle segments spanning linebreaks
	
	if(hl->flags.writeCol > hl->flags.writeLine->length) {
		hl->flags.writeCol = 0;
		hl->flags.writeLine = hl->flags.writeLine->next;
		hl->ctx.dirtyLines--;
	}
	
}


static void* a_malloc(Allocator* a, size_t sz) {
	return malloc(sz);
}
static void* a_calloc(Allocator* a, size_t sz) {
	return calloc(1, sz);
}
static void* a_realloc(Allocator* a, void* p, size_t sz) {
	return realloc(p, sz);
}
static void a_free(Allocator* a, void* p) {
	free(p);
}

void GUIBufferEditControl_MarkRefreshHighlight(GUIBufferEditControl* w) {
	w->needsRehighlight = 1;
}

void GUIBufferEditControl_RefreshHighlight(GUIBufferEditControl* w) {
	double then = getCurrentTime();
	
	Buffer* b = w->b;
	
	if(!b->first) return; // the buffer is empty
	Highlighter* h = w->h;
	if(!h || !h->plugin) return; // there is no highlighter
	
	static Allocator al = {
		.malloc = a_malloc,
		.calloc = a_calloc,
		.realloc = a_realloc,
		.free = a_free,
	};
	
	HLContextInternal hlc = {
		.ctx.alloc = &al,
		.ctx.getNextLine = (void*)getNextLine,
		.ctx.writeSection = (void*)writeSection,
		.ctx.writeFlags = (void*)writeFlags,
		
		.ctx.dirtyLines = b->numLines,
		
		.b = b,
		.color.readLine = b->first,
		.color.writeLine = b->first,
		.color.writeCol = 0,
		.flags.readLine = b->first,
		.flags.writeLine = b->first,
		.flags.writeCol = 0,
	};
	
	// clear existing styles
	BufferLine* bl = b->first;
	for(int i = 0; i < b->numLines && bl; i++) {
		VEC_TRUNC(&bl->style);
		
		bl = bl->next;
	}
//	printf("\n");
	h->plugin->refreshStyle(&hlc.ctx);
	
// 	printf("hl time: %f\n", timeSince(then)  * 1000.0);
}



void GBEC_SetHighlighter(GUIBufferEditControl* w, Highlighter* h) {

	w->h = h;
	
	char* ext = h->plugin->name;
	
	char* tmp = sprintfdup("%s/%s_colors.txt", w->gs->highlightStylesPath, ext);
	Highlighter_LoadStyles(w->h, tmp);
	free(tmp);

}


void GUIBufferEditControl_SetBuffer(GUIBufferEditControl* w, Buffer* b) {
	w->b = b;
	CURSOR_LINE(w->sel) = b->first;
	CURSOR_COL(w->sel) = 0;
	
	Buffer_RegisterChangeListener(b, bufferChangeNotify, w);
}



void GBEC_ReplayMacro(GUIBufferEditControl* w, int index) {
	int needRehighlight;
	
	if(index >= RING_LEN(&w->macros)) {
		return;
	}
	
	BufferEditorMacro* m = &RING_ITEM(&w->macros, index);
	
	VEC_EACHP(&m->cmds, i, cmd) {
		
		GBEC_ProcessCommand(w, cmd, &needRehighlight);
			
		if(needRehighlight || cmd->flags & GUICMD_FLAG_rehighlight) {
			GUIBufferEditControl_MarkRefreshHighlight(w);
		}

		needRehighlight = 0;
	}
}


int GBEC_ProcessCommand(GUIBufferEditControl* w, GUI_Cmd* cmd, int* needRehighlight) {
	Buffer* b = w->b;
	Buffer* b2 = NULL;
	
	// keep this command around if a macro is being recorded
	if(w->isRecording && cmd->cmd != GUICMD_Buffer_MacroToggleRecording) {
		BufferEditorMacro* m = &RING_HEAD(&w->macros);
		VEC_PUSH(&m->cmds, *cmd);
	}
	
	
	char cc[2] = {cmd->amt, 0};
	
	switch(cmd->cmd) {
	
		case GUICMD_Buffer_MacroToggleRecording: 
			w->isRecording = !w->isRecording;
			if(w->isRecording) {
				RING_PUSH(&w->macros, (BufferEditorMacro){});
				BufferEditorMacro* m = &RING_HEAD(&w->macros);
				VEC_TRUNC(&m->cmds);
			}
			break;
			
		case GUICMD_Buffer_MacroReplay: 
			GBEC_ReplayMacro(w, cmd->amt + w->isRecording);
			break;
	
		case GUICMD_Buffer_MoveCursorV:
			GBEC_ClearAllSelections(w);
			GBEC_MoveCursorV(w, w->sel, cmd->amt);
			break;		
		
		case GUICMD_Buffer_MoveCursorH:
			GBEC_ClearAllSelections(w);
			GBEC_MoveCursorH(w, w->sel, cmd->amt);
			break;
		
		case GUICMD_Buffer_GrowSelectionH:
			if(!HAS_SELECTION(w->sel)) {
				w->sel->selecting = 1;
				PIVOT_LINE(w->sel) = CURSOR_LINE(w->sel);
				PIVOT_COL(w->sel) = CURSOR_COL(w->sel);
			}
			GBEC_MoveCursorH(w, w->sel, cmd->amt);
			GBEC_SetSelectionFromPivot(w);
			break;
		
		case GUICMD_Buffer_GrowSelectionToNextSequence:
			if(!HAS_SELECTION(w->sel)) {
				w->sel->selecting = 1;
				PIVOT_LINE(w->sel) = CURSOR_LINE(w->sel);
				PIVOT_COL(w->sel) = CURSOR_COL(w->sel);
			}
			GBEC_MoveToNextSequence(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), cmd->str);
			GBEC_SetSelectionFromPivot(w);
			break;
		
		case GUICMD_Buffer_GrowSelectionToPrevSequence:
			if(!HAS_SELECTION(w->sel)) {
				w->sel->selecting = 1;
				PIVOT_LINE(w->sel) = CURSOR_LINE(w->sel);
				PIVOT_COL(w->sel) = CURSOR_COL(w->sel);
			}
			GBEC_MoveToPrevSequence(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), cmd->str);
			GBEC_SetSelectionFromPivot(w);
			break;
		
		case GUICMD_Buffer_GrowSelectionToSOL:
			if(!HAS_SELECTION(w->sel)) {
				w->sel->selecting = 1;
				PIVOT_LINE(w->sel) = CURSOR_LINE(w->sel);
				PIVOT_COL(w->sel) = CURSOR_COL(w->sel);
			}
			GBEC_MoveToFirstCharOrSOL(w, CURSOR_LINE(w->sel));
			GBEC_SetSelectionFromPivot(w);
			break;
		
		case GUICMD_Buffer_GrowSelectionToEOL:
			if(!HAS_SELECTION(w->sel)) {
				w->sel->selecting = 1;
				PIVOT_LINE(w->sel) = CURSOR_LINE(w->sel);
				PIVOT_COL(w->sel) = CURSOR_COL(w->sel);
			}
			GBEC_MoveToLastCharOfLine(w, CURSOR_LINE(w->sel));
			GBEC_SetSelectionFromPivot(w);
			break;
			
		
		case GUICMD_Buffer_GrowSelectionV:
			if(!HAS_SELECTION(w->sel)) {
				w->sel->selecting = 1;
				PIVOT_LINE(w->sel) = CURSOR_LINE(w->sel);
				PIVOT_COL(w->sel) = CURSOR_COL(w->sel);
			}
			GBEC_MoveCursorV(w, w->sel, cmd->amt);
			GBEC_SetSelectionFromPivot(w);
			break;
		
		case GUICMD_Buffer_ClearSelection:
			if(HAS_SELECTION(w->sel)) GBEC_ClearAllSelections(w);
			break;
			
		case GUICMD_Buffer_SelectSequenceUnder:
			GBEC_SelectSequenceUnder(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), cmd->str); 
			break;
			
		case GUICMD_Buffer_MoveToNextSequence:
			GBEC_MoveToNextSequence(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), cmd->str);
			break;
			
		case GUICMD_Buffer_MoveToPrevSequence:
			GBEC_MoveToPrevSequence(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), cmd->str);
			break;
			
		case GUICMD_Buffer_DeleteToNextSequence:
			GBEC_DeleteToNextSequence(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), cmd->str);
			break;
			
		case GUICMD_Buffer_DeleteToPrevSequence:
			GBEC_DeleteToPrevSequence(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), cmd->str);
			break;
			
		case GUICMD_Buffer_PrependToSequence:
			GBEC_PrependToSequence(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), cmd->pstr[0], cmd->pstr[1]);
			GBEC_MoveCursorH(w, w->sel, strlen(cmd->pstr[1]));
			break;
		
		case GUICMD_Buffer_AppendToSequence:
			GBEC_AppendToSequence(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), cmd->pstr[0], cmd->pstr[1]);
			break;
			
		case GUICMD_Buffer_SurroundSequence:
			GBEC_SurroundSequence(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), cmd->pstr[0], cmd->pstr[1], cmd->pstr[2]);
			GBEC_MoveCursorH(w, w->sel, strlen(cmd->pstr[1]));
			break;
		
		case GUICMD_Buffer_GoToEOL:
			if(HAS_SELECTION(w->sel)) GBEC_ClearAllSelections(w);
			CURSOR_COL(w->sel) = CURSOR_LINE(w->sel)->length;
			break;
			
		case GUICMD_Buffer_GoToSOL:
			if(HAS_SELECTION(w->sel)) GBEC_ClearAllSelections(w);
			CURSOR_COL(w->sel) = 0;
			break;
		
		case GUICMD_Buffer_GoToAfterIndent:
			if(HAS_SELECTION(w->sel)) GBEC_ClearAllSelections(w);
			GBEC_MoveCursorTo(w, CURSOR_LINE(w->sel), BufferLine_GetIndentCol(CURSOR_LINE(w->sel)));
			break;

/*
		case GUICMD_Buffer_MoveCursorHSel:
			GBEC_MoveCursorHSel(w, cmd->amt);
			GBEC_ClearAllSelections(w);
			break;
*/		
		case GUICMD_Buffer_ScrollLinesV:
			GBEC_ScrollDir(w, cmd->amt, 0);
			break;
			
		case GUICMD_Buffer_ScrollScreenPctV:
			GBEC_ScrollDir(w, ((float)cmd->amt * 0.1) * w->linesOnScreen, 0);
			break;
			
		case GUICMD_Buffer_ScrollColsH:
			GBEC_ScrollDir(w, 0, cmd->amt);
			break;
			
		case GUICMD_Buffer_ScrollScreenPctH:
			GBEC_ScrollDir(w, 0, ((float)cmd->amt * 0.1) * w->colsOnScreen);
			break;
			
		case GUICMD_Buffer_MovePage:
			GBEC_ClearAllSelections(w);
			GBEC_MoveCursorV(w, w->sel, cmd->amt * w->linesOnScreen);
			w->scrollLines = MAX(0, MIN(w->scrollLines + cmd->amt * w->linesOnScreen, w->b->numLines - 1));
			break;
		
		case GUICMD_Buffer_InsertChar:
			GBEC_InsertCharsMC(w, cc, 1);
			GBEC_MoveCursorHMC(w, 1);
			break;
			
		case GUICMD_Buffer_InsertString: {
			size_t len = strlen(cmd->amt);
			GBEC_InsertCharsMC(w, cmd->amt, len);
			GBEC_MoveCursorHMC(w, len);
			break;
		}
		
		case GUICMD_Buffer_Backspace:
			if(HAS_SELECTION(w->sel)) {
				Buffer_UndoSequenceBreak(b, 0, CURSOR_LINE(w->sel)->lineNum, CURSOR_COL(w->sel), 
					PIVOT_LINE(w->sel)->lineNum, PIVOT_COL(w->sel), 0);
				Buffer_DeleteSelectionContents(b, w->sel);
				GBEC_MoveCursorTo(w, w->sel->line[0], w->sel->col[0]); // correct;
				
				GBEC_ClearAllSelections(w);
			}
			else {
				BufferLine* bl = CURSOR_LINE(w->sel);
				intptr_t col = CURSOR_COL(w->sel);
				GBEC_MoveCursorH(w, w->sel, -1);
				Buffer_BackspaceAt(b, bl, col);		
			}
			break;
		
		case GUICMD_Buffer_Delete:
			if(HAS_SELECTION(w->sel)) {
				Buffer_DeleteSelectionContents(b, w->sel);
				GBEC_MoveCursorTo(w, w->sel->line[0], w->sel->col[0]); // correct;
				GBEC_ClearAllSelections(w);
			}
			else {
				Buffer_DeleteAt(b, CURSOR_LINE(w->sel), CURSOR_COL(w->sel));
			}
			break;
		
		case GUICMD_Buffer_SplitLine:
			GBEC_InsertLinebreak(w);
			break;

		case GUICMD_Buffer_SplitLineIndent:
			GBEC_InsertLinebreak(w);
			intptr_t tabs = Buffer_IndentToPrevLine(b, CURSOR_LINE(w->sel));
			GBEC_MoveCursorTo(w, CURSOR_LINE(w->sel), tabs);
			break;
		
		case GUICMD_Buffer_DeleteCurLine: {
			// preserve proper cursor position
			if(HAS_SELECTION(w->sel)) {
				BufferLine* cur;
				
				if(CURSOR_LINE(w->sel)->lineNum < PIVOT_LINE(w->sel)->lineNum) {
					cur = CURSOR_LINE(w->sel)->prev;
					if(!cur) cur = PIVOT_LINE(w->sel)->next;
				}
				else {
					cur = CURSOR_LINE(w->sel)->next;
					if(!cur) cur = PIVOT_LINE(w->sel)->prev;
				}
				intptr_t col = CURSOR_COL(w->sel);
				
				assert(w->sel->line[0]);
				assert(w->sel->line[1]);
				assert(w->sel->col[0] >= 0);
				assert(w->sel->col[1] >= 0);
				
				BufferLine* bl = w->sel->line[0];
				BufferLine* next;
				BufferLine* last = w->sel->line[1];
				if(w->sel->col[1] == 0) {
					last = last->prev;
					cur = PIVOT_LINE(w->sel);
				}
				
				while(bl) {
					next = bl->next;
					Buffer_DeleteLine(b, bl);
					
					if(bl == last) break;
					bl = next;
				}
				
				GBEC_ClearCurrentSelection(w);
				
				if(cur) GBEC_MoveCursorTo(w, cur, col);
			}
			else {
				BufferLine* cur = CURSOR_LINE(w->sel)->next;
				if(!cur) cur = CURSOR_LINE(w->sel)->prev;
				intptr_t col = CURSOR_COL(w->sel);
				
				Buffer_DeleteLine(b, CURSOR_LINE(w->sel));
				
				if(cur) GBEC_MoveCursorTo(w, cur, col);
			}
			break;
		}
		
		case GUICMD_Buffer_LinePrependText:
			if(HAS_SELECTION(w->sel)) {
				Buffer_LinePrependTextSelection(b, w->sel, cmd->str);
			}
			else {
				Buffer_LinePrependText(b, CURSOR_LINE(w->sel), cmd->str);
			}
			break;
			
		case GUICMD_Buffer_LineAppendText:
			if(HAS_SELECTION(w->sel)) {
				Buffer_LineAppendTextSelection(b, w->sel, cmd->str, strlen(cmd->str));
			}
			else {
				Buffer_LineAppendText(b, CURSOR_LINE(w->sel), cmd->str, strlen(cmd->str));
			}
			break;
			
		case GUICMD_Buffer_LineEnsureEnding:
			if(HAS_SELECTION(w->sel)) {
				Buffer_LineEnsureEndingSelection(b, w->sel, cmd->str, strlen(cmd->str));
			}
			else {
				Buffer_LineEnsureEnding(b, CURSOR_LINE(w->sel), cmd->str, strlen(cmd->str));
			}
			break;
			
		case GUICMD_Buffer_SurroundSelection:
			if(HAS_SELECTION(w->sel))
				GBEC_SurroundCurrentSelection(w, cmd->pstr[0], cmd->pstr[1]);
			break;
			
		case GUICMD_Buffer_ReplaceLineWithSelectionTransform:
			if(HAS_SELECTION(w->sel))
				GBEC_ReplaceLineWithSelectionTransform(w, cmd->pstr[0], cmd->pstr[1], cmd->pstr[2]);
			break;
		
		case GUICMD_Buffer_SmartComment:
			if(!cmd->pstr[0] || !cmd->pstr[1] || !cmd->pstr[2]) break;
			if(!HAS_SELECTION(w->sel)) { // simple case
				Buffer_LinePrependText(b, CURSOR_LINE(w->sel), cmd->pstr[0]);
				break;
			}
			
			if(BufferRange_CompleteLinesOnly(w->sel)) {
				Buffer_LinePrependTextSelection(b, w->sel, cmd->pstr[0]);
			}
			else {
				GBEC_SurroundCurrentSelection(w, cmd->pstr[1], cmd->pstr[2]);
			}
			break;
		
		case GUICMD_Buffer_LineUnprependText:
			if(HAS_SELECTION(w->sel)) {
				Buffer_LineUnprependTextSelection(b, w->sel, cmd->str);
			}
			else {
				Buffer_LineUnprependText(b, CURSOR_LINE(w->sel), cmd->str);
			}
			break;
			
		case GUICMD_Buffer_UnsurroundSelection:
			if(HAS_SELECTION(w->sel))
				GBEC_UnsurroundCurrentSelection(w, cmd->pstr[0], cmd->pstr[1]);
			break;
		
		case GUICMD_Buffer_StrictUncomment:
			if(!cmd->pstr[0] || !cmd->pstr[1] || !cmd->pstr[2]) break;
			if(!HAS_SELECTION(w->sel)) { // simple case
				Buffer_LineUnprependText(b, CURSOR_LINE(w->sel), cmd->pstr[0]);
				break;
			}
			
			if(BufferRange_CompleteLinesOnly(w->sel)) {
				Buffer_LineUnprependTextSelection(b, w->sel, cmd->pstr[0]);
			}
			else {
				GBEC_UnsurroundCurrentSelection(w, cmd->pstr[1], cmd->pstr[2]);
			}
			break;
		
		case GUICMD_Buffer_GoToFirstCharOrSOL:
			GBEC_MoveToFirstCharOrSOL(w, CURSOR_LINE(w->sel));
			break;
		
		case GUICMD_Buffer_GoToFirstCharOfLine: 
			GBEC_MoveToFirstCharOfLine(w, CURSOR_LINE(w->sel));
			break;
			
		case GUICMD_Buffer_GoToLastCharOfLine:
			GBEC_MoveToLastCharOfLine(w, CURSOR_LINE(w->sel));
			break;
		
		case GUICMD_Buffer_GoToFirstColOfFile:
			// TODO: undo
			GBEC_MoveCursorTo(w, b->first, 0);
			break;
		
		case GUICMD_Buffer_GoToLastColOfFile:
			// TODO: undo
			CURSOR_LINE(w->sel) = b->last;
			if(b->last) CURSOR_COL(w->sel) = b->last->length;
			break;
			
		case GUICMD_Buffer_Indent:
			if(HAS_SELECTION(w->sel)) {
				Buffer_IndentSelection(w->b, w->sel);
			}
			else {
				Buffer_LineIndent(w->b, CURSOR_LINE(w->sel));
				GBEC_MoveCursorH(w, w->sel, 1);
			}		
			break;
			
		case GUICMD_Buffer_Unindent: 
			if(HAS_SELECTION(w->sel)) {
				Buffer_UnindentSelection(w->b, w->sel);
			}
			else {
				Buffer_LineUnindent(w->b, CURSOR_LINE(w->sel)); 
			}
			break;
			
		case GUICMD_Buffer_DuplicateLine:
			if(HAS_SELECTION(w->sel)) {
				Buffer_DuplicateSelection(b, w->sel, cmd->amt);
				// inconsistent with single-line version, but not sure how this one should work
				// maybe the DuplicateLines function should have an "above" or "below" option.
//				GBEC_MoveCursorTo(w, PIVOT_LINE(w->sel), PIVOT_COL(w->sel));
			}
			else {
				Buffer_DuplicateLines(b, CURSOR_LINE(w->sel), cmd->amt);
				GBEC_MoveCursorV(w, w->sel, cmd->amt);
			}
			break;
		
		case GUICMD_Buffer_Copy:
			if(HAS_SELECTION(w->sel)) {
				b2 = Buffer_FromSelection(b, w->sel);
				Clipboard_PushBuffer(cmd->amt/*CLIP_PRIMARY*/, b2);
			}
			break;
		
		case GUICMD_Buffer_Cut:
			if(HAS_SELECTION(w->sel)) {
				b2 = Buffer_FromSelection(b, w->sel);
				Clipboard_PushBuffer(cmd->amt/*CLIP_PRIMARY*/, b2);
				// TODO: move cursor to cut spot, if it isn't already
				if(!w->sel->cursor) {
					// TODO current
//					w->current = w->sel->line[0];
//					w->curCol = w->sel->col[0];
				}
				
				Buffer_DeleteSelectionContents(b, w->sel);
				GBEC_ClearAllSelections(w);
			}
			break;
		
		case GUICMD_Buffer_SmartCut: {
			int smart = 0;
			if(!HAS_SELECTION(w->sel)) {
				// make the current line the selection
				GBEC_SetCurrentSelection(w, CURSOR_LINE(w->sel), 0, CURSOR_LINE(w->sel), CURSOR_LINE(w->sel)->length);
				smart = 1;
			}
			if(HAS_SELECTION(w->sel)) {
				// stolen from GUICMD_Buffer_Cut
				b2 = Buffer_FromSelection(b, w->sel);
				Clipboard_PushBuffer(cmd->amt, b2);
				// TODO: move cursor to cut spot, if it isn't already
				if(!w->sel->cursor) {
//					w->current = w->sel->line[0];
//					w->curCol = w->sel->col[0];
				}
				
				Buffer_DeleteSelectionContents(b, w->sel);
				GBEC_ClearAllSelections(w);
			}
			if(smart) {
				// stolen from GUICMD_Buffer_DeleteCurLine
				// preserve proper cursor position
				BufferLine* cur = CURSOR_LINE(w->sel)->next;
				if(!cur) cur = CURSOR_LINE(w->sel)->prev;
				intptr_t col = CURSOR_COL(w->sel);
				
				Buffer_DeleteLine(b, CURSOR_LINE(w->sel));
				
				if(cur) GBEC_MoveCursorTo(w, cur, col);
			}
			break;
		}
		
		case GUICMD_Buffer_Paste:
			b2 = Clipboard_PopBuffer(cmd->amt/*CLIP_PRIMARY*/);
			if(b2) {
				if(HAS_SELECTION(w->sel)) {
					GBEC_MoveCursorTo(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel));
					Buffer_DeleteSelectionContents(b, w->sel);
					GBEC_ClearAllSelections(w);
				}
				
				// TODO: undo
				BufferRange pasteRange;
				Buffer_InsertBufferAt(b, b2, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), &pasteRange);
				
				GBEC_MoveCursorTo(w, pasteRange.line[1], pasteRange.col[1]);
			}
			break;
		
		case GUICMD_Buffer_SelectNone:
			GBEC_ClearAllSelections(w);
			break;
		
		case GUICMD_Buffer_SelectAll:
			GBEC_SetCurrentSelection(w, b->first, 1, b->last, b->last->length+1);
			break;
		
		case GUICMD_Buffer_SelectLine:
			GBEC_SetCurrentSelection(w, CURSOR_LINE(w->sel), 1, CURSOR_LINE(w->sel), CURSOR_LINE(w->sel)->length);
			break;
			
		case GUICMD_Buffer_SelectToEOL:
			GBEC_SetCurrentSelection(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel), CURSOR_LINE(w->sel), CURSOR_LINE(w->sel)->length);
			break;
			
		case GUICMD_Buffer_SelectFromSOL:
			GBEC_SetCurrentSelection(w, CURSOR_LINE(w->sel), 1, CURSOR_LINE(w->sel), CURSOR_COL(w->sel));
			break;
			
		case GUICMD_Buffer_PushCursor:
			GBEC_PushCursor(w, CURSOR_LINE(w->sel), CURSOR_COL(w->sel));
			break;
		
		case GUICMD_Buffer_PrintACDict:
			printf("Autocomplete Dictionary:\n");
			Buffer_PrintDict(b);
			break;
		
		case GUICMD_Buffer_ACProvoke:
			w->autocompleteOptions = Buffer_FindDictMatches(b, w->sel);
			if(w->autocompleteOptions) {
				w->showAutocomplete = 1;
				w->inputState.mode = 11;
			}
			break;
			
		case GUICMD_Buffer_ACMoveCursor:
			if(!w->autocompleteOptions) {
				w->inputState.mode = 0;
				GBEC_CancelAutocomplete(w);
				break;
			}
			
			w->inputState.mode = 11; // move from 10 to 11 if needed
			w->autocompleteSelectedItem += cmd->amt;
			w->autocompleteSelectedItem = MAX(0, MIN(w->autocompleteSelectedItem, w->autocompleteOptions->len - 1));
			break;
			
		case GUICMD_Buffer_ACReplaceWithSelected: {
			BufferACMatchSet* ms = w->autocompleteOptions;
			if(!w->autocompleteOptions) {
				w->inputState.mode = 0;
				GBEC_CancelAutocomplete(w);
				break;
			}
			
			Buffer_DeleteSelectionContents(b, &w->autocompleteOptions->target);
			
			Buffer_LineInsertChars(b, 
				ms->target.line[0], 
				ms->matches[w->autocompleteSelectedItem].s,
				ms->target.col[0], 
				ms->matches[w->autocompleteSelectedItem].len
			);
			
			GBEC_MoveCursorTo(w, ms->target.line[0], ms->target.col[0] + ms->matches[w->autocompleteSelectedItem].len);
			
			w->inputState.mode = 0;
			GBEC_CancelAutocomplete(w);
			break;
		}
		case GUICMD_Buffer_ACCompletePrefix: {
			BufferACMatchSet* ms = w->autocompleteOptions;
//			dbg("pref len %d", ms->commonPrefixLen);
			if(ms->commonPrefixLen <= 0) break;
			if(ms->commonPrefixLen <= ms->targetLen) break;
			
			if(!ms) {
				w->inputState.mode = 0;
				GBEC_CancelAutocomplete(w);
				break;
			}
			
			Buffer_DeleteSelectionContents(b, &ms->target);
			
			Buffer_LineInsertChars(b, 
				ms->target.line[0], 
				ms->matches[0].s,
				ms->target.col[0], 
				ms->commonPrefixLen
			);
			
			GBEC_MoveCursorTo(w, ms->target.line[0], ms->target.col[0] + ms->commonPrefixLen);
			
			// retry autocomplete with the new location
			GBEC_CancelAutocomplete(w);
			w->autocompleteOptions = Buffer_FindDictMatches(b, w->sel);
			if(w->autocompleteOptions) {
				w->showAutocomplete = 1;
				w->inputState.mode = 11;
			}
			
			break;
		}
		case GUICMD_Buffer_ACCancel:
			w->inputState.mode = 0;
			GBEC_CancelAutocomplete(w);
			break;
		
		case GUICMD_Buffer_SetBookmark:       Buffer_SetBookmarkAt(b, CURSOR_LINE(w->sel));    break; 
		case GUICMD_Buffer_RemoveBookmark:    Buffer_RemoveBookmarkAt(b, CURSOR_LINE(w->sel)); break; 
		case GUICMD_Buffer_ToggleBookmark:    Buffer_ToggleBookmarkAt(b, CURSOR_LINE(w->sel)); break; 
		
		case GUICMD_Buffer_GoToNextBookmark:  
			GBEC_ClearAllSelections(w);
			GBEC_NextBookmark(w);
			break; 
		case GUICMD_Buffer_GoToPrevBookmark:
			GBEC_ClearAllSelections(w);
			GBEC_PrevBookmark(w);
			break;
			 
		case GUICMD_Buffer_GoToFirstBookmark: 
			GBEC_ClearAllSelections(w);
			GBEC_FirstBookmark(w);
			break; 
		
		case GUICMD_Buffer_GoToLastBookmark:
			GBEC_ClearAllSelections(w);
			GBEC_LastBookmark(w);
			break; 
		
		default:
			return Buffer_ProcessCommand(w->b, cmd, needRehighlight);
	}


	// perhaps an ugly hack:
	if(w->autocompleteOptions) {
		if(
			CURSOR_LINE(w->sel)->lineNum != CURSOR_LINE(&w->autocompleteOptions->r)->lineNum
			|| CURSOR_COL(w->sel) != CURSOR_COL(&w->autocompleteOptions->r)
		) {
//			w->inputMode = 0;
			GBEC_CancelAutocomplete(w);
		}
	}
	
	// check flags
	
	if(cmd->flags & GUICMD_FLAG_scrollToCursor) {
		GBEC_scrollToCursor(w);
	}
	
	if(cmd->flags & GUICMD_FLAG_rehighlight) {
		GUIBufferEditControl_MarkRefreshHighlight(w);
	}
	
	if(cmd->flags & GUICMD_FLAG_resetCursorBlink) {
		w->cursorBlinkTimer = 0;
	}
	
	if(cmd->flags & GUICMD_FLAG_undoSeqBreak) {
		if(!HAS_SELECTION(w->sel)) {
//					printf("seq break without selection\n");
			Buffer_UndoSequenceBreak(
				w->b, 0, 
				CURSOR_LINE(w->sel)->lineNum, CURSOR_COL(w->sel),
				0, 0, 0
			);
		}
		else {
//					printf("seq break with selection\n");
			Buffer_UndoSequenceBreak(
				w->b, 0, 
				CURSOR_LINE(w->sel)->lineNum, CURSOR_COL(w->sel),
				PIVOT_LINE(w->sel)->lineNum, PIVOT_COL(w->sel),
				w->sel->cursor // TODO check pivot locations
			);
		}			
	}
	
	if(cmd->flags & GUICMD_FLAG_centerOnCursor) {
		GBEC_scrollToCursorOpt(w, 1);
	}
	
	if(cmd->flags & GUICMD_FLAG_provokeAC) {
		GBEC_CancelAutocomplete(w);
		
		w->autocompleteOptions = Buffer_FindDictMatches(b, w->sel);
		if(w->autocompleteOptions) {
			w->showAutocomplete = 1;
			if(w->inputState.mode != 11) w->inputState.mode = 10;
		}
		else {
			w->inputState.mode = 0;
		}
	}
	
	if(cmd->flags & GUICMD_FLAG_closeAC) {
		GBEC_CancelAutocomplete(w);
		w->inputState.mode = 0;
	}
	
	// more kludgy hacks
	if((w->inputState.mode == 10 || w->inputState.mode == 11) && !w->autocompleteOptions) {
		w->inputState.mode = 0;
	}
	
	return 0;
}



void GBEC_CancelAutocomplete(GUIBufferEditControl* w) {
	if(!w->autocompleteOptions) return;
	if(w->autocompleteOptions->matches) free(w->autocompleteOptions->matches);
	free(w->autocompleteOptions);
	w->autocompleteOptions = NULL;
	w->showAutocomplete = 0;
	w->autocompleteSelectedItem = 0;
}


void GBEC_MoveToNextSequence(GUIBufferEditControl* w, BufferLine* l, colnum_t col, char* charSet) {
	if(!l || col > l->length) return;
	
	// TODO: handle selection size changes
	if(!l) return;
	
	Buffer_FindSequenceEdgeForward(w->b, &l, &col, charSet);
	GBEC_MoveCursorTo(w, l, col);
}

void GBEC_MoveToPrevSequence(GUIBufferEditControl* w, BufferLine* l, colnum_t col, char* charSet) {
	if(!l || col > l->length) return;
	
	// TODO: handle selection size changes
	if(!l) return;
	
	Buffer_FindSequenceEdgeBackward(w->b, &l, &col, charSet);
	GBEC_MoveCursorTo(w, l, col);
	
	return;
}

void GBEC_MoveToFirstCharOrSOL(GUIBufferEditControl* w, BufferLine* bl) {
	
	for(intptr_t i = 0; i < bl->length; i++) {
		if(!isspace(bl->buf[i])) {
			if(i == CURSOR_COL(w->sel)) {
				GBEC_MoveCursorTo(w, bl, 0);
			} else {
				GBEC_MoveCursorTo(w, bl, i);
			}
			return;
		}
	}
	
	// TODO: multicursor
	GBEC_MoveCursorTo(w, CURSOR_LINE(w->sel), 0);
}

void GBEC_MoveToFirstCharOfLine(GUIBufferEditControl* w, BufferLine* bl) {
	
	for(intptr_t i = 0; i < bl->length; i++) {
		if(!isspace(bl->buf[i])) {
			GBEC_MoveCursorTo(w, bl, i);
			return;
		}
	}
	// TODO: multicursor
	GBEC_MoveCursorTo(w, CURSOR_LINE(w->sel), 0);
}

void GBEC_MoveToLastCharOfLine(GUIBufferEditControl* w, BufferLine* bl) {
	
	for(intptr_t i = bl->length - 1; i >= 0; i--) {
		if(!isspace(bl->buf[i])) {
			GBEC_MoveCursorTo(w, bl, i + 1);
			return;
		}
	}
	// TODO: multicursor
	GBEC_MoveCursorTo(w, bl, bl->length);
}


// TODO: undo
void GBEC_ClearCurrentSelection(GUIBufferEditControl* w) {
	if(HAS_SELECTION(w->sel)) {
		// TODO current
		w->sel->selecting = 0;
		PIVOT_LINE(w->sel) = NULL;
		PIVOT_COL(w->sel) = -1;
	}
	
	// TODO current
	
	// TODO: multicursor
	GBEC_SelectionChanged(w);
}


void GBEC_SetCurrentSelection(GUIBufferEditControl* w, BufferLine* startL, colnum_t startC, BufferLine* endL, colnum_t endC) {

	// TODO: undo
	
	assert(startL != NULL);
	assert(endL != NULL);
	assert(w->sel != NULL);
	
	startC = MIN(startL->length, MAX(startC, 0));
	endC = MIN(endL->length, MAX(endC, 0));
	
	w->sel->line[0] = startL;
	w->sel->line[1] = endL;
	w->sel->col[0] = startC;
	w->sel->col[1] = endC;
	
	BufferRange_Normalize(w->sel);
	
	GBEC_SelectionChanged(w);
}

void GBEC_SetCurrentSelectionRange(GUIBufferEditControl* w, BufferRange* r) {
	GBEC_SetCurrentSelection(w, r->line[0], r->col[0], r->line[1], r->col[1]);
}


void GBEC_InsertLinebreak(GUIBufferEditControl* w) {
	BufferLine* l = CURSOR_LINE(w->sel);
	Buffer* b = w->b;
	// TODO multicursor
	
	if(CURSOR_COL(w->sel) == 0) {
		Buffer_InsertEmptyLineBefore(b, CURSOR_LINE(w->sel));
	}
	else {
		BufferLine* n = Buffer_InsertLineAfter(b, l, l->buf + CURSOR_COL(w->sel), MAX(l->length - CURSOR_COL(w->sel), 0));
		Buffer_LineTruncateAfter(b, l, CURSOR_COL(w->sel));
		
		CURSOR_LINE(w->sel) = CURSOR_LINE(w->sel)->next;
		// TODO: undo cursor move
	}
	
	GBEC_MoveCursorTo(w, CURSOR_LINE(w->sel), 0);
	
	// TODO: undo
	// TODO: maybe shrink the alloc
}


void GBEC_ClearAllSelections(GUIBufferEditControl* w) {
	VEC_EACH(&w->selSet->ranges, i, r) {
		r->selecting = 0;
		PIVOT_LINE(r) = NULL;
		PIVOT_COL(r) = -1;
	}
	
	GBEC_SelectionChanged(w);
}


// also fixes cursor and selection
void GBEC_SurroundCurrentSelection(GUIBufferEditControl* w, char* begin, char* end) {
	VEC_EACH(&w->selSet->ranges, i, r) {
		GBEC_SurroundRange(w, r, begin, end);
	}
}

void GBEC_SurroundRange(GUIBufferEditControl* w, BufferRange* r, char* begin, char* end) {
	Buffer* b = w->b;

	if(!HAS_SELECTION(r) || !begin) return;
	 
	Buffer_SurroundSelection(b, r, begin, end);
		
	// fix the selection to include the new text
	size_t len = strlen(begin) + strlen(end);
	if(len) {
		GBEC_MoveEndH(w, r, len);
	}
}

// also fixes cursor and selection
void GBEC_UnsurroundCurrentSelection(GUIBufferEditControl* w, char* begin, char* end) {
	VEC_EACH(&w->selSet->ranges, i, r) {
		GBEC_UnsurroundRange(w, r, begin, end);
	}
}

void GBEC_UnsurroundRange(GUIBufferEditControl* w, BufferRange* r, char* begin, char* end) {
	Buffer* b = w->b;

	int res = Buffer_UnsurroundSelection(b, r, begin, end);
	if(res == 0) return;
			
	// fix the selection to include the new text
	size_t len1 = strlen(begin);
	size_t len2 = strlen(end);
	
	if(len1) GBEC_MoveStartH(w, r, len1);
	if(len2) GBEC_MoveEndH(w, r, -len2);
}


void GBEC_PrependToSequence(GUIBufferEditControl* w, BufferLine* line, colnum_t col, char* seq, char* str) {
	Buffer_FindSequenceEdgeBackward(w->b, &line, &col, seq);
	Buffer_LineInsertChars(w->b, line, str, col, strlen(str));
}

void GBEC_AppendToSequence(GUIBufferEditControl* w, BufferLine* line, colnum_t col, char* seq, char* str) {
	Buffer_FindSequenceEdgeForward(w->b, &line, &col, seq);
	Buffer_LineInsertChars(w->b, line, str, col, strlen(str));
}

void GBEC_SurroundSequence(GUIBufferEditControl* w, BufferLine* line, colnum_t col, char* seq, char* left, char* right) {
	GBEC_AppendToSequence(w, line, col, seq, right);
	GBEC_PrependToSequence(w, line, col, seq, left);
}


void GBEC_ReplaceLineWithSelectionTransform(
	GUIBufferEditControl* w, 
	char* selectionToken,
	char* cursorToken,
	char* format
) {
	BufferRange cursor;
	
	Buffer* f = Buffer_New(NULL);
	Buffer_AppendRawText(f, format, strlen(format));
	
	// find first cursor token and remove it
	int haveCursor = !Buffer_strstr(f, cursorToken, &cursor);
	if(haveCursor) {
		// delete the token itself; start l/c still hold the cursor pos
		Buffer_DeleteSelectionContents(f, &cursor);
	}
	
	BufferRange sel = *w->sel;
	GBEC_ClearCurrentSelection(w);
	GBEC_MoveCursorTo(w, sel.line[0], sel.col[0]);
	
	Buffer* sb = Buffer_FromSelection(w->b, &sel);
	
	Buffer_ReplaceAllString(f, selectionToken, sb);
	
	sel.col[0] = 0;
	sel.col[1] = sel.line[1]->length;
	Buffer_DeleteSelectionContents(w->b, &sel);
	Buffer_InsertBufferAt(w->b, f, w->sel->line[0], w->sel->col[0], NULL);
	
	if(haveCursor) {
		// BUG: may be very broken post-multicursor
		GBEC_MoveCursorTo(w, cursor.line[0], cursor.col[0]);
//		GBEC_MoveCursorV(w, cursor.line[0]->lineNum - 1);
//		GBEC_MoveCursorH(w, cursor.col[0]);
		
	}
	
}




colnum_t GBEC_NominalColFromVisual(GUIBufferEditControl* w, BufferLine* bl, colnum_t wanted) {
	if(bl->buf == NULL) return 0;
	
	int tabwidth = w->bs->tabWidth;
	intptr_t screenCol = 0;
	intptr_t charCol = 0;
	
	while(screenCol < wanted && charCol < bl->length) {
		if(bl->buf[charCol] == '\t') screenCol += tabwidth;
		else screenCol++;
		charCol++;
	}
	
	return MAX(0, MIN(charCol, bl->length));
}


colnum_t GBEC_VisualColFromNominal(GUIBufferEditControl* w, BufferLine* bl, colnum_t col) {
	if(bl->buf == NULL) return 0;
	
	int tabwidth = w->bs->tabWidth;
	intptr_t screenCol = 0;
	
	
	for(intptr_t charCol = 0; charCol < col && charCol < bl->length; charCol++) {
		if(bl->buf[charCol] == '\t') screenCol += tabwidth;
		else screenCol++;
	}
	
	return MAX(0, screenCol);
}


void GBEC_MoveMarkerH(GUIBufferEditControl* w, BufferRange* r, int c, colnum_t cols) {
	// TODO: undo
	Buffer_RelPosH(r->line[c], r->col[c], cols, &r->line[c], &r->col[c]);
	BufferRange_Normalize(r);
}


void GBEC_MoveCursorH(GUIBufferEditControl* w, BufferRange* r, colnum_t cols) {
	GBEC_MoveMarkerH(w, r, r->cursor, cols);
		
	// set the wanted column
	r->colWanted = GBEC_VisualColFromNominal(w, CURSOR_LINE(r), CURSOR_COL(r));
}
void GBEC_MovePivotH(GUIBufferEditControl* w, BufferRange* r, colnum_t cols) {
	GBEC_MoveMarkerH(w, r, !r->cursor, cols);
}
void GBEC_MoveStartH(GUIBufferEditControl* w, BufferRange* r, colnum_t cols) {
	GBEC_MoveMarkerH(w, r, 0, cols);
}
void GBEC_MoveEndH(GUIBufferEditControl* w, BufferRange* r, colnum_t cols) {
	GBEC_MoveMarkerH(w, r, 1, cols);
}



void GBEC_MoveMarkerV(GUIBufferEditControl* w, BufferRange* r, int c, linenum_t lines) {
	// TODO: undo
	BufferLine* old_line = r->line[c];
	Buffer_RelPosV(r->line[c], r->col[c], lines, &r->line[c], &r->col[c]);
	
	// jump to start/end if already on first/last line
	if(r->line[c] == old_line) {
		if(lines > 0) r->col[c] = r->line[c]->length;
		else if(lines < 0) r->col[c] = 0;
		r->colWanted = r->col[c];
	}
	
	BufferRange_Normalize(r);
}


void GBEC_MoveCursorV(GUIBufferEditControl* w, BufferRange* r, linenum_t lines) {
	GBEC_MoveMarkerV(w, r, r->cursor, lines);
	
	// adjust for wanted column
	CURSOR_COL(r) = MIN(CURSOR_LINE(r)->length, GBEC_NominalColFromVisual(w, CURSOR_LINE(r), r->colWanted));
}
void GBEC_MovePivotV(GUIBufferEditControl* w, BufferRange* r, linenum_t lines) {
	GBEC_MoveMarkerV(w, r, !r->cursor, lines);
}
void GBEC_MoveStartV(GUIBufferEditControl* w, BufferRange* r, linenum_t lines) {
	GBEC_MoveMarkerV(w, r, 0, lines);
}
void GBEC_MoveEndV(GUIBufferEditControl* w, BufferRange* r, linenum_t lines) {
	GBEC_MoveMarkerV(w, r, 1, lines);
}



//void GBEC_MoveCursorH(GUIBufferEditControl* w, colnum_t cols) {
//	BufferRange_MoveCursorH(w->sel, cols);
//}


/*
void GBEC_MoveCursorV(GUIBufferEditControl* w, linenum_t lines) {
	int i = lines;
	// TODO: undo
	// TODO: multicursor
	if(i > 0) while(i-- > 0 && CURSOR_LINE(w->sel)->next) {
		CURSOR_LINE(w->sel) = CURSOR_LINE(w->sel)->next;
	}
	else while(i++ < 0 && CURSOR_LINE(w->sel)->prev) {
		CURSOR_LINE(w->sel) = CURSOR_LINE(w->sel)->prev;
	}
	
	// not regular move, to preserve w->curColWanted
	CURSOR_COL(w->sel) = GBEC_NominalColFromVisual(w, CURSOR_LINE(w->sel), w->sel->colWanted);
}
*/



/* appears to be dead code
void GBEC_MoveCursorHSel(GUIBufferEditControl* w, colnum_t cols) {
	// TODO: multicursor
	
	if(HAS_SELECTION(w->sel) && cols < 0) {
		GBEC_MoveCursorTo(w, w->sel->line[0], w->sel->col[0]);
		cols++;
	} else if(w->sel->line[1] && cols > 0) {
		GBEC_MoveCursorTo(w, w->sel->line[1], w->sel->col[1]);
		cols--;
	}
	
	GBEC_MoveCursorH(w, cols);
}
*/

// absolute move
void GBEC_MoveRangeCursorTo(GUIBufferEditControl* w, BufferRange* r, BufferLine* bl, colnum_t col) {
	// TODO: undo
	
	/*
	if(PIVOT_LINE(r) == bl && PIVOT_COL(r) == col) {
		r->cursor = !r->cursor;
		return;
	}
	*/
	
	CURSOR_LINE(r) = bl;
	CURSOR_COL(r) = MIN(col, bl->length);;
	
	r->colWanted = GBEC_VisualColFromNominal(w, CURSOR_LINE(r), CURSOR_COL(r));;
	
	BufferRange_Normalize(r);
}

// absolute move
void GBEC_MoveCursorTo(GUIBufferEditControl* w, BufferLine* bl, colnum_t col) {
	// TODO: undo
	GBEC_MoveRangeCursorTo(w, w->sel, bl, col);
}

// absolute move
void GBEC_MoveCursorToNum(GUIBufferEditControl* w, linenum_t line, colnum_t col) {
	// TODO: undo
	
	BufferLine* bl;
	if(line <= 0) bl = w->b->first;
	else if(line > w->b->numLines) bl = w->b->last;
	else bl = Buffer_raw_GetLineByNum(w->b, line);
	
	GBEC_MoveRangeCursorTo(w, w->sel, bl, col);
}



void GBEC_NextBookmark(GUIBufferEditControl* w) {
	BufferLine* bl = CURSOR_LINE(w->sel)->next;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->next;
	}
	
	if(bl) GBEC_MoveCursorTo(w, bl, 0);
}

void GBEC_PrevBookmark(GUIBufferEditControl* w) {
	BufferLine* bl = CURSOR_LINE(w->sel)->prev;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->prev;
	}
	
	if(bl) GBEC_MoveCursorTo(w, bl, 0);
}

void GBEC_FirstBookmark(GUIBufferEditControl* w) {
	BufferLine* bl = w->b->first;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->next;
	}
	
	if(bl) GBEC_MoveCursorTo(w, bl, 0);
}

void GBEC_LastBookmark(GUIBufferEditControl* w) {
	BufferLine* bl = w->b->last;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->prev;
	}
	
	if(bl) GBEC_MoveCursorTo(w, bl, 0);
}


void GBEC_StopSelecting(GUIBufferEditControl* w) {
	VEC_EACH(&w->selSet->ranges, i, r) {
		r->selecting = 0;
	}
}


void GBEC_SelectSequenceUnder(GUIBufferEditControl* w, BufferLine* l, colnum_t col, char* charSet) {
	BufferRange sel;
	
	Buffer_GetSequenceUnder(w->b, l, col, charSet, &sel);
	
	GBEC_SetCurrentSelection(w, sel.line[0], sel.col[0], sel.line[1], sel.col[1]);
	w->sel->cursor = 1; // put the cursor at the end
}


void GBEC_DeleteToNextSequence(GUIBufferEditControl* w, BufferLine* l, colnum_t col, char* charSet) {
	Buffer* b = w->b;
	if(!l || col > l->length) return;
	
	
	// TODO: handle selection size changes
	if(!l) return;
	
	BufferRange sel;
	sel.line[0] = l;
	sel.line[1] = l;
	sel.col[0] = col;
	sel.col[1] = col;
	
	Buffer_FindSequenceEdgeForward(b, &sel.line[1], &sel.col[1], charSet);
	Buffer_DeleteSelectionContents(b, &sel);
	GBEC_MoveCursorTo(w, sel.line[0], sel.col[0]);
}


void GBEC_DeleteToPrevSequence(GUIBufferEditControl* w, BufferLine* l, colnum_t col, char* charSet) {
	Buffer* b = w->b;
	if(!l || col > l->length) return;
	
	// TODO: handle selection size changes
	if(!l) return;
	
	BufferRange sel;
	sel.line[0] = l;
	sel.line[1] = l;
	sel.col[0] = col;
	sel.col[1] = col;
	
	Buffer_FindSequenceEdgeBackward(b, &sel.line[0], &sel.col[0], charSet);
	
	Buffer_DeleteSelectionContents(b, &sel);
	GBEC_MoveCursorTo(w, sel.line[0], sel.col[0]);
	
	GBEC_SelectionChanged(w);
}


void GBEC_SelectionChanged(GUIBufferEditControl* w) {
	Buffer* b = NULL;
	
	if(HAS_SELECTION(w->sel)) {
		b = Buffer_FromSelection(w->b, w->sel);
	}
	
	if(b && b->first) {
		Clipboard_PushBuffer(CLIP_SELECTION, b);
		Buffer_DecRef(b);
		Buffer_Delete(b);
	}
	// undo system hook?
}



void GBEC_PushCursor(GUIBufferEditControl* w, BufferLine* bl, colnum_t col) {
	
	BufferRange* r = BufferRange_New(w);
	CURSOR_LINE(r) = bl;
	PIVOT_LINE(r) = NULL;
	CURSOR_COL(r) = col;
	PIVOT_COL(r) = -1;
	
	r->frozen = 1;
	
	VEC_PUSH(&w->selSet->ranges, r);
}


void GBEC_InsertCharsMC(GUIBufferEditControl* w, char* s, size_t cnt) {
	VEC_EACH(&w->selSet->ranges, ir, r) {
		if(HAS_SELECTION(r)) {
			Buffer_DeleteSelectionContents(w->b, r);
		}
		
		Buffer_LineInsertChars(w->b, CURSOR_LINE(r), s, CURSOR_COL(r), cnt);
	}
}

void GBEC_MoveCursorHMC(GUIBufferEditControl* w, intptr_t cols) {
	VEC_EACH(&w->selSet->ranges, ir, r) {
		GBEC_MoveCursorH(w, r, cols);
	}
}


