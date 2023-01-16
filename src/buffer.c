#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "ui/gui.h"
#include "ui/gui_internal.h"
#include "clipboard.h"

#include "commands.h"



//#define LOG_UNDO(...) __VA_ARGS__
#define LOG_UNDO(...)




extern int g_DisableSave;



Buffer* Buffer_New() {
	Buffer* b = pcalloc(b);
	
	b->nextLineID = 1;
	b->refs = 1;
	
	b->undoMax = 4096; // TODO: settings
	b->undoRing = calloc(1, b->undoMax * sizeof(*b->undoRing));
	
	
	// HACK should go in settings files
	b->useDict = 1;
	b->dictCharSet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
	pcalloc(b->dictRoot);
	
	return b;
}


void Buffer_AddRef(Buffer* b) {
	b->refs++;
}

static void ac_free_tree(BufferPrefixNode* n) {
	BufferPrefixNode* k = n->kids;
	while(k) {
		BufferPrefixNode* s = k->next;
		ac_free_tree(k);
		k = s;
	}
	
	free(n);
}

void Buffer_Delete(Buffer* b) {
//	b->refs--;
//	if(b->refs > 0) return;
	
	if(b->filePath) free(b->filePath);
	
	// free all the lines
	BufferLine* bl = b->first;
	BufferLine* bln = NULL;
	
	while(bl) {
		bln = bl->next;
		BufferLine_Delete(b, bl);
		bl = bln;
		
	}
	
	VEC_FREE(&b->changeListeners);
	
	ac_free_tree(b->dictRoot);
	
	Buffer_FreeAllUndo(b);
	
	free(b);
}

void Buffer_InitEmpty(Buffer* b) {
	b->first = BufferLine_New(b);
	b->last = b->first;
	b->first->lineNum = 1;
}


BufferCache* BufferCache_New() {
	BufferCache* bc = pcalloc(bc);
	HT_init(&bc->byRealPath, 64);
	return bc;
}

Buffer* BufferCache_GetPath(BufferCache* bc, char* path) {
	char* rp = NULL;
	Buffer* b;
	
	if(strlen(path) == 0) path = NULL;
	
	if(path) {
		rp = resolve_path(path);
		
		if(!HT_get(&bc->byRealPath, rp, &b)) {
			return b;
		}
	}
	
	b = Buffer_New();
	
	if(!path || Buffer_LoadFromFile(b, rp)) {
		Buffer_InitEmpty(b);
	}
	
	if(path && rp) {
		HT_set(&bc->byRealPath, rp, b);
	}
	
	return b;
}


void BufferCache_RemovePath(BufferCache* bc, char* realPath) {
	HT_delete(&bc->byRealPath, realPath);
}

//
//    Undo Functions
//


static int undo_avail(Buffer* b) {
	return b->undoMax - b->undoFill;
}

// adds a new undo-able event to the undo stack
// wipes out any redo-able events above the current stack point
static BufferUndo* undo_inc(Buffer* b, int changeCount) {
	// TODO: adjust size dynamically sometimes
	BufferUndo* u = b->undoRing + b->undoCurrent;
	memset(u, 0, sizeof(*u));
	
	// handle initialization
	if(b->undoFill == 0) {
		b->undoCurrent = (b->undoCurrent + 1) % b->undoMax; //the top of undo
		b->undoNewest = b->undoCurrent; // the top of redo
		
		b->undoFill++;
		b->undoUndoLen++;
//		b->undoRedoLen = 0;
		return u;
	}
	
	if(b->undoMax - b->undoFill <= 0) {
		// the undo ring is full
		// handle wrapping of the ring buffer
		// the oldest item slips off the stack
		
		// clean up the old data
		switch(u->action) {
			case UndoAction_DeleteText:
			case UndoAction_InsertText:
			case UndoAction_DeleteLine:
			case UndoAction_InsertLineAt:
				if(u->text) free(u->text);
				u->text = NULL;
		}
		
		b->undoOldest = (b->undoOldest + 1) % b->undoMax;
//		printf("oldest adjustment\n");
		
		// undoFill remains the same.
		// undoUndoLen will be recalculated below because the redo stack might be truncated
	}
	else {
		// the undo ring is not full yet
		b->undoFill++;
//		b->undoUndoLen++;
	}
	
	b->undoCurrent = (b->undoCurrent + 1) % b->undoMax;
	
	// the redo stack is wiped out
	b->undoNewest = b->undoCurrent;
	b->undoRedoLen = 0;
	
	b->undoUndoLen = b->undoCurrent > b->undoOldest ? b->undoCurrent - b->undoOldest : b->undoMax - b->undoOldest + b->undoCurrent;
	
	b->changeCounter += changeCount;	
	return u;
}

// pointer to the next position to be overwritte
// one past the most recent undoable action
static BufferUndo* undo_current(Buffer* b) {
	return b->undoRing + b->undoCurrent;
}

// the most recent undo-able action
static BufferUndo* undo_prev(Buffer* b) { // the one before the current one
	return b->undoRing + ((b->undoCurrent - 1 + b->undoMax) % b->undoMax);
}

// called while undoing a change
// move the current pointer backwards
// newest remains at the end of the redo stack
// returns the most recent undo-able action, which is now what current points to
static BufferUndo* undo_dec(Buffer* b) {
	if(b->undoUndoLen <= 0) return NULL;
	
//	if(b->undoOldest == b->undoCurrent && undo_avail(b) != 0) {
//		printf("wrap around undo\n");
//		return NULL;
//	}
	
	b->undoCurrent = (b->undoCurrent - 1 + b->undoMax) % b->undoMax;
	
	// next is not moved; it is the end of the redo stack
// 	b->undoNext = (b->undoCurrent - 1 + b->undoMax) % b->undoMax;
	
	b->undoUndoLen--;
	b->undoRedoLen++;
	
	return b->undoRing + b->undoCurrent;
}

// like dec but doesn't move current
static BufferUndo* undo_peek(Buffer* b) {
	if(b->undoUndoLen <= 0) return NULL;
//	if(b->undoOldest == b->undoCurrent) return NULL;
	
	size_t c = (b->undoCurrent - 1 + b->undoMax) % b->undoMax;
	
	// next is not moved; it is the end of the redo stack
// 	b->undoNext = (b->undoCurrent - 1 + b->undoMax) % b->undoMax;
	
	return b->undoRing + c;
}

// clean up all memory related to the undo system
void Buffer_FreeAllUndo(Buffer* b) {
	for(intptr_t i = 0; i < b->undoFill; i++) {
		if(b->undoRing[i].text) {
			free(b->undoRing[i].text);
		}
	}
	
	free(b->undoRing);
}


void Buffer_UndoInsertText(
	Buffer* b, 
	linenum_t line, 
	colnum_t col, 
	char* txt, 
	intptr_t len
) {
	
	if(len == 0) return;
	
// 	VEC_INC(&b->undoStack);
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_inc(b, 1);
	
	u->action = UndoAction_InsertText;
	u->lineNum = line;
	u->colNum = col;
	u->text = NULL;
	if(txt) {
		u->text = malloc(sizeof(*txt) * len);
		memcpy(u->text, txt, len);
	}
	u->length = len;
}

void Buffer_UndoDeleteText(Buffer* b, BufferLine* bl, intptr_t offset, intptr_t len) {
	
	if(len == 0) return;
	
// 	VEC_INC(&b->undoStack);
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_inc(b, 1);
	
	u->action = UndoAction_DeleteText;
	u->lineNum = bl->lineNum;
	u->colNum = offset;
	u->text = NULL;
	if(len) {
		u->text = malloc(sizeof(*bl->buf) * len);
		memcpy(u->text, bl->buf + offset, len);
	}
	u->length = len;
}


void Buffer_UndoSetSelection(Buffer* b, linenum_t startL, colnum_t startC, linenum_t endL, colnum_t endC) {
	BufferUndo* u = undo_inc(b, 0);
	
	u->action = UndoAction_SetSelection;
	u->lineNum = startL;
	u->colNum = startC;
	u->endLine = endL;
	u->endCol = endC;
}

