#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "gui.h"
#include "gui_internal.h"
#include "clipboard.h"

#include "highlighters/c.h"




Buffer* Buffer_New() {
	Buffer* b = pcalloc(b);
	
	VEC_INIT(&b->undoStack);
	
	return b;
}


void Buffer_UndoInsertText(
	Buffer* b, 
	size_t line, 
	size_t col, 
	char* txt, 
	size_t len
) {
	
	VEC_INC(&b->undoStack);
	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	
	u->action = UndoAction_InsertText;
	u->lineNum = line;
	u->colNum = col - 1;
	u->text = strndup(txt, len);
	u->length = len;
}

void Buffer_UndoDeleteText(Buffer* b, BufferLine* bl, size_t offset, size_t len) {
	
	VEC_INC(&b->undoStack);
	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	
	u->action = UndoAction_DeleteText;
	u->lineNum = bl->lineNum;
	u->colNum = offset - 1;
	u->text = strndup(bl->buf + offset - 1, len); // BUG: off by 1?
	u->length = len;
}


void Buffer_UndoSequenceBreak(Buffer* b) {
	VEC_INC(&b->undoStack);
	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	
	u->action = UndoAction_SequenceBreak;
	u->lineNum = 0;
	u->colNum = 0;
	u->text = NULL;
	u->length = 0;
}

void Buffer_UndoInsertLineAfter(Buffer* b, BufferLine* before) {
	VEC_INC(&b->undoStack);
	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	
	u->action = UndoAction_InsertLineAfter;
	u->lineNum = before ? before->lineNum : 0; // 0 means the first line 
	u->colNum = 0;
	u->text = NULL;
	u->length = 0;
}


void Buffer_UndoDeleteLine(Buffer* b, BufferLine* bl) {
	VEC_INC(&b->undoStack);
	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	
	u->action = UndoAction_DeleteLine;
	u->lineNum = bl->lineNum; // null means the first line 
	u->colNum = 0;
	u->text = strndup(bl->buf, bl->length);
	u->length = bl->length;
}


// executes a single undo action
void Buffer_UndoReplayTop(Buffer* b) {
	BufferLine* bl;
	
	if(VEC_LEN(&b->undoStack) == 0) return;
	BufferUndo* u = &VEC_TAIL(&b->undoStack);
	
	// these all need to be the inverse
	switch(u->action) {
		case UndoAction_InsertText: // delete text
			bl = Buffer_raw_GetLine(b, u->lineNum);
			Buffer_raw_DeleteChars(b, bl, u->colNum, u->length);
			break;
			
		case UndoAction_DeleteText: // re-insert the text
			bl = Buffer_raw_GetLine(b, u->lineNum);
			Buffer_raw_InsertChars(b, bl, u->text, u->colNum, u->length);
			b->current = bl; // BUG not right after <delete> key
			b->curCol = u->colNum + u->length;
			break;
			
		case UndoAction_InsertLineAfter: // delete the line
			bl = Buffer_raw_GetLine(b, u->lineNum);
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
			
		case UndoAction_MoveCursorTo:
			fprintf(stderr, "UndoAction_MoveCursorTo nyi\n");
			break;
			
		case UndoAction_UnmodifiedFlag:
			fprintf(stderr, "UndoAction_MoveCursorTo nyi\n");
			
			break;
			
		case UndoAction_SequenceBreak:
			// do nothing at all for now
			break;
			
		default:
			fprintf(stderr, "Unknown undo action: %d\n", u->action);
	}
	
	// cleanup
	if(u->text) free(u->text);
	u->text = NULL;
	
	VEC_POP1(&b->undoStack);
}



BufferLine* Buffer_InsertEmptyLineAfter(Buffer* b, BufferLine* before) {
	Buffer_UndoInsertLineAfter(b, before);
	return Buffer_raw_InsertLineAfter(b, before);
}
BufferLine* Buffer_InsertEmptyLineBefore(Buffer* b, BufferLine* after) {
	Buffer_UndoInsertLineAfter(b, after->prev);
	return Buffer_raw_InsertLineBefore(b, after);
}


