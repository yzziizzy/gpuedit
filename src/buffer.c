#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "gui.h"
#include "gui_internal.h"



void BufferLine_SetText(BufferLine* l, char* text, size_t len) {
	if(len == 0) {
		l->length = 0;
		return;
	};
	
	if(l->buf == NULL) {
		l->allocSz = nextPOT(len + 1);
		l->buf = calloc(1, l->allocSz);
// 		l->style = calloc(1, l->allocSz);
	}
	else if(l->allocSz < len + 1) {
		l->allocSz = nextPOT(len + 1);
		l->buf = realloc(l->buf, l->allocSz);
// 		l->style = realloc(l->style, l->allocSz);
		// BUG: check OOM and maybe try to crash gracefully
	}
	
	strncpy(l->buf, text, len);
	l->length = len;
}


void Buffer_RenumberLines(BufferLine* start, size_t num) {
	if(!start) return;
	
	// renumber the rest of the lines
	BufferLine* q = start;
	q->lineNum = num++;
	while(q) {
		q->lineNum = num++;
		q = q->next;
	}
}


BufferLine* BufferLine_New() {
	BufferLine* l = pcalloc(l);
	return l;
}

void BufferLine_Delete(BufferLine* l) {
	if(l->buf) free(l->buf);
	VEC_FREE(&l->style);
}

BufferLine* BufferLine_FromStr(char* text, size_t len) {
	BufferLine* l = BufferLine_New();
	BufferLine_SetText(l, text, len);
	return l;
}

BufferLine* BufferLine_Copy(BufferLine* orig) {
	BufferLine* l = BufferLine_New();
	l->length = orig->length;
	l->allocSz = orig->allocSz;
	l->buf = calloc(1, l->allocSz);
	strncpy(l->buf, orig->buf, l->length);
	VEC_COPY(&l->style, &orig->style);
	return l;
}

void BufferLine_EnsureAlloc(BufferLine* l, size_t len) {
	if(l->buf == NULL) {
		l->allocSz = MAX(32, nextPOT(len + 1));
		l->buf = calloc(1, l->allocSz);
// 		l->style = calloc(1, l->allocSz);
	}
	else if(l->allocSz < len + 1) {
		l->allocSz = nextPOT(len + 1);
		l->buf = realloc(l->buf, l->allocSz);
// 		l->style = realloc(l->style, l->allocSz);
		// BUG: check OOM and maybe try to crash gracefully
	}
}


// does NOT handle embedded linebreak chars
void BufferLine_InsertText(BufferLine* l, char* text, size_t len, size_t col) {
	if(len == 0) len = strlen(text);
	if(len == 0) return;
	
	BufferLine_EnsureAlloc(l, l->length + len);
	
	if(col - 1 < l->length) {
		memmove(l->buf + col - 1 + len, l->buf + col - 1, l->length - col);
	}
	
	memcpy(l->buf + col - 1, text, len);
	
	l->length += len;
}

// does NOT handle embedded linebreak chars
void BufferLine_AppendText(BufferLine* l, char* text, size_t len) {
	if(len == 0) len = strlen(text);
	if(len == 0) return;
	
	BufferLine_EnsureAlloc(l, l->length + len);
	
	memcpy(l->buf + l->length, text, len);
	
	l->length += len;
}