void Buffer_UndoSequenceBreak(Buffer* b, int saved, 
	linenum_t startL, colnum_t startC, linenum_t endL, colnum_t endC, char isReverse) {
	
	// don't add duplicates
	if(b->undoFill > 0) {
		if(undo_prev(b)->action == UndoAction_SequenceBreak) {
			if(saved) b->undoSaveIndex = b->undoCurrent;
			return;
		}
	}
	
// 	VEC_INC(&b->undoStack);
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_inc(b, 0);
	
	//printf("saving seq break: %ld:%ld -> %ld->%ld\n",
	//	startL, startC, endL, endC);
	
	u->action = UndoAction_SequenceBreak;
	u->lineNum = startL;
	u->colNum = startC;
	u->endLine = endL;
	u->endCol = endC;
	u->isReverse = isReverse;
	
	if(saved) b->undoSaveIndex = b->undoCurrent;
}

void Buffer_UndoInsertLineAfter(Buffer* b, BufferLine* before) {
// 	VEC_INC(&b->undoStack);
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_inc(b, 1);
	
	u->action = UndoAction_InsertLineAt;
	u->lineNum = before ? before->lineNum + 1 : 1; 
	u->colNum = 0;
	u->text = NULL;
	u->length = 0;
}

void Buffer_UndoInsertLineBefore(Buffer* b, BufferLine* after) {
// 	VEC_INC(&b->undoStack);
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_inc(b, 1);
	
	u->action = UndoAction_InsertLineAt;
	u->lineNum = after->lineNum; 
	u->colNum = 0;
	u->text = NULL;
	u->length = 0;
}


void Buffer_UndoDeleteLine(Buffer* b, BufferLine* bl) {
// 	VEC_INC(&b->undoStack);
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_inc(b, 1);
	
	u->action = UndoAction_DeleteLine;
	u->lineNum = bl->lineNum; // null means the first line 
	u->colNum = 0;
	u->text = strndup(bl->buf, bl->length);
	u->length = bl->length;
}


// clears the entire undo buffer
void Buffer_UndoTruncateStack(Buffer* b) {
// 	VEC_TRUNC(&b->undoStack);
	
	// clean up any text
	for(intptr_t i = 0; i < b->undoFill; i++) {
		if(b->undoRing[i].text) {
			free(b->undoRing[i].text);
			b->undoRing[i].text = NULL;
		}
	}
	
	b->undoFill = 0;
	b->undoOldest = 0;
	b->undoNewest = 0;
	b->undoCurrent = 0;
	b->undoUndoLen = 0;
	b->undoRedoLen = 0;
}

// rolls back to the previous sequence break
void Buffer_UndoReplayToSeqBreak(Buffer* b) {
	
	// pop off a lone sequence break if it's on top of the stack
	BufferUndo* u = undo_peek(b);
	if(u && u->action == UndoAction_SequenceBreak) undo_dec(b);
	
	while(Buffer_UndoReplayTop(b));
}

// rolls forward until a sequence break or undoNext
void Buffer_RedoReplayToSeqBreak(Buffer* b) {
	if(b->undoRedoLen == 0) {
// 		printf("redo: current reached next\n");
		return;
	}
	
	BufferUndo* u;
	u = b->undoRing + b->undoCurrent;
	b->undoCurrent = (b->undoCurrent + 1) % b->undoMax;
	b->undoRedoLen--;
	b->undoUndoLen++;
	
	Buffer_RedoReplay(b, u);
	
	// pop off the sequence break
// 	undo_dec(b);
// 	if(VEC_LEN(&b->undoStack) >= 0) VEC_POP1(&b->undoStack);
}



// executes a single undo action
// returns 0 on a sequence break
int Buffer_UndoReplayTop(Buffer* b) {
	BufferLine* bl;
	
//	printf("undo top\n");
	if(b->undoUndoLen == 0) return 0;
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_dec(b);
	if(u == NULL) return 0;
	
//	printf("  undo good\n");
	
	// these all need to be the inverse
	switch(u->action) {
		case UndoAction_InsertText: // delete text
			bl = Buffer_raw_GetLineByNum(b, u->lineNum);
			LOG_UNDO(printf("UNDO: deleting text at col %ld '%.*s'\n", (int64_t)u->colNum, (int)u->length, bl->buf));
			Buffer_raw_DeleteChars(b, bl, u->colNum, u->length);
			break;
			
		case UndoAction_DeleteText: // re-insert the text
			bl = Buffer_raw_GetLineByNum(b, u->lineNum);
			LOG_UNDO(printf("UNDO: inserting text at col %ld '%.*s'\n", (int64_t)u->colNum, (int)u->length, u->text));
			Buffer_raw_InsertChars(b, bl, u->text, u->colNum, u->length);
			//b->current = bl; // BUG not right after <delete> key
			//b->curCol = u->colNum + u->length;
			break;
			
		case UndoAction_InsertLineAt: // delete the line
			bl = Buffer_raw_GetLineByNum(b, u->lineNum);
			LOG_UNDO(printf("UNDO: deleting line number (%ld) '%.*s'\n", (int64_t)u->lineNum, (int)bl->length, bl->buf));
			
			Buffer_NotifyLineDeletion(b, bl, bl);
			Buffer_raw_DeleteLine(b, bl);
			break; 
			
		case UndoAction_DeleteLine: // insert the line
			if(u->lineNum == 0) {
				LOG_UNDO(printf("UNDO: insert first line\n"));
				BufferLine* bln = Buffer_raw_InsertLineBefore(b, b->first);
				Buffer_raw_InsertChars(b, bln, u->text, 0, u->length);
			}
			else {
				LOG_UNDO(printf("UNDO: insert line before (%ld) '%.*s'\n", (int64_t)u->lineNum, (int)u->length, u->text));
				bl = Buffer_raw_GetLineByNum(b, u->lineNum);
				BufferLine* bln = Buffer_raw_InsertLineBefore(b, bl);
				Buffer_raw_InsertChars(b, bln, u->text, 0, u->length);
			}
			break;
			
		case UndoAction_SetSelection:
			/*Buffer_SetCurrentSelection(b, 
				Buffer_raw_GetLine(b, u->lineNum),
				u->colNum,
				Buffer_raw_GetLine(b, u->line[1]),
				u->col[1]);*/
			break;
			
		case UndoAction_MoveCursorTo:
			fprintf(stderr, "UndoAction_MoveCursorTo nyi\n");
			break;
			
// 		case UndoAction_UnmodifiedFlag:
// 			fprintf(stderr, "UndoAction_MoveCursorTo nyi\n");
// 			
// 			break;
			
		case UndoAction_SequenceBreak:
			// move cursor back
			if(u->endLine > 0) {
				printf("undo notify selection %ld:%ld -> %ld:%ld\n",
					(int64_t)u->lineNum, (int64_t)u->colNum,
					(int64_t)u->endLine, (int64_t)u->endLine
				);
				Buffer_NotifyUndoSetSelection(b, 
					Buffer_raw_GetLineByNum(b, u->lineNum), u->colNum,
					Buffer_raw_GetLineByNum(b, u->endLine), u->endCol,
					u->isReverse
				);
			}
			else if(u->lineNum > 0) {
				Buffer_NotifyUndoMoveCursor(b, Buffer_raw_GetLineByNum(b, u->lineNum), u->colNum);
			}
			
			return 0;
			
		default:
			fprintf(stderr, "Unknown undo action: %d\n", u->action);
	}
	
	// cleanup
// 	if(u->text) free(u->text);
// 	u->text = NULL;
	
// 	VEC_POP1(&b->undoStack);
	
	return 1;
}