BufferLine* Buffer_InsertLineAfter(Buffer* b, BufferLine* before, char* text, size_t length) {
	BufferLine* bl = Buffer_InsertEmptyLineAfter(b, before);

	Buffer_UndoInsertText(b, bl->lineNum, 0, text, length);
	Buffer_raw_InsertChars(b, bl, text, 0, length);
	
	return bl;
}


BufferLine* Buffer_InsertLineBefore(Buffer* b, BufferLine* after, char* text, size_t length) {
	BufferLine* bl = Buffer_InsertEmptyLineBefore(b, after);
	
	Buffer_UndoInsertText(b, bl->lineNum, 0, text, length);
	Buffer_raw_InsertChars(b, bl, text, 0, length);
	
	return bl;
}



void Buffer_DeleteLine(Buffer* b, BufferLine* bl) {
	Buffer_UndoDeleteLine(b, bl);
	Buffer_raw_DeleteLine(b, bl);
}


void Buffer_LineInsertChars(Buffer* b, BufferLine* bl, char* text, size_t offset, size_t length) {
	Buffer_UndoInsertText(b, bl->lineNum, offset, text, length);
	Buffer_raw_InsertChars(b, bl, text, offset - 1, length);
}


void Buffer_LineAppendText(Buffer* b, BufferLine* bl, char* text, size_t length) {
	Buffer_UndoInsertText(b, bl->lineNum, 
		bl->length, // BUG: off by 1?
		text, length);
	Buffer_raw_InsertChars(b, bl, text, bl->length, length); // BUG: of by 1?
}

void Buffer_LineAppendLine(Buffer* b, BufferLine* target, BufferLine* src) {
	Buffer_LineAppendText(b, target, src->buf, src->length);
}

void Buffer_LineDeleteChars(Buffer* b, BufferLine* bl, size_t col, size_t length) {
	Buffer_UndoDeleteText(b, bl, col - 1, length);
	Buffer_raw_DeleteChars(b, bl, col, length);
}


void Buffer_LineTruncateAfter(Buffer* b, BufferLine* bl, size_t col) {
	Buffer_LineDeleteChars(b, bl, col, bl->length - col - 1); // BUG: off by 1?
}

// deletes chars but also handles line removal and edge cases
// does not move the cursor
void Buffer_BackspaceAt(Buffer* b, BufferLine* l, size_t col) {
	if(!b->first) return; // empty buffer
	
	
	if(col == 1) { 
		printf("col 1 \n");
		// first col of first row; do nothing
		if(b->first == l) return;
		
		if(l->length > 0) {
			// merge with the previous line
			Buffer_LineAppendLine(b, l->prev, l);
		}
		
		// TODO: move cursor
		
		Buffer_DeleteLine(b, l);
		
		return;
	} 
	
	Buffer_LineDeleteChars(b, l, col, 1);

	
}


// deletes chars but also handles line removal and edge cases
// does not move the cursor
void Buffer_DeleteAt(Buffer* b, BufferLine* l, size_t col) {
	if(!b->first) return; // empty buffer
	
	Buffer_LineDeleteChars(b, l, col + 1, 1);
	
	if(col == l->length + 1) {
		// last col of last row; do nothing
		if(b->last == l) return;
		
		if(l->next->length > 0) {
			// merge with the next line
			Buffer_LineAppendText(b, l, l->next->buf, l->next->length);
		}
		
		Buffer_DeleteLine(b, l->next);
		
		return;
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
			bl = Buffer_InsertLineBefore(b, bl, src->buf, src->length);
		}
	}
}



void Buffer_InsertLinebreak(Buffer* b) {
	BufferLine* l = b->current;
	
	if(b->curCol == 1) {
		Buffer_InsertEmptyLineBefore(b, b->current);
	}
	else {
		BufferLine* n = Buffer_InsertLineAfter(b, l, l->buf + b->curCol - 1, strlen(l->buf + b->curCol - 1));
		Buffer_LineTruncateAfter(b, l, b->curCol);
		
		// TODO: undo cursor move
		b->current = b->current->next;
	}
	
	b->curCol = 1;
	
	// TODO: undo
	// TODO: maybe shrink the alloc
}