size_t drawCharacter(
	GUIManager* gm, 
	TextDrawParams* tdp, 
	struct Color4* fgColor, 
	struct Color4* bgColor, 
	int c, 
	Vector2 tl
) {
// 		printf("'%s'\n", bl->buf);
	GUIFont* f = tdp->font;
	float size = tdp->fontSize; // HACK
	float hoff = size * f->ascender;//gt->header.size.y * .75; // HACK
		
	struct charInfo* ci = &f->regular[c];
	GUIUnifiedVertex* v;
	
	// background
	if(bgColor->a > 0) {
		v = GUIManager_reserveElements(gm, 1);
		
		*v = (GUIUnifiedVertex){
			.pos.t = tl.y,
			.pos.l = tl.x,
			.pos.b = tl.y + tdp->lineHeight,
			.pos.r = tl.x + tdp->charWidth,
			
			.guiType = 0, // box
			
			.texOffset1.x = ci->texNormOffset.x * 65535.0,
			.texOffset1.y = ci->texNormOffset.y * 65535.0,
			.texSize1.x = ci->texNormSize.x *  65535.0,
			.texSize1.y = ci->texNormSize.y * 65535.0,
			.texIndex1 = ci->texIndex,
			
			.bg = *bgColor,
			
			.z = .5,
			
			// disabled in the shader right now
			.clip = {0,0, 1000000,1000000},
		};
	}
	
	// character
	if(c != ' ' && c != '\t') { // TODO: proper printable character check
		v = GUIManager_reserveElements(gm, 1);
		
		*v = (GUIUnifiedVertex){
			.pos.t = tl.y + hoff - ci->topLeftOffset.y * size,
			.pos.l = tl.x + ci->topLeftOffset.x * size,
			.pos.b = tl.y + hoff + ci->size.y * size - ci->topLeftOffset.y * size,
			.pos.r = tl.x + ci->size.x * size + ci->topLeftOffset.x * size,
			
			.guiType = 1, // text
			
			.texOffset1.x = ci->texNormOffset.x * 65535.0,
			.texOffset1.y = ci->texNormOffset.y * 65535.0,
			.texSize1.x = ci->texNormSize.x *  65535.0,
			.texSize1.y = ci->texNormSize.y * 65535.0,
			.texIndex1 = ci->texIndex,
			
			.fg = *fgColor,
			
			.z = 1,
			
			// disabled in the shader right now
			.clip = {0,0, 1000000,1000000},
		};
	}
	

}


// assumes no linebreaks
void drawTextLine(GUIManager* gm, TextDrawParams* tdp, ThemeDrawParams* theme, char* txt, int charCount, Vector2 tl) {
// 		printf("'%s'\n", bl->buf);
	if(txt == NULL || charCount == 0) return;
	
	int charsDrawn = 0;
	GUIFont* f = tdp->font;
	float size = tdp->fontSize; // HACK
	float hoff = size * f->ascender;//gt->header.size.y * .75; // HACK
	float adv = 0;
	
	
	
	float spaceadv = f->regular[' '].advance;
	
	for(int n = 0; txt[n] != 0 && charsDrawn < charCount; n++) {
		char c = txt[n];
		
		struct charInfo* ci = &f->regular[c];
		
		if(c == '\t') {
			adv += tdp->charWidth * tdp->tabWidth;
			charsDrawn += tdp->charWidth * tdp->tabWidth;
		}
		else if(c != ' ') {
			GUIUnifiedVertex* v = GUIManager_checkElemBuffer(gm, 1);
			
			Vector2 off = tl;
			
			float offx = ci->texNormOffset.x;//TextRes_charTexOffset(gm->font, 'A');
			float offy = ci->texNormOffset.y;//TextRes_charTexOffset(gm->font, 'A');
			float widx = ci->texNormSize.x;//TextRes_charWidth(gm->font, 'A');
			float widy = ci->texNormSize.y;//TextRes_charWidth(gm->font, 'A');
			
			v->pos.t = off.y + hoff - ci->topLeftOffset.y * size;
			v->pos.l = off.x + adv + ci->topLeftOffset.x * size;
			v->pos.b = off.y + hoff + ci->size.y * size - ci->topLeftOffset.y * size;
			v->pos.r = off.x + adv + ci->size.x * size + ci->topLeftOffset.x * size;
			
			v->guiType = 1; // text
			
			v->texOffset1.x = offx * 65535.0;
			v->texOffset1.y = offy * 65535.0;
			v->texSize1.x = widx *  65535.0;
			v->texSize1.y = widy * 65535.0;
			v->texIndex1 = ci->texIndex;
			
			v->clip.t = 0; // disabled in the shader right now
			v->clip.l = 0;
			v->clip.b = 1000000;
			v->clip.r = 1000000;
			
			adv += tdp->charWidth; // ci->advance * size; // BUG: needs sdfDataSize added in?
			v->fg = theme->textColor,
			//v++;
			gm->elementCount++;
			charsDrawn++;
		}
		else {
			adv += tdp->charWidth;
			charsDrawn++;
		}
		
		
	}
	
}



void BufferLine_InsertChar(BufferLine* l, char c, size_t col) {
	BufferLine_EnsureAlloc(l, l->length + 1);
	
	if(col - 1 < l->length) {
		memmove(l->buf + col, l->buf + col - 1, l->length - col + 1);
	}
	
	l->buf[col - 1] = c;
	l->length += 1;
}