// un-executes a single undo action
// returns 0 on a sequence break
int Buffer_RedoReplay(Buffer* b, BufferUndo* u) {
	BufferLine* bl;
	
	// these all need to be the inverse
	switch(u->action) {
		case UndoAction_DeleteText:
			bl = Buffer_raw_GetLineByNum(b, u->lineNum);
			Buffer_raw_DeleteChars(b, bl, u->colNum, u->length);
			break;
			
		case UndoAction_InsertText:
			bl = Buffer_raw_GetLineByNum(b, u->lineNum);
			Buffer_raw_InsertChars(b, bl, u->text, u->colNum, u->length);
			//b->current = bl; // BUG not right after <delete> key
			//b->curCol = u->colNum + u->length;
			break;
			
		case UndoAction_DeleteLine: 
			bl = Buffer_raw_GetLineByNum(b, u->lineNum);
			Buffer_raw_DeleteLine(b, bl);
			break; 
			
		case UndoAction_InsertLineAt:
		
			// TODO: fix for s/After/At/
			if(u->lineNum == 0) {
				BufferLine* bln = Buffer_raw_InsertLineBefore(b, b->first);
				Buffer_raw_InsertChars(b, bln, u->text, 0, u->length);
			}
			else {
				bl = Buffer_raw_GetLineByNum(b, u->lineNum);
				BufferLine* bln = Buffer_raw_InsertLineBefore(b, bl);
				Buffer_raw_InsertChars(b, bln, u->text, 0, u->length);
			}
			break;
			
		case UndoAction_MoveCursorTo:
			fprintf(stderr, "RedoAction_MoveCursorTo nyi\n");
			break;
			
// 		case UndoAction_UnmodifiedFlag:
// 			fprintf(stderr, "RedoAction_MoveCursorTo nyi\n");
// 			
// 			break;
			
		case UndoAction_SequenceBreak:
			// do nothing at all for now
			return 0;
			
		default:
			fprintf(stderr, "Unknown redo action: %d\n", u->action);
	}
	
	// cleanup
// 	if(u->text) free(u->text);
// 	u->text = NULL;
	
// 	VEC_POP1(&b->undoStack);
	
	return 1;
}



BufferLine* Buffer_InsertEmptyLineAfter(Buffer* b, BufferLine* before) {
	Buffer_UndoInsertLineAfter(b, before);
	return Buffer_raw_InsertLineAfter(b, before);
}
BufferLine* Buffer_InsertEmptyLineBefore(Buffer* b, BufferLine* after) {
	Buffer_UndoInsertLineBefore(b, after);
	return Buffer_raw_InsertLineBefore(b, after);
}


BufferLine* Buffer_InsertLineAfter(Buffer* b, BufferLine* before, char* text, intptr_t length) {
	BufferLine* bl = Buffer_InsertEmptyLineAfter(b, before);

	Buffer_UndoInsertText(b, bl->lineNum, 0, text, length);
	Buffer_raw_InsertChars(b, bl, text, 0, length);
	
	return bl;
}


BufferLine* Buffer_InsertLineBefore(Buffer* b, BufferLine* after, char* text, intptr_t length) {
	BufferLine* bl = Buffer_InsertEmptyLineBefore(b, after);
	
	Buffer_UndoInsertText(b, bl->lineNum, 0, text, length);
	Buffer_raw_InsertChars(b, bl, text, 0, length);
	
	return bl;
}



void Buffer_DeleteLine(Buffer* b, BufferLine* bl) {
	Buffer_UndoDeleteLine(b, bl);
	Buffer_NotifyLineDeletion(b, bl, bl);
	Buffer_raw_DeleteLine(b, bl);
}


void Buffer_LineInsertChars(Buffer* b, BufferLine* bl, char* text, intptr_t offset, intptr_t length) {
	Buffer_UndoInsertText(b, bl->lineNum, offset, text, length);
	Buffer_raw_InsertChars(b, bl, text, offset, length);
}


void Buffer_LineAppendText(Buffer* b, BufferLine* bl, char* text, intptr_t length) {
	Buffer_UndoInsertText(b, bl->lineNum, 
		bl->length, // used to have +1
		text, length);
	Buffer_raw_InsertChars(b, bl, text, bl->length, length); // BUG: of by 1?
}

void Buffer_LineAppendLine(Buffer* b, BufferLine* target, BufferLine* src) {
	Buffer_LineAppendText(b, target, src->buf, src->length);
}

void Buffer_LineDeleteChars(Buffer* b, BufferLine* bl, colnum_t col, intptr_t length) {
	
	intptr_t len = MIN(bl->length - col, length);
	if(len <= 0) return;
	
	Buffer_UndoDeleteText(b, bl, col, len);
	Buffer_raw_DeleteChars(b, bl, col, len);
}


void Buffer_LineTruncateAfter(Buffer* b, BufferLine* bl, colnum_t col) {
	Buffer_LineDeleteChars(b, bl, col, bl->length - col); // BUG: off by 1?
}

// deletes chars but also handles line removal and edge cases
// does not move the cursor
void Buffer_BackspaceAt(Buffer* b, BufferLine* l, colnum_t col) {
	if(!b->first) return; // empty buffer
	
	
	if(col <= 0) { 
		// first col of first row; do nothing
		if(b->first == l) return;
		
		//Buffer_MoveCursorH(b, -1);
		
		if(l->length > 0) {
			// merge with the previous line
			Buffer_LineAppendLine(b, l->prev, l);
		}
		
		Buffer_NotifyLineDeletion(b, l, l);
		Buffer_DeleteLine(b, l);
		
		return;
	} 
	
	Buffer_LineDeleteChars(b, l, col - 1, 1);
	//Buffer_MoveCursorH(b, -1);
}


// deletes chars but also handles line removal and edge cases
// does not move the cursor
void Buffer_DeleteAt(Buffer* b, BufferLine* l, colnum_t col) {
	if(!b->first) return; // empty buffer
	colnum_t oldLen = l->length;
	
	Buffer_LineDeleteChars(b, l, col, 1);
	
	if(col >= oldLen) {
		// last col of last row; do nothing
		if(b->last == l) return;
		
		if(l->next->length > 0) {
			// merge with the next line (doin't append empty text from an ampty line, just delete it)
			Buffer_LineAppendText(b, l, l->next->buf, l->next->length);
		}
		
		Buffer_NotifyLineDeletion(b, l->next, l->next);
		Buffer_DeleteLine(b, l->next);
		
		return;
	} 
	
}



void Buffer_DuplicateSelection(Buffer* b, BufferRange* sel, int amt) {
	BufferLine* src;
	BufferLine* endLine = sel->line[1];
	if(sel->col[1] == 0 && sel->line[1] != sel->line[0]) {
		endLine = endLine->prev;
	}
	BufferLine* target = endLine;
	BufferLine* start_target = endLine;
	
	if(amt < 0) {
		amt = -amt;
		target = sel->line[0];
		
		while(amt-- > 0) {
			BufferLine* src = endLine;
			while(src && src->next != sel->line[0]) {
				target = Buffer_InsertLineBefore(b, target, src->buf, src->length);
				
				src = src->prev;
			}
		}
	}
	else {
		
		while(amt-- > 0) {
			BufferLine* src = sel->line[0];
			while(src && src->prev != endLine) {
				target = Buffer_InsertLineAfter(b, target, src->buf, src->length);
				
				src = src->next;
			}
		}
		
		// fix the selection and cursor if it turns out we inserted lines in between	
		if(endLine != sel->line[1]) {
			//if(b->current == sel->line[1]) b->current = line[1]->next;
			sel->line[1] = endLine->next;
			
		}
	}

}


void Buffer_DuplicateLines(Buffer* b, BufferLine* src, int amt) {
	if(amt == 0) return;
	BufferLine* bl;
	
	if(amt > 0) {
		while(amt-- > 0) {
			bl = Buffer_InsertLineAfter(b, src, src->buf, src->length);
		}
	}
	else {
		amt = -amt;
		
		while(amt-- > 0) {
			bl = Buffer_InsertLineBefore(b, src, src->buf, src->length);
		}
	}
}

intptr_t Buffer_IndentToPrevLine(Buffer* b, BufferLine* bl) {
	intptr_t i = 0;
	
	BufferLine* p = bl->prev;
	if(!p || !p->buf || p->length == 0) return 0;
	
	for(i = 0; i < p->length; i++) {
		if(p->buf[i] != '\t') break;
		
		Buffer_LineInsertChars(b, bl, "\t", 0, 1);
	}
	
	return i;
}