void Buffer_ClearAllSelections(Buffer* b) {
	if(!b->sel) return;
	free(b->sel);
	b->sel = NULL;
	
	// TODO: undo
	// TODO: clear the selection list too
}


void Buffer_DeleteSelectionContents(Buffer* b, BufferSelection* sel) {
	if(!sel) return;
	
	// TODO: undo selection change 
	
	if(sel->startLine == sel->endLine) {
		// move the end down
// 		BufferLine_DeleteRange(sel->startLine, sel->startCol, sel->endCol);
		Buffer_LineDeleteChars(b, sel->startLine, sel->startCol, sel->endCol - sel->startCol);
	}
	else {
		// truncate start line after selection start
		Buffer_LineTruncateAfter(b, sel->startLine, sel->startCol);

		// append end line after selection ends to first line
		Buffer_LineAppendText(b, sel->startLine, sel->endLine->buf + sel->endCol, strlen(sel->endLine->buf + sel->endCol));
		
		// delete lines 1-n
		BufferLine* bl = sel->startLine->next;
		BufferLine* next;
		while(bl) {
			next = bl->next; 
			Buffer_DeleteLine(b, bl);
			
			if(bl == sel->endLine) break;
			bl = next;
		}
	}
	
	Buffer_ClearCurrentSelection(b);
}


void Buffer_MoveCursorV(Buffer* b, ptrdiff_t lines) {
	int i = lines;
	// TODO: undo
	if(i > 0) while(i-- > 0 && b->current->next) {
		b->current = b->current->next;
	}
	else while(i++ < 0 && b->current->prev) {
		b->current = b->current->prev;
	}
}


void Buffer_MoveCursorH(Buffer* b, ptrdiff_t cols) {
	int i = cols;
	// TODO: undo
	if(i < 0) while(i++ < 0) {
		if(b->curCol <= 1) {
			
			if(b->current->prev == NULL) {
				b->curCol = 1;
				return;
			}
			
			b->current = b->current->prev;
			b->curCol = b->current->length + 1;
		}
		else {
			b->curCol--;
		}
	}
	else while(i-- > 0) {
		if(b->curCol > b->current->length) {
			
			if(b->current->next == NULL) {
				return;
			}
			
			b->current = b->current->next;
			b->curCol = 1;
		}
		else {
			b->curCol++;
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

void Buffer_NextBookmark(Buffer* b) {
	if(!b->current) return;
	BufferLine* bl = b->current->next;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->next;
	}
	if(bl) b->current = bl;
}

void Buffer_PrevBookmark(Buffer* b) {
	if(!b->current) return;
	BufferLine* bl = b->current->prev;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->prev;
	}
	if(bl) b->current = bl;
}

void Buffer_FirstBookmark(Buffer* b) {
	BufferLine* bl = b->first;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->next;
	}
	if(bl) b->current = bl;
}

void Buffer_LastBookmark(Buffer* b) {
	BufferLine* bl = b->last;
	while(bl && !(bl->flags & BL_BOOKMARK_FLAG)) {
		bl = bl->prev;
	}
	if(bl) b->current = bl;
}


