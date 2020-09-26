
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "gui.h"
#include "gui_internal.h"
#include "clipboard.h"

#include "highlighters/c.h"






size_t GBEC_lineFromPos(GUIBufferEditControl* w, Vector2 pos) {
	return floor((pos.y - w->header.absTopLeft.y) / w->bdp->tdp->lineHeight) + 1 + w->scrollLines;
}

size_t GBEC_getColForPos(GUIBufferEditControl* w, BufferLine* bl, float x) {
	if(bl->buf == NULL) return 0;
	
	// must handle tabs
	float a = (x - w->header.absTopLeft.x - w->textAreaOffsetX) / w->bdp->tdp->charWidth;
	ptrdiff_t screenCol = floor(a + .5) + w->scrollCols;
	
	int tabwidth = w->bdp->tdp->tabWidth;
	ptrdiff_t charCol = 0;
	
	while(screenCol > 0) {
		if(bl->buf[charCol] == '\t') screenCol -= tabwidth;
		else screenCol--;
		charCol++;
	}
	
	return MAX(0, MIN(charCol, bl->length));
}


static void render(GUIBufferEditControl* w, PassFrameParams* pfp) {
// HACK
	GUIBufferEditControl_Draw(w, w->header.gm, w->scrollLines, + w->scrollLines + w->linesOnScreen + 2, 0, 100);
	
// 	if(w->lineNumTypingMode) {
// 		GUIHeader_render(&w->lineNumEntryBox->header, pfp); 
// 	}
	
	GUIHeader_renderChildren(&w->header, pfp);
}




static void scrollUp(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditControl* w = (GUIBufferEditControl*)w_;
	if(gev->originalTarget != w) return;
	w->scrollLines = MAX(0, w->scrollLines - w->linesPerScrollWheel);
}

static void scrollDown(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditControl* w = (GUIBufferEditControl*)w_;
	if(gev->originalTarget != w) return;
	w->scrollLines = MIN(w->buffer->numLines - w->linesOnScreen, w->scrollLines + w->linesPerScrollWheel);
}

static void dragStart(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditControl* w = (GUIBufferEditControl*)w_;
	Buffer* b = w->buffer;
	
	if(gev->originalTarget == w->scrollbar) {
		printf("scrollbar drag start\n");
		return;
	}
	
	BufferLine* bl = Buffer_raw_GetLine(b, GBEC_lineFromPos(w, gev->pos));
	size_t col = GBEC_getColForPos(w, bl, gev->pos.x);
	
	w->selectPivotLine = bl;
	w->selectPivotCol = col;
}

static void dragStop(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditControl* w = (GUIBufferEditControl*)w_;
// 	w->scrollLines = MIN(w->buffer->numLines - w->linesOnScreen, w->scrollLines + w->linesPerScrollWheel);
}

