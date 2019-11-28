#include <stdio.h>
#include <string.h>

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
		l->style = calloc(1, l->allocSz);
	}
	else if(l->allocSz < len + 1) {
		l->allocSz = nextPOT(len + 1);
		l->buf = realloc(l->buf, l->allocSz);
		l->style = realloc(l->style, l->allocSz);
		// BUG: check OOM and maybe try to crash gracefully
	}
	
	strncpy(l->buf, text, len);
	l->length = len;
}


void Buffer_RenumberLines(BufferLine* start, size_t num) {
		// renumber the rest of the lines
	BufferLine* q = start;
	q->lineNum = num++;
	while(q) {
		q->lineNum = num++;
		q = q->next;
	}
}



void test(Buffer* b) {
	BufferLine* q;
	
	printf("\n");
	
	q = b->first;
	while(q) {
		printf("line %d: %p '%s'\n", q->lineNum, q, q->buf); 
		q = q->next;
	}
	
	printf("\n");
}


BufferLine* BufferLine_New() {
	BufferLine* l = pcalloc(l);
	return l;
}

void BufferLine_Delete(BufferLine* l) {
	if(l->buf) free(l->buf);
	if(l->style) free(l->style);
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
	return l;
}

void BufferLine_EnsureAlloc(BufferLine* l, size_t len) {
	if(l->buf == NULL) {
		l->allocSz = MAX(32, nextPOT(len + 1));
		l->buf = calloc(1, l->allocSz);
		l->style = calloc(1, l->allocSz);
	}
	else if(l->allocSz < len + 1) {
		l->allocSz = nextPOT(len + 1);
		l->buf = realloc(l->buf, l->allocSz);
		l->style = realloc(l->style, l->allocSz);
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


// assumes no linebreaks
void drawTextLine(GUIManager* gm, TextDrawParams* tdp, char* txt, int charCount, Vector2 tl) {
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
			v->fg = (struct Color4){255, 128, 64, 255},
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
	
	if(l->next) l->next->prev = l->prev;
	if(l->prev) l->prev->next = l->next;
	
	if(l == b->first) b->first = l->next;
	if(l == b->last) b->last = l->prev;
	
	if(l == b->current) {
		if(l->prev) b->current = l->prev;
		else if(l->prev) b->current = l->next;
		else {
			printf("current line set to null in DeleteLine\n");
			b->current = NULL;
		}
	}
	
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
	
// 	printf("line/col %d:%d %d\n", b->current->lineNum, b->curCol, b->current->length);
}

size_t getColOffset(char* txt, int col, int tabWidth) {
	size_t w = 0;
	for(int i = 0; i < col && txt[i] != 0; i++) {
		if(txt[i] == '\t') w += tabWidth;
		else w++;
	}
	
	return w;
}

void Buffer_Draw(Buffer* b, GUIManager* gm, int lineFrom, int lineTo, int colFrom, int colTo) {
	GUIFont* f = b->font;
	int line = 1;
	int linesRendered = 0;
	char lnbuf[32];
	GUIUnifiedVertex* v;
	/*
		// draw background
	v = GUIManager_reserveElements(gm, 1);
	*v = (GUIUnifiedVertex){
		.pos = {0,0, 800, 800},
		.clip = {0, 0, 18000, 18000},
		
		.guiType = 0, // window (just a box)
		
		.texIndex1 = 0, .texIndex2 = 0, .texFade = 0,
		.texOffset1 = 0, .texOffset2 = 0, .texSize1 = {65535.0,65535.0}, .texSize2 = 1,
		
		.fg = {0, 0, 255, 255}, // TODO: border color
		.bg = {12, 12, 12, 255}, 
		
		.z = .025,
		.alpha = 1,
	};
	*/
	
	TextDrawParams tdp;
	tdp.font = b->font;
	tdp.fontSize = .5;
	tdp.charWidth = 10;
	tdp.lineHeight = 20;
	tdp.tabWidth = 4;
	Vector2 tl = (Vector2){50, 0};
	
	BufferLine* bl = b->first; // BUG broken 
	while(bl) {
		
		sprintf(lnbuf, "%d", bl->lineNum);
		drawTextLine(gm, &tdp, lnbuf, 100, (Vector2){tl.x - 50, tl.y});
		
		drawTextLine(gm, &tdp, bl->buf, 100, tl);
		
		tl.y += tdp.lineHeight;
		bl = bl->next;
		linesRendered++;
		if(linesRendered > lineTo - lineFrom) break;
		
	}
	

	
	// draw cursor
	tl = (Vector2){50, 0};
	v = GUIManager_reserveElements(gm, 1);
	float cursorOff = getColOffset(b->current->buf, b->curCol - 1, tdp.tabWidth) * tdp.charWidth;
	float cursory = (b->current->lineNum - 1) * tdp.lineHeight;
	*v = (GUIUnifiedVertex){
		.pos = {tl.x + cursorOff, tl.y + cursory, tl.x + cursorOff + 2, tl.y + cursory + tdp.lineHeight},
		.clip = {0, 0, 18000, 18000},
		
		.guiType = 0, // window (just a box)
		
		.texIndex1 = 0, .texIndex2 = 0, .texFade = 0,
		.texOffset1 = 0, .texOffset2 = 0, .texSize1 = 0, .texSize2 = 0,
		
		.fg = {255, 128, 64, 255}, // TODO: border color
		.bg = {255, 255, 255, 255}, 
		
		.z = 2.5,
		.alpha = 1,
	};
	
	
	// HACK
// 	gt->header.hitbox.max = (Vector2){adv, gt->header.size.y};
}


static void render(Buffer* w, PassFrameParams* pfp) {
// HACK
	Buffer_Draw(w, w->header.gm, 0, 100, 0, 100);
	
}

Buffer* Buffer_New(GUIManager* gm) {
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
	};
	
	
	Buffer* b = pcalloc(b);
	
	gui_headerInit(&b->header, gm, &static_vt);
	
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
	
	return 0;
}
