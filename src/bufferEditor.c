#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "gui.h"
#include "gui_internal.h"
#include "clipboard.h"

#include "highlighters/c.h"


extern int g_DisableSave;


    //////////////////////////////////
   //                              //
  //       GUIBufferEditor        //
 //                              //
//////////////////////////////////


static size_t lineFromPos(GUIBufferEditor* w, Vector2 pos) {
	return floor((pos.y - w->header.absTopLeft.y) / w->bdp->tdp->lineHeight) + 1 + w->scrollLines;
}

static size_t getColForPos(GUIBufferEditor* w, BufferLine* bl, float x) {
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


static void render(GUIBufferEditor* w, PassFrameParams* pfp) {
// HACK
	GUIBufferEditor_Draw(w, w->header.gm, w->scrollLines, + w->scrollLines + w->linesOnScreen + 2, 0, 100);
	
	if(w->lineNumTypingMode) {
		GUIHeader_render(&w->lineNumEntryBox->header, pfp); 
	}
	
	GUIHeader_renderChildren(&w->header, pfp);
}

static void scrollUp(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	if(gev->originalTarget != w) return;
	w->scrollLines = MAX(0, w->scrollLines - w->linesPerScrollWheel);
}

static void scrollDown(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	if(gev->originalTarget != w) return;
	w->scrollLines = MIN(w->buffer->numLines - w->linesOnScreen, w->scrollLines + w->linesPerScrollWheel);
}

static void dragStart(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	Buffer* b = w->buffer;
	
	if(gev->originalTarget == w->scrollbar) {
		printf("scrollbar drag start\n");
		return;
	}
	
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
// 	w->header.gm
	
	if(gev->originalTarget == w->scrollbar) {
		gev->cancelled = 1;
		
		float val;
		
		val = /*gev->dragStartPos.y -*/ gev->pos.y / w->header.size.y;
		
		
		w->scrollLines = MIN(MAX(0, b->numLines * val), b->numLines);
		
		return;
	}
	
	
	BufferLine* bl = Buffer_raw_GetLine(b, lineFromPos(w, gev->pos));
	size_t col = getColForPos(w, bl, gev->pos.x);
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
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	Buffer* b = w->buffer;
	
	if(!b->current) return;
	
	Buffer_ClearCurrentSelection(b);
	
	Vector2 tl = w->header.absTopLeft;
	Vector2 sz = w->header.size;
	
	// TODO: reverse calculate cursor position
	if(gev->pos.x < tl.x + 50 || gev->pos.x > tl.x + sz.x) return;   
	if(gev->pos.y < tl.y || gev->pos.y > tl.y + sz.y - w->trayHeight) return;
	
	size_t line = lineFromPos(w, gev->pos);
	b->current = Buffer_raw_GetLine(b, line);
	
	b->curCol = getColForPos(w, b->current, gev->pos.x);
	b->curColDisp = getDisplayColFromActual(b, b->current, b->curCol);
	b->curColWanted = b->curColDisp;
	
	w->cursorBlinkTimer = 0;
	
	// maybe nudge the screen down a tiny bit
	GUIBufferEditor_scrollToCursor(w);
}




static void keyDown(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	int needRehighlight = 0;
	
	
	if(isprint(gev->character) && (gev->modifiers & (~(GUIMODKEY_SHIFT | GUIMODKEY_LSHIFT | GUIMODKEY_RSHIFT))) == 0) {
		Buffer_ProcessCommand(w->buffer, &(BufferCmd){
			BufferCmd_InsertChar, gev->character
		}, &needRehighlight);
		
		GUIBufferEditor_RefreshHighlight(w);
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
		
		Cmd cmds[] = {
			{0,    XK_Left,      BufferCmd_MoveCursorH,    -1, scrollToCursor | resetCursorBlink},
			{0,    XK_Right,     BufferCmd_MoveCursorH,     1, scrollToCursor | resetCursorBlink},
			{0,    XK_Up,        BufferCmd_MoveCursorV,    -1, scrollToCursor | resetCursorBlink},
			{0,    XK_Down,      BufferCmd_MoveCursorV,     1, scrollToCursor | resetCursorBlink},
			{0,    XK_BackSpace, BufferCmd_Backspace,       0, scrollToCursor | resetCursorBlink | rehighlight},
			{0,    XK_Delete,    BufferCmd_Delete,          0, scrollToCursor | resetCursorBlink | rehighlight},
			{0,    XK_Return,    BufferCmd_SplitLine,       0, scrollToCursor | resetCursorBlink | rehighlight},
			{0,    XK_Prior,     BufferCmd_MovePage,       -1, 0}, // PageUp
			{0,    XK_Next,      BufferCmd_MovePage,        1, 0}, // PageDown
			{0,    XK_Home,      BufferCmd_Home,            0, scrollToCursor},
			{0,    XK_End,       BufferCmd_End,             0, scrollToCursor}, 
			{S,    XK_Left,      BufferCmd_GrowSelectionH, -1, scrollToCursor | resetCursorBlink},
			{S,    XK_Right,     BufferCmd_GrowSelectionH,  1, scrollToCursor | resetCursorBlink},
			{S,    XK_Up,        BufferCmd_GrowSelectionV, -1, scrollToCursor | resetCursorBlink},
			{S,    XK_Down,      BufferCmd_GrowSelectionV,  1, scrollToCursor | resetCursorBlink},
			{0,    XK_Tab,       BufferCmd_Indent,      0, rehighlight | undoSeqBreak},
			{S,    XK_Tab,       BufferCmd_Unindent,    0, rehighlight | undoSeqBreak},
			{S,    XK_ISO_Left_Tab, BufferCmd_Unindent, 0, rehighlight | undoSeqBreak}, // wtf?
			{C,    'k',       BufferCmd_DeleteCurLine,  0, scrollToCursor | rehighlight | undoSeqBreak},
			{C|A,  XK_Down,   BufferCmd_DuplicateLine,  1, scrollToCursor | rehighlight | undoSeqBreak}, 
			{C|A,  XK_Up,     BufferCmd_DuplicateLine, -1, scrollToCursor | rehighlight | undoSeqBreak}, 
			{C,    'x',       BufferCmd_Cut,            0, rehighlight | undoSeqBreak}, 
			{C,    'c',       BufferCmd_Copy,           0, 0}, 
			{C,    'v',       BufferCmd_Paste,          0, scrollToCursor | rehighlight | undoSeqBreak}, 
			{C,    'a',       BufferCmd_SelectAll,      0, 0}, 
			{C|S,  'a',       BufferCmd_SelectNone,     0, 0}, 
			{C,    'l',       BufferCmd_SelectToEOL,    0, 0}, 
			{C|S,  'l',       BufferCmd_SelectFromSOL,  0, 0}, 
			{C,    'g',       BufferCmd_GoToLine,       0, scrollToCursor}, 
			{C,    'z',       BufferCmd_Undo,           0, scrollToCursor | rehighlight}, 
			{C|S,  'z',       BufferCmd_Redo,           0, scrollToCursor | rehighlight}, 
			{C,    's',       BufferCmd_Save,           0, 0}, 
			{0,    XK_F5,     BufferCmd_Reload,         0, rehighlight}, 
			{C,    'f',       BufferCmd_FindStart,      0, 0}, 
			{C,    'q',       BufferCmd_Debug,          0, scrollToCursor}, 
			{C,    'w',       BufferCmd_Debug,          1, scrollToCursor}, 
			{C|S,  'r',       BufferCmd_RehilightWholeBuffer, 0, scrollToCursor | rehighlight}, 
			{C|A,  'b',       BufferCmd_SetBookmark,          0, scrollToCursor}, 
			{C|S,  'b',       BufferCmd_RemoveBookmark,       0, scrollToCursor}, 
			{C,    'b',       BufferCmd_ToggleBookmark,       0, scrollToCursor}, 
			{A,    XK_Next,   BufferCmd_GoToNextBookmark,     0, scrollToCursor}, // PageUp
			{A,    XK_Prior,  BufferCmd_GoToPrevBookmark,     0, scrollToCursor}, // PageDown
			{A,    XK_Home,   BufferCmd_GoToFirstBookmark,    0, scrollToCursor}, 
			{A,    XK_End,    BufferCmd_GoToLastBookmark,     0, scrollToCursor}, 
			{0,    XK_Escape, BufferCmd_CloseTray,            0, 0}, 
			{0,0,0,0,0},
		};
		
		Cmd found;
		unsigned int iter = 0;
		while(Commands_ProbeCommand(gev, cmds, &found, &iter)) {
			// GUIBufferEditor will pass on commands to the buffer
			GUIBufferEditor_ProcessCommand(w, &(BufferCmd){
				found.cmd, found.amt 
			}, &needRehighlight);
			
			
			if(found.flags & scrollToCursor) {
				GUIBufferEditor_scrollToCursor(w);
			}
			
			if(found.flags & rehighlight) {
				GUIBufferEditor_RefreshHighlight(w);
			}
			
			if(found.flags & resetCursorBlink) {
				w->cursorBlinkTimer = 0;
			}
			
			if(found.flags & undoSeqBreak) {
				Buffer_UndoSequenceBreak(w->buffer);
			}
		}
		
	}
	
}

static void parentResize(GUIBufferEditor* w, GUIEvent* gev) {
	w->header.size = gev->size;
	if(w->trayRoot) {
		w->trayRoot->header.size.x = w->header.size.x;
	}
}

static void updatePos(GUIBufferEditor* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	gui_defaultUpdatePos(w, grp, pfp);
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
}




GUIBufferEditor* GUIBufferEditor_New(GUIManager* gm) {
	
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
	
	
	GUIBufferEditor* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	w->header.cursor = GUIMOUSECURSOR_TEXT;
	
	w->scrollbar = GUIWindow_New(gm);
	GUIResize(w->scrollbar, (Vector2){10, 50});
	w->scrollbar->color = (Vector){.9,.9,.9};
	w->scrollbar->header.z = 100;
	w->scrollbar->header.gravity = GUI_GRAV_TOP_RIGHT;
	
	GUIRegisterObject(w->scrollbar, w);
	
	w->sourceFile = "savetest.c";
	
	// HACK
	w->linesPerScrollWheel = 3;
	w->cursorBlinkOnTime = 0.6;
	w->cursorBlinkOffTime = 0.6;
	
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





void GUIBufferEditor_SetSelectionFromPivot(GUIBufferEditor* gbe) {
	Buffer* b = gbe->buffer;
	if(!b->sel) {
		pcalloc(b->sel);
	}
	
	b->sel->startLine = b->current;
	b->sel->startCol = b->curCol;
	
	b->sel->endLine = gbe->selectPivotLine;
	b->sel->endCol = gbe->selectPivotCol;
	
// 	BufferRange* br = b->sel;
// 	printf("sel0: %d:%d -> %d:%d\n", br->startLine->lineNum, br->startCol, br->endLine->lineNum, br->endCol);
	
	BufferRange_Normalize(&b->sel);
}


int GUIBufferEditor_FindWord(GUIBufferEditor* w, char* word) {
	Buffer* b = w->buffer;
	
	if(!b->first) return;
	
	BufferLine* bl = b->first;
	while(bl) {
		if(bl->buf) {
			char* r = strstr(bl->buf, word);//, bl->length); 
			if(r != NULL) {
				intptr_t dist = r - bl->buf;
				
// 				printf("found: %d, %d\n", bl->lineNum, dist);
				
				b->current = bl;
				b->curCol = dist;
				
				if(!b->sel) b->sel = calloc(1, sizeof(*b->sel));
				b->sel->startLine = bl; 
				b->sel->endLine = bl; 
				b->sel->startCol = dist;
				b->sel->endCol = dist + strlen(word);
				
				w->selectPivotLine = bl;
				w->selectPivotCol = b->sel->endCol;
				
				return 0;
			}
		}
		bl = bl->next;
	}
	
	return 1;
// 	printf("not found: '%s'\n", word);
}



void GUIBufferEditor_MoveCursorTo(GUIBufferEditor* gbe, intptr_t line, intptr_t col) {
	if(line < 1) line = 1;
	if(col < 0) col = 0;
	gbe->buffer->current = Buffer_raw_GetLine(gbe->buffer, line);
	gbe->buffer->curCol = MIN(col, gbe->buffer->current->length); // TODO: check for bounds
}


// event callbacks for the Go To Line edit box
static void gotoline_onchange(GUIEdit* ed, void* gbe_) {
	GUIBufferEditor* w = (GUIBufferEditor*)gbe_;
	
	intptr_t line = GUIEdit_GetDouble(ed);
	if(line <= 0) return; 
	
	GUIBufferEditor_MoveCursorTo(w, line, 0);
	GUIBufferEditor_scrollToCursor(w);
}

static void gotoline_onenter(GUIEdit* ed, void* gbe_) {
	GUIBufferEditor* w = (GUIBufferEditor*)gbe_;
	
	GUIBufferEditor_CloseTray(w);
	w->lineNumTypingMode = 0;
	
	GUIManager_popFocusedObject(w->header.gm);
	
}

// event callbacks for the Find edit box
static void find_onchange(GUIEdit* ed, void* gbe_) {
	GUIBufferEditor* w = (GUIBufferEditor*)gbe_;
	
	char* word = GUIEdit_GetText(ed);
// 	printf("word: '%s'\n", word);
	if(word == 0 || strlen(word) == 0) return; 
	
	GUIBufferEditor_FindWord(w, word);
	GUIBufferEditor_scrollToCursor(w);
}

static void find_onenter(GUIEdit* ed, void* gbe_) {
	GUIBufferEditor* w = (GUIBufferEditor*)gbe_;
	
	GUIBufferEditor_CloseTray(w);
	w->findTypingMode = 0;
	
	GUIManager_popFocusedObject(w->header.gm);
	
}

static void loadfile_onenter(GUIEdit* ed, void* gbe_) {
	GUIBufferEditor* w = (GUIBufferEditor*)gbe_;
	
	char* word = GUIEdit_GetText(ed);
	if(word == 0 || strlen(word) == 0) return; 
	
	GUIBufferEditor_CloseTray(w);
	w->loadTypingMode = 0;
	GUIManager_popFocusedObject(w->header.gm);
	
}


void GUIBufferEditor_ProcessCommand(GUIBufferEditor* w, BufferCmd* cmd, int* needRehighlight) {
	GUIEdit* e;
	
	switch(cmd->type){
		case BufferCmd_MovePage:
			Buffer_ProcessCommand(w->buffer, &(BufferCmd){
				BufferCmd_MoveCursorV, 
				cmd->amt * w->linesOnScreen
			}, needRehighlight);
			
			w->scrollLines = MAX(0, MIN(w->scrollLines + cmd->amt * w->linesOnScreen, w->buffer->numLines - 1));
			break;
		
			
			// TODO: init selectoin and pivots if no selection active
		case BufferCmd_GrowSelectionH:
// 			if(!w->selectPivotLine) {
			if(!w->buffer->sel) {
				w->selectPivotLine = w->buffer->current;
				w->selectPivotCol = w->buffer->curCol;
			}
// 			printf("pivot: %d, %d\n", w->selectPivotLine->lineNum, w->selectPivotCol);
			Buffer_MoveCursorH(w->buffer, cmd->amt);
			GUIBufferEditor_SetSelectionFromPivot(w);
			break;
		
		case BufferCmd_GrowSelectionV:
			if(!w->buffer->sel) {
				w->selectPivotLine = w->buffer->current;
				w->selectPivotCol = w->buffer->curCol;
			}
// 			printf("pivot: %d, %d\n", w->selectPivotLine->lineNum, w->selectPivotCol);
			Buffer_MoveCursorV(w->buffer, cmd->amt);
			GUIBufferEditor_SetSelectionFromPivot(w);
			break;
			
		case BufferCmd_GoToLine:
			GUIBufferEditor_ToggleTray(w, 50);
		
			if(!w->lineNumTypingMode) {
				w->lineNumTypingMode = 1;
				// activate the line number entry box
				w->lineNumEntryBox = GUIEdit_New(w->header.gm, "");
				GUIResize(&w->lineNumEntryBox->header, (Vector2){200, 20});
				w->lineNumEntryBox->header.topleft = (Vector2){0,5};
				w->lineNumEntryBox->header.gravity = GUI_GRAV_TOP_CENTER;
				w->lineNumEntryBox->header.z = 600;
				w->lineNumEntryBox->numType = 1; // integers
				
				w->lineNumEntryBox->onChange = gotoline_onchange;
				w->lineNumEntryBox->onChangeData = w;
				
				w->lineNumEntryBox->onEnter = gotoline_onenter;
				w->lineNumEntryBox->onEnterData = w;
				
				GUIRegisterObject(w->lineNumEntryBox, w->trayRoot);
				
				w->cursorBlinkPaused = 1;
				GUIManager_pushFocusedObject(w->header.gm, w->lineNumEntryBox);
			}
			else {
				GUIBufferEditor_CloseTray(w);
				w->lineNumTypingMode = 0;
				w->cursorBlinkPaused = 0;
// 				guiDelete(w->lineNumEntryBox);
				GUIManager_popFocusedObject(w->header.gm);
			}
		// TODO: change hooks
			break;
		
		case BufferCmd_FindStart:
			GUIBufferEditor_ToggleTray(w, 50);
			if(!w->findTypingMode) {
				w->findTypingMode = 1;
				
				e = GUIEdit_New(w->header.gm, "");
				GUIResize(&e->header, (Vector2){400, 20});
				e->header.topleft = (Vector2){0,5};
				e->header.gravity = GUI_GRAV_TOP_CENTER;
				e->header.z = 600;
				
				e->onChange = find_onchange;
				e->onChangeData = w;
				
				e->onEnter = find_onenter;
				e->onEnterData = w;
				
				w->cursorBlinkPaused = 1;
				GUIRegisterObject(e, w->trayRoot);
				GUIManager_pushFocusedObject(w->header.gm, e);
				
				w->findBox = e;
			}
			else {
				GUIBufferEditor_CloseTray(w);
				w->findTypingMode = 0;
				w->cursorBlinkPaused = 0;
// 				guiDelete(w->findBox);
				GUIManager_popFocusedObject(w->header.gm);
			}
			
			break;
			
		case BufferCmd_PromptLoad:
			GUIBufferEditor_ToggleTray(w, 50);
			if(!w->loadTypingMode) {
				w->loadTypingMode = 1;
				
				e = GUIEdit_New(w->header.gm, "");
				GUIResize(&e->header, (Vector2){400, 20});
				e->header.topleft = (Vector2){0,5};
				e->header.gravity = GUI_GRAV_TOP_CENTER;
				e->header.z = 600;
				
				e->onEnter = find_onenter;
				e->onEnterData = w;
				
				w->cursorBlinkPaused = 1;
				GUIRegisterObject(e, w->trayRoot);
				GUIManager_pushFocusedObject(w->header.gm, e);
				
				w->loadBox = e;
			}
			else {
				GUIBufferEditor_CloseTray(w);
				w->loadTypingMode = 0;
				w->cursorBlinkPaused = 0;
// 				guiDelete(w->findBox);
				GUIManager_popFocusedObject(w->header.gm);
			}
			
			break;
			
		case BufferCmd_CloseTray:
			if(w->trayOpen) {
				GUIBufferEditor_CloseTray(w);
				w->lineNumTypingMode = 0;
				w->findTypingMode = 0;
				w->loadTypingMode = 0;
				w->cursorBlinkPaused = 0;
				GUIManager_popFocusedObject(w->header.gm);
			}
			break;
			
		case BufferCmd_Save:
			if(!g_DisableSave) {
				Buffer_SaveToFile(w->buffer, w->sourceFile);
			}
			else {
				printf("Buffer saving disabled.\n");
			}
			break;
			
		case BufferCmd_Reload:
		{
			struct hlinfo* hl = w->buffer->hl; // preserve the meta info
			EditorParams* ep = w->buffer->ep;
			
			Buffer_Delete(w->buffer);
			w->selectPivotLine = NULL;
			w->selectPivotCol = 0;
			// keep the scroll lines
			w->buffer = Buffer_New();
			Buffer_LoadFromFile(w->buffer, w->sourceFile);
			
			w->buffer->hl = hl;
			w->buffer->ep = ep;
			w->scrollLines = MIN(w->scrollLines, w->buffer->numLines);
		}
		break;
		
		default:
			Buffer_ProcessCommand(w->buffer, cmd, needRehighlight);
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
	
// 	printf("hl time: %f\n", timeSince(then)  * 1000.0);
}


void GUIBufferEditor_CloseTray(GUIBufferEditor* w) {
	if(!w->trayOpen) return;
	
	w->trayOpen = 0;
	w->trayHeight = 0;
	guiDelete(w->trayRoot);
	w->trayRoot = NULL;
}

void GUIBufferEditor_ToggleTray(GUIBufferEditor* w, float height) {
	if(w->trayOpen) GUIBufferEditor_CloseTray(w);
	else GUIBufferEditor_OpenTray(w, height);
}

void GUIBufferEditor_OpenTray(GUIBufferEditor* w, float height) {
	if(w->trayOpen) {
		if(w->trayHeight != height) {
			w->trayHeight = height;
			return;
		}
	}
	
	w->trayOpen = 1; 
	w->trayHeight = height;
	w->trayRoot = GUIWindow_New(w->header.gm);
	w->trayRoot->header.gravity = GUI_GRAV_BOTTOM_LEFT;
	w->trayRoot->header.size.y = height; 
	w->trayRoot->header.size.x = w->header.size.x;
	w->trayRoot->header.z = 500;
	
	w->trayRoot->color = (Vector){.8,.2,.3,1};
	w->trayRoot->padding = (AABB2){{5,5}, {5,5}};
	
	GUIRegisterObject(w->trayRoot, w);
}
