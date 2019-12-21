#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "gui.h"
#include "gui_internal.h"
#include "clipboard.h"

#include "highlighters/c.h"




    //////////////////////////////////
   //                              //
  //       GUIBufferEditor        //
 //                              //
//////////////////////////////////


static size_t lineFromPos(GUIBufferEditor* w, Vector2 pos) {
	return floor(pos.y / w->bdp->tdp->lineHeight) + w->scrollLines;
}

static size_t getColForPos(GUIBufferEditor* w, BufferLine* bl, float x) {
	if(bl->buf == NULL) return 1;
	
	// must handle tabs
	ptrdiff_t screenCol = floor((x - w->header.absTopLeft.x - 50) / w->bdp->tdp->charWidth) + 1 + w->scrollCols;
	
	int tabwidth = w->bdp->tdp->tabWidth;
	ptrdiff_t charCol = 0;
	
	while(screenCol > 0) {
		if(bl->buf[charCol] == '\t') screenCol -= tabwidth;
		else screenCol--;
		charCol++;
	}
	
	return MAX(0, MIN(charCol, bl->length + 1));
}


static void render(GUIBufferEditor* w, PassFrameParams* pfp) {
// HACK
	GUIBufferEditor_Draw(w, w->header.gm, w->scrollLines, + w->scrollLines + w->linesOnScreen + 2, 0, 100);
	
	if(w->lineNumTypingMode) {
		GUIHeader_render(&w->lineNumEntryBox->header, pfp); 
	}
}

static void scrollUp(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	w->scrollLines = MAX(0, w->scrollLines - w->linesPerScrollWheel);
}

static void scrollDown(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	w->scrollLines = MIN(w->buffer->numLines - w->linesOnScreen, w->scrollLines + w->linesPerScrollWheel);
}

static void dragStart(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	Buffer* b = w->buffer;
	
	BufferLine* bl = Buffer_raw_GetLine(b, lineFromPos(w, gev->pos));
	size_t col = getColForPos(w, bl, gev->pos.x);
	
	w->selectPivotLine = bl;
	w->selectPivotCol = col;
}

static void dragStop(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
// 	w->scrollLines = MIN(w->buffer->numLines - w->linesOnScreen, w->scrollLines + w->linesPerScrollWheel);
}

static void dragMove(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	Buffer* b = w->buffer;
	
	BufferLine* bl = Buffer_raw_GetLine(b, lineFromPos(w, gev->pos));
	size_t col = getColForPos(w, bl, gev->pos.x);
	
	Buffer_SetCurrentSelection(b, bl, col, w->selectPivotLine, w->selectPivotCol);
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
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	Buffer* b = w->buffer;
	
	if(!b->current) return;
	
	Buffer_ClearCurrentSelection(b);
	
	Vector2 tl = w->header.absTopLeft;
	Vector2 sz = w->header.size;
	
	// TODO: reverse calculate cursor position
	if(gev->pos.x < tl.x + 50 || gev->pos.x > tl.x + sz.x) return;   
	if(gev->pos.y < tl.y || gev->pos.y > tl.y + sz.y) return;
	
	size_t line = lineFromPos(w, gev->pos);
	b->current = Buffer_raw_GetLine(b, line);
	
	b->curCol = getColForPos(w, b->current, gev->pos.x);
	
	// maybe nudge the screen down a tiny bit
	GUIBufferEditor_scrollToCursor(w);
}