void BufferLine_DeleteChar(BufferLine* l, size_t col) {
	if(l->length == 0) return;
	if(col > l->length + 2) return; // strange overrun
	
	if(col - 2 < l->length) {
		memmove(l->buf + col - 2, l->buf + col - 1, l->length - col + 3);
	}
	
	l->length -= 1;
}


BufferLine* Buffer_InsertLineAfter(Buffer* b, BufferLine* before) {
	
	if(before == NULL && b->first) {
		printf("before is null in InsertLineAfter\n");
		return NULL;
	}
	
	b->numLines++;
	BufferLine* after = BufferLine_New();
	
	if(b->first == NULL) {
		b->first = after;
		b->last = after;
		b->current = after;
		after->lineNum = 1;
		return after;
	}
	
	after->next = before->next;
	after->prev = before;
	before->next = after;
	if(after->next) after->next->prev = after;
	
	if(before == b->last) {
		b->last = after;
	}
	
	Buffer_RenumberLines(after, before->lineNum);
	
	return after;
}

BufferLine* Buffer_InsertLineBefore(Buffer* b, BufferLine* after) {
	
	if(after == NULL && b->first) {
		printf("after is null in InsertLineBefore\n");
		return NULL;
	}
	
	b->numLines++;
	BufferLine* before = BufferLine_New();
	
	if(b->first == NULL) {
		b->first = before;
		b->last = before;
		b->current = before;
		before->lineNum = 1;
		return before;
	}
	
	before->next = after;
	before->prev = after->prev;
	after->prev = before;
	if(before->prev) before->prev->next = before;
	
	if(after == b->first) {
		b->first = before;
	}
	
	Buffer_RenumberLines(before, after->lineNum - 1);
	
	return before;
}

void Buffer_DeleteLine(Buffer* b, BufferLine* l) {
	
	if(l == b->first) b->first = l->next;
	if(l == b->last) b->last = l->prev;
	
	if(l == b->current) {
		if(l->next) b->current = l->next;
		else if(l->prev) b->current = l->prev;
		else {
			printf("current line set to null in DeleteLine\n");
			b->current = NULL;
		}
	}

	if(l->next) l->next->prev = l->prev;
	if(l->prev) l->prev->next = l->next;

	
	// TODO check selections
	// TODO renumber lines 
	Buffer_RenumberLines(l->next, l->lineNum - 1);
	
	BufferLine_Delete(l);
	
	
	b->numLines--;
}

void Buffer_InsertLinebreak(Buffer* b) {
	BufferLine* l = b->current;
	
	if(b->curCol == 1) {
		Buffer_InsertLineBefore(b, b->current);
	}
	else {
		BufferLine* n = Buffer_InsertLineAfter(b, l);
		BufferLine_SetText(n, l->buf + b->curCol - 1, strlen(l->buf + b->curCol - 1));
		
		l->buf[b->curCol - 1] = 0;
		l->length = b->curCol;
		
		b->current = b->current->next;
	}
	
	b->curCol = 1;
	
	// TODO: maybe shrink the alloc
}


// deletes chars but also handles line removal and edge cases
// does not move the cursor
void Buffer_BackspaceAt(Buffer* b, BufferLine* l, size_t col) {
	if(!b->first) return; // empty buffer
	
	if(col == 1) {
		// first col of first row; do nothing
		if(b->first == l) return;
		
		if(l->length > 0) {
			// merge with the previous line
			BufferLine_AppendText(l->prev, l->buf, l->length);
		}
		
		Buffer_DeleteLine(b, l);
		
		return;
	} 
	
	BufferLine_DeleteChar(l, col);
}


// deletes chars but also handles line removal and edge cases
// does not move the cursor
void Buffer_DeleteAt(Buffer* b, BufferLine* l, size_t col) {
	if(!b->first) return; // empty buffer
	
	if(col == l->length + 1) {
		// last col of last row; do nothing
		if(b->last == l) return;
		
		if(l->next->length > 0) {
			// merge with the next line
			BufferLine_AppendText(l, l->next->buf, l->next->length);
		}
		
		Buffer_DeleteLine(b, l->next);
		
		return;
	} 
	
	BufferLine_DeleteChar(l, col + 1);
}

