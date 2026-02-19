#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"
#include "math.h"

#include "buffer.h"
#include "ui/gui.h"
#include "ui/gui_internal.h"
#include "clipboard.h"






size_t getColOffset(char* txt, int col, int tabWidth) {
	size_t w = 0;
	
	if(!txt) return 0;
	
	for(int i = 0; i < col && txt[i] != 0; i++) {
		if(txt[i] == '\t') w += tabWidth;
		else w++;
	}
	
	return w;
}




#include "ui/macros_on.h"

// draws the editor's text area and line numbers
void GUIBufferEditControl_Draw(GUIBufferEditControl* gbe, GUIManager* gm, Vector2 tl, Vector2 sz,
	 int lineFrom, int lineTo, int colFrom, int colTo, PassFrameParams* pfp) {
	Buffer* b = gbe->b;
	
	if(!b) return;
	if(!b->first) return;
	
	if(b->reloadGCCErrorFile) Buffer_ReloadGCCErrorFile(b);
	
//	BufferUndo_DebugRender(gm, b, V(500, 20), V(200, 800), pfp);
	
	
	float z = gm->curZ;
	
	ThemeSettings* ts = gbe->ts;
	BufferSettings* bs = gbe->bs;
	float fsize = bs->fontSize; 
	GUIFont* f = gbe->font;
	
	int isBitmap = 0;
	if(fsize >= 4 && fsize + .4999f <= 36) {
		int b = floor(fsize + .4999f) - 4;
		if(f->bitmapFonts[b]) {
			f = f->bitmapFonts[b];
			isBitmap = 1;
			fsize = floor(fsize + .4999f);
		}
	}
	
	
	float ascender = f->ascender * fsize;
	int line = 1;
	int linesRendered = 0;
	char lnbuf[32];
	GUIUnifiedVertex* v;
	
	float edh = sz.y;
	
	// draw general background	
	gm->curZ = z + 0.1;
	GUI_Rect(tl, sz, &ts->bgColor);
	gm->curZ = z;
	
	float lineNumWidth = ceil(LOGB(bs->lineNumBase, b->numLines + 1)) * bs->charWidth + bs->lineNumExtraWidth;
	float hsoff = -gbe->scrollCols * bs->charWidth;
	

	if(bs->showLineNums) {
		gm->curZ += 1;
		GUI_Rect(tl, V(lineNumWidth, edh), &ts->lineNumBgColor);
		gm->curZ -= 1;
		
		tl.x += lineNumWidth;
	}

	gbe->textAreaOffsetX = tl.x; // save for other functions
	
	BufferLine* bl = b->first; // BUG broken 
	
	// scroll down
	// TODO: cache a pointer
	long scrollPixels = gbe->scrollLines * gbe->bs->lineHeight;
	linenum_t sl = 0;
	for(; VEC_item(&gbe->lineOffsets, sl) < scrollPixels; sl++); 
	for(intptr_t i = 0; i < sl && bl->next; i++) bl = bl->next; 
	
	
	int inSelection = 0;
	int maxCols = 10000;
	
	struct Color4* fg = &ts->textColor; 
	struct Color4* bg = &ts->bgColor;
	
	struct Color4 lineColors[] = {
		ts->lineNumColor,
		ts->lineNumBookmarkColor,
		ts->lineNumBreakpointColor,
		{.5,.5,1,1}
	};

	StyleInfo* styles;
	if(ts->is_dark) {
		styles = gbe->h->stylesDark;
	} else {
		styles = gbe->h->stylesLight;
	}
	
	
	float ytop = tl.y;
	float yoff = tl.y + f->ascender * fsize;
	
	int xx = 0;
	// draw lines
	while(bl) {
		
		// line numbers
		if(bs->showLineNums) {
			sprintlongb(lnbuf, bs->lineNumBase, bl->lineNum, bs->lineNumCharset);
			float nw = (floor(LOGB(bs->lineNumBase, bl->lineNum)) + 1) * bs->charWidth;

			Color4* lnc = &lineColors[0];
			if(bl->flags & BL_BOOKMARK_FLAG) lnc = &lnc[1];
			if(bl->flags & BL_BREAKPOINT_FLAG) lnc = &lnc[2];
			if(bl->flags & BL_ANNOTATION_FLAG) lnc = &lnc[3]; //temp

			gm->curZ++;
			drawTextLine(
				gm, 
				bs,
				f,
				lnc, 
				lnbuf, 
				100, 
				(Vector2){tl.x - nw - bs->lineNumExtraWidth, yoff}
			);
			gm->curZ--;
		}
		
		float adv = 0;
		
		
		// highlight current line
		if(bl == CURSOR_LINE(gbe->sel) && gbe->outlineCurLine && !HAS_SELECTION(gbe->sel)) {
			gm->curZ = z + 10;
			GUI_Box(
				V(tl.x - 1, yoff + bs->outlineCurrentLineYOffset - ascender), 
				V(sz.x - gbe->textAreaOffsetX, bs->lineHeight + bs->outlineCurrentLineYOffset + 2), // +1 to not cover underscores
				1,
				&ts->outlineCurrentLineBorderColor
			);
		}
		
		
		if(bl->length) { // only draw lines with text
			
			// look up the highlighter info
			size_t styleIndex = 0;
			size_t styleCols = 0;
			TextStyleAtom* atom = NULL;
			if(VEC_len(&bl->style)) {
				atom = &VEC_head(&bl->style);
				styleCols = atom->length;
			}
			
			// draw characters
			for(int i = 0; i < bl->length; i++) { 
				int noInvert = 0;
				if(BufferRangeSet_test(gbe->selSet, bl, i)) {
					// inside main selection
					inSelection = 1;
					fg = &ts->hl_textColor;
					bg = &ts->hl_bgColor;
				}
				else {
					if(BufferRangeSet_test(gbe->findSet, bl, i)) {
						// inside other selection
						inSelection = 1;
						fg = &ts->find_textColor;
						bg = &ts->find_bgColor;
					}
					else if(BufferRangeSet_test(gbe->findSearchSpace, bl, i)) {
						// inside other selection
						inSelection = 1;
						noInvert = 1;
//						fg = &ts->findSpace_textColor;
						bg = &ts->findSpace_bgColor;
					}
					else {
						inSelection = 0;
						fg = &ts->textColor;
						bg = &ts->bgColor;	
					}
				}
				
				
				int c = bl->buf[i]; 
//				if(c == 0) break;
				
				struct Color4 color;
				
				// highlighter color
				if(atom) {
					StyleInfo* si = &styles[atom->styleIndex];
					// si->fgSelColor is never changed from generated values in lexer.c
					color = (struct Color4){
						si->fgColor.r,
						si->fgColor.g,
						si->fgColor.b,
						si->fgColor.a,
					};
					
					fg = &color;
				}
				
				// hack before selection theme colors are done
				Color4 invfg;
				if(!noInvert && inSelection && bs->invertSelection) {
					invfg.r = 1.0 - fg->r;
					invfg.g = 1.0 - fg->g;
					invfg.b = 1.0 - fg->b;
					invfg.a = 1.0;
					fg = &invfg;
				}
				
				
				
				
				// special drawing for null characters
				if(c == 0) {
					Color4 pulseColor = {
						sin(fmod(pfp->appTime*.5, 6.28)) * .5 + .5,
						cos(fmod(pfp->appTime*.5, 6.28)) * .5 + .5,
						0,
						1
					};
					Color4 pulseColorBg = {
						cos(fmod(pfp->appTime* 2, 6.28)) * .5 + .5,
						sin(fmod(pfp->appTime*2, 6.28)) * .5 + .5,
						0,
						1
					};
					
					if(adv >= gbe->scrollCols * bs->charWidth && adv < (gbe->scrollCols * bs->charWidth) + sz.x) {
						gm->curZ = z + 4;
						GUI_BitmapChar_NoGuard('0', V(tl.x + hsoff + adv, yoff), f, fsize, &pulseColor);
						gm->curZ = z + 2;
						GUI_Rect(V(tl.x + adv + hsoff, yoff - ascender), V(MAX(5, (float)bs->charWidth), bs->lineHeight), &pulseColorBg);
					}
					
					adv += bs->charWidth;	
				}
				else if(c == '\t') {
					if(inSelection) { // tabs inside selection
						gm->curZ = z + 2;
						GUI_Rect(V(tl.x + adv + hsoff, yoff - ascender), V(adv + hsoff + (bs->charWidth * bs->tabWidth), bs->lineHeight), bg);
					}
					
					adv += bs->charWidth * bs->tabWidth;
				}
				else { 
					// normal, non-tab text		
					if(adv >= gbe->scrollCols * bs->charWidth && adv < (gbe->scrollCols * bs->charWidth) + sz.x) {
						gm->curZ = z + 4;
						GUI_BitmapChar_NoGuard(c, V(tl.x + hsoff + adv, yoff), f, fsize, fg);
						gm->curZ = z + 2;
						if(inSelection) GUI_Rect(V(tl.x + adv + hsoff, yoff - ascender), V(MAX(5, (float)bs->charWidth), bs->lineHeight), bg);
					}
					
					adv += bs->charWidth;
				}
				
				
				// check on the style
				styleCols--;
				
				if(atom && styleCols <= 0) {
					styleIndex++;
					if(styleIndex < VEC_len(&bl->style)) {
						atom = &VEC_item(&bl->style, styleIndex);
						styleCols = atom->length;
					}
					else atom = NULL;
				}
			}
		}
		else { // empty lines
			// check selection
			
			if(BufferRangeSet_test(gbe->selSet, bl, 0)) {
				// inside main selection
				inSelection = 1;
				bg = &ts->hl_bgColor;
			}
			else {
				if(BufferRangeSet_test(gbe->findSet, bl, 0)) {
					// inside other selection
					inSelection = 1;
					bg = &ts->find_bgColor;
				}
				else if(BufferRangeSet_test(gbe->findSearchSpace, bl, 0)) {
					// inside other selection
					inSelection = 1;
					bg = &ts->findSpace_bgColor;
				}
				else {
					inSelection = 0;
					bg = &ts->bgColor;	
				}
			}
		}

        // draw a little half-width selection background at the end of selected lines (and empty ones)
		if(inSelection) {
			gm->curZ = z + 2;
			GUI_Rect(V(tl.x + adv + hsoff, yoff - ascender), V(MAX(5, (float)bs->charWidth / 1.0), bs->lineHeight), bg);
		}

		if(ytop > edh) break; // end of buffer control


		// annotations
		
		if(bl->flags & BL_ANNOTATION_FLAG) {

			BufferAnnotation* ann;
			HT_get(&b->gccErrors, bl->lineNum, &ann);
			
			if(ann) {
				yoff += bs->lineHeight;
				ytop += bs->lineHeight;
			
				drawTextLine(
					gm, 
					bs,
					f,
					&lineColors[0], 
					ann->message, 
					100, 
					(Vector2){tl.x, yoff}
				);
			}
		}
		

		// advance to the next line
		yoff += bs->lineHeight;
		ytop += bs->lineHeight;
		bl = bl->next;
		linesRendered++;
		if(linesRendered > lineTo - lineFrom) break;
		
		

		
	}
	
	
	
	
	
	// draw cursors
	VEC_EACH(&gbe->selSet->ranges, i, r) {
		if(gbe == gm->activeID) {
			if(r == gbe->sel && (gbe->cursorBlinkTimer > gbe->cursorBlinkOnTime)) continue;
		}
		
		//tl = (Vector2){0,0};//{gbe->header.absTopLeft.x + lineNumWidth, gbe->header.absTopLeft.y}; // TODO IMGUI
		float cursorOff = hsoff + getColOffset(CURSOR_LINE(r)->buf, CURSOR_COL(r), bs->tabWidth) * bs->charWidth;
		float cursory = (CURSOR_LINE(r)->lineNum - 1 - gbe->scrollLines) * bs->lineHeight;
		
		
		
		gm->curZ = z + 10;
		GUI_Rect(V(tl.x + cursorOff, tl.y + cursory), V(2, bs->lineHeight), &ts->cursorColor);
	}

	gm->curZ = z;
	
	
	// autocomplete box
	if(gbe->showAutocomplete && gbe->autocompleteOptions && gbe->autocompleteOptions->len > 0) {
		
		BufferACMatchSet* ms = gbe->autocompleteOptions;
		
		if(ms->maxLineWidth == 0) ms->maxLineWidth = bs->charWidth * ms->longestMatchLen;
		
		int aclines = ms->len;
		Vector2 acsz = V(ms->maxLineWidth + 2 * 3, aclines * bs->lineHeight + 2 * 3);
		
		float cursorOff = hsoff + getColOffset(CURSOR_LINE(&ms->r)->buf, CURSOR_COL(&ms->r), bs->tabWidth) * bs->charWidth;
		float cursory = (CURSOR_LINE(&ms->r)->lineNum - gbe->scrollLines) * bs->lineHeight;
		
		Vector2 actl = V(tl.x + cursorOff, tl.y + cursory);
		
		gm->curZ += 20;
		GUI_BoxFilled(actl, acsz, 1, &C4H(880000ff), &C4H(181818ff));
		
		
		gm->curZ += 2;
		
		for(int i = 0; i < ms->len; i++) {
			/*
			GUI_TextLine(
				ms->matches[i].s, 
				ms->matches[i].len, 
				V(actl.x + 3, actl.y + 3 + i * bs->lineHeight),
				f->name,
				bs->fontSize,
				&ts->textColor
			);
			*/
			
			struct Color4* color = &ts->textColor;
			
			if(i == gbe->autocompleteSelectedItem) color = &ts->hl_textColor;
			
			drawTextLine(gm, bs, f, color,
				ms->matches[i].s, 
				ms->matches[i].len,
				V(actl.x + 3, actl.y - 1 + (i + 1) * bs->lineHeight)
			);
			
		}
		
		gm->curZ -= 22;
//		size_t popupLines = MIN(VEC_len(&gbe->autocompleteOptions), gbe->maxAutocompleteLines);
		
//		float cursory = (gbe->sel->startLine->lineNum - 1 - gbe->scrollLines) * bs->lineHeight;
		//	.pos = {tl.x + cursorOff + 2, tl.y + cursory + bs->lineHeight},
		
		
	}
	
	
	// HACK
// 	gt->header.hitbox.max = (Vector2){adv, gt->header.size.y};
}





