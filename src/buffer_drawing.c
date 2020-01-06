#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"
#include "math.h"

#include "buffer.h"
#include "gui.h"
#include "gui_internal.h"
#include "clipboard.h"

#include "highlighters/c.h"



size_t getColOffset(char* txt, int col, int tabWidth) {
	size_t w = 0;
	
	if(!txt) return 0;
	
	for(int i = 0; i < col && txt[i] != 0; i++) {
		if(txt[i] == '\t') w += tabWidth;
		else w++;
	}
	
	return w;
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
	
	return 0;
}


void GUIBufferEditor_Draw(GUIBufferEditor* gbe, GUIManager* gm, int lineFrom, int lineTo, int colFrom, int colTo) {
	Buffer* b = gbe->buffer;
	
	if(!b) return;
	if(!b->first) return;
	
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
		.pos = {
			gbe->header.absTopLeft.x, 
			gbe->header.absTopLeft.y, 
			gbe->header.absTopLeft.x + gbe->header.size.x, 
			gbe->header.absTopLeft.y + gbe->header.size.y
		},
		.clip = {0, 0, 18000, 18000},		
		.guiType = 0, // window (just a box)
		.fg = {0, 0, 255, 255}, // TODO: border color
		.bg = theme->bgColor, 
		.z = .025,
		.alpha = 1,
	};
	
	float lineNumWidth = ceil(log10(b->numLines)) * tdp->charWidth + bdp->lineNumExtraWidth;
	
	Vector2 tl = gbe->header.absTopLeft;
	if(bdp->showLineNums) tl.x += lineNumWidth;
	
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
	
	struct Color4 lineColors[] = {
		{255,255,255,255},
		{ 55,255, 55,255}
	};
	
	
	// draw
	while(bl) {
		
		// line numbers
		if(bdp->showLineNums) {
			sprintf(lnbuf, "%d", bl->lineNum);
			float nw = (floor(log10(bl->lineNum)) + 1) * tdp->charWidth;
			drawTextLine(
				gm, 
				tdp, 
				&lineColors[!!(bl->flags & BL_BOOKMARK_FLAG)], 
				lnbuf, 
				100, 
				(Vector2){tl.x - nw - bdp->lineNumExtraWidth, tl.y});
		}
		
		float adv = 0;
		
		// handle selections ending on empty lines
		if(b->sel && b->sel->endLine == bl && bl->length == 0) {
			inSelection = 0;
			fg = &theme->textColor;
			bg = &theme->bgColor;
		}
		
		
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
				if(b->sel && b->sel->startLine == bl && b->sel->startCol <= i + gbe->scrollCols) {
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
					if(!inSelection) {
						
						if(atom) {
							StyleInfo* si = &gbe->h->styles[atom->styleIndex];
							struct Color4 color = {
								si->fgColor.x * 255,
								si->fgColor.y * 255,
								si->fgColor.z * 255,
								si->fgColor.w * 255,
							};
							
							drawCharacter(gm, tdp, &color, bg, c, (Vector2){tl.x + adv, tl.y});
						}
						else 
							drawCharacter(gm, tdp, fg, bg, c, (Vector2){tl.x + adv, tl.y});
					}
					else {
						if(atom) {
							StyleInfo* si = &gbe->h->styles[atom->styleIndex];
							struct Color4 color = {
								si->fgSelColor.x * 255,
								si->fgSelColor.y * 255,
								si->fgSelColor.z * 255,
								si->fgSelColor.w * 255,
							};
							struct Color4 bcolor = {
								si->bgSelColor.x * 255,
								si->bgSelColor.y * 255,
								si->bgSelColor.z * 255,
								si->bgSelColor.w * 255,
							};
							
							drawCharacter(gm, tdp, &color, &bcolor, c, (Vector2){tl.x + adv, tl.y});
						}
						else 
							drawCharacter(gm, tdp, fg2, bg2, c, (Vector2){tl.x + adv, tl.y});
					}
					
					
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
	if(gbe->cursorBlinkTimer <= gbe->cursorBlinkOnTime) {
		tl = (Vector2){gbe->header.absTopLeft.x + lineNumWidth, gbe->header.absTopLeft.y};
		v = GUIManager_reserveElements(gm, 1);
		float cursorOff = getColOffset(b->current->buf, b->curCol, tdp->tabWidth) * tdp->charWidth;
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
	}
	
	
	// HACK
// 	gt->header.hitbox.max = (Vector2){adv, gt->header.size.y};
}





// assumes no linebreaks
void drawTextLine(GUIManager* gm, TextDrawParams* tdp, struct Color4* textColor, char* txt, int charCount, Vector2 tl) {
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
			v->fg = *textColor,
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








