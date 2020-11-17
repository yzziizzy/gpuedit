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



Buffer* Buffer_New() {
	Buffer* b = pcalloc(b);
	
	b->refs = 1;
	
	b->undoMax = 4096; // TODO: settings
	b->undoRing = calloc(1, b->undoMax * sizeof(*b->undoRing));
	
	HT_init(&b->dict, 128);
	
	// HACK should go in settings files
	b->useDict = 1;
	b->dictCharSet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
	
	return b;
}


void Buffer_AddRef(Buffer* b) {
	b->refs++;
}

void Buffer_Delete(Buffer* b) {
	b->refs--;
	if(b->refs > 0) return;
	
	if(b->filePath) free(b->filePath);
	
	// free all the lines
	BufferLine* bl = b->first;
	BufferLine* bln = NULL;
	
	while(bl) {
		bln = bl->next;
		BufferLine_Delete(bl);
		bl = bln;
	}
	
	free(b);
	
	Buffer_FreeAllUndo(b);
}


static int undo_avail(Buffer* b) {
	return b->undoMax - b->undoFill;
}

static BufferUndo* undo_inc(Buffer* b) {
	// TODO: adjust size dynamically sometimes
	BufferUndo* u = b->undoRing + b->undoCurrent;
	
	if(b->undoFill == 0) {
		b->undoCurrent = (b->undoCurrent + 1) % b->undoMax;
		b->undoNext = b->undoCurrent;
		b->undoFill++;
		return u;
	}
	
	if(undo_avail(b) <= 0) {
		// clean up the old data
		if(u->text) free(u->text);
		u->text = NULL;
		b->undoOldest = (b->undoOldest + 1) % b->undoMax;
	}
	else {
		b->undoFill++;
	}
	
	b->undoCurrent = (b->undoCurrent + 1) % b->undoMax;
	b->undoNext = b->undoCurrent;
	
	return u;
}

static BufferUndo* undo_current(Buffer* b) {
	return b->undoRing + b->undoCurrent;
}

static BufferUndo* undo_dec(Buffer* b) {
	if(b->undoFill <= 0) return NULL;
	if(b->undoOldest == b->undoCurrent) return NULL;
	
	b->undoCurrent = (b->undoCurrent - 1 + b->undoMax) % b->undoMax;
	
	// next is not moved; it is the end of the redo stack
// 	b->undoNext = (b->undoCurrent - 1 + b->undoMax) % b->undoMax;
	
	return b->undoRing + b->undoCurrent;
}

// like dec but doesn't move current
static BufferUndo* undo_peek(Buffer* b) {
	if(b->undoFill <= 0) return NULL;
	if(b->undoOldest == b->undoCurrent) return NULL;
	
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
	intptr_t line, 
	intptr_t col, 
	char* txt, 
	intptr_t len
) {
	
	if(len == 0) return;
	
// 	VEC_INC(&b->undoStack);
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_inc(b);
	
	u->action = UndoAction_InsertText;
	u->lineNum = line;
	u->colNum = col;
	u->text = txt ? strndup(txt, len) : NULL;
	u->length = len;
}

void Buffer_UndoDeleteText(Buffer* b, BufferLine* bl, intptr_t offset, intptr_t len) {
	
	if(len == 0) return;
	
// 	VEC_INC(&b->undoStack);
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_inc(b);
	
	u->action = UndoAction_DeleteText;
	u->lineNum = bl->lineNum;
	u->colNum = offset;
	u->text = bl->buf ? strndup(bl->buf + offset, len) : NULL;
	u->length = len;
}


void Buffer_UndoSetSelection(Buffer* b, intptr_t startL, intptr_t startC, intptr_t endL, intptr_t endC) {
	BufferUndo* u = undo_inc(b);
	
	u->action = UndoAction_SetSelection;
	u->lineNum = startL;
	u->colNum = startC;
	u->endLine = endL;
	u->endCol = endC;
}

void Buffer_UndoSequenceBreak(Buffer* b, int saved, 
	intptr_t startL, intptr_t startC, intptr_t endL, intptr_t endC, char isReverse) {
	
	// don't add duplicates
	if(b->undoFill > 0) {
		if(undo_current(b)->action == UndoAction_SequenceBreak) {
			if(saved) b->undoSaveIndex = b->undoCurrent;
			return;
		}
	}
	
// 	VEC_INC(&b->undoStack);
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_inc(b);
	
	printf("saving seq break: %ld:%ld -> %ld->%ld\n",
		startL, startC, endL, endC);
	
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
	BufferUndo* u = undo_inc(b);
	
	u->action = UndoAction_InsertLineAfter;
	u->lineNum = before ? before->lineNum : 0; // 0 means the first line 
	u->colNum = 0;
	u->text = NULL;
	u->length = 0;
}