// assumes no linebreaks
void drawTextLine(GUIManager* gm, BufferSettings* bs, GUIFont* f, struct Color4* textColor, char* txt, int charCount, Vector2 tl) {
// 		printf("'%s'\n", bl->buf);
	if(txt == NULL || charCount == 0) return;
	
	int charsDrawn = 0;
	float size = bs->fontSize; 
	float hoff = 0;//size * f->ascender;
	float adv = 0;
	
	
	int isBitmap = 1;
	
	float spaceadv = f->regular[' '].advance;
	
	for(int n = 0; txt[n] != 0 && charsDrawn < charCount; n++) {
		char c = txt[n];
		
		struct charInfo* ci = &f->regular[c];
		
		if(c == '\t') {
			adv += bs->charWidth * bs->tabWidth;
			charsDrawn += bs->charWidth * bs->tabWidth;
		}
		else if(c != ' ') {

//			GUI_Char_NoGuard(c, V(tl.x + adv, tl.y + hoff), f, size, textColor);
			
			GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
			
			Vector2 off = V(tl.x + adv, tl.y + hoff);
			
			float offx = ci->texNormOffset.x;
			float offy = ci->texNormOffset.y;
			float widx = ci->texNormSize.x;
			float widy = ci->texNormSize.y;
			
			v->pos.t = off.y + hoff + ci->topLeftOffset.y * size;
			v->pos.l = off.x + ci->topLeftOffset.x * size;
			v->pos.b = off.y + hoff + ci->bottomRightOffset.y * size;
			v->pos.r = off.x + ci->bottomRightOffset.x * size;
			
			if(isBitmap) { // align the quads to pixel boundaries for bitmap fonts, else there is terrible blurring
				float t = round(v->pos.t);
				float l = round(v->pos.l);
				v->pos.b -= v->pos.t - t;
				v->pos.r -= v->pos.l - l;
				v->pos.t = t;
				v->pos.l = l;
			}
			
			v->guiType = isBitmap ? 100 : 1; // text
			
			v->texOffset1.x = offx * 65535.0;
			v->texOffset1.y = offy * 65535.0;
			v->texSize1.x = widx * 65535.0;
			v->texSize1.y = widy * 65535.0;
			v->texIndex1 = ci->texIndex;
			
			v->clip = GUI_AABB2_TO_SHADER(gm->curClip);
			v->fg = GUI_COLOR4_TO_SHADER(*textColor);
			v->bg = GUI_COLOR4_TO_SHADER(*textColor);
			v->z = gm->curZ;
			
//			adv += ci->advance * size;
			
			
			adv += bs->charWidth; // ci->advance * size; // BUG: needs sdfDataSize added in?
			
			charsDrawn++;
		}
		else {
			adv += bs->charWidth;
			charsDrawn++;
		}
		
		
	}
	
}


#include "ui/macros_off.h"