static void keyDown(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	
	
	
	if(isprint(gev->character) && (gev->modifiers & (~(GUIMODKEY_SHIFT | GUIMODKEY_LSHIFT | GUIMODKEY_RSHIFT))) == 0) {
		Buffer_ProcessCommand(w->buffer, &(BufferCmd){
			BufferCmd_InsertChar, gev->character
		});
	
	}
	else {
		// special commands
		unsigned int S = GUIMODKEY_SHIFT;
		unsigned int C = GUIMODKEY_CTRL;
		unsigned int A = GUIMODKEY_ALT;
		unsigned int T = GUIMODKEY_TUX;
		
		struct {
			unsigned int mods;
			int keysym;
			enum BufferCmdType bcmd;
			int amt_action;
			int amt;
			char scrollToCursor;
		} cmds[] = {
			{GUIMODKEY_CTRL, 'k', BufferCmd_DeleteCurLine, 0, 0, 1},
			{0, XK_Left,      BufferCmd_MoveCursorH,   1, -1, 1},
			{0, XK_Right,     BufferCmd_MoveCursorH,   1,  1, 1},
			{0, XK_Up,        BufferCmd_MoveCursorV,   1, -1, 1},
			{0, XK_Down,      BufferCmd_MoveCursorV,   1,  1, 1},
			{0, XK_BackSpace, BufferCmd_Backspace,     0,  0, 1},
			{0, XK_Delete,    BufferCmd_Delete,        0,  0, 1},
			{0, XK_Return,    BufferCmd_SplitLine,     0,  0, 1},
			{0, XK_Prior,     BufferCmd_MovePage,      1, -1, 0}, // PageUp
			{0, XK_Next,      BufferCmd_MovePage,      1,  1, 0}, // PageDown
			{0, XK_Home,      BufferCmd_Home,          0,  0, 1},
			{0, XK_End,       BufferCmd_End,           0,  0, 1}, 
			{C|A,  XK_Down,   BufferCmd_DuplicateLine, 1,  1, 1}, 
			{C|A,  XK_Up,     BufferCmd_DuplicateLine, 1, -1, 1}, 
			{C,    'x',       BufferCmd_Cut,           0,  0, 0}, 
			{C,    'c',       BufferCmd_Copy,          0,  0, 0}, 
			{C,    'v',       BufferCmd_Paste,         0,  0, 0}, 
			{C,    'a',       BufferCmd_SelectAll,     0,  0, 0}, 
			{C|S,  'a',       BufferCmd_SelectNone,    0,  0, 0}, 
			{C,    'l',       BufferCmd_SelectToEOL,   0,  0, 0}, 
			{C|S,  'l',       BufferCmd_SelectFromSOL, 0,  0, 0}, 
			{C,    'g',       BufferCmd_GoToLine,      0,  0, 0}, 
			{C|S,  'r',       BufferCmd_RehilightWholeBuffer, 0, 0, 1}, 
			{C|A,  'b',       BufferCmd_SetBookmark,          0, 0, 1}, 
			{C|S,  'b',       BufferCmd_RemoveBookmark,       0, 0, 1}, 
			{C,    'b',       BufferCmd_ToggleBookmark,       0, 0, 1}, 
			{A,    XK_Next,   BufferCmd_GoToNextBookmark,     0, 0, 1}, // PageUp
			{A,    XK_Prior,  BufferCmd_GoToPrevBookmark,     0, 0, 1}, // PageDown
			{A,    XK_Home,   BufferCmd_GoToFirstBookmark,    0, 0, 1}, 
			{A,    XK_End,    BufferCmd_GoToLastBookmark,     0, 0, 1}, 
			{0,0,0,0,0},
		};
		
		unsigned int ANY = (GUIMODKEY_SHIFT | GUIMODKEY_CTRL | GUIMODKEY_ALT | GUIMODKEY_TUX);
		unsigned int ANY_MASK = ~ANY;
		for(int i = 0; cmds[i].bcmd != 0; i++) {
// 			printf("%d, '%c', %x \n", gev->keycode, gev->keycode, gev->modifiers);
			if(cmds[i].keysym != tolower(gev->keycode)) continue;
			if((cmds[i].mods & ANY) != (gev->modifiers & ANY)) continue;
			// TODO: specific mods
			
			
			// GUIBufferEditor will pass on commands to the buffer
			GUIBufferEditor_ProcessCommand(w, &(BufferCmd){
				cmds[i].bcmd, cmds[i].amt 
			});
			
			
			if(cmds[i].scrollToCursor) {
				GUIBufferEditor_scrollToCursor(w);
			}
		}
		
	}
	
}