void Buffer_DeleteSelectionContents(Buffer* b, BufferRange* sel) {
	if(!sel) return;
	
	// the selection might point to something modified during line
	//   delete notifications; cache it.
	BufferRange s = *sel;
	
	Buffer_UndoSetSelection(b, s.line[0]->lineNum, s.col[0], s.line[1]->lineNum, s.col[1]);
	
	if(s.line[0] == s.line[1]) {
		// move the end down
		if(s.col[0] < s.col[1]) {
			Buffer_LineDeleteChars(b, s.line[0], s.col[0], s.col[1] - s.col[0]);
			LOG_UNDO(printf(" > moving the end down\n"));
		}
	}
	else {
		// don't truncate start line if scol is at the end
		LOG_UNDO(printf(" > non-single-line selection deletion\n"));
	
		// truncate start line after selection start
		LOG_UNDO(printf(" > truncating line '%.*s'\n", (int)s.line[0]->length - (int)s.col[0], s.line[0]->buf));
		Buffer_LineTruncateAfter(b, s.line[0], s.col[0]);			
		
		// append end line after selection ends to first line
		char* elb = s.line[1]->buf + s.col[1];
		
		LOG_UNDO(printf(" > append text '%.*s'\n", (int)s.line[1]->length - (int)s.col[1], elb));
		Buffer_LineAppendText(b, s.line[0], elb, s.line[1]->length - s.col[1]);
		
		// delete lines 1-n
		BufferLine* bl = s.line[0]->next;
		BufferLine* next;
		while(bl != s.line[1]) {
			next = bl->next; 
			//Buffer_NotifyLineDeletion(b, bl, bl);
			LOG_UNDO(printf(" > delete line %.*s\n", (int)bl->length, bl->buf));
			Buffer_DeleteLine(b, bl);
			
			if(bl == s.line[1]) break;
			bl = next;
		}
		
		LOG_UNDO(printf(" > delete last line %.*s\n", (int)s.line[1]->length, s.line[1]->buf));
		Buffer_DeleteLine(b, s.line[1]);
	}
	
	
	sel->line[1] = NULL;
	sel->col[1] = -1;
	sel->cursor = 0;
	sel->selecting = 0;
}





void Buffer_SetBookmarkAt(Buffer* b, BufferLine* bl) {
	bl->flags |= BL_BOOKMARK_FLAG;
}

void Buffer_RemoveBookmarkAt(Buffer* b, BufferLine* bl) {
	bl->flags &= ~BL_BOOKMARK_FLAG;
}

void Buffer_ToggleBookmarkAt(Buffer* b, BufferLine* bl) {
	bl->flags ^= BL_BOOKMARK_FLAG;
}



void Buffer_RelPosH(
	BufferLine* startL, 
	colnum_t startC, 
	colnum_t cols,
	BufferLine** outL,
	colnum_t* outC
) {
	
	BufferLine* bl = startL;
	intptr_t curCol = startC;
	
	intptr_t i = cols;
	if(i < 0) while(i++ < 0) {
		if(curCol <= 0) {
			
			if(bl->prev == NULL) {
				curCol = 0;
				break;
			}
			
			bl = bl->prev;
			curCol = bl->length;
		}
		else {
			curCol--;
		}
	}
	else while(i-- > 0) {
		if(curCol >= bl->length) {
			
			if(bl->next == NULL) {
				break;
			}
			
			bl = bl->next;
			curCol = 0;
		}
		else {
			curCol++;
		}
	}
	
	if(outC) *outC = curCol;
	if(outL) *outL = bl;
}



// TODO: fix max col
void Buffer_RelPosV(
	BufferLine* startL, 
	colnum_t startC, 
	linenum_t lines,
	BufferLine** outL,
	colnum_t* outC
) {
	intptr_t i = lines;
	BufferLine* bl = startL;
	
	if(i > 0) while(i-- > 0 && bl->next) {
		bl = bl->next;
	}
	else while(i++ < 0 && bl->prev) {
		bl = bl->prev;
	}
	
	if(outC) *outC = MIN(startC, bl->length);
	if(outL) *outL = bl;
}



// unindents all lines of the selection or the cursor
void Buffer_UnindentSelection(Buffer* b, BufferRange* sel) {
	/*if(!b->sel) {
		Buffer_LineUnindent(b, b->current);
		b->curCol--; // TODO: undo
		return;
	}
	*/
	BufferLine* bl = sel->line[0];
	do {
		Buffer_LineUnindent(b, bl);
	} while(bl != sel->line[1] && (bl = bl->next) && bl);
	
	sel->col[0]--;
	sel->col[1]--; // TODO: undo
	
	// careful of reaching col 0
	//b->curCol--; // TODO: undo
}


// indents all lines of the selection or the cursor
void Buffer_IndentSelection(Buffer* b, BufferRange* sel) {
	/*if(!b->sel) {
		Buffer_LineIndent(b, b->current);
		b->curCol++; // TODO: undo
		return;
	}
	*/
	BufferLine* bl = sel->line[0];
	do {
		Buffer_LineIndent(b, bl);
	} while(bl != sel->line[1] && (bl = bl->next) && bl);
	
	sel->col[0]++;
	sel->col[1]++; // TODO: undo
	
	//b->curCol++; // TODO: undo
}


void Buffer_LineIndent(Buffer* b, BufferLine* bl) {
	Buffer_LineInsertChars(b, bl, "\t", 0, 1); // TODO: adjustable 
}

void Buffer_LineUnindent(Buffer* b, BufferLine* bl) {
	if(bl->length == 0) return;
	if(bl->buf[0] != '\t') return;
	
	Buffer_LineDeleteChars(b, bl, 0, 1); // TODO: adjustable 
}



// only undo sequence breaks are handled here; all other undo actions must be 
//    added by the called functions
int Buffer_ProcessCommand(Buffer* b, GUI_Cmd* cmd, int* needRehighlight) {
	Buffer* b2;
	
	char cc[2] = {cmd->amt, 0};
	
	switch(cmd->cmd) {
			
		case GUICMD_Buffer_Undo:
			Buffer_UndoReplayToSeqBreak(b);  
			break;
			
		case GUICMD_Buffer_Redo:
			Buffer_RedoReplayToSeqBreak(b);  
			
			break;
			
		case GUICMD_Buffer_Save:
			if(!g_DisableSave) {
				Buffer_SaveToFile(b, b->sourceFile);
			}
			else {
				printf("Buffer saving disabled.\n");
			}
			break;
			
		case GUICMD_Buffer_Debug:
			switch(cmd->amt) {
				case 0: Buffer_DebugPrint(b); break;
				case 1: Buffer_DebugPrintUndoStack(b); break;
			}
			break;
			
		default:
			return 1; 
		
	}
	
	
// 	if(b->undoFill > 0) {
// 		BufferUndo* u = undo_current(b);
// 		b->isModified = u->action != UndoAction_SequenceBreak || u->unModified == 0 ;
// 	}

	
	
	
// 	printf("line/col %d:%d %d\n", b->current->lineNum, b->curCol, b->current->length);
	return 0;
}


Buffer* Buffer_Copy(Buffer* src) {
	Buffer* b = Buffer_New();
	BufferLine* blc, *blc_prev, *bl;
	
	// TODO: undo
	
	b->numLines = src->numLines;
	
	// filePath is not copied
	// selections are not copied
	
	if(src->first) {
		bl = src->first;
		blc = BufferLine_Copy(b, src->first);
		
		b->first = blc;
		bl = bl->next;
		blc_prev = blc;
		
		while(bl) {
			
			blc = BufferLine_Copy(b, bl);
			
			blc->prev = blc_prev;
			blc_prev->next = blc; 
			
			blc_prev = blc;
			bl = bl->next;
		}
		
		b->last = blc;
	}
	
	return b;
}