static void dragMove(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditControl* w = (GUIBufferEditControl*)w_;
	Buffer* b = w->buffer;
// 	w->header.gm
	
	if(gev->originalTarget == w->scrollbar) {
		gev->cancelled = 1;
		
		float val;
		
		val = /*gev->dragStartPos.y -*/ gev->pos.y / w->header.size.y;
		
		
		w->scrollLines = MIN(MAX(0, b->numLines * val), b->numLines);
		
		return;
	}
	
	
	BufferLine* bl = Buffer_raw_GetLine(b, GBEC_lineFromPos(w, gev->pos));
	size_t col = GBEC_getColForPos(w, bl, gev->pos.x);
	Buffer_SetCurrentSelection(b, bl, col, w->selectPivotLine, w->selectPivotCol);
	
	w->buffer->current = bl;
	w->buffer->curCol = col;
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


static void click(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditControl* w = (GUIBufferEditControl*)w_;
	Buffer* b = w->buffer;
	
	if(!b->current) return;
	
	Buffer_ClearCurrentSelection(b);
	
	Vector2 tl = w->header.absTopLeft;
	Vector2 sz = w->header.size;
	
	// TODO: reverse calculate cursor position
	if(gev->pos.x < tl.x + 50 || gev->pos.x > tl.x + sz.x) return;   
	if(gev->pos.y < tl.y || gev->pos.y > tl.y + sz.y) return;
	
	size_t line = GBEC_lineFromPos(w, gev->pos);
	b->current = Buffer_raw_GetLine(b, line);
	
	b->curCol = GBEC_getColForPos(w, b->current, gev->pos.x);
	b->curColDisp = getDisplayColFromActual(b, b->current, b->curCol);
	b->curColWanted = b->curColDisp;
	
	w->cursorBlinkTimer = 0;
	
	// maybe nudge the screen down a tiny bit
	GUIBufferEditControl_scrollToCursor(w);
}




static void keyDown(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditControl* w = (GUIBufferEditControl*)w_;
	int needRehighlight = 0;
	/*
	if(isprint(gev->character) && (gev->modifiers & (~(GUIMODKEY_SHIFT | GUIMODKEY_LSHIFT | GUIMODKEY_RSHIFT))) == 0) {
		Buffer_ProcessCommand(w->buffer, &(BufferCmd){
			BufferCmd_InsertChar, gev->character
		}, &needRehighlight);
		
		GUIBufferEditControl_RefreshHighlight(w);
	}
	else {
		// special commands
		unsigned int S = GUIMODKEY_SHIFT;
		unsigned int C = GUIMODKEY_CTRL;
		unsigned int A = GUIMODKEY_ALT;
		unsigned int T = GUIMODKEY_TUX;
		
		unsigned int scrollToCursor   = 1 << 0;
		unsigned int rehighlight      = 1 << 1;
		unsigned int resetCursorBlink = 1 << 2;
		unsigned int undoSeqBreak     = 1 << 3;
		unsigned int hideMouse        = 1 << 4;
		
		
		Cmd found;
		unsigned int iter = 0;
		while(Commands_ProbeCommand(gev, w->commands, &found, &iter)) {
			// GUIBufferEditor will pass on commands to the buffer
			GUIBufferEditor_ProcessCommand(w, &(BufferCmd){
				found.cmd, found.amt 
			}, &needRehighlight);
			
			
			if(found.flags & scrollToCursor) {
				GUIBufferEditControl_scrollToCursor(w);
			}
			
			if(found.flags & rehighlight) {
				GUIBufferEditControl_RefreshHighlight(w);
			}
			
			if(found.flags & resetCursorBlink) {
				w->cursorBlinkTimer = 0;
			}
			
			if(found.flags & undoSeqBreak) {
				Buffer_UndoSequenceBreak(w->buffer, 0);
			}
		}
		
	}
	*/
}


static void parentResize(GUIBufferEditor* w, GUIEvent* gev) {
// 	w->header.size = gev->size;
}



static void updatePos(GUIHeader* w_, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIBufferEditControl* w = (GUIBufferEditControl*)w_;
	
	Buffer* b = w->buffer;
	
	// cursor blink
	float t = w->cursorBlinkOnTime + w->cursorBlinkOffTime;
	w->cursorBlinkTimer = fmod(w->cursorBlinkTimer + pfp->timeElapsed, t);
	
	w->sbMinHeight = 20;
	// scrollbar position calculation
	// calculate scrollbar height
	float wh = w->header.size.y;
	float sbh = fmax(wh / (b->numLines - w->linesOnScreen), w->sbMinHeight);
	
	// calculate scrollbar offset
	float sboff = ((wh - sbh) / b->numLines) * (w->scrollLines);
	
	GUIResize(w->scrollbar, (Vector2){10, sbh});
	w->scrollbar->header.topleft.y = sboff;

	gui_defaultUpdatePos(w, grp, pfp);
}



GUIBufferEditControl* GUIBufferEditControl_New(GUIManager* gm) {
	
	
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.KeyDown = keyDown,
		.Click = click,
		.ScrollUp = scrollUp,
		.ScrollDown = scrollDown,
		.DragStart = dragStart,
		.DragStop = dragStop,
		.DragMove = dragMove,
		.ParentResize = parentResize,
	};
	
	
	GUIBufferEditControl* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	w->header.cursor = GUIMOUSECURSOR_TEXT;
	
	w->linesPerScrollWheel = gm->gs->Buffer_linesPerScrollWheel;
	w->cursorBlinkOnTime = gm->gs->Buffer_cursorBlinkOnTime;
	w->cursorBlinkOffTime = gm->gs->Buffer_cursorBlinkOffTime;
	w->outlineCurLine = gm->gs->Buffer_outlineCurrentLine;
	
	
	w->scrollbar = GUIWindow_New(gm);
	GUIResize(w->scrollbar, (Vector2){10, 50});
	w->scrollbar->color = (Color4){.9,.9,.9, 1};
	w->scrollbar->header.z = 100;
	w->scrollbar->header.gravity = GUI_GRAV_TOP_RIGHT;
	
	GUIRegisterObject(w, w->scrollbar);
	
	
	
	return w;
}