// only undo sequence breaks are handled here; all other undo actions must be 
//    added by the called functions
void Buffer_ProcessCommand(Buffer* b, BufferCmd* cmd, int* needRehighlight) {
	Buffer* b2;
	
	char cc[2] = {cmd->amt, 0};
	
	switch(cmd->type) {
		case BufferCmd_MoveCursorV:
			Buffer_MoveCursorV(b, cmd->amt);
			break;
		
		case BufferCmd_MoveCursorH:
			Buffer_MoveCursorH(b, cmd->amt);
			break;
		
		case BufferCmd_InsertChar:
			// TODO: update
			Buffer_LineInsertChars(b, b->current, cc, b->curCol, 1);
			b->curCol++;
			break;
		
		case BufferCmd_Backspace:
			Buffer_BackspaceAt(b, b->current, b->curCol);
			b->curCol--;
			break;
		
		case BufferCmd_Delete:
			Buffer_DeleteAt(b, b->current, b->curCol);
			break;
		
		case BufferCmd_SplitLine:
			Buffer_InsertLinebreak(b);
			break;
		
		case BufferCmd_DeleteCurLine:
			Buffer_DeleteLine(b, b->current);
			break;
		
		case BufferCmd_Home:
			// TODO: undo
			b->current = b->first;
			b->curCol = 1;
			break;
		
		case BufferCmd_End:
			// TODO: undo
			b->current = b->last;
			if(b->last) b->curCol = b->last->length + 1;
			break;
		
		case BufferCmd_DuplicateLine:
			Buffer_DuplicateLines(b, b->current, cmd->amt);
			Buffer_MoveCursorV(b, cmd->amt);
			break;
		
		case BufferCmd_Copy:
			if(b->sel) {
				b2 = Buffer_FromSelection(b, b->sel);
				Clipboard_PushBuffer(b2);
			}
			break;
		
		case BufferCmd_Cut:
			if(b->sel) {
				b2 = Buffer_FromSelection(b, b->sel);
				Clipboard_PushBuffer(b2);
				Buffer_DeleteSelectionContents(b, b->sel);
			}
			break;
		
		case BufferCmd_Paste:
			b2 = Clipboard_PopBuffer();
			if(b2) {
				// TODO: undo
				Buffer_InsertBufferAt(b, b2, b->current, b->curCol);
				// TODO: move cursor to end of pasted text
			}
			break;
		
		case BufferCmd_SelectNone:
			Buffer_ClearAllSelections(b);
			break;
		
		case BufferCmd_SelectAll:
			Buffer_SetCurrentSelection(b, b->first, 1, b->last, b->last->length+1);
			break;
		
		case BufferCmd_SelectLine:
			Buffer_SetCurrentSelection(b, b->current, 1, b->current, b->current->length);
			break;
			
		case BufferCmd_SelectToEOL:
			Buffer_SetCurrentSelection(b, b->current, b->curCol, b->current, b->current->length);
			break;
			
		case BufferCmd_SelectFromSOL:
			Buffer_SetCurrentSelection(b, b->current, 1, b->current, b->curCol - 1);
			break;
		
		case BufferCmd_SetBookmark:       Buffer_SetBookmarkAt(b, b->current);    break; 
		case BufferCmd_RemoveBookmark:    Buffer_RemoveBookmarkAt(b, b->current); break; 
		case BufferCmd_ToggleBookmark:    Buffer_ToggleBookmarkAt(b, b->current); break; 
		case BufferCmd_GoToNextBookmark:  Buffer_NextBookmark(b);  break; 
		case BufferCmd_GoToPrevBookmark:  Buffer_PrevBookmark(b);  break; 
		case BufferCmd_GoToFirstBookmark: Buffer_FirstBookmark(b); break; 
		case BufferCmd_GoToLastBookmark:  Buffer_LastBookmark(b);  break; 
		
		case BufferCmd_Undo:
			Buffer_UndoReplayTop(b);  
		
			break;
			
		case BufferCmd_Debug:
			switch(cmd->amt) {
				case 0: Buffer_DebugPrint(b); break;
				case 1: Buffer_DebugPrintUndoStack(b); break;
			} 
	}
// 	printf("line/col %d:%d %d\n", b->current->lineNum, b->curCol, b->current->length);
}


Buffer* Buffer_Copy(Buffer* src) {
	Buffer* b = Buffer_New();
	BufferLine* blc, *blc_prev, *bl;
	
	// TODO: undo
	
	b->numLines = src->numLines;
	b->curCol = src->curCol;
	
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
			
			if(bl == src->current) {
				b->current = blc;
			}
			
			blc_prev = blc;
			bl = bl->next;
		}
		
		b->last = blc;
	}
	
	return b;
}