Buffer* Buffer_FromSelection(Buffer* src, BufferRange* sel) {
	Buffer* b = Buffer_New();
	
	LOG_UNDO(printf("\n\n"));
	
	BufferLine* blc, *blc_prev, *bl;
	
	if(!src->first || !sel->line[0]) return b;
	
	// single-line selection
	if(sel->line[0] == sel->line[1]) {
		blc = BufferLine_FromStr(b, sel->line[0]->buf + sel->col[0], sel->col[1] - sel->col[0]);
		
		b->first = blc;
		b->last = blc;
		b->numLines = 1;
		
		return b;
	}
	
	// multi-line selection
	bl = sel->line[0];
	if(sel->col[0] == 0) {
		blc = BufferLine_Copy(b, sel->line[0]);
		LOG_UNDO(printf("copy entire first line\n"));
	}
	else {
		// copy only the end
		blc = BufferLine_FromStr(b, sel->line[0]->buf + sel->col[0], sel->line[0]->length - sel->col[0]);
		LOG_UNDO(printf("copying end of the first line\n"));
	}
	
	b->numLines++;
	b->first = blc;
	bl = bl->next;
	blc_prev = blc;
	
	while(bl && bl != sel->line[1]) {
		
		blc = BufferLine_Copy(b, bl);
		
		blc->prev = blc_prev;
		blc_prev->next = blc; 
		
		b->numLines++;
		blc_prev = blc;
		bl = bl->next;
	}
	

	// copy the beginning of the last line
	blc = BufferLine_FromStr(b, sel->line[1]->buf, MIN(sel->col[1], sel->line[1]->length));
	blc->prev = blc_prev;
	blc_prev->next = blc;

	b->numLines++;
	b->last = blc;

	LOG_UNDO(printf("\n"));
	
	return b;
}


void Buffer_DebugPrint(Buffer* b) {
	BufferLine* bl;
	intptr_t i = 1, actualLines = 0;
	
	// count real lines
	bl = b->first;
	while(bl) {
		actualLines++;
		bl = bl->next;
	}
	
	printf("Buffer %p: expected lines: %ld, actual lines: %ld\n", b, b->numLines, actualLines);
	
	
	bl = b->first;
	while(bl) {
		printf("%ld: [%ld/%ld] '%.*s'\n", i, (int64_t)bl->length, (uint64_t)bl->allocSz, (int)bl->length, bl->buf);
		bl = bl->next;
		i++;
	}
}



void Buffer_InsertBufferAt(Buffer* target, Buffer* graft, BufferLine* tline, colnum_t tcol, BufferRange* outRange) {
	size_t tmplen = 0;
	char* tmp = NULL;
	BufferLine* blc, *bl;
	
	LOG_UNDO(printf("\n"));
	
	if(outRange) {
		outRange->line[0] = tline;
		outRange->col[0] = tcol;
	}
		
	// check for easy special  cases
	if(graft->numLines == 0) {
		LOG_UNDO(printf("zero-line buffer insert\n\n"));
		return;
	}
	
	if(graft->numLines == 1) {
		LOG_UNDO(printf("single-line buffer insert\n\n"));
		Buffer_LineInsertChars(target, tline, graft->first->buf, tcol, graft->first->length);
		
		if(outRange) {
			outRange->line[1] = tline;
			outRange->col[1] = tcol + graft->first->length;
		}
		
		return;
	}
	
	LOG_UNDO(printf("multi-line buffer insert:\n"));

	BufferLine* t = tline;
	BufferLine* gbl = graft->first->next;
	
	// cutting the remainder of the first line to a temporary buffer
	if(tline->length && tcol > 0) {
		tmplen = tline->length - tcol;
		tmp = malloc(tmplen);
		memcpy(tmp, tline->buf + tcol, tmplen);
		LOG_UNDO(printf(" > tmp: '%.*s'\n", (int)tmplen, tmp));
		
		tline->length = tcol;
	}
	else if(graft->last->length == 0) {
		LOG_UNDO(printf(" > inserting empty line before\n"));
		t = Buffer_InsertEmptyLineBefore(target, tline);
	}
	
	LOG_UNDO(printf(" > inserting first line '%.*s'\n", (int)graft->first->length, graft->first->buf));
	Buffer_LineAppendText(target, t, graft->first->buf, graft->first->length);
	
	// copy in the middle lines
	while(gbl && gbl != graft->last) {
		
		LOG_UNDO(printf(" > inserting line '%.*s'\n", (int)gbl->length, gbl->buf));
		t = Buffer_InsertLineAfter(target, t, gbl->buf, gbl->length);
		
		gbl = gbl->next;
	}
	
	if(graft->last->length > 0) {
		LOG_UNDO(printf(" > inserting last line '%.*s'\n", (int)gbl->length, gbl->buf));
		t = Buffer_InsertLineAfter(target, t, gbl->buf, gbl->length);
	}
	
	// prepend the last line to the temp buffer
	if(tmplen) {
		LOG_UNDO(printf(" > appending last line (%d)'%.*s'\n", (int)tmplen, (int)tmplen, tmp));
	
		Buffer_LineAppendText(target, t, tmp, tmplen);
	}
	
	
	if(outRange) {
		if(graft->last->length == 0) t = t->next;
		outRange->line[1] = t;
		outRange->col[1] = gbl->length;
	}
	
	if(tmp)	free(tmp);
}



void Buffer_AppendRawText(Buffer* b, char* source, intptr_t len) {
	if(len == 0) len = strlen(source);
	
	if(len == 0) return; 
	
	char* s = source;
	for(size_t i = 0; s < source + len; i++) {
		
		if(*s == 0) break;
		
		char* e = strpbrk(s, "\r\n");
		
		if(e == NULL) {
			Buffer_AppendLine(b, s, strlen(s));
			return;
		}
		
		// TODO: robust input handling with unicode later
// 		if(*e == '\n') {
			Buffer_AppendLine(b, s, (intptr_t)(e-s));
// 			BufferLine* bl = Buffer_raw_InsertLineAfter(b, b->last);
// 			Buffer_raw_InsertChars(b, bl, s, 0, (intptr_t)(e-s)); 
// 		}
		
// 		if(b->last->buf && b->last->length > 0) {
// 			b->last->style = calloc(1, sizeof(*b->last->style));
// 			HL_acceptLine(b->last->buf, b->last->length, b->last->style);
// 		}
		
		s = e + 1;
	}
}



// TODO: undo
/*
BufferLine* Buffer_Prepline[1](GUIBufferEditControl* w, char* text, intptr_t len) {
	assert(0); // should be deprecated
	Buffer* b = w->buffer;
	BufferLine* l = pcalloc(l);
	
	b->numLines++;
	
	BufferLine_SetText(l, text, len);
	
	if(b->last == NULL) {
		b->first = l;
		b->last = l;
		b->current
		l->lineNum = 1;
		return l;
	}
	
	l->prev = b->current->prev;
	l->next = b->current;
	b->current->prev = l;
	if(b->current->next) b->current->next->prev = l;
	
	Buffer_raw_RenumberLines(l, b->current->lineNum);
	
	if(b->current->prev == NULL) {
		b->first = l;
		l->lineNum = 1;
		return l;
	}
	
	return l;
}
*/

BufferLine* Buffer_AppendLine(Buffer* b, char* text, intptr_t len) {
	return Buffer_InsertLineAfter(b, b->last, text, len);
}

// TODO: undo
void Buffer_ToRawText(Buffer* b, char** out, size_t* outLen) {
	
	// calculate the buffer size
	size_t len = 0;
	BufferLine* l = b->first;
	while(l) {
		len += l->length + 1;
		l = l->next;
	}
	
	char* o = malloc(len + 1);
	char* end = o;
	
	int i = 0;
	// copy the lines one at a time
	l = b->first;
	while(l) {
		if(l->buf) strncpy(end, l->buf, l->length);
		end += l->length;
		*end = '\n';
		end++;
		l = l->next;
		
		i++;
	}
	
	*out = o;
	*outLen = len;
}

// TODO: undo
int Buffer_SaveToFile(Buffer* b, char* path) {
	FILE* f;
	
	f = fopen(path, "wb");
	if(!f) return 1;
	
	char* o;
	size_t len;
	Buffer_ToRawText(b, &o, &len);
	
	fwrite(o, 1, len, f);
	
	free(o);
	fclose(f);
	printf("'%s' Saved\n", path);
	Buffer_UndoSequenceBreak(b, 1, -1, -1, -1, -1, 0);
	
	return 0;
}

// TODO: undo
int Buffer_LoadFromFile(Buffer* b, char* path) {
	FILE* f;
	char* o;
	
	f = fopen(path, "rb");
	if(!f) return 1;
	
	
	fseek(f, 0, SEEK_END); 
	size_t len = ftell(f);
	fseek(f, 0, SEEK_SET);
	
	o = malloc(len + 1);
	o[len] = 0;
	fread(o, 1, len, f);
	Buffer_AppendRawText(b, o, len);
	
	free(o);
	fclose(f);
	
	Buffer_UndoTruncateStack(b);
	
	if(b->sourceFile) free(b->sourceFile);
	b->sourceFile = strdup(path);
	
	return 0;
}


