
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
	return floor((pos.y /*- w->header.absTopLeft.y*/) / w->bdp->tdp->lineHeight) + 1 + w->scrollLines; // TODO IMGUI
}

size_t GBEC_getColForPos(GUIBufferEditControl* w, BufferLine* bl, float x) {
	if(bl->length == 0) return 0;
	
	// must handle tabs
	float a = (x - /*w->header.absTopLeft.x - */w->textAreaOffsetX) / w->bdp->tdp->charWidth; // TODO IMGUI
	ptrdiff_t screenCol = floor(a + 0) + w->scrollCols;
	
	if(screenCol <= 0) return 0;
	
	int tabwidth = w->bdp->tdp->tabWidth;
	ptrdiff_t charCol = 0;
	
	while(screenCol > 0) {
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





static void scrollUp(GUIBufferEditControl* w) {
	w->scrollLines = MAX(0, w->scrollLines - w->linesPerScrollWheel);
}

static void scrollDown(GUIBufferEditControl* w) {
	w->scrollLines = MIN(MAX(0, w->buffer->numLines - w->linesOnScreen), w->scrollLines + w->linesPerScrollWheel);
}


static void dragStart(GUIBufferEditControl* w, GUIManager* gm) {
	Buffer* b = w->buffer;
	
	/* scrollbar dragging
	if(gev->originalTarget == (void*)w->scrollbar) {
		printf("scrollbar drag start\n");
		return;
	}
	*/
	
	Vector2 mp = GUI_EventPos();
	
	if(gm->curEvent.button == 1) {
		BufferLine* bl = Buffer_raw_GetLine(b, GBEC_lineFromPos(w, mp));
		size_t col = GBEC_getColForPos(w, bl, mp.x);
		
		w->selectPivotLine = bl;
		w->selectPivotCol = col;
		
		w->isDragSelecting = 1;
	}
}

static void dragStop(GUIBufferEditControl* w, GUIManager* gm) {

	if(gm->curEvent.button == 1) {
		w->isDragSelecting = 0;
		w->isDragScrollCoasting = 0;
	}
	
// 	w->scrollLines = MIN(w->buffer->numLines - w->linesOnScreen, w->scrollLines + w->linesPerScrollWheel);
}

static void dragMove(GUIBufferEditControl* w, GUIManager* gm) {
	Buffer* b = w->buffer;
// 	w->header.gm

	Vector2 mp = GUI_EventPos();
	
	/* handle scrollbar dragging
	if(gev->originalTarget == (void*)w->scrollbar) {
		gev->cancelled = 1;
		
		float val;
		
		val = /*gev->dragStartPos.y -* / mp.y / w->sz.y;
		
		
		w->scrollLines = MIN(MAX(0, b->numLines * val), b->numLines);
		
		return;
	}
	*/

	
	if(w->isDragSelecting) {
		BufferLine* bl = Buffer_raw_GetLine(b, GBEC_lineFromPos(w, mp));
		
		size_t col = GBEC_getColForPos(w, bl, mp.x);
		
		GBEC_SetCurrentSelection(w, bl, col, w->selectPivotLine, w->selectPivotCol);
		
		w->current = bl;
		w->curCol = col;
		
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
		
		GBEC_MoveCursorTo(w, w->current, w->curCol);
		GBEC_scrollToCursor(w);
	}
	/*
	if(bl->lineNum < w->selectPivotLine->lineNum) {
		b->sel->startLine = bl;
		b->sel->endLine = w->selectPivotLine;
		b->sel->startCol = col;
		b->sel->endCol = w->selectPivotCol - 1;
	}
	else if(bl->lineNum > w->selectPivotLine->lineNum) {
		b->sel->startLine = w->selectPivotLine;
		b->sel->endLine = bl;
		b->sel->startCol = w->selectPivotCol;
		b->sel->endCol = col;
	}
	else { // same line
		b->sel->startLine = bl;
		b->sel->endLine = bl;
		
		if(col < w->selectPivotCol) {
			b->sel->startCol = col;
			b->sel->endCol = w->selectPivotCol - 1;
		}
		else {
			b->sel->startCol = w->selectPivotCol;
			b->sel->endCol = col;
		}
	}*/
	
}


static void click(GUIBufferEditControl* w, GUIManager* gm) {
	Buffer* b = w->buffer;
	
	if(!w->current) return; // empty buffer
	
	unsigned int shift = gm->curEvent.modifiers & GUIMODKEY_SHIFT;
	
	Vector2 mp = GUI_EventPos();
	Vector2 tl = w->tl;
	Vector2 sz = w->sz;
	
	if(gm->curEvent.button == 1) { // left click

		if(!shift) GBEC_ClearCurrentSelection(w);
		
		// TODO: reverse calculate cursor position
		if(mp.x < tl.x + w->textAreaOffsetX || mp.x > tl.x + sz.x) return;
		if(mp.y < tl.y || mp.y > tl.y + sz.y) return;
		
		if(shift && !w->sel) {
			w->selectPivotLine = w->current;
			w->selectPivotCol = w->curCol;
		}
		
		size_t line = GBEC_lineFromPos(w, mp);
		w->current = Buffer_raw_GetLine(b, line);
		
		w->curCol = GBEC_getColForPos(w, w->current, mp.x);
		
		w->cursorBlinkTimer = 0;
		
		if(shift) {
			GBEC_SetSelectionFromPivot(w);
		}
		
		// maybe nudge the screen down a tiny bit
		GBEC_MoveCursorTo(w, w->current, w->curCol);
		GBEC_scrollToCursor(w);
			
		if(gm->curEvent.multiClick == 2) {
			GBEC_SelectSequenceUnder(w, w->current, w->curCol, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_");
		}
		else if(gm->curEvent.multiClick == 3) {
			GBEC_SetCurrentSelection(w, w->current, 0, w->current, w->current->length);
		}
		

		
	}
	else if(gm->curEvent.button == 2) { // middle click	
	
		// TODO: reverse calculate cursor position
		if(mp.x < tl.x + w->textAreaOffsetX || mp.x > tl.x + sz.x) return;
		if(mp.y < tl.y || mp.y > tl.y + sz.y) return;
		
		size_t lineNum = GBEC_lineFromPos(w, mp);
		BufferLine* line = Buffer_raw_GetLine(b, lineNum);
		size_t col = GBEC_getColForPos(w, w->current, mp.x);
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


#include "ui/macros_on.h"

void GBEC_Render(GUIBufferEditControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	void* id = w;
	
	w->tl = tl;
	w->sz = sz;
	
	HOVER_HOT(id);
	CLICK_HOT_TO_ACTIVE(id);
	

	if(!gm->drawMode) {
	
		GBEC_Update(w, tl, sz, pfp);
		
		if(gm->activeID == w) {
			
			Vector2 mp = GUI_EventPos();
			int inbox = GUI_PointInBoxV(tl, sz, mp);
			
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
	
	GUIBufferEditControl_Draw(w, gm, (Vector2){0,0}, sz, w->scrollLines, + w->scrollLines + w->linesOnScreen + 2, 0, 100, pfp);
}
#include "ui/macros_off.h"


void GBEC_Update(GUIBufferEditControl* w, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
//static void updatePos(GUIHeader* w_, GUIRenderParams* grp, PassFrameParams* pfp) {
	
	Buffer* b = w->buffer;
	
	float lineNumWidth = ceil(LOGB(w->gs->Buffer_lineNumBase, b->numLines + 0.5)) * w->bdp->tdp->charWidth + w->bdp->lineNumExtraWidth;
	
	w->linesOnScreen = sz.y / w->gs->Buffer_lineHeight;
	w->colsOnScreen = (sz.x - lineNumWidth) / w->gs->Buffer_charWidth;

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
		
//	if(w->scrollbar) {
//		w->sbMinHeight = 20;
		// scrollbar position calculation
		// calculate scrollbar height
//		float wh = sz.y;
//		/*float*/ sbh = fmax(wh / (b->numLines - w->linesOnScreen), w->sbMinHeight);
		
		// calculate scrollbar offset
//		float sboff = ((wh - sbh) / b->numLines) * (w->scrollLines);
		
//		w->scrollbar->header.topleft.y = sboff;
//	}

}




// called by the buffer when things change
static void bufferChangeNotify(BufferChangeNotification* note, void* _w) {
	GUIBufferEditControl* w = (GUIBufferEditControl*)_w;
	
	if(note->action == BCA_Undo_MoveCursor) {
//		printf("move notify\n");
		GBEC_MoveCursorTo(w, note->sel.startLine, note->sel.startCol);
	}
	else if(note->action == BCA_Undo_SetSelection) {
		/*printf("selection notify %ld:%ld -> %ld:%ld\n",
			note->sel.startLine->lineNum, note->sel.startCol,
			note->sel.endLine->lineNum, note->sel.endCol
		);*/
		if(note->isReverse) {
			w->selectPivotLine = note->sel.endLine;
			w->selectPivotCol = note->sel.endCol;
			GBEC_MoveCursorTo(w, note->sel.startLine, note->sel.startCol);
			GBEC_SetCurrentSelection(w, 
				note->sel.endLine, note->sel.endCol,
				note->sel.startLine, note->sel.startCol);
		}
		else {
			w->selectPivotLine = note->sel.startLine;
			w->selectPivotCol = note->sel.startCol;
			GBEC_MoveCursorTo(w, note->sel.endLine, note->sel.endCol);
			GBEC_SetCurrentSelection(w, 
				note->sel.startLine, note->sel.startCol,
				note->sel.endLine, note->sel.endCol);
		}
	}
	else if(note->action == BCA_DeleteLines) {
		if(BufferLine_IsInRange(w->current, &note->sel)) {
			w->current = note->sel.startLine->prev;
			if(!w->current) {		
				w->current = note->sel.endLine->next;		
			}
			w->curCol = MIN(w->curCol, w->current->length);
		}
		
		if(BufferLine_IsInRange(w->selectPivotLine, &note->sel)) {
			w->selectPivotLine = note->sel.startLine->prev;
			if(!w->selectPivotLine) {		
				w->selectPivotLine = note->sel.endLine->next;		
			}
		}
		
		if(w->sel) {
			BufferRange_DeleteLineNotify(w->sel, &note->sel);
		}
		
		if(w->findSet && VEC_LEN(&w->findSet->ranges)) {
			VEC_EACH(&w->findSet->ranges, i, r) {
				BufferRange_DeleteLineNotify(r, &note->sel);		
			}
			// TODO: check deleted chars and re-regex		
		}
		// TODO: check scrollLines and scrollCols
	
	}
	
}



void BufferRange_DeleteLineNotify(BufferRange* r, BufferRange* dsel) {
	if(BufferLine_IsInRange(r->startLine, dsel)) {
		r->startLine = dsel->startLine->prev;
		if(!r->startLine) {		
			r->startLine = dsel->endLine->next;		
		}
	}

	if(BufferLine_IsInRange(r->endLine, dsel)) {
		r->endLine = dsel->startLine->prev;
		if(!r->endLine) {		
			r->endLine = dsel->endLine->next;		
		}
	}

}



GUIBufferEditControl* GUIBufferEditControl_New(GUIManager* gm) {
	
	
	GUIBufferEditControl* w = pcalloc(w);
	
	
//	w->header.cmdElementType = CUSTOM_ELEM_TYPE_Buffer;
	
	
	pcalloc(w->selSet);
	
	return w;
}

void GUIBufferEditControl_UpdateSettings(GUIBufferEditControl* w, GlobalSettings* s) {
	w->gs = s;
	
	w->linesPerScrollWheel = s->Buffer_linesPerScrollWheel;
	w->cursorBlinkOnTime = s->Buffer_cursorBlinkOnTime;
	w->cursorBlinkOffTime = s->Buffer_cursorBlinkOffTime;
	w->outlineCurLine = s->Buffer_outlineCurrentLine;
	
}

void GUIBufferEditControl_SetScroll(GUIBufferEditControl* w, intptr_t line, intptr_t col) {
	w->scrollLines = MIN(MAX(0, line), w->buffer->numLines);
	w->scrollCols = MAX(0, col);
}

// move the view by this delta
void GBEC_ScrollDir(GUIBufferEditControl* w, intptr_t lines, intptr_t cols) {
	GUIBufferEditControl_SetScroll(w, w->scrollLines + lines, w->scrollCols + cols);
}


// makes sure the cursor is on screen, with minimal necessary movement
void GBEC_scrollToCursor(GUIBufferEditControl* w) {
	GBEC_scrollToCursorOpt(w, 0);
}

void GBEC_scrollToCursorOpt(GUIBufferEditControl* w, int centered) {
	if(!w || !w->current) return;
	
	intptr_t scroll_first = w->scrollLines;
	intptr_t scroll_last = w->scrollLines + w->linesOnScreen;
	
	if(centered) {
		w->scrollLines = w->current->lineNum - w->linesOnScreen/2;
	}
	else if(w->current->lineNum <= scroll_first) {
		w->scrollLines = w->current->lineNum - 1;
	}
	else if(w->current->lineNum > scroll_last) {
		w->scrollLines = scroll_first + (w->current->lineNum - scroll_last);
	}
	
	intptr_t col_first = w->scrollCols;
	intptr_t col_last = w->scrollCols + w->colsOnScreen;
	
	w->curColDisp = getDisplayColFromActual(w, w->current, w->curCol);
	if(w->curColDisp <= col_first) {
		w->scrollCols = w->curColDisp - 1;
	}
	else if(w->curColDisp >= col_last - 3) {
		w->scrollCols = col_first + 3 + (w->curColDisp - col_last);
	}
	
	GUIBufferEditControl_SetScroll(w, w->scrollLines, w->scrollCols);
}


void GBEC_SetSelectionFromPivot(GUIBufferEditControl* w) {
	if(!w->sel) {
		pcalloc(w->sel);
		
		VEC_PUSH(&w->selSet->ranges, w->sel);
	}
	
	w->sel->startLine = w->current;
	w->sel->startCol = w->curCol;
	
	w->sel->endLine = w->selectPivotLine;
	w->sel->endCol = w->selectPivotCol;
	
 //	BufferRange* br = w->sel;
 //	printf("sel0: %d:%d -> %d:%d\n", br->startLine->lineNum, br->startCol, br->endLine->lineNum, br->endCol);
	
	if(BufferRange_Normalize(&w->sel)) {
		VEC_TRUNC(&w->selSet->ranges);
	}
	
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

void GUIBufferEditControl_RefreshHighlight(GUIBufferEditControl* w) {
	double then = getCurrentTime();
	
	Buffer* b = w->buffer;
	
	if(!b->first) return;
	Highlighter* h = w->h;
	
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
	char* homedir = getenv("HOME");
	
	char* tmp = sprintfdup("%s/.gpuedit/%s_colors.txt", homedir, ext);
	
	Highlighter_LoadStyles(w->h, tmp);
	free(tmp);

}


void GUIBufferEditControl_SetBuffer(GUIBufferEditControl* w, Buffer* b) {
	w->buffer = b;
	w->current = b->first;
	w->curCol = 0;
	
	Buffer_RegisterChangeListener(b, bufferChangeNotify, w);
}


void GUIBufferEditControl_ProcessCommand(GUIBufferEditControl* w, GUI_Cmd* cmd, int* needRehighlight) {
	Buffer* b = w->buffer;
	Buffer* b2 = NULL;
	
	char cc[2] = {cmd->amt, 0};
	
	switch(cmd->cmd) {
		case GUICMD_Buffer_MoveCursorV:
			GBEC_MoveCursorV(w, cmd->amt);
			GBEC_ClearAllSelections(w);
			break;		
		
		case GUICMD_Buffer_MoveCursorH:
			GBEC_MoveCursorH(w, cmd->amt);
			GBEC_ClearAllSelections(w);
			break;

		case GUICMD_Buffer_MoveCursorHSel:
			GBEC_MoveCursorHSel(w, cmd->amt);
			GBEC_ClearAllSelections(w);
			break;
		
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
		
		case GUICMD_Buffer_InsertChar:
			// TODO: update
			if(w->sel) {
				w->current = w->sel->startLine; // TODO: undo cursor
				w->curCol = w->sel->startCol;
				Buffer_DeleteSelectionContents(b, w->sel);
				GBEC_ClearAllSelections(w);
			}
			Buffer_LineInsertChars(b, w->current, cc, w->curCol, 1);
			GBEC_MoveCursorH(w, 1);
			break;
		
		case GUICMD_Buffer_Backspace:
			if(w->sel) {
				w->current = w->sel->startLine;
				w->curCol = w->sel->startCol;
				Buffer_UndoSequenceBreak(b, 0, w->sel->startLine->lineNum, w->sel->startCol, 
				w->sel->endLine->lineNum, w->sel->endCol, 0);
				Buffer_DeleteSelectionContents(b, w->sel);
				
				GBEC_ClearAllSelections(w);
			}
			else {
				BufferLine* bl = w->current;
				intptr_t col = w->curCol;
				GBEC_MoveCursorH(w, -1);
				Buffer_BackspaceAt(b, bl, col);		
			}
			break;
		
		case GUICMD_Buffer_Delete:
			if(w->sel) {
				w->current = w->sel->startLine;
				w->curCol = w->sel->startCol;
				Buffer_DeleteSelectionContents(b, w->sel);
				GBEC_ClearAllSelections(w);
			}
			else {
				Buffer_DeleteAt(b, w->current, w->curCol);
			}
			break;
		
		case GUICMD_Buffer_SplitLine:
			GBEC_InsertLinebreak(w);
			break;

		case GUICMD_Buffer_SplitLineIndent:
			GBEC_InsertLinebreak(w);
			intptr_t tabs = Buffer_IndentToPrevLine(b, w->current);
			GBEC_MoveCursorTo(w, w->current, tabs);
			break;
		
		case GUICMD_Buffer_DeleteCurLine: {
			// preserve proper cursor position
			if(w->sel) {
				BufferLine* cur = w->sel->startLine->prev;
				if(!cur) cur = w->sel->endLine->next;
				intptr_t col = w->curCol;
				
				BufferLine* bl = w->sel->startLine;
				BufferLine* next = bl->next;
				BufferLine* last = w->sel->endLine;
				if(w->sel->endCol == 0) {
					last = last->prev;
					cur = w->sel->endLine;
				}
				
				while(bl) {
					Buffer_DeleteLine(b, bl);
					
					if(bl == last) break;
					bl = next;
					next = bl->next;
				}
				
				GBEC_ClearCurrentSelection(w);
				
				if(cur) GBEC_MoveCursorTo(w, cur, col);
			}
			else {
				BufferLine* cur = w->current->next;
				if(!cur) cur = w->current->prev;
				intptr_t col = w->curCol;
				
				Buffer_DeleteLine(b, w->current);
				
				if(cur) GBEC_MoveCursorTo(w, cur, col);
			}
			break;
		}
		
		case GUICMD_Buffer_LinePrependText:
			if(w->sel) {
				Buffer_LinePrependTextSelection(b, w->sel, cmd->str);
			}
			else {
				Buffer_LinePrependText(b, w->current, cmd->str);
			}
			break;
			
		case GUICMD_Buffer_LineAppendText:
			if(w->sel) {
				Buffer_LineAppendTextSelection(b, w->sel, cmd->str, strlen(cmd->str));
			}
			else {
				Buffer_LineAppendText(b, w->current, cmd->str, strlen(cmd->str));
			}
			break;
			
		case GUICMD_Buffer_LineEnsureEnding:
			if(w->sel) {
				Buffer_LineEnsureEndingSelection(b, w->sel, cmd->str, strlen(cmd->str));
			}
			else {
				Buffer_LineEnsureEnding(b, w->current, cmd->str, strlen(cmd->str));
			}
			break;
			
		case GUICMD_Buffer_SurroundSelection:
			if(w->sel)
				GBEC_SurroundCurrentSelection(w, cmd->pstr[0], cmd->pstr[1]);
			break;
			
		case GUICMD_Buffer_ReplaceLineWithSelectionTransform:
			if(w->sel)
				GBEC_ReplaceLineWithSelectionTransform(w, cmd->pstr[0], cmd->pstr[1], cmd->pstr[2]);
			break;
		
		case GUICMD_Buffer_SmartComment:
			if(!cmd->pstr[0] || !cmd->pstr[1] || !cmd->pstr[2]) break;
			if(!w->sel) { // simple case
				Buffer_LinePrependText(b, w->current, cmd->pstr[0]);
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
			if(w->sel) {
				Buffer_LineUnprependTextSelection(b, w->sel, cmd->str);
			}
			else {
				Buffer_LineUnprependText(b, w->current, cmd->str);
			}
			break;
			
		case GUICMD_Buffer_UnsurroundSelection:
			if(w->sel)
				GBEC_UnsurroundCurrentSelection(w, cmd->pstr[0], cmd->pstr[1]);
			break;
		
		case GUICMD_Buffer_StrictUncomment:
			if(!cmd->pstr[0] || !cmd->pstr[1] || !cmd->pstr[2]) break;
			if(!w->sel) { // simple case
				Buffer_LineUnprependText(b, w->current, cmd->pstr[0]);
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
			GBEC_MoveToFirstCharOrSOL(w, w->current);
			break;
		
		case GUICMD_Buffer_GoToFirstCharOfLine: 
			GBEC_MoveToFirstCharOfLine(w, w->current);
			break;
			
		case GUICMD_Buffer_GoToLastCharOfLine:
			GBEC_MoveToLastCharOfLine(w, w->current);
			break;
		
		case GUICMD_Buffer_GoToFirstColOfFile:
			// TODO: undo
			GBEC_MoveCursorTo(w, b->first, 0);
			break;
		
		case GUICMD_Buffer_GoToLastColOfFile:
			// TODO: undo
			w->current = b->last;
			if(b->last) w->curCol = b->last->length;
			break;
			
		case GUICMD_Buffer_Indent:
			if(w->sel) {
				Buffer_IndentSelection(w->buffer, w->sel);
			}
			else {
				Buffer_LineIndent(w->buffer, w->current);
				GBEC_MoveCursorH(w, 1);
			}		
			break;
			
		case GUICMD_Buffer_Unindent: 
			if(w->sel) {
				Buffer_UnindentSelection(w->buffer, w->sel);
			}
			else {
				Buffer_LineUnindent(w->buffer, w->current); 
			}
			break;
			
		case GUICMD_Buffer_DuplicateLine:
			if(w->sel) {
				Buffer_DuplicateSelection(b, w->sel, cmd->amt);
				GBEC_MoveCursorTo(w, w->sel->endLine, w->sel->endCol);
			}
			else {
				Buffer_DuplicateLines(b, w->current, cmd->amt);
				GBEC_MoveCursorV(w, cmd->amt);
			}
			break;
		
		case GUICMD_Buffer_Copy:
			if(w->sel) {
				b2 = Buffer_FromSelection(b, w->sel);
				Clipboard_PushBuffer(cmd->amt/*CLIP_PRIMARY*/, b2);
			}
			break;
		
		case GUICMD_Buffer_Cut:
			if(w->sel) {
				b2 = Buffer_FromSelection(b, w->sel);
				Clipboard_PushBuffer(cmd->amt/*CLIP_PRIMARY*/, b2);
				// TODO: move cursor to cut spot, if it isn't already
				if(!w->sel->reverse) {
					w->current = w->sel->startLine;
					w->curCol = w->sel->startCol;
				}
				
				Buffer_DeleteSelectionContents(b, w->sel);
				GBEC_ClearAllSelections(w);
			}
			break;
		
		case GUICMD_Buffer_SmartCut: {
			int smart = 0;
			if(!w->sel) {
				// make the current line the selection
				GBEC_SetCurrentSelection(w, w->current, 0, w->current, w->current->length);
				smart = 1;
			}
			if(w->sel) {
				// stolen from GUICMD_Buffer_Cut
				b2 = Buffer_FromSelection(b, w->sel);
				Clipboard_PushBuffer(cmd->amt, b2);
				// TODO: move cursor to cut spot, if it isn't already
				if(!w->sel->reverse) {
					w->current = w->sel->startLine;
					w->curCol = w->sel->startCol;
				}
				
				Buffer_DeleteSelectionContents(b, w->sel);
				GBEC_ClearAllSelections(w);
			}
			if(smart) {
				// stolen from GUICMD_Buffer_DeleteCurLine
				// preserve proper cursor position
				BufferLine* cur = w->current->next;
				if(!cur) cur = w->current->prev;
				intptr_t col = w->curCol;
				
				Buffer_DeleteLine(b, w->current);
				
				if(cur) GBEC_MoveCursorTo(w, cur, col);
			}
			break;
		}
		
		case GUICMD_Buffer_Paste:
			b2 = Clipboard_PopBuffer(cmd->amt/*CLIP_PRIMARY*/);
			if(b2) {
				if(w->sel) {
					GBEC_MoveCursorTo(w, w->sel->startLine, w->sel->startCol);
					Buffer_DeleteSelectionContents(b, w->sel);
					GBEC_ClearAllSelections(w);
				}
				
				// TODO: undo
				BufferRange pasteRange;
				Buffer_InsertBufferAt(b, b2, w->current, w->curCol, &pasteRange);
				// TODO: move cursor to end of pasted text
				
				w->current = pasteRange.endLine;
				w->curCol = pasteRange.endCol;
				
			}
			break;
		
		case GUICMD_Buffer_SelectNone:
			GBEC_ClearAllSelections(w);
			break;
		
		case GUICMD_Buffer_SelectAll:
			GBEC_SetCurrentSelection(w, b->first, 1, b->last, b->last->length+1);
			break;
		
		case GUICMD_Buffer_SelectLine:
			GBEC_SetCurrentSelection(w, w->current, 1, w->current, w->current->length);
			break;
			
		case GUICMD_Buffer_SelectToEOL:
			GBEC_SetCurrentSelection(w, w->current, w->curCol, w->current, w->current->length);
			break;
			
		case GUICMD_Buffer_SelectFromSOL:
			GBEC_SetCurrentSelection(w, w->current, 1, w->current, w->curCol);
			break;
		
		case GUICMD_Buffer_SetBookmark:       Buffer_SetBookmarkAt(b, w->current);    break; 
		case GUICMD_Buffer_RemoveBookmark:    Buffer_RemoveBookmarkAt(b, w->current); break; 
		case GUICMD_Buffer_ToggleBookmark:    Buffer_ToggleBookmarkAt(b, w->current); break; 
		case GUICMD_Buffer_GoToNextBookmark:  GBEC_NextBookmark(w);  break; 
		case GUICMD_Buffer_GoToPrevBookmark:  GBEC_PrevBookmark(w);  break; 
		case GUICMD_Buffer_GoToFirstBookmark: GBEC_FirstBookmark(w); break; 
		case GUICMD_Buffer_GoToLastBookmark:  GBEC_LastBookmark(w);  break; 
		default:
			Buffer_ProcessCommand(w->buffer, cmd, needRehighlight);
			return;
	}

	
	// check flags
	
	if(cmd->flags & GUICMD_FLAG_scrollToCursor) {
		GBEC_scrollToCursor(w);
	}
	
	if(cmd->flags & GUICMD_FLAG_rehighlight) {
		GUIBufferEditControl_RefreshHighlight(w);
	}
	
	if(cmd->flags & GUICMD_FLAG_resetCursorBlink) {
		w->cursorBlinkTimer = 0;
	}
	
	if(cmd->flags & GUICMD_FLAG_undoSeqBreak) {
		if(!w->sel) {
//					printf("seq break without selection\n");
			Buffer_UndoSequenceBreak(
				w->buffer, 0, 
				w->current->lineNum, w->curCol,
				0, 0, 0
			);
		}
		else {
//					printf("seq break with selection\n");
			Buffer_UndoSequenceBreak(
				w->buffer, 0, 
				w->sel->startLine->lineNum, w->sel->startCol,
				w->sel->endLine->lineNum, w->sel->endCol,
				1 // TODO check pivot locations
			);
		}			
	}
	
	if(cmd->flags & GUICMD_FLAG_centerOnCursor) {
		GBEC_scrollToCursorOpt(w, 1);
	}
	
	
}




void GBEC_MoveCursorV(GUIBufferEditControl* w, ptrdiff_t lines) {
	int i = lines;
	// TODO: undo
	if(i > 0) while(i-- > 0 && w->current->next) {
		w->current = w->current->next;
	}
	else while(i++ < 0 && w->current->prev) {
		w->current = w->current->prev;
	}
	
	// not regular move, to preserve w->curColWanted
	w->curCol = getActualColFromWanted(w, w->current, w->curColWanted);
}


void GBEC_MoveToNextSequence(GUIBufferEditControl* w, BufferLine* l, intptr_t col, char* charSet) {
	if(!l || col > l->length) return;
	
	// TODO: handle selection size changes
	if(!l) return;
	
	Buffer_FindSequenceEdgeForward(w->buffer, &l, &col, charSet);
	GBEC_MoveCursorTo(w, l, col);
}

void GBEC_MoveToPrevSequence(GUIBufferEditControl* w, BufferLine* l, intptr_t col, char* charSet) {
	if(!l || col > l->length) return;
	
	// TODO: handle selection size changes
	if(!l) return;
	
	Buffer_FindSequenceEdgeBackward(w->buffer, &l, &col, charSet);
	GBEC_MoveCursorTo(w, l, col);
	
	return;
}

void GBEC_MoveToFirstCharOrSOL(GUIBufferEditControl* w, BufferLine* bl) {
	w->current = bl;
	
	for(intptr_t i = 0; i < bl->length; i++) {
		if(!isspace(bl->buf[i])) {
			if(i == w->curCol) {
				GBEC_MoveCursorTo(w, w->current, 0);
			} else {
				GBEC_MoveCursorTo(w, w->current, i);
			}
			return;
		}
	}
	
	GBEC_MoveCursorTo(w, w->current, 0);
}

void GBEC_MoveToFirstCharOfLine(GUIBufferEditControl* w, BufferLine* bl) {
	w->current = bl;
	
	for(intptr_t i = 0; i < bl->length; i++) {
		if(!isspace(bl->buf[i])) {
			GBEC_MoveCursorTo(w, w->current, i);
			return;
		}
	}
	
	GBEC_MoveCursorTo(w, w->current, 0);
}

void GBEC_MoveToLastCharOfLine(GUIBufferEditControl* w, BufferLine* bl) {
	w->current = bl;
	
	for(intptr_t i = bl->length - 1; i >= 0; i--) {
		if(!isspace(bl->buf[i])) {
			GBEC_MoveCursorTo(w, w->current, i + 1);
			return;
		}
	}
	
	w->curCol = bl->length;
	GBEC_MoveCursorTo(w, w->current, bl->length);
}


// TODO: undo
void GBEC_ClearCurrentSelection(GUIBufferEditControl* w) {
	if(w->sel) free(w->sel);
	w->sel = NULL;
	VEC_TRUNC(&w->selSet->ranges);
	
	GBEC_SelectionChanged(w);
}


void GBEC_SetCurrentSelection(GUIBufferEditControl* w, BufferLine* startL, intptr_t startC, BufferLine* endL, intptr_t endC) {
	if(!w->sel) {
		pcalloc(w->sel);
		
		VEC_PUSH(&w->selSet->ranges, w->sel); 
	}
	
	// TODO: undo
	
	assert(startL != NULL);
	assert(endL != NULL);
	
	startC = MIN(startL->length, MAX(startC, 0));
	endC = MIN(endL->length, MAX(endC, 0));
	
	if(startL->lineNum < endL->lineNum) {
		w->sel->startLine = startL;
		w->sel->endLine = endL;
		w->sel->startCol = startC;
		w->sel->endCol = endC;
	}
	else if(startL->lineNum > endL->lineNum) {
		w->sel->startLine = endL;
		w->sel->endLine = startL;
		w->sel->startCol = endC;
		w->sel->endCol = startC;
	}
	else { // same line
		w->sel->startLine = startL;
		w->sel->endLine = startL;
		
		if(startC < endC) {
			w->sel->startCol = startC;
			w->sel->endCol = endC;
		}
		else {
			w->sel->startCol = endC;
			w->sel->endCol = startC;
		}
	}
	
	GBEC_SelectionChanged(w);
}

void GBEC_SetCurrentSelectionRange(GUIBufferEditControl* w, BufferRange* r) {
	GBEC_SetCurrentSelection(w, r->startLine, r->startCol, r->endLine, r->endCol);
}


void GBEC_InsertLinebreak(GUIBufferEditControl* w) {
	BufferLine* l = w->current;
	Buffer* b = w->buffer;
	
	if(w->curCol == 0) {
		Buffer_InsertEmptyLineBefore(b, w->current);
	}
	else {
		BufferLine* n = Buffer_InsertLineAfter(b, l, l->buf + w->curCol, MAX(l->length - w->curCol, 0));
		Buffer_LineTruncateAfter(b, l, w->curCol);
		
		w->current = w->current->next;
		// TODO: undo cursor move
	}
	
	GBEC_MoveCursorTo(w, w->current, 0);
	
	// TODO: undo
	// TODO: maybe shrink the alloc
}


void GBEC_ClearAllSelections(GUIBufferEditControl* w) {
	if(!w->sel) return;
	free(w->sel);
	w->sel = NULL;
	
	VEC_TRUNC(&w->selSet->ranges);
	// TODO: undo
	// TODO: clear the selection list too
	GBEC_SelectionChanged(w);
}


// also fixes cursor and selection
void GBEC_SurroundCurrentSelection(GUIBufferEditControl* w, char* begin, char* end) {
	Buffer* b = w->buffer;

	if(!w->sel || !begin) return;
	 
	Buffer_SurroundSelection(b, w->sel, begin, end);
		
	// fix the selection to include the new text
	size_t len1 = strlen(begin);
	size_t len2 = strlen(end);
	if(len1 || len2) {
		if(w->sel->endLine == w->current && w->sel->endCol == w->curCol) {
			// forward selection
			GBEC_MoveCursorH(w, len2 + (w->sel->startLine == w->current ? len1 : 0));
			GBEC_SetSelectionFromPivot(w);
		}
		else {
			// reverse selection
			w->selectPivotCol += len2 + (w->sel->startLine == w->selectPivotLine ? len1 : 0);
			GBEC_SetSelectionFromPivot(w);
		}
	}
}

// also fixes cursor and selection
void GBEC_UnsurroundCurrentSelection(GUIBufferEditControl* w, char* begin, char* end) {
	Buffer* b = w->buffer;

	int res = Buffer_UnsurroundSelection(b, w->sel, begin, end);
	if(res == 0) return;
			
	// fix the selection to include the new text
	size_t len1 = strlen(begin);
	size_t len2 = strlen(end);
	
	if(!len1 && !len2) return;
	
	if(w->sel->endLine == w->current && w->sel->endCol == w->curCol) {
		// forward selection
		GBEC_MoveCursorH(w, -len2 - (w->sel->startLine == w->current ? len1 : 0));
		GBEC_SetSelectionFromPivot(w);
	}
	else {
		// reverse selection
		w->selectPivotCol -= len2 + (w->sel->startLine == w->selectPivotLine ? len1 : 0);
		GBEC_SetSelectionFromPivot(w);
	}

}



void GBEC_ReplaceLineWithSelectionTransform(
	GUIBufferEditControl* w, 
	char* selectionToken,
	char* cursorToken,
	char* format
) {
	BufferRange cursor;
	
	Buffer* f = Buffer_New();
	Buffer_AppendRawText(f, format, strlen(format));
	
	// find first cursor token and remove it
	int haveCursor = !Buffer_strstr(f, cursorToken, &cursor);
	if(haveCursor) {
		// delete the token itself; start l/c still hold the cursor pos
		Buffer_DeleteSelectionContents(f, &cursor);
	}
	
	BufferRange sel = *w->sel;
	GBEC_ClearCurrentSelection(w);
	GBEC_MoveCursorTo(w, sel.startLine, sel.startCol);
	
	Buffer* sb = Buffer_FromSelection(w->buffer, &sel);
	
	Buffer_ReplaceAllString(f, selectionToken, sb);
	
	sel.startCol = 0;
	sel.endCol = sel.endLine->length;
	Buffer_DeleteSelectionContents(w->buffer, &sel);
	Buffer_InsertBufferAt(w->buffer, f, w->current, w->curCol, NULL);
	
	if(haveCursor) {
		GBEC_MoveCursorTo(w, w->current, 0);
		GBEC_MoveCursorV(w, cursor.startLine->lineNum - 1);
		GBEC_MoveCursorH(w, cursor.startCol);
		
	}
	
}



intptr_t getActualColFromWanted(GUIBufferEditControl* w, BufferLine* bl, intptr_t wanted) {
	if(bl->buf == NULL) return 0;
	
	int tabwidth = w->buffer->ep->tabWidth;
	intptr_t screenCol = 0;
	intptr_t charCol = 0;
	
	while(screenCol < wanted && charCol < bl->length) {
		if(bl->buf[charCol] == '\t') screenCol += tabwidth;
		else screenCol++;
		charCol++;
	}
	
	return MAX(0, MIN(charCol, bl->length));
}


intptr_t getDisplayColFromActual(GUIBufferEditControl* w, BufferLine* bl, intptr_t col) {
	if(bl->buf == NULL) return 0;
	
	int tabwidth = w->buffer->ep->tabWidth;
	intptr_t screenCol = 0;
	
	
	for(intptr_t charCol = 0; charCol < col && charCol < bl->length; charCol++) {
		if(bl->buf[charCol] == '\t') screenCol += tabwidth;
		else screenCol++;
	}
	
	return MAX(0, screenCol);
}


void GBEC_MoveCursorH(GUIBufferEditControl* w, ptrdiff_t cols) {
	int i = cols;
	// TODO: undo
	
	if(i < 0) while(i++ < 0) {
		if(w->curCol <= 0) {
			
			if(w->current->prev == NULL) {
				w->curCol = 0;
				break;
			}
			
			w->current = w->current->prev;
			w->curCol = w->current->length;
		}
		else {
			w->curCol--;
		}
	}
	else while(i-- > 0) {
		if(w->curCol >= w->current->length) {
			
			if(w->current->next == NULL) {
				break;
			}
			
			w->current = w->current->next;
			w->curCol = 0;
		}
		else {
			w->curCol++;
		}
	}
	
	GBEC_MoveCursorTo(w, w->current, w->curCol);
}


void GBEC_MoveCursorHSel(GUIBufferEditControl* w, ptrdiff_t cols) {
	if(w->sel && cols < 0) {
		GBEC_MoveCursorTo(w, w->sel->startLine, w->sel->startCol);
		cols++;
	} else if(w->sel && cols > 0) {
		GBEC_MoveCursorTo(w, w->sel->endLine, w->sel->endCol);
		cols--;
	}
	
	GBEC_MoveCursorH(w, cols);
}


// absolute move
void GBEC_MoveCursorTo(GUIBufferEditControl* w, BufferLine* bl, intptr_t col) {
	// TODO: undo
	w->current = bl;
	w->curCol = col;
	w->curColWanted = getDisplayColFromActual(w, bl, col);
}



void GBEC_NextBookmark(GUIBufferEditControl* w) {
	if(!w->current) return;
	BufferLine* bl = w->current->next;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->next;
	}
	if(bl) w->current = bl;
}

void GBEC_PrevBookmark(GUIBufferEditControl* w) {
	if(!w->current) return;
	BufferLine* bl = w->current->prev;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->prev;
	}
	if(bl) w->current = bl;
}

void GBEC_FirstBookmark(GUIBufferEditControl* w) {
	BufferLine* bl = w->buffer->first;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->next;
	}
	if(bl) w->current = bl;
}

void GBEC_LastBookmark(GUIBufferEditControl* w) {
	BufferLine* bl = w->buffer->last;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->prev;
	}
	if(bl) w->current = bl;
}



// only to be used for cursor-based selection growth.
void GBEC_GrowSelectionH(GUIBufferEditControl* w, intptr_t cols) {
	Buffer* b = w->buffer;
	
	if(!w->sel) {
		pcalloc(w->sel);
		
		w->sel->startLine = w->current;
		w->sel->endLine = w->current;
		w->sel->startCol = w->curCol;
		w->sel->endCol = w->curCol;
	
		if(cols > 0) {
			Buffer_RelPosH(b, w->sel->endLine, w->sel->endCol, cols, &w->sel->endLine, &w->sel->endCol);
		}
		else {
			w->sel->reverse = 1;
			Buffer_RelPosH(b, w->sel->startLine, w->sel->startCol, cols, &w->sel->startLine, &w->sel->startCol);
		}
		
		return;
	}
	
	if(!w->sel->reverse) {
		Buffer_RelPosH(b, w->sel->endLine, w->sel->endCol, cols, &w->sel->endLine, &w->sel->endCol);
	}
	else {
		Buffer_RelPosH(b, w->sel->startLine, w->sel->startCol, cols, &w->sel->startLine, &w->sel->startCol);
	}
	
	// clear zero length selections
	if(w->sel->startLine == w->sel->endLine && w->sel->startCol == w->sel->endCol) {
		free(w->sel);
		w->sel = NULL;
	}
	
	GBEC_SelectionChanged(w);
}



void GBEC_GrowSelectionV(GUIBufferEditControl* w, intptr_t lines) {
	Buffer* b = w->buffer;
	if(!w->sel) {
		pcalloc(w->sel);
		
		w->sel->startLine = w->current;
		w->sel->endLine = w->current;
		w->sel->startCol = w->curCol;
		w->sel->endCol = w->curCol;
	
		if(lines > 0) {
			Buffer_RelPosV(b, w->sel->endLine, w->sel->endCol, lines, &w->sel->endLine, &w->sel->endCol);
		}
		else {
			w->sel->reverse = 1;
			Buffer_RelPosV(b, w->sel->startLine, w->sel->startCol, lines, &w->sel->startLine, &w->sel->startCol);
		}
		
		return;
	}
	
	if(!w->sel->reverse) {
		Buffer_RelPosV(b, w->sel->endLine, w->sel->endCol, lines, &w->sel->endLine, &w->sel->endCol);
	}
	else {
		Buffer_RelPosV(b, w->sel->startLine, w->sel->startCol, lines, &w->sel->startLine, &w->sel->startCol);
	}
	
	// TODO: handle pivoting around the beginning
	
	// clear zero length selections
	if(w->sel->startLine == w->sel->endLine && w->sel->startCol == w->sel->endCol) {
		free(w->sel);
		w->sel = NULL;
	}
	
	GBEC_SelectionChanged(w);
}



void GBEC_SelectSequenceUnder(GUIBufferEditControl* w, BufferLine* l, intptr_t col, char* charSet) {
	BufferRange sel;
	
	Buffer_GetSequenceUnder(w->buffer, l, col, charSet, &sel);
	
	GBEC_SetCurrentSelection(w, sel.startLine, sel.startCol, sel.endLine, sel.endCol);
	GBEC_MoveCursorTo(w, sel.endLine, sel.endCol);
}


void GBEC_DeleteToNextSequence(GUIBufferEditControl* w, BufferLine* l, intptr_t col, char* charSet) {
	Buffer* b = w->buffer;
	if(!l || col > l->length) return;
	
	
	// TODO: handle selection size changes
	if(!l) return;
	
	BufferRange sel;
	sel.startLine = l;
	sel.endLine = l;
	sel.startCol = col;
	sel.endCol = col;
	
	Buffer_FindSequenceEdgeForward(b, &sel.endLine, &sel.endCol, charSet);
	Buffer_DeleteSelectionContents(b, &sel);
	GBEC_MoveCursorTo(w, sel.startLine, sel.startCol);
}


void GBEC_DeleteToPrevSequence(GUIBufferEditControl* w, BufferLine* l, intptr_t col, char* charSet) {
	Buffer* b = w->buffer;
	if(!l || col > l->length) return;
	
	// TODO: handle selection size changes
	if(!l) return;
	
	BufferRange sel;
	sel.startLine = l;
	sel.endLine = l;
	sel.startCol = col;
	sel.endCol = col;
	
	Buffer_FindSequenceEdgeBackward(b, &sel.startLine, &sel.startCol, charSet);
	
	Buffer_DeleteSelectionContents(b, &sel);
	GBEC_MoveCursorTo(w, sel.startLine, sel.startCol);
	
	GBEC_SelectionChanged(w);
}


void GBEC_SelectionChanged(GUIBufferEditControl* w) {
	
	Buffer* b = NULL;
	
	if(w->sel) {
		b = Buffer_FromSelection(w->buffer, w->sel);
	}
	
	if(b) {
		Clipboard_PushBuffer(CLIP_SELECTION, b);
		Buffer_Delete(b);
	}
	// undo system hook?
}