void Buffer_ProcessCommand(Buffer* b, BufferCmd* cmd) {
	if(cmd->type == BufferCmd_MoveCursorV) {
		int i = cmd->amt;
		
		if(i > 0) while(i-- > 0 && b->current->next) {
			b->current = b->current->next;
		}
		else while(i++ < 0 && b->current->prev) {
			b->current = b->current->prev;
		}
		
// 		printf("current: %p\n", b->current);
	}
	else if(cmd->type == BufferCmd_MoveCursorH) {
		b->curCol = MAX(MIN(b->current->length + 1, b->curCol + cmd->amt), 1);
	}
	else if(cmd->type == BufferCmd_InsertChar) {
		BufferLine_InsertChar(b->current, cmd->amt, b->curCol);
		b->curCol++;
	}
	else if(cmd->type == BufferCmd_Backspace) {
		Buffer_BackspaceAt(b, b->current, b->curCol);
		b->curCol--;
	}
	else if(cmd->type == BufferCmd_Delete) {
		Buffer_DeleteAt(b, b->current, b->curCol);
	}
	else if(cmd->type == BufferCmd_SplitLine) {
		Buffer_InsertLinebreak(b);
	}
	else if(cmd->type == BufferCmd_DeleteCurLine) {
		Buffer_DeleteLine(b, b->current);
	}
	
// 	printf("line/col %d:%d %d\n", b->current->lineNum, b->curCol, b->current->length);
}

size_t getColOffset(char* txt, int col, int tabWidth) {
	size_t w = 0;
	
	if(!txt) return 0;
	
	for(int i = 0; i < col && txt[i] != 0; i++) {
		if(txt[i] == '\t') w += tabWidth;
		else w++;
	}
	
	return w;
}


Buffer* Buffer_Copy(Buffer* src) {
	Buffer* b = Buffer_New();
	BufferLine* blc, *blc_prev, *bl;
	
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


void Buffer_InsertBufferAt(Buffer* target, Buffer* graft, BufferLine* tline, size_t tcol) {
	
	BufferLine* blc, *bl;
	
	// check for easy special  cases
	if(graft->numLines == 0) return;
	if(graft->numLines == 1) {
		BufferLine_InsertText(tline, graft->first, graft->first->length, tcol);
		return;
	}
	
	// cutting the remainder of the first line to a temporary buffer
	size_t tmplen = tline->length - tcol + 1;
	char* tmp = malloc(tmplen);
	tmp[tmplen] = 0;
	memcpy(tmp, tline->buf + tcol, tmplen);
	
	tline->length = tcol;
	tline->buf[tcol] = 0;
	BufferLine_AppendText(tline, graft->first, graft->first->length);
	
	// copy in the middle lines
	BufferLine* gbl = graft->first->next;
	BufferLine* t = tline;
	while(gbl && gbl != graft->last) {
		
		t = Buffer_InsertLineAfter(target, t);
		BufferLine_SetText(t, gbl->buf, gbl->length);
		
		gbl = gbl->next;
	}
	
	// prepend the last line to the temp buffer
	t = Buffer_InsertLineAfter(target, t);
	BufferLine_SetText(t, graft->last->buf, graft->last->length);
	BufferLine_AppendText(t, tmp, tmplen);
}


Buffer* Buffer_New() {
	
	Buffer* b = pcalloc(b);
	
	return b;
}


void Buffer_AppendRawText(Buffer* b, char* source, size_t len) {
	if(len == 0) len = strlen(source);
	
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
			Buffer_AppendLine(b, s, e-s);
// 		}
		
		if(b->last->buf && b->last->length > 0) {
// 			b->last->style = calloc(1, sizeof(*b->last->style));
// 			HL_acceptLine(b->last->buf, b->last->length, b->last->style);
		}
		
		s = e + 1;
	}
}




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
	
	Buffer_RenumberLines(l, b->current->lineNum);
	
	if(b->current->prev == NULL) {
		b->first = l;
		l->lineNum = 1;
		return l;
	}
	
	return l;
}


BufferLine* Buffer_AppendLine(Buffer* b, char* text, size_t len) {
	BufferLine* l = Buffer_InsertLineAfter(b, b->last);
	BufferLine_SetText(l, text, len);
	return l;
}


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
	
	// HACK
	Buffer_RefreshHighlight(b);
	
	return 0;
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