void Buffer_LineEnsureEnding(Buffer* b, BufferLine* bl, char* text, intptr_t length) {
	if(!text) return;
	if(!length) return;
	
	if(bl->length >= length) {
		if(0 == strncmp(bl->buf + bl->length - length, text, length)) return;
	}
	
	Buffer_LineAppendText(b, bl, text, length);
}

void Buffer_LineEnsureEndingSelection(Buffer* b, BufferRange* sel, char* text, intptr_t length) {
	if(!text) return;
	if(!length) return;

	BufferLine* bl = sel->line[0];
	while(bl) {
		Buffer_LineEnsureEnding(b, bl, text, length);
		
		if(bl == sel->line[1]) break;
		bl = bl->next;
	}
}


void Buffer_LinePrependText(Buffer* b, BufferLine* bl, char* text) {
	if(!text) return;
	size_t len = strlen(text);
	if(!len) return;
	
	Buffer_LineInsertChars(b, bl, text, 0, len);
}

void Buffer_LinePrependTextSelection(Buffer* b, BufferRange* sel, char* text) {
	if(!text) return;
	size_t len = strlen(text);
	if(!len) return;

	BufferLine* bl = sel->line[0];
	while(bl) {
		Buffer_LinePrependText(b, bl, text);
		
		if(bl == sel->line[1]) break;
		bl = bl->next;
	}
}
	
void Buffer_LineAppendTextSelection(Buffer* b, BufferRange* sel, char* text, intptr_t length) {
	if(!text) return;
	if(!length) return;

	BufferLine* bl = sel->line[0];
	while(bl) {
		Buffer_LineAppendText(b, bl, text, length);
		
		if(bl == sel->line[1]) break;
		bl = bl->next;
	}
}
	

void Buffer_LineUnprependText(Buffer* b, BufferLine* bl, char* text) {
	if(!text || !bl->buf) return;
	size_t len = strlen(text);
	if(!len) return;
	
	if(0 != strncmp(bl->buf, text, len)) return;
	
	Buffer_LineDeleteChars(b, bl, 0, len);
}

void Buffer_LineUnprependTextSelection(Buffer* b, BufferRange* sel, char* text) {
	if(!text) return;
	size_t len = strlen(text);
	if(!len) return;

	BufferLine* bl = sel->line[0];
	while(bl) {
		Buffer_LineUnprependText(b, bl, text);
		
		if(bl == sel->line[1]) break;
		bl = bl->next;
	}
}


void Buffer_SurroundSelection(Buffer* b, BufferRange* sel, char* begin, char* end) {
	if(!sel) return;
	
	// end must be first
	if(end) {
		BufferLine_InsertChars(
			sel->line[1],
			end, 
			sel->col[1],
			strlen(end)
		);
	}

	if(begin) {
		BufferLine_InsertChars(
			sel->line[0], 
			begin,  
			sel->col[0],
			strlen(begin)
		);
	}
}

int Buffer_UnsurroundSelection(Buffer* b, BufferRange* sel, char* begin, char* end) {
	int out = 0;
	
	if(!sel) return 0;

	// end must be first
	size_t elen = strlen(end);
	if(0 == strncmp(sel->line[1]->buf + sel->col[1] - elen, end, elen)) {
		Buffer_LineDeleteChars(b, sel->line[1], sel->col[1] - elen, elen);
		out |= (1 << 1);
	}
	
	size_t slen = strlen(begin);
	if(0 == strncmp(sel->line[0]->buf + sel->col[0], begin, slen)) {
		Buffer_LineDeleteChars(b, sel->line[0], sel->col[0], slen);
		out |= (1 << 0);
	}
	
	return out;
}




/*
void Buffer_AddCurrentSelectionToRing(Buffer* b) {
	if(!b->selectionRing->first) {
		b->selectionRing->first = b->sel;
		b->selectionRing->last = b->sel;
		return;
	}
	// TODO: undo
	// find where to insert
	BufferRange* bs = b->selectionRing->first;
	while(bs != b->selectionRing->last) {
		
		if(bs->line[0]->lineNum < b->sel->line[0]->lineNum) {
			
		}
		
	}
	
}
*/

char* Buffer_StringFromSelection(Buffer* b, BufferRange* sel, size_t* outLen) {
	char* out;
	size_t len = 0;
	size_t alloc = 64;
	
	out = malloc(alloc * sizeof(*out));
	out[0] = 0;
	
	BufferLine* bl = sel->line[0];
	while(bl) {
		size_t scol = bl == sel->line[0] ? sel->col[0] : 0;
		size_t ecol = bl == sel->line[1] ? sel->col[1] : bl->length;
		size_t cols = ecol - scol;
		
		if(alloc - len < cols + 1) {
			alloc = MAX(alloc * 2, alloc + cols + 1);
			out = realloc(out, sizeof(*out) * alloc);
		}
		
		// TODO: not slow, shitty algorithm
		strncat(out, bl->buf + scol, cols);
		
		len += cols;
		
		if(bl == sel->line[1]) break;
		
		strcat(out, "\n");
		len++;
		bl = bl->next;
	}
	
	if(outLen) *outLen = len;
	return out;
}

void Buffer_GetSequenceUnder(Buffer* b, BufferLine* l, colnum_t col, char* charSet, BufferRange* out) {
	intptr_t start, end;
	
	for(start = col; start >= 0; start--) {
		if(NULL == strchr(charSet, l->buf[start])) {
			start++;
			break;
		}
	}
	if(start == -1) start = 0;
	
	for(end = col; end < l->length; end++) {
		if(NULL == strchr(charSet, l->buf[end])) {
			break;
		}
	}
	if(end > l->length) end = l->length;
	
	out->line[0] = l;
	out->line[1] = l;
	out->col[0] = start;
	out->col[1] = end;
}


int Buffer_FindEndOfSequence(Buffer* b, BufferLine** linep, colnum_t* colp, char* charSet) {
	intptr_t col = *colp;
	BufferLine* l = *linep;
	
	if(!l) return 0;
	
	
	while(1) {
		// handle line endings
		while(col >= l->length) {
			if(!l->next) { // end of file
				*linep = l;
				*colp = l->length;
				return 0;
			}
			
			// TODO check charset for \n
			l = l->next;
			col = 0;
		}
		
		
		int c = l->buf[col];
		
		if(NULL == strchr(charSet, c)) {
			// found the next sequence
			*linep = l;
			*colp = col;
			return 0; // in a sequence
		}
		
		col++;
	}
	
	return 1;
}

int Buffer_FindEndOfInvSequence(Buffer* b, BufferLine** linep, colnum_t* colp, char* charSet) {
	intptr_t col = *colp;
	BufferLine* l = *linep;
	
	if(!l) return 0;
	
	while(1) {
		// handle line endings
		while(col >= l->length) {
			if(!l->next) { // end of file
				*linep = l;
				*colp = l->length;
				return 0;
			}
			
			// TODO check charset for \n
			l = l->next;
			col = 0;
		}
		
		
		int c = l->buf[col];
		
		if(NULL != strchr(charSet, c)) {
			// found the next sequence
			*linep = l;
			*colp = col;
			return 0; // in a sequence
		}
		
		col++;
	}
	
	// shouldn't reach here
	
	return 1;
}

int Buffer_FindSequenceEdgeForward(Buffer* b, BufferLine** linep, colnum_t* colp, char* charSet) {
	intptr_t col = *colp;
	BufferLine* l = *linep;
	
	int inside = -1;
	int c;
	
	if(!l) return 0;
	
	while(1) {
		// handle line endings
		while(col > l->length) {
			if(!l->next) { // end of file
				*linep = l;
				*colp = l->length;
				return 0;
			}
			
			// TODO check charset for \n
			l = l->next;
			col = 0;
		}
		
		
		c = (col == l->length) ? '\n' : l->buf[col];
		
		char* r = strchr(charSet, c);
		
		if(inside == 1) {
			if(r == NULL) {
				// found the transition
				*linep = l;
				*colp = col;
				return 0; // in a sequence
			}
		}
		else if(inside == 0) {
			if(r != NULL) {
				// found the transition
				*linep = l;
				*colp = col;
				return 0; // in a sequence
			}
		}
		else {
			inside = r != NULL;
		}
		
		col++;
	}
	
	// shouldn't reach here
	
	return 1;
}

