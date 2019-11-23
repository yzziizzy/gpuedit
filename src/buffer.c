#include <stdio.h>
#include <string.h>

#include "common_math.h"


#include "buffer.h"
#include "gui.h"
#include "gui_internal.h"



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
		b->last = l;
		return l;
	}
	
	// insert the link
	l->prev = b->current;
	l->next = b->current->next;
	
	b->current->prev = l;
	l->next->prev = l;
	
	// renumber the rest of the lines
	BufferLine* q = l->next;
	while(q) {
		q->lineNum++;
		q = q->next;
	}
	
	return l;
}



void Buffer_insertText(Buffer* b, char* text, size_t len) {
	if(len == 0) len = strlen(text);
	if(len == 0) return;
	
	BufferLine* l = b->current; 
	
	if(l->buf == NULL) {
		l->allocSz = nextPOT(len + 1);
		l->buf = calloc(1, l->allocSz);
	}
	else if(l->allocSz < l->length + len + 1) {
		l->allocSz = nextPOT(l->length + len + 1);
		l->buf = realloc(l->buf, l->allocSz);
		// BUG: check OOM and maybe try to crash gracefully
	}
	
	if(b->curCol - 1 < l->length) {
		memmove(l->buf + b->curCol - 1 + len, l->buf + b->curCol - 1, l->length - b->curCol);
	}
	
	memcpy(l->buf + b->curCol - 1, text, len);
}


void Buffer_Draw(Buffer* b, GUIManager* gm, int lineFrom, int lineTo, int colFrom, int colTo) {
	GUIFont* f = b->font;
	int line = 1;
	int linesRendered = 0;
	
	Vector2 tl = (Vector2){100, 100};
	
	BufferLine* bl = b->first; // BUG broken 
	while(bl) {
		
		char* txt = bl->buf;
		
		float size = 1.45; // HACK
		float hoff = 150;//gt->header.size.y * .75; // HACK
		float adv = 0;
		
		float spaceadv = f->regular[' '].advance;
		
		// this algorithm needs to be kept in sync with the width calculation algorithm below
		for(int n = 0; txt[n] != 0; n++) {
			char c = txt[n];
			
			struct charInfo* ci = &f->regular[c];
			
			
			if(c != ' ') {
				GUIUnifiedVertex* v = GUIManager_checkElemBuffer(gm, 1);
				
				Vector2 off = {0,50};
				
				float offx = ci->texNormOffset.x;//TextRes_charTexOffset(gm->font, 'A');
				float offy = ci->texNormOffset.y;//TextRes_charTexOffset(gm->font, 'A');
				float widx = ci->texNormSize.x;//TextRes_charWidth(gm->font, 'A');
				float widy = ci->texNormSize.y;//TextRes_charWidth(gm->font, 'A');
				
				v->pos.t = off.y + line - ci->topLeftOffset.y * size;
				v->pos.l = off.x + adv + ci->topLeftOffset.x * size;
				v->pos.b = off.y + line + ci->size.y * size - ci->topLeftOffset.y * size;
				v->pos.r = off.x + adv + ci->size.x * size + ci->topLeftOffset.x * size;
				
// 				v->pos.t = 1000;
// 				v->pos.l = -1000;
// 				v->pos.b = 5000;
// 				v->pos.r = -5000;
				
				v->guiType = 1; // text
				
				v->texOffset1.x = offx * 65535.0;
				v->texOffset1.y = offy * 65535.0;
				v->texSize1.x = widx *  65535.0;
				v->texSize1.y = widy * 65535.0;
				v->texIndex1 = ci->texIndex;
				
				
				
				v->clip.t = 0;
				v->clip.l = 0;
				v->clip.b = 1000000;
				v->clip.r = 1000000;
				
				adv += ci->advance * size; // BUG: needs sdfDataSize added in?
				v->fg = (struct Color4){255, 128, 64, 255},
				//v++;
				gm->elementCount++;
			}
			else {
				adv += spaceadv;
			}
			
		}
		
		line++;
		bl = bl->next;
		linesRendered++;
		if(linesRendered > lineTo - lineFrom) break;
		
	}
	
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