Buffer* Buffer_FromSelection(Buffer* src, BufferSelection* sel) {
	Buffer* b = Buffer_New();
	
	
	BufferLine* blc, *blc_prev, *bl;
	
	if(!src->first || !sel->startLine) return b;
	
	b->curCol = 1;
	
	// single-line selection
	if(sel->startLine == sel->endLine) {
		blc = BufferLine_FromStr(sel->startLine->buf + sel->startCol - 1, sel->endCol - sel->startCol);
		
		b->first = blc;
		b->last = blc;
		b->current = blc;
		b->numLines = 1;
		
		return b;
	}
	
	// multi-line selection
	bl = sel->startLine;
	if(sel->startCol == 1) {
		blc = BufferLine_Copy(src->first);
	}
	else {
		// copy only the end
		char* start = sel->startLine->buf + sel->startCol - 1;
		blc = BufferLine_FromStr(start, strlen(start));
	}
	
	b->numLines++;
	b->first = blc;
	b->current = blc;
	bl = bl->next;
	blc_prev = blc;
	
	while(bl && bl != sel->endLine) {
		
		blc = BufferLine_Copy(bl);
		
		blc->prev = blc_prev;
		blc_prev->next = blc; 
		
		if(bl == src->current) {
			b->current = blc;
		}
		
		b->numLines++;
		blc_prev = blc;
		bl = bl->next;
	}
	
	// copy the beginning of the last line
	blc = BufferLine_FromStr(sel->endLine->buf, sel->endCol);
	blc->prev = blc_prev;
	blc_prev->next = blc;
		
	b->numLines++;
	b->last = blc;
	
	return b;
}


void Buffer_DebugPrint(Buffer* b) {
	BufferLine* bl;
	size_t i = 1, actualLines = 0;
	
	// count real lines
	bl = b->first;
	while(bl) {
		actualLines++;
		bl = bl->next;
	}
	
	printf("Buffer %p: expected lines: %ld, actual lines: %ld\n", b, b->numLines, actualLines);
	
	
	bl = b->first;
	while(bl) {
		printf("%ld: [%ld/%ld] '%.*s'\n", i, bl->length, bl->allocSz, bl->length, bl->buf);
		bl = bl->next;
		i++;
	}
}



// BUG doesn't paste first and last line properly
void Buffer_InsertBufferAt(Buffer* target, Buffer* graft, BufferLine* tline, size_t tcol) {
	
	BufferLine* blc, *bl;
	
	// TODO: undo
	
	// check for easy special  cases
	if(graft->numLines == 0) return;
	if(graft->numLines == 1) {
		BufferLine_InsertChars(tline, graft->first->buf, graft->first->length, tcol);
		return;
	}
	
	// cutting the remainder of the first line to a temporary buffer
	size_t tmplen = tline->length - tcol + 1;
	char* tmp = malloc(tmplen);
	tmp[tmplen] = 0;
	memcpy(tmp, tline->buf + tcol, tmplen);
	
	tline->length = tcol;
	tline->buf[tcol] = 0;
	BufferLine_AppendText(tline, graft->first->buf, graft->first->length);
	
	// copy in the middle lines
	BufferLine* gbl = graft->first->next;
	BufferLine* t = tline;
	while(gbl && gbl != graft->last) {
		
		t = Buffer_InsertEmptyLineAfter(target, t);
		BufferLine_SetText(t, gbl->buf, gbl->length);
		
		gbl = gbl->next;
	}
	
	// prepend the last line to the temp buffer
	t = Buffer_InsertEmptyLineAfter(target, t);
	BufferLine_SetText(t, graft->last->buf, graft->last->length);
	BufferLine_AppendText(t, tmp, tmplen);
	
}