int Buffer_FindSequenceEdgeBackward(Buffer* b, BufferLine** linep, colnum_t* colp, char* charSet) {
	intptr_t col = *colp;
	BufferLine* l = *linep;
	
	int inside = -1;
	int c;
	
	col--;
	
	if(!l) return 0;
	
	while(1) {
		// handle line endings
		while(col <= 0) {
			if(!l->prev) { // beginning of file
				*linep = l;
				*colp = 0;
				return 0;
			}
			
			// TODO check charset for \n
			l = l->prev;
			col = l->length;
		}
		
		
		c = (col == l->length) ? '\n' : l->buf[col];
		
		char* r = strchr(charSet, c);
		
		if(inside == 1) {
			if(r == NULL) {
				// found the transition
				*linep = l;
				*colp = MIN(col+1, l->length);
				return 0; // in a sequence
			}
		}
		else if(inside == 0) {
			if(r != NULL) {
				// found the transition
				*linep = l;
				*colp = MIN(col+1, l->length);
				return 0; // in a sequence
			}
		}
		else {
			inside = r != NULL;
		}
		
		col--;
	}
	
	// shouldn't reach here
	
	return 1;
}


void Buffer_DebugPrintUndoStack(Buffer* b) {
	printf("\n");
	printf("Undo stack for %p (%d entries)\n", b, b->undoFill);
	
	char* names[] = { 
		"InsText",
		"DelText",
		"InsChar",
		"DelChar",
		"InsLine", 
		"DelLine",
		"MvCurTo",
		"SetSel ",
//		"UnmodFl",
		"SequBrk",
	};
	
	for(int ii = 0; ii < b->undoFill; ii++) {
		int i = (ii + b->undoOldest) % b->undoMax;
		BufferUndo* u = b->undoRing + ii;
		char c = ' ';
		if(ii == b->undoSaveIndex) c  = 's';
		if(ii == b->undoCurrent) c  = '>';
		
		switch(u->action) {
			case UndoAction_DeleteText:
			case UndoAction_InsertText:
			case UndoAction_DeleteLine:
			case UndoAction_InsertLineAt:
				printf("%c%d [%s] {line:%ld:%ld} len:%ld:'%.*s'\n", c, i, names[u->action], 
					(uint64_t)u->lineNum, (uint64_t)u->colNum, u->length, (int)u->length, u->text);
				break;
				
			case UndoAction_MoveCursorTo:
			case UndoAction_SequenceBreak:
				printf("%c%d [%s] {line:%ld:%ld} len:%ld\n", c, i, names[u->action],
					(uint64_t)u->lineNum, (uint64_t)u->colNum, u->length);
				break;
				
			default:
				fprintf(stderr, "Unknown redo action: %d\n", u->action);
		}
	}
	
	printf(" Undo save index: %d\n", b->undoSaveIndex);
	printf(" Undo curr index: %d\n", b->undoCurrent);
	printf("\n");
}



void Buffer_CollapseWhitespace(Buffer* b, BufferLine* l, colnum_t col) {
	intptr_t start, end;
	
	if(col > l->length) return;
	
	// find start of ws
	for(start = col; start >= 0; start--) {
		if(!isspace(l->buf[start])) {
			start++;
			break;
		}
	}
	if(start == -1) start = 0;
	
	// find end of ws
	for(end = col; end < l->length; end++) {
		if(!isspace(l->buf[end])) {
			break;
		}
	}
	if(end > l->length) end = l->length;
	
	if(end - start > 1) { 
		Buffer_LineDeleteChars(b, l, start, end - start);
		Buffer_LineInsertChars(b, l, " ", start, 1);
		//Buffer_MoveCursorTo(b, l, start + 1);
	}
}


static void ac_insert_word(BufferPrefixNode* n, char* s, size_t len) {
	n->refs++;
	
	if(!*s || len == 0) {
		n->trefs++;
		return;
	}
	
	
	BufferPrefixNode* k = n->kids;
	while(k && k->c != *s) k = k->next;
	
	if(!k) {
		k = pcalloc(k);
//		k->parent = n;
		k->next = n->kids;
		n->kids = k;
		k->c = *s;
	}
	
	ac_insert_word(k, s + 1, len - 1);
}

// returns 1 if the node was freed
static void ac_remove_word(BufferPrefixNode* n, char* s, size_t len) {
	n->refs--;


	// TODO: GC zeroed nodes
	if(!*s || len == 0) {
		n->trefs--;
		
		return;
	}
	
	BufferPrefixNode* k = n->kids;
	while(k && k->c != *s) k = k->next;
	
	if(!k) {
		n->trefs--;
		
		return;
	}
	
	ac_remove_word(k, s + 1, len - 1);
}

int Buffer_AddDictWord(Buffer* b, char* word, size_t len) {
	ac_insert_word(b->dictRoot, word, len);
	return 0;
}

int Buffer_RemoveDictWord(Buffer* b, char* word, size_t len) {
	ac_remove_word(b->dictRoot, word, len);
	return 0;
}



void Buffer_AddLineToDict(Buffer* b, BufferLine* l) {
		
	size_t n, e = 0;
	char* s = l->buf;
	
	if(!s) return;
	
	// BUG: requires null terminators
	while(1) {
		// skip the junk
		for(; e < l->length; e++, s++) {
			if(strchr(b->dictCharSet, *s)) break;
		}
		
		// span the word		
		for(n = 0; e < l->length; n++, e++) {
			if(NULL == strchr(b->dictCharSet, s[n])) break;
		}

		if(n <= 0) break;
		
		Buffer_AddDictWord(b, s, n);
		
		s += n;
	}
}

void Buffer_RemoveLineFromDict(Buffer* b, BufferLine* l) {
	
	size_t n, e = 0;
	char* s = l->buf;
	
	if(!s) return;
	
	while(1) {
		// skip the junk
		for(; e < l->length; e++, s++) {
			if(strchr(b->dictCharSet, *s)) break;
		}
		
		// span the word		
		for(n = 0; e < l->length; n++, e++) {
			if(NULL == strchr(b->dictCharSet, s[n])) break;
		}

		if(n <= 0) break;
		
		Buffer_RemoveDictWord(b, s, n);
		
		s += n;
	}
}


void ac_print_tree(BufferPrefixNode* n, char* buf, int len) {
	if(len > 1023) return;
	
	buf[len] = n->c;
//	if(n->terminal) 
	printf("%.*s: %ld, %ld\n", len, buf + 1, n->refs, n->trefs);
	
		
	BufferPrefixNode* k = n->kids;
	while(k) {
		ac_print_tree(k, buf, len + 1);
		k = k->next;
	}
}

void Buffer_PrintDict(Buffer* b) {
	char buf[1024];
	
	ac_print_tree(b->dictRoot, buf, 0);
}



BufferPrefixNode* ac_find_tail(BufferPrefixNode* n, char* s, int len) {
	
	if(len == 0) return n;
	if(!n) return NULL;
	
	BufferPrefixNode* k = n->kids;
	while(k && !((k->c) == (*s))) k = k->next;
	
	if(!k) return NULL;
	
	return ac_find_tail(k, s + 1, len - 1);
}





