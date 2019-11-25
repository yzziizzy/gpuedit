#include <stdio.h>
#include <string.h>

#include "common_math.h"


#include "buffer.h"
#include "gui.h"
#include "gui_internal.h"



void BufferLine_SetText(BufferLine* l, char* text, size_t len) {
	if(len == 0) len = strlen(text);
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
}


BufferLine* Buffer_AddLineBelow(Buffer* b) {
	BufferLine* l = pcalloc(l);
	b->numLines++;
	
	// special cases first
	if(b->first == NULL) {
		b->first = l;
		b->last = l;
		b->current = l;
		l->lineNum = 1;
		return l;
	}
	
	l->lineNum = b->current->lineNum + 1;
	
	if(b->current == b->last) {
		l->prev = b->last;
		b->last->next = l;
		b->last = l;
		return l;
	}
	
	// insert the link
	l->prev = b->current;
	l->next = b->current->next;
	
	b->current->next = l;
	if(l->next) l->next->prev = l;
	
	// renumber the rest of the lines
	BufferLine* q = l->next;
	while(q) {
		q->lineNum++;
		q = q->next;
	}
	
	
	return l;
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


BufferLine* Buffer_AdvanceLines(Buffer* b, int n) {
	
	BufferLine* l = b->current;
	while(l->next && n) {
		l = l->next;
		b->curLine++;
		n--;
	}
	
	b->current = l;
	
	return l;
}


void Buffer_insertText(Buffer* b, char* text, size_t len) {
	if(len == 0) len = strlen(text);
	if(len == 0) return;
	
	BufferLine* l = b->current; 
	
	if(l->buf == NULL) {
		l->allocSz = nextPOT(len + 1);
		l->buf = calloc(1, l->allocSz);
		l->style = calloc(1, l->allocSz);
	}
	else if(l->allocSz < l->length + len + 1) {
		l->allocSz = nextPOT(l->length + len + 1);
		l->buf = realloc(l->buf, l->allocSz);
		l->style = realloc(l->style, l->allocSz);
		// BUG: check OOM and maybe try to crash gracefully
	}
	
	if(b->curCol - 1 < l->length) {
		memmove(l->buf + b->curCol - 1 + len, l->buf + b->curCol - 1, l->length - b->curCol);
	}
	
	memcpy(l->buf + b->curCol - 1, text, len);
}


// assumes no linebreaks
void drawTextLine(GUIManager* gm, TextDrawParams* tdp, char* txt, int charCount, Vector2 tl) {
// 		printf("'%s'\n", bl->buf);
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


void Buffer_Draw(Buffer* b, GUIManager* gm, int lineFrom, int lineTo, int colFrom, int colTo) {
	GUIFont* f = b->font;
	int line = 1;
	int linesRendered = 0;
	char lnbuf[32];
	
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
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	float cursorOff = (b->curCol - 1) * tdp.charWidth;
	float cursory = (b->curLine - 1) * tdp.lineHeight;
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


void Buffer_loadRawText(Buffer* b, char* source, size_t len) {
	if(len == 0) len = strlen(source);
	
	
	for(size_t i = 0; i < len; i++) {
		char* s = source + i;
		char* e = strpbrk(s, "\r\n");
		
		if(e == NULL) {
			Buffer_AppendLine(b, s, 0);
			return;
		}
		
		if(*e == '\n') {
			Buffer_AppendLine(b, s, e-s);
			i += e - s;
		}
		
		if(*e == '\r') {
			Buffer_AppendLine(b, s, e-s);
			i += e - s + 1;
		}
		
		
	}
	
	
}




BufferLine* Buffer_AppendLine(Buffer* b, char* text, size_t len) {
	BufferLine* l = pcalloc(l);
	
	l->lineNum = b->last->lineNum + 1;
	
	if(b->last == NULL) {
		b->first = l;
		b->last = l;
		b->current = l;
		l->lineNum = 1;
		return l;
	}
	
	l->prev = b->last;
	b->last->next = l;
	b->last = l;
	
	BufferLine_SetText(l, text, len);
	
	return l;
}