void Buffer_UndoDeleteLine(Buffer* b, BufferLine* bl) {
// 	VEC_INC(&b->undoStack);
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_inc(b);
	
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
	b->undoNext = 0;
	b->undoCurrent = 0;
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
	if(b->undoCurrent == b->undoNext) {
// 		printf("redo: current reached next\n");
		return;
	}
	
	BufferUndo* u;
	u = b->undoRing + b->undoCurrent;
	b->undoCurrent = (b->undoCurrent + 1) % b->undoMax;
	
	
	Buffer_RedoReplay(b, u);
	
	// pop off the sequence break
// 	undo_dec(b);
// 	if(VEC_LEN(&b->undoStack) >= 0) VEC_POP1(&b->undoStack);
}


// executes a single undo action
// returns 0 on a sequence break
int Buffer_UndoReplayTop(Buffer* b) {
	BufferLine* bl;
	
	if(b->undoFill == 0) return 0;
// 	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	BufferUndo* u = undo_dec(b);
	if(u == NULL) return 0;
	
	// these all need to be the inverse
	switch(u->action) {
		case UndoAction_InsertText: // delete text
			bl = Buffer_raw_GetLine(b, u->lineNum);
			Buffer_raw_DeleteChars(b, bl, u->colNum, u->length);
			break;
			
		case UndoAction_DeleteText: // re-insert the text
			bl = Buffer_raw_GetLine(b, u->lineNum);
			Buffer_raw_InsertChars(b, bl, u->text, u->colNum, u->length);
			//b->current = bl; // BUG not right after <delete> key
			//b->curCol = u->colNum + u->length;
			break;
			
		case UndoAction_InsertLineAfter: // delete the line
			bl = Buffer_raw_GetLine(b, u->lineNum + 1);
			Buffer_raw_DeleteLine(b, bl);
			break; 
			
		case UndoAction_DeleteLine: // insert the line
			if(u->lineNum == 0) {
				BufferLine* bln = Buffer_raw_InsertLineBefore(b, b->first);
				Buffer_raw_InsertChars(b, bln, u->text, 0, u->length);
			}
			else {
				bl = Buffer_raw_GetLine(b, u->lineNum);
				BufferLine* bln = Buffer_raw_InsertLineBefore(b, bl);
				Buffer_raw_InsertChars(b, bln, u->text, 0, u->length);
			}
			break;
			
		case UndoAction_SetSelection:
			/*Buffer_SetCurrentSelection(b, 
				Buffer_raw_GetLine(b, u->lineNum),
				u->colNum,
				Buffer_raw_GetLine(b, u->endLine),
				u->endCol);*/
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
					u->lineNum, u->colNum,
					u->endLine, u->endCol
				);
				Buffer_NotifyUndoSetSelection(b, 
					Buffer_raw_GetLine(b, u->lineNum), u->colNum,
					Buffer_raw_GetLine(b, u->endLine), u->endCol,
					u->isReverse
				);
			}
			else if(u->lineNum > 0) {
				Buffer_NotifyUndoMoveCursor(b, Buffer_raw_GetLine(b, u->lineNum), u->colNum);
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
			bl = Buffer_raw_GetLine(b, u->lineNum);
			Buffer_raw_DeleteChars(b, bl, u->colNum, u->length);
			break;
			
		case UndoAction_InsertText:
			bl = Buffer_raw_GetLine(b, u->lineNum);
			Buffer_raw_InsertChars(b, bl, u->text, u->colNum, u->length);
			//b->current = bl; // BUG not right after <delete> key
			//b->curCol = u->colNum + u->length;
			break;
			
		case UndoAction_DeleteLine: 
			bl = Buffer_raw_GetLine(b, u->lineNum);
			Buffer_raw_DeleteLine(b, bl);
			break; 
			
		case UndoAction_InsertLineAfter:
			if(u->lineNum == 0) {
				BufferLine* bln = Buffer_raw_InsertLineBefore(b, b->first);
				Buffer_raw_InsertChars(b, bln, u->text, 0, u->length);
			}
			else {
				bl = Buffer_raw_GetLine(b, u->lineNum);
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
	Buffer_UndoInsertLineAfter(b, after->prev);
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
		bl->length + 1,
		text, length);
	Buffer_raw_InsertChars(b, bl, text, bl->length, length); // BUG: of by 1?
}