static void ac_find_matches(BufferACMatchSet* ms, BufferPrefixNode* n, char* buf, int len, char* search, int searchLen) {
	
	if(len >= 100) return;
	
	// prune bad search branches
	if(ms->len >= ms->alloc && n->refs < ms->worst) return;
	if(n->refs <= 0) return;
	
	
	buf[len] = n->c;
	
	
	if(len + 1 < searchLen) { // recurse into the tree along the search prefix
		BufferPrefixNode* k = n->kids;
		while(k) {
			if(tolower(k->c) == tolower(search[len + 1])) {
				ac_find_matches(ms, k, buf, len + 1, search, searchLen);
			}
			
			k = k->next;
		}
		
		return;
	}
	
	
	//                       don't autocomplete a word to itself; this is case sensitive
	if(n->trefs > 0 && !(ms->targetLen == len + 1 && !strncmp(buf, ms->targetStart, len + 1))) {
		
		// look for the sorted position this match should fall in to
		for(int i = 0; i < ms->len; i++) {
			if(n->refs > ms->matches[i].rank) {
				if(i < ms->alloc - 1) {
					
					// clean up the string about to fall off the end
					if(ms->len == ms->alloc) {
						if(ms->matches[ms->len - 1].s) free(ms->matches[ms->len - 1].s);
					}
					
					// move worse matches down
					memmove(ms->matches + i + 1, ms->matches + i, sizeof(*ms->matches) * (ms->alloc - i - 1));
				}
				
				// insert this match into sorted position
				ms->matches[i].s = strndup(buf, len + 1);
				ms->matches[i].len = len + 1;
				ms->matches[i].rank = n->refs;
				
				ms->worst = ms->matches[ms->len - 1].rank;
				ms->len = MIN(ms->len + 1, ms->alloc); 
				
				goto FOUND;
			}
		
		}
		
		// this match is worse than the entire list
		// insert is at the end of the list if there is room
		if(ms->len < ms->alloc) {
			ms->matches[ms->len].s = strndup(buf, len + 1);
			ms->matches[ms->len].len = len + 1;
			ms->matches[ms->len].rank = n->refs;
			ms->len++;
		}
	}

FOUND:
	
	// recurse through kids
	BufferPrefixNode* k = n->kids;
	while(k) {
		ac_find_matches(ms, k, buf, len + 1, search, searchLen);
		k = k->next;
	}
}


static void ac_find_matches_root(BufferACMatchSet* ms, BufferPrefixNode* n, char* buf, int len, char* search, int searchLen) {
	BufferPrefixNode* k = n->kids;
	while(k) {
		if(tolower(k->c) == tolower(search[0])) {
			ac_find_matches(ms, k, buf, 0, search, searchLen);
		}
		
		k = k->next;
	}
	
}


BufferACMatchSet* Buffer_FindDictMatches(Buffer* b, BufferRange* r) {
	char buf[1024];
	colnum_t i, e;
	
	BufferLine* l = CURSOR_LINE(r);
	colnum_t c = CURSOR_COL(r);
	
	if(c == 0) return NULL;
	
	for(i = c - 1; i >= 0; i--) {
		if(!strchr(b->dictCharSet, l->buf[i])) break;
	}
	i++;
	
	for(e = c; e < l->length; e++) {
		if(!strchr(b->dictCharSet, l->buf[e])) break;
	}

	int len = c - i;
	if(len < 1) return NULL;
	
	BufferACMatchSet* ms = pcalloc(ms);
	ms->alloc = 16;
	ms->matches = calloc(1, sizeof(*ms->matches) * ms->alloc);
	ms->r = *r;
	ms->target.line[0] = l;
	ms->target.line[1] = l;
	ms->target.col[0] = i;
	ms->target.col[1] = e;
	ms->targetLen = e - i;
	ms->targetStart = l->buf + i;
	
	ac_find_matches_root(ms, b->dictRoot, buf, 0, l->buf + i, len);
	
	if(ms->len == 0) { // never return an empty result set
		free(ms->matches);
		free(ms);
		return NULL;
	}
//	for(int j = 0; j < ms->len; j++) {
//		printf("%.*s: %f\n", ms->matches[j].len, ms->matches[j].s, ms->matches[j].rank);
//	}

	// cache some values for rendering
	ms->shortestMatchLen = ms->matches[0].len;
	for(int j = 0; j < ms->len; j++) {
		ms->longestMatchLen = MAX(ms->longestMatchLen, ms->matches[j].len);
		ms->shortestMatchLen = MIN(ms->shortestMatchLen, ms->matches[j].len);
	}
	
	// find common prefix, if any
	ms->commonPrefixLen = ms->shortestMatchLen;
	int a;
	for(a = 0; a < ms->shortestMatchLen; a++) {
		
		int c = ms->matches[0].s[a];
		for(int j = 1; j < ms->len; j++) {
			if(ms->matches[j].s[a] == c) continue;
		
			// mismatch
			ms->commonPrefixLen = a;
			goto PREFIX_DONE;
		}
	}
	
PREFIX_DONE:
	


	return ms;
}

void Buffer_NotifyUndoSetSelection(Buffer* b, BufferLine* startL, colnum_t startC, BufferLine* endL, colnum_t endC, char isReverse) {
	BufferChangeNotification note = {
		.b = b,
		.sel.line[0] = startL,
		.sel.line[1] = endL,
		.sel.col[0] = startC,
		.sel.col[1] = endC,
		
		.action = BCA_Undo_SetSelection,
	};

	Buffer_NotifyChanges(&note);	
}


void Buffer_NotifyUndoMoveCursor(Buffer* b, BufferLine* line, colnum_t col) {
	BufferChangeNotification note = {
		.b = b,
		.sel.line[0] = line,
		.sel.line[1] = 0,
		.sel.col[0] = col,
		.sel.col[1] = 0,
		
		.action = BCA_Undo_MoveCursor,
	};

	Buffer_NotifyChanges(&note);	
}


void Buffer_NotifyLineDeletion(Buffer* b, BufferLine* sLine, BufferLine* eLine) {
	BufferChangeNotification note = {
		.b = b,
		.sel.line[0] = sLine,
		.sel.line[1] = eLine,
		.sel.col[0] = 0,
		.sel.col[1] = eLine->length,
		
		.action = BCA_DeleteLines,
	};

	Buffer_NotifyChanges(&note);
}

void Buffer_NotifyChanges(BufferChangeNotification* note) {
	VEC_EACH(&note->b->changeListeners, i, list) {
		if(list.fn) list.fn(note, list.data);
	}
}

void Buffer_RegisterChangeListener(Buffer* b, bufferChangeNotifyFn fn, void* data) {
	VEC_INC(&b->changeListeners);
	VEC_TAIL(&b->changeListeners).fn = fn;
	VEC_TAIL(&b->changeListeners).data = data;
}


static char* strnstrn(char* hay, size_t hlen, char* needle, size_t nlen, char** end_out) {
	if(nlen > hlen) return NULL;
	
	size_t maxlen = hlen - nlen;
	
	for(size_t h = 0; h <= maxlen; h++) {
		for(size_t n = 0; n < nlen; n++) {
			if(hay[h + n] != needle[n]) goto NOPE;
		}
		
		// found match
		*end_out = hay + h + nlen;
		return hay + h;
		
	NOPE:
		continue;
	}
	
	return NULL; // match failed
}


// returns 0 on success
// finds first match only
// no support for newlines in needle
int Buffer_strstr(Buffer* b, char* needle, BufferRange* out) {
	
	BufferLine* bl;
	
	char* nlen = strlen(needle);
	
	bl = b->first;
	while(bl) {
		char* start, *end;
	
		start = strnstrn(bl->buf, bl->length, needle, nlen, &end);
		if(start) {
			out->line[0] = bl;
			out->line[1] = bl;
			out->col[0] = start - bl->buf;
			out->col[1] = end - bl->buf;
			
			return 0;
		}
		
		
		bl = bl->next;
	}
	
	return 1;
}



int Buffer_ReplaceAllString(Buffer* dst, char* needle, Buffer* src) {
	
	BufferRange r;
	BufferLine* dbl;
	size_t nlen = strlen(needle);
	
	dbl = dst->first;
		
	while(dbl) {
		size_t scol = 0;
		
		while(1) {
			char* start, *end;
			
			start = strnstrn(dbl->buf + scol, dbl->length - scol, needle, nlen, &end);
			if(!start) break;
			
			r.line[0] = dbl;
			r.line[1] = dbl;
			r.col[0] = start - dbl->buf;
			r.col[1] = end - dbl->buf;
			
			Buffer_DeleteSelectionContents(dst, &r);
			Buffer_InsertBufferAt(dst, src, dbl, r.col[0], &r);
			
			dbl = r.line[1];
			scol = r.col[1];
		}
		
		
		dbl = dbl->next;
	}
	
	return 0;
}

	