void GUIBufferEditControl_UpdateSettings(GUIBufferEditControl* w, GlobalSettings* s) {
	w->linesPerScrollWheel = s->Buffer_linesPerScrollWheel;
	w->cursorBlinkOnTime = s->Buffer_cursorBlinkOnTime;
	w->cursorBlinkOffTime = s->Buffer_cursorBlinkOffTime;
	w->outlineCurLine = s->Buffer_outlineCurrentLine;
	
}

void GUIBufferEditControl_SetScroll(GUIBufferEditControl* w, intptr_t line, intptr_t col) {
	w->scrollLines = line;
	w->scrollCols = col;
}


// makes sure the cursor is on screen, with minimal necessary movement
void GUIBufferEditControl_scrollToCursor(GUIBufferEditControl* w) {
	Buffer* b = w->buffer;
	
	if(!b || !b->current) return;
	
	size_t scroll_first = w->scrollLines;
	size_t scroll_last = w->scrollLines + w->linesOnScreen;
	
	if(b->current->lineNum <= scroll_first) {
		w->scrollLines = b->current->lineNum - 1;
	}
	else if(b->current->lineNum > scroll_last) {
		w->scrollLines = scroll_first + (b->current->lineNum - scroll_last);
	}
}


void GUIBufferEditControl_SetSelectionFromPivot(GUIBufferEditControl* w) {
	Buffer* b = w->buffer;
	if(!b->sel) {
		pcalloc(b->sel);
	}
	
	b->sel->startLine = b->current;
	b->sel->startCol = b->curCol;
	
	b->sel->endLine = w->selectPivotLine;
	b->sel->endCol = w->selectPivotCol;
	
// 	BufferRange* br = b->sel;
// 	printf("sel0: %d:%d -> %d:%d\n", br->startLine->lineNum, br->startCol, br->endLine->lineNum, br->endCol);
	
	BufferRange_Normalize(&b->sel);
}



int getNextLine(HLContextInternal* hl, char** txt, size_t* len) {
	BufferLine* l = hl->readLine;
	
	if(!hl->readLine) return 1;
	
	// TODO: end of file
	hl->readLine = hl->readLine->next;
	
	*txt = l->buf;
	*len = l->length;
	
	return 0;
}

void writeSection(HLContextInternal* hl, unsigned char style, unsigned char len) {
	if(len == 0) return;
	
	VEC_INC(&hl->writeLine->style);
	VEC_TAIL(&hl->writeLine->style).length = len;
	VEC_TAIL(&hl->writeLine->style).styleIndex = style;
	
	hl->writeCol += len;
	
	// TODO: handle overlapping style segments
	// TODO: handle segments spanning linebreaks
	
	if(hl->writeCol > hl->writeLine->length) {
		hl->writeCol = 0;
		hl->writeLine = hl->writeLine->next;
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
		.ctx.getNextLine = getNextLine,
		.ctx.writeSection = writeSection,
		
		.ctx.dirtyLines = b->numLines,
		
		.b = b,
		.readLine = b->first,
		.writeLine = b->first,
		.writeCol = 0,
	};
	
	// clear existing styles
	BufferLine* bl = b->first;
	for(int i = 0; i < b->numLines && bl; i++) {
		VEC_TRUNC(&bl->style);
		
		bl = bl->next;
	}
	
	h->plugin->refreshStyle(&hlc.ctx);
	
// 	printf("hl time: %f\n", timeSince(then)  * 1000.0);
}

void GUIBufferEditControl_SetBuffer(GUIBufferEditControl* w, Buffer* b) {
	w->buffer = b;
}