void Buffer_LineAppendLine(Buffer* b, BufferLine* target, BufferLine* src) {
	Buffer_LineAppendText(b, target, src->buf, src->length);
}

void Buffer_LineDeleteChars(Buffer* b, BufferLine* bl, intptr_t col, intptr_t length) {
	Buffer_UndoDeleteText(b, bl, col, length);
	Buffer_raw_DeleteChars(b, bl, col, length);
}


void Buffer_LineTruncateAfter(Buffer* b, BufferLine* bl, intptr_t col) {
	Buffer_LineDeleteChars(b, bl, col, bl->length - col); // BUG: off by 1?
}

// deletes chars but also handles line removal and edge cases
// does not move the cursor
void Buffer_BackspaceAt(Buffer* b, BufferLine* l, intptr_t col) {
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
void Buffer_DeleteAt(Buffer* b, BufferLine* l, intptr_t col) {
	if(!b->first) return; // empty buffer
	
	Buffer_LineDeleteChars(b, l, col, 1);
	
	if(col >= l->length) {
		// last col of last row; do nothing
		if(b->last == l) return;
		
		if(l->next->length > 0) {
			// merge with the next line
			Buffer_LineAppendText(b, l, l->next->buf, l->next->length);
		}
		
		Buffer_NotifyLineDeletion(b, l->next, l->next);
		Buffer_DeleteLine(b, l->next);
		
		return;
	} 
	
}



void Buffer_DuplicateSelection(Buffer* b, BufferRange* sel, int amt) {
	BufferLine* src;
	BufferLine* endline = sel->endLine;
	if(sel->endCol == 0 && sel->endLine != sel->startLine) {
		endline = endline->prev;
	}
	BufferLine* target = endline;
	BufferLine* start_target = endline;
	
	if(amt < 0) {
		amt = -amt;
		target = sel->startLine;
		
		while(amt-- > 0) {
			BufferLine* src = endline;
			while(src && src->next != sel->startLine) {
				target = Buffer_InsertLineBefore(b, target, src->buf, src->length);
				
				src = src->prev;
			}
		}
	}
	else {
		
		while(amt-- > 0) {
			BufferLine* src = sel->startLine;
			while(src && src->prev != endline) {
				target = Buffer_InsertLineAfter(b, target, src->buf, src->length);
				
				src = src->next;
			}
		}
		
		// fix the selection and cursor if it turns out we inserted lines in between	
		if(endline != sel->endLine) {
			//if(b->current == sel->endLine) b->current = endline->next;
			sel->endLine = endline->next;
			
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
	
	Buffer_UndoSetSelection(b, s.startLine->lineNum, s.startCol, s.endLine->lineNum, s.endCol);
	
	if(s.startLine == s.endLine) {
		// move the end down
		Buffer_LineDeleteChars(b, s.startLine, s.startCol, s.endCol - s.startCol);
	}
	else {
		// truncate start line after selection start
		Buffer_LineTruncateAfter(b, s.startLine, s.startCol);
		
		// append end line after selection ends to first line
		char* elb = s.endLine->buf ? s.endLine->buf + s.endCol : "";
		Buffer_LineAppendText(b, s.startLine, elb, strlen(elb));
		
		
		// delete lines 1-n
		BufferLine* bl = s.startLine->next;
		BufferLine* next;
		while(bl) {
			next = bl->next; 
			//Buffer_NotifyLineDeletion(b, bl, bl);
			Buffer_DeleteLine(b, bl);
			
			if(bl == s.endLine) break;
			bl = next;
		}
	}
	
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

// make sure a range goes in the right direction
// delete selections that are zero
void BufferRange_Normalize(BufferRange** pbr) {
	BufferRange* br = *pbr;
	if(!br) return;
	
	if(br->startLine == br->endLine) {
		if(br->startCol == br->endCol) {
			free(br); // delete empty ranges
			*pbr = NULL;
		}
		else if(br->startCol > br->endCol) {
			intptr_t t = br->startCol;
			br->startCol = br->endCol;
			br->endCol = t;
		}
	}
	else if(br->startLine->lineNum > br->endLine->lineNum) {
		void* tl = br->startLine;
		br->startLine = br->endLine;
		br->endLine = tl;
		intptr_t t = br->startCol;
		br->startCol = br->endCol;
		br->endCol = t;
	}
	
// 	printf("sel: %d:%d -> %d:%d\n", br->startLine->lineNum, br->startCol, br->endLine->lineNum, br->endCol);

}



void Buffer_RelPosH(
	Buffer* b, 
	BufferLine* startL, 
	intptr_t startC, 
	intptr_t cols,
	BufferLine** outL,
	intptr_t* outC
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
	Buffer* b, 
	BufferLine* startL, 
	intptr_t startC, 
	intptr_t lines,
	BufferLine** outL,
	intptr_t* outC
) {
	intptr_t i = lines;
	BufferLine* bl = startL;
	
	if(i > 0) while(i-- > 0 && bl->next) {
		bl = bl->next;
	}
	else while(i++ < 0 && bl->prev) {
		bl = bl->prev;
	}
	
	if(outC) *outC = startC;
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
	BufferLine* bl = sel->startLine;
	do {
		Buffer_LineUnindent(b, bl);
	} while(bl != sel->endLine && (bl = bl->next) && bl);
	
	sel->startCol--;
	sel->endCol--; // TODO: undo
	
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
	BufferLine* bl = sel->startLine;
	do {
		Buffer_LineIndent(b, bl);
	} while(bl != sel->endLine && (bl = bl->next) && bl);
	
	sel->startCol++;
	sel->endCol++; // TODO: undo
	
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
void Buffer_ProcessCommand(Buffer* b, BufferCmd* cmd, int* needRehighlight) {
	Buffer* b2;
	
	char cc[2] = {cmd->amt, 0};
	
	switch(cmd->type) {
			
		case BufferCmd_Undo:
			Buffer_UndoReplayToSeqBreak(b);  
			break;
			
		case BufferCmd_Redo:
			Buffer_RedoReplayToSeqBreak(b);  
			
			break;
			
		case BufferCmd_Save:
			if(!g_DisableSave) {
				Buffer_SaveToFile(b, b->sourceFile);
			}
			else {
				printf("Buffer saving disabled.\n");
			}
			break;
			
		case BufferCmd_Debug:
			switch(cmd->amt) {
				case 0: Buffer_DebugPrint(b); break;
				case 1: Buffer_DebugPrintUndoStack(b); break;
			} 
		
	}
	
	
// 	if(b->undoFill > 0) {
// 		BufferUndo* u = undo_current(b);
// 		b->isModified = u->action != UndoAction_SequenceBreak || u->unModified == 0 ;
// 	}

	
	
	
// 	printf("line/col %d:%d %d\n", b->current->lineNum, b->curCol, b->current->length);
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
		blc = BufferLine_Copy(src->first);
		
		b->first = blc;
		bl = bl->next;
		blc_prev = blc;
		
		while(bl) {
			
			blc = BufferLine_Copy(bl);
			
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
	
	
	BufferLine* blc, *blc_prev, *bl;
	
	if(!src->first || !sel->startLine) return b;
	
	// single-line selection
	if(sel->startLine == sel->endLine) {
		blc = BufferLine_FromStr(sel->startLine->buf + sel->startCol, sel->endCol - sel->startCol);
		
		b->first = blc;
		b->last = blc;
		b->numLines = 1;
		
		return b;
	}
	
	// multi-line selection
	bl = sel->startLine;
	if(sel->startCol == 0) {
		blc = BufferLine_Copy(sel->startLine);
	}
	else {
		// copy only the end
		char* start = sel->startLine->buf ? sel->startLine->buf + sel->startCol : "";
		blc = BufferLine_FromStr(start, strlen(start));
	}
	
	b->numLines++;
	b->first = blc;
	bl = bl->next;
	blc_prev = blc;
	
	while(bl && bl != sel->endLine) {
		
		blc = BufferLine_Copy(bl);
		
		blc->prev = blc_prev;
		blc_prev->next = blc; 
		
		b->numLines++;
		blc_prev = blc;
		bl = bl->next;
	}
	
	// copy the beginning of the last line
	blc = BufferLine_FromStr(sel->endLine->buf, MIN(sel->endCol, sel->endLine->length));
	blc->prev = blc_prev;
	blc_prev->next = blc;
		
	b->numLines++;
	b->last = blc;
	
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
		printf("%ld: [%ld/%ld] '%.*s'\n", i, bl->length, bl->allocSz, (int)bl->length, bl->buf);
		bl = bl->next;
		i++;
	}
}



void Buffer_InsertBufferAt(Buffer* target, Buffer* graft, BufferLine* tline, intptr_t tcol, BufferRange* outRange) {
	size_t tmplen;
	char* tmp = NULL;
	BufferLine* blc, *bl;
	
	if(outRange) {
		outRange->startLine = tline;
		outRange->startCol = tcol;
	}
		
	// check for easy special  cases
	if(graft->numLines == 0) return;
	if(graft->numLines == 1) {
		Buffer_LineInsertChars(target, tline, graft->first->buf, tcol, graft->first->length);
		
		if(outRange) {
			outRange->endLine = tline;
			outRange->endCol = tcol + graft->first->length;
		}
		
		return;
	}
	
	// cutting the remainder of the first line to a temporary buffer
	if(tline->length) {
		tmplen = tline->length - tcol;
		tmp = malloc(tmplen);
		tmp[tmplen] = 0;
		memcpy(tmp, tline->buf + tcol, tmplen);
		
		tline->length = tcol;
		tline->buf[tcol] = 0;
	}
	

	Buffer_LineAppendText(target, tline, graft->first->buf, graft->first->length);
	
	// copy in the middle lines
	BufferLine* gbl = graft->first->next;
	BufferLine* t = tline;
	while(gbl && gbl != graft->last) {
		
		t = Buffer_InsertLineAfter(target, t, gbl->buf, gbl->length);
		
		gbl = gbl->next;
	}
	
	if(tline->length) {
		// prepend the last line to the temp buffer
		t = Buffer_InsertLineAfter(target, t, gbl->buf, gbl->length);
		Buffer_LineAppendText(target, t, tmp, tmplen);
	}
	
	if(outRange) {
		outRange->endLine = t;
		outRange->endCol = gbl->length;
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
			Buffer_AppendLine(b, s, 0);
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
BufferLine* Buffer_PrependLine(GUIBufferEditControl* w, char* text, intptr_t len) {
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

	BufferLine* bl = sel->startLine;
	while(bl) {
		Buffer_LinePrependText(b, bl, text);
		
		if(bl == sel->endLine) break;
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

	BufferLine* bl = sel->startLine;
	while(bl) {
		Buffer_LineUnprependText(b, bl, text);
		
		if(bl == sel->endLine) break;
		bl = bl->next;
	}
}


void Buffer_SurroundSelection(Buffer* b, BufferRange* sel, char* begin, char* end) {
	if(!sel) return;
	
	// end must be first
	if(end) {
		BufferLine_InsertChars(
			sel->endLine,
			end, 
			sel->endCol,
			strlen(end)
		);
	}

	if(begin) {
		BufferLine_InsertChars(
			sel->startLine, 
			begin,  
			sel->startCol,
			strlen(begin)
		);
	}
}

int Buffer_UnsurroundSelection(Buffer* b, BufferRange* sel, char* begin, char* end) {
	int out = 0;
	
	if(!sel) return 0;

	// end must be first
	size_t elen = strlen(end);
	if(0 == strncmp(sel->endLine->buf + sel->endCol - elen, end, elen)) {
		Buffer_LineDeleteChars(b, sel->endLine, sel->endCol - elen, elen);
		out |= (1 << 1);
	}
	
	size_t slen = strlen(begin);
	if(0 == strncmp(sel->startLine->buf + sel->startCol, begin, slen)) {
		Buffer_LineDeleteChars(b, sel->startLine, sel->startCol, slen);
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
		
		if(bs->startLine->lineNum < b->sel->startLine->lineNum) {
			
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
	
	BufferLine* bl = sel->startLine;
	while(bl) {
		size_t scol = bl == sel->startLine ? sel->startCol : 0;
		size_t ecol = bl == sel->endLine ? sel->endCol : bl->length;
		size_t cols = ecol - scol;
		
		if(alloc - len < cols + 1) {
			alloc = MAX(alloc * 2, alloc + cols + 1);
			out = realloc(out, sizeof(*out) * alloc);
		}
		
		// TODO: not slow, shitty algorithm
		strncat(out, bl->buf + scol, cols);
		
		len += cols;
		
		if(bl == sel->endLine) break;
		
		strcat(out, "\n");
		len++;
	}
	
	if(outLen) *outLen = len;
	return out;
}

void Buffer_GetSequenceUnder(Buffer* b, BufferLine* l, intptr_t col, char* charSet, BufferRange* out) {
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
	
	out->startLine = l;
	out->endLine = l;
	out->startCol = start;
	out->endCol = end;
}


int Buffer_FindEndOfSequence(Buffer* b, BufferLine** linep, intptr_t* colp, char* charSet) {
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

int Buffer_FindEndOfInvSequence(Buffer* b, BufferLine** linep, intptr_t* colp, char* charSet) {
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

int Buffer_FindSequenceEdgeForward(Buffer* b, BufferLine** linep, intptr_t* colp, char* charSet) {
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

int Buffer_FindSequenceEdgeBackward(Buffer* b, BufferLine** linep, intptr_t* colp, char* charSet) {
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
		
		printf("%c%d [%s] {%ld,%ld} %ld:'%.*s'\n", c, i, names[u->action], 
			u->lineNum, u->colNum, u->length, (int)u->length, u->text);
	}
	
	printf(" Undo save index: %d\n", b->undoSaveIndex);
	printf(" Undo curr index: %d\n", b->undoCurrent);
	printf("\n");
}



void Buffer_CollapseWhitespace(Buffer* b, BufferLine* l, intptr_t col) {
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

int Buffer_AddDictWord(Buffer* b, char* word) {
	return 0; // DEBUG
	int* refs;
	if(HT_getp(&b->dict, word, &refs)) {
		// not found, add it
		int one = 1;
		HT_set(&b->dict, word, one);
		return 1;
	}
	else {
		(*refs)++;
		return *refs;
	}
	
}

int Buffer_RemoveDictWord(Buffer* b, char* word) {
	return 0; // DEBUG
	int* refs;
	if(HT_getp(&b->dict, word, &refs)) {
		return 0;
	}
	else {
		(*refs)--;
		if(*refs == 0) HT_delete(&b->dict, word);
		
		return *refs;
	}
}

void Buffer_AddLineToDict(Buffer* b, BufferLine* l) {
	size_t n;
	char* s = l->buf;
	
	if(!s) return;
	
	while(1) {
		n = strcspn(s, b->dictCharSet);
		s += n;
		
		n = strspn(s, b->dictCharSet);
		if(n <= 0) break;
		
		char* w = strndup(s, n);
		Buffer_AddDictWord(b, w);
		free(w);
		
		s += n;
	}
}

void Buffer_RemoveLineFromDict(Buffer* b, BufferLine* l) {
	size_t n;
	char* s = l->buf;
	
	if(!s) return;
	
	while(1) {
		n = strcspn(s, b->dictCharSet);
		s += n;
		
		n = strspn(s, b->dictCharSet);
		if(n <= 0) break;
		
		char* w = strndup(s, n);
		Buffer_RemoveDictWord(b, w);
		free(w);
		
		s += n;
	}
}


void Buffer_NotifyUndoSetSelection(Buffer* b, BufferLine* startL, intptr_t startC, BufferLine* endL, intptr_t endC, char isReverse) {
	BufferChangeNotification note = {
		.b = b,
		.sel.startLine = startL,
		.sel.endLine = endL,
		.sel.startCol = startC,
		.sel.endCol = endC,
		
		.action = BCA_Undo_SetSelection,
	};

	Buffer_NotifyChanges(&note);	
}


void Buffer_NotifyUndoMoveCursor(Buffer* b, BufferLine* line, intptr_t col) {
	BufferChangeNotification note = {
		.b = b,
		.sel.startLine = line,
		.sel.endLine = 0,
		.sel.startCol = col,
		.sel.endCol = 0,
		
		.action = BCA_Undo_MoveCursor,
	};

	Buffer_NotifyChanges(&note);	
}


void Buffer_NotifyLineDeletion(Buffer* b, BufferLine* sLine, BufferLine* eLine) {
	BufferChangeNotification note = {
		.b = b,
		.sel.startLine = sLine,
		.sel.endLine = eLine,
		.sel.startCol = 0,
		.sel.endCol = eLine->length,
		
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


int BufferRange_CompleteLinesOnly(BufferRange* sel) {
	if(!sel) return 0;
	
	if(sel->startCol != 0 && sel->startCol != sel->startLine->length) return 0;
	if(sel->endCol != 0 && sel->endCol != sel->endLine->length) return 0;
	
	return 1;
}