#include "highlighters/c.h"

void Buffer_RefreshHighlight(Buffer* b) {
	
	if(!b->first) return;
	
	hlinfo hl = {
		.getNextLine = getNextLine,
		.writeSection = writeSection,
		
		.dirtyLines = 10,
		
		.b = b,
		.readLine = b->first,
		.writeLine = b->first,
		.writeCol = 0,
	};
	
	// clear existing styles
	BufferLine* bl = b->first;
	for(int i = 0; i < 10 && bl; i++) {
		VEC_TRUNC(&bl->style);
		
		bl = bl->next;
	}
	
	hlfn(&hl);
	
}


BufferLine* Buffer_GetLine(Buffer* b, size_t line) {
	if(line > b->numLines) return b->last;
	
	// TODO: faster algorithm
	
	BufferLine* bl = b->current;;
	if(line < b->current->lineNum) {
		while(bl->lineNum > line && bl->prev) bl = bl->prev; 
	}
	else if(line > b->current->lineNum) {
		while(bl->lineNum <= line - 1 && bl->next) bl = bl->next;
	}
	
	return bl;
}


void Buffer_CommentLine(Buffer* b, BufferLine* bl) {
	if(!b->ep->lineCommentPrefix) return;
	
	BufferLine_InsertText(bl, b->ep->lineCommentPrefix, strlen(b->ep->lineCommentPrefix), 0);
}