void Buffer_AppendRawText(Buffer* b, char* source, size_t len) {
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
			Buffer_AppendLine(b, s, (size_t)(e-s));
// 			BufferLine* bl = Buffer_raw_InsertLineAfter(b, b->last);
// 			Buffer_raw_InsertChars(b, bl, s, 0, (size_t)(e-s)); 
// 		}
		
// 		if(b->last->buf && b->last->length > 0) {
// 			b->last->style = calloc(1, sizeof(*b->last->style));
// 			HL_acceptLine(b->last->buf, b->last->length, b->last->style);
// 		}
		
		s = e + 1;
	}
}



// TODO: undo
BufferLine* Buffer_PrependLine(Buffer* b, char* text, size_t len) {
	BufferLine* l = pcalloc(l);
	
	b->numLines++;
	
	BufferLine_SetText(l, text, len);
	
	if(b->last == NULL) {
		b->first = l;
		b->last = l;
		b->current = l;
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


BufferLine* Buffer_AppendLine(Buffer* b, char* text, size_t len) {
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
	
	return 0;
}



// TODO: undo
void Buffer_CommentLine(Buffer* b, BufferLine* bl) {
	if(!b->ep->lineCommentPrefix) return;
	
	BufferLine_InsertChars(bl, b->ep->lineCommentPrefix, strlen(b->ep->lineCommentPrefix), 0);
}

// TODO: undo
void Buffer_CommentSelection(Buffer* b, BufferSelection* sel) {
	if(!sel) return;
	
	BufferLine_InsertChars(
		sel->startLine, 
		b->ep->selectionCommentPrefix, 
		strlen(b->ep->selectionCommentPrefix), 
		sel->startCol
	);
	
	BufferLine_InsertChars(
		sel->endLine, 
		b->ep->selectionCommentPostfix, 
		strlen(b->ep->selectionCommentPostfix), 
		sel->endCol
	);
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
	BufferSelection* bs = b->selectionRing->first;
	while(bs != b->selectionRing->last) {
		
		if(bs->startLine->lineNum < b->sel->startLine->lineNum) {
			
		}
		
	}
	
}
*/

// TODO: undo
void Buffer_ClearCurrentSelection(Buffer* b) {
	if(b->sel) free(b->sel);
	b->sel = NULL;
}


void Buffer_SetCurrentSelection(Buffer* b, BufferLine* startL, size_t startC, BufferLine* endL, size_t endC) {
	if(!b->sel) pcalloc(b->sel);
	
	// TODO: undo
	
	assert(startL != NULL);
	assert(endL != NULL);
	
	startC = MIN(startL->length, MAX(startC, 1));
	endC = MIN(endL->length, MAX(endC, 1));
	
	if(startL->lineNum < endL->lineNum) {
		b->sel->startLine = startL;
		b->sel->endLine = endL;
		b->sel->startCol = startC;
		b->sel->endCol = endC;
	}
	else if(startL->lineNum > endL->lineNum) {
		b->sel->startLine = endL;
		b->sel->endLine = startL;
		b->sel->startCol = endC;
		b->sel->endCol = startC;
	}
	else { // same line
		b->sel->startLine = startL;
		b->sel->endLine = startL;
		
		if(startC < endC) {
			b->sel->startCol = startC;
			b->sel->endCol = endC;
		}
		else {
			b->sel->startCol = endC;
			b->sel->endCol = startC;
		}
	}
}




void Buffer_DebugPrintUndoStack(Buffer* b) {
	printf("Undo stack for %p (%ld entries)\n", b, VEC_LEN(&b->undoStack));
	
	char* names[] = { 
		"InsText",
		"DelText",
		"InsChar",
		"DelChar",
		"InsLine", 
		"DelLine",
		"MvCurTo",
		"SetSel ",
		"UnmodFl",
		"SequBrk",
	};
	
	VEC_EACH(&b->undoStack, i, u) {
		printf(" %d [%s] {%d,%d} %d:'%.*s'\n", i, names[u.action], 
			u.lineNum, u.colNum, u.length, u.length, u.text);
	}
	
	
}