GUIBufferEditor* GUIBufferEditor_New(GUIManager* gm) {
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.KeyDown = keyDown,
		.Click = click,
		.ScrollUp = scrollUp,
		.ScrollDown = scrollDown,
		.DragStart = dragStart,
		.DragStop = dragStop,
		.DragMove = dragMove,
	};
	
	
	GUIBufferEditor* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	// HACK
	w->linesPerScrollWheel = 3;
	
	return w;
}


// makes sure the cursor is on screen, with minimal necessary movement
void GUIBufferEditor_scrollToCursor(GUIBufferEditor* gbe) {
	Buffer* b = gbe->buffer;
	
	if(!b || !b->current) return;
	
	size_t scroll_first = gbe->scrollLines;
	size_t scroll_last = gbe->scrollLines + gbe->linesOnScreen;
	
	if(b->current->lineNum <= scroll_first) {
		gbe->scrollLines = b->current->lineNum - 1;
	}
	else if(b->current->lineNum > scroll_last) {
		gbe->scrollLines = scroll_first + (b->current->lineNum - scroll_last);
	}
}



void GUIBufferEditor_ProcessCommand(GUIBufferEditor* w, BufferCmd* cmd) {
	if(cmd->type == BufferCmd_MovePage) {
		Buffer_ProcessCommand(w->buffer, &(BufferCmd){
			BufferCmd_MoveCursorV, 
			cmd->amt * w->linesOnScreen
		});
		
		w->scrollLines = MAX(0, MIN(w->scrollLines + cmd->amt * w->linesOnScreen, w->buffer->numLines - 1));
	}
	else if(cmd->type == BufferCmd_RehilightWholeBuffer) {
		GUIBufferEditor_RefreshHighlight(w);
	}
	else if(cmd->type == BufferCmd_GoToLine) {
		
		if(!w->lineNumTypingMode) {
			w->lineNumTypingMode = 1;
			// activate the line number entry box
			w->lineNumEntryBox = GUIEdit_New(w->header.gm, "");
			GUIResize(&w->lineNumEntryBox->header, (Vector2){200, 20});
			w->lineNumEntryBox->header.topleft = (Vector2){20,20};
			w->lineNumEntryBox->header.gravity = GUI_GRAV_TOP_LEFT;
			
			GUIRegisterObject(w->lineNumEntryBox, w);
			
			GUIManager_pushFocusedObject(w->header.gm, w->lineNumEntryBox);
		}
		// TODO: change hooks
		
	}
	else { // pass it on
		Buffer_ProcessCommand(w->buffer, cmd);
	}
	
// 	printf("line/col %d:%d %d\n", b->current->lineNum, b->curCol, b->current->length);
}




int getNextLine(hlinfo* hl, char** txt, size_t* len) {
	BufferLine* l = hl->readLine;
	
	if(!hl->readLine) return 1;
	
	// TODO: end of file
	hl->readLine = hl->readLine->next;
	
	*txt = l->buf;
	*len = l->length;
	
	return 0;
}

void writeSection(hlinfo* hl, unsigned char style, unsigned char len) {
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
		hl->dirtyLines--;
	}
	
}


void GUIBufferEditor_RefreshHighlight(GUIBufferEditor* gbe) {
	double then = getCurrentTime();
	
	Buffer* b = gbe->buffer;
	
	if(!b->first) return;
	Highlighter* h = gbe->h;

	hlinfo hl = {
		.getNextLine = getNextLine,
		.writeSection = writeSection,
		
		.dirtyLines = b->numLines,
		
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
	
	hlfn(h, &hl);
	
	printf("hl time: %f\n", timeSince(then)  * 1000.0);
}