void Buffer_CommentSelection(Buffer* b, BufferSelection* sel) {
	if(!sel) return;
	
	BufferLine_InsertText(
		sel->startLine, 
		b->ep->selectionCommentPrefix, 
		strlen(b->ep->selectionCommentPrefix), 
		sel->startCol
	);
	
	BufferLine_InsertText(
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
	
	// find where to insert
	BufferSelection* bs = b->selectionRing->first;
	while(bs != b->selectionRing->last) {
		
		if(bs->startLine->lineNum < b->sel->startLine->lineNum) {
			
		}
		
	}
	
}
*/

void Buffer_ClearCurrentSelection(Buffer* b) {
	if(b->sel) free(b->sel);
	b->sel = NULL;
}


void Buffer_SetCurrentSelection(Buffer* b, BufferLine* startL, size_t startC, BufferLine* endL, size_t endC) {
	if(!b->sel) pcalloc(b->sel);
	
	assert(startL != NULL);
	assert(endL != NULL);
	
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


    //////////////////////////////////
   //                              //
  //       GUIBufferEditor        //
 //                              //
//////////////////////////////////


static size_t lineFromPos(GUIBufferEditor* w, Vector2 pos) {
	return floor(pos.y / w->bdp->tdp->lineHeight) + w->scrollLines;
}

static size_t getColForPos(GUIBufferEditor* w, BufferLine* bl, float x) {
	size_t col = floor((x - w->header.absTopLeft.x - 50) / w->bdp->tdp->charWidth) + 1 + w->scrollCols;
	return MAX(0, MIN(col, bl->length + 1));

}


static void render(GUIBufferEditor* w, PassFrameParams* pfp) {
// HACK
	GUIBufferEditor_Draw(w, w->header.gm, w->scrollLines, + w->scrollLines + w->linesOnScreen + 2, 0, 100);
	
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
	
	BufferLine* bl = Buffer_GetLine(b, lineFromPos(w, gev->pos));
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
	
	BufferLine* bl = Buffer_GetLine(b, lineFromPos(w, gev->pos));
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
	
	b->current = Buffer_GetLine(b, line);
	
	b->curCol = getColForPos(w, b->current, gev->pos.x);
	
	// maybe nudge the screen down a tiny bit
	GUIBufferEditor_scrollToCursor(w);
}



static void keyUp(GUIObject* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	
	
	
	if(isprint(gev->character) && (gev->modifiers & (~(GUIMODKEY_SHIFT | GUIMODKEY_LSHIFT | GUIMODKEY_RSHIFT))) == 0) {
		Buffer_ProcessCommand(w->buffer, &(BufferCmd){
			BufferCmd_InsertChar, gev->character
		});
	
	}
	else if(gev->type == GUIEVENT_KeyUp) {
		// special commands
		struct {
			unsigned int mods;
			int keysym;
			enum BufferCmdType bcmd;
			int amt_action;
			int amt;
			char scrollToCursor;
		} cmds[] = {
			{GUIMODKEY_CTRL, 'k', BufferCmd_DeleteCurLine, 0, 0, 1},
			{0, XK_Left,      BufferCmd_MoveCursorH,  1, -1, 1},
			{0, XK_Right,     BufferCmd_MoveCursorH,  1,  1, 1},
			{0, XK_Up,        BufferCmd_MoveCursorV,  1, -1, 1},
			{0, XK_Down,      BufferCmd_MoveCursorV,  1,  1, 1},
			{0, XK_BackSpace, BufferCmd_Backspace,    0,  0, 1},
			{0, XK_Delete,    BufferCmd_Delete,       0,  0, 1},
			{0, XK_Return,    BufferCmd_SplitLine,    0,  0, 1},
			{0, XK_Prior,     BufferCmd_MovePage,     1, -1, 1}, // PageUp
			{0, XK_Next,      BufferCmd_MovePage,     1,  1, 1}, // PageDown
			{0, XK_Home,      BufferCmd_Home,         0,  0, 1},
			{0, XK_End,       BufferCmd_End,          0,  0, 1}, 
			{0,0,0,0,0},
		};
		
		unsigned int ANY = (GUIMODKEY_CTRL | GUIMODKEY_ALT | GUIMODKEY_TUX);
		for(int i = 0; cmds[i].bcmd != 0; i++) {
			if(cmds[i].keysym != gev->keycode) continue;
			if(cmds[i].mods & GUIMODKEY_CTRL && !(gev->modifiers && GUIMODKEY_CTRL)) continue;
			if(cmds[i].mods & GUIMODKEY_ALT && !(gev->modifiers && GUIMODKEY_ALT)) continue;
			if(cmds[i].mods & GUIMODKEY_TUX && !(gev->modifiers && GUIMODKEY_TUX)) continue;
			
			// TODO: specific mods
			Buffer_ProcessCommand(w->buffer, &(BufferCmd){
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
		.KeyUp = keyUp,
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


void GUIBufferEditor_Draw(GUIBufferEditor* gbe, GUIManager* gm, int lineFrom, int lineTo, int colFrom, int colTo) {
	Buffer* b = gbe->buffer;
	
	if(!b) return;
	
	BufferDrawParams* bdp = gbe->bdp;
	TextDrawParams* tdp = bdp->tdp;
	ThemeDrawParams* theme = bdp->theme;
	GUIFont* f = gbe->font;
	int line = 1;
	int linesRendered = 0;
	char lnbuf[32];
	GUIUnifiedVertex* v;
	
	// TODO: move to gbe->resize or somewhere appropriate
	gbe->linesOnScreen = gbe->header.size.y / tdp->lineHeight;
	
	// draw general background
	v = GUIManager_reserveElements(gm, 1);
	*v = (GUIUnifiedVertex){
		.pos = {gbe->header.absTopLeft.x, gbe->header.absTopLeft.y, gbe->header.absTopLeft.x + 800, gbe->header.absTopLeft.y + 800},
		.clip = {0, 0, 18000, 18000},		
		.guiType = 0, // window (just a box)
		.fg = {0, 0, 255, 255}, // TODO: border color
		.bg = theme->bgColor, 
		.z = .025,
		.alpha = 1,
	};
	
	
	Vector2 tl = gbe->header.absTopLeft;
	if(bdp->showLineNums) tl.x += bdp->lineNumWidth;
	
	BufferLine* bl = b->first; // BUG broken 
	
	// scroll down
	// TODO: cache a pointer
	for(size_t i = 0; i < gbe->scrollLines && bl->next; i++) bl = bl->next; 
	
	int inSelection = 0;
	int maxCols = 100;
	
	struct Color4* fg = &theme->textColor; 
	struct Color4* bg = &theme->bgColor;
	struct Color4* fg2 = &theme->hl_textColor; 
	struct Color4* bg2 = &theme->hl_bgColor;
	
	struct Color4* fga[] = {fg, fg2};
	struct Color4* bga[] = {bg, bga};
	
	// draw
	while(bl) {
		
		// line numbers
		if(bdp->showLineNums) {
			sprintf(lnbuf, "%d", bl->lineNum);
			drawTextLine(gm, tdp, theme, lnbuf, 100, (Vector2){tl.x - bdp->lineNumWidth, tl.y});
		}
		
		float adv = 0;

		
		if(bl->buf) { // only draw lines with text
			
			size_t styleIndex = 0;
			size_t styleCols = 0;
			TextStyleAtom* atom = NULL;
			if(VEC_LEN(&bl->style)) {
				atom = &VEC_HEAD(&bl->style);
				styleCols = atom->length;
			}
			
			// main text
			for(int i = 0; i < maxCols; i++) { 
				if(b->sel && b->sel->startLine == bl && b->sel->startCol - 1 <= i + gbe->scrollCols) {
					inSelection = 1;
					fg = &theme->hl_textColor;
					bg = &theme->hl_bgColor;
				}
				if(b->sel && b->sel->endLine == bl && b->sel->endCol <= i + gbe->scrollCols) {
					inSelection = 0;
					fg = &theme->textColor;
					bg = &theme->bgColor;
				} 
				
				int c = bl->buf[gbe->scrollCols + i]; 
				if(c == 0) break;
				
				if(c == '\t') {
					adv += tdp->charWidth * tdp->tabWidth;
				}
				else {
					if(atom) 
						drawCharacter(gm, tdp, fga[atom->styleIndex], bg, c, (Vector2){tl.x + adv, tl.y});
					else 
						drawCharacter(gm, tdp, fg, bg, c, (Vector2){tl.x + adv, tl.y});
					
					
					adv += tdp->charWidth;
				}
				
				// check on the style
				styleCols--;
				
				if(atom && styleCols <= 0) {
					styleIndex++;
					if(styleIndex < VEC_LEN(&bl->style)) {
						atom = &VEC_ITEM(&bl->style, styleIndex);
						styleCols = atom->length;
					} 
					else atom = NULL;
				}
			}
		}
		
		// advance to the next line
		tl.y += tdp->lineHeight;
		bl = bl->next;
		linesRendered++;
		if(linesRendered > lineTo - lineFrom) break;
		
		if(tl.y > gbe->header.size.y) break; // end of buffer control
	}
	
	
	// draw cursor
	tl = (Vector2){gbe->header.absTopLeft.x + 50, gbe->header.absTopLeft.y};
	v = GUIManager_reserveElements(gm, 1);
	float cursorOff = getColOffset(b->current->buf, b->curCol - 1, tdp->tabWidth) * tdp->charWidth;
	float cursory = (b->current->lineNum - 1 - gbe->scrollLines) * tdp->lineHeight;
	*v = (GUIUnifiedVertex){
		.pos = {tl.x + cursorOff, tl.y + cursory, tl.x + cursorOff + 2, tl.y + cursory + tdp->lineHeight},
		.clip = {0, 0, 18000, 18000},
		.guiType = 0, // window (just a box)
		.fg = {255, 128, 64, 255}, // TODO: border color
		.bg = theme->cursorColor, 
		.z = 2.5,
		.alpha = 1,
	};
	
	
	// draw scrollbar
	
	float sbWidth = 20;
	float sbMinHeight = 20;
	
	// calculate scrollbar height
	float wh = gbe->header.size.y;
	float sbh = fmax(wh / b->numLines, sbMinHeight);
	
	// calculate scrollbar offset
	float sboff = ((wh - sbh) / b->numLines) * (b->current->lineNum - 1);
	
	tl = (Vector2){gbe->header.absTopLeft.x + gbe->header.size.x - sbWidth, gbe->header.absTopLeft.y};
// 	tl = (Vector2){0,0};
	v = GUIManager_reserveElements(gm, 1);
	*v = (GUIUnifiedVertex){
		.pos = {tl.x, tl.y + sboff, tl.x + sbWidth, tl.y + sboff + sbh},
		.clip = {0, 0, 18000, 18000},
		.guiType = 0, // window (just a box)
		.fg = {255, 255, 255, 255}, // TODO: border color
		.bg = {255, 255, 255, 255}, 
		.z = 2.5,
		.alpha = 1,
	};
	
	
	// HACK
// 	gt->header.hitbox.max = (Vector2){adv, gt->header.size.y};
}
