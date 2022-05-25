#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"
#include "math.h"

#include "buffer.h"
#include "ui/gui.h"
#include "ui/gui_internal.h"
#include "clipboard.h"



static void drawTextLine(GUIManager* gm, TextDrawParams* tdp, struct Color4* textColor, char* txt, int charCount, Vector2 tl);



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
	
//	BufferUndo_DebugRender(gm, b, V(500, 20), V(200, 800), pfp);
	
	
	float z = gm->curZ;
	
	ThemeSettings* ts = gbe->ts;
	BufferSettings* bs = gbe->bs;
	BufferDrawParams* bdp = gbe->bdp;
	TextDrawParams* tdp = bdp->tdp;
	float fsize = bs->fontSize; 
	GUIFont* f = gbe->font;
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
	
	float lineNumWidth = ceil(LOGB(bs->lineNumBase, b->numLines + 1)) * tdp->charWidth + bdp->lineNumExtraWidth;
	float hsoff = -gbe->scrollCols * tdp->charWidth;
	
	if(bdp->showLineNums) tl.x += lineNumWidth;
	
	gbe->textAreaOffsetX = tl.x; // save for other functions

	if(bdp->showLineNums) {
		gm->curZ += 1;
		GUI_Rect(V(0,0), V(lineNumWidth, edh), &ts->lineNumBgColor);
		gm->curZ -= 1;
	}
	
	BufferLine* bl = b->first; // BUG broken 
	
	// scroll down
	// TODO: cache a pointer
	for(intptr_t i = 0; i < gbe->scrollLines && bl->next; i++) bl = bl->next; 
	
	int inSelection = 0;
	int maxCols = 10000;
	
	struct Color4* fg = &ts->textColor; 
	struct Color4* bg = &ts->bgColor;
	
	struct Color4 lineColors[] = {
		ts->lineNumColor,
		ts->lineNumBookmarkColor,
		{.95,.05,.05,1.0}, // breakpoint
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
		if(bdp->showLineNums) {
			sprintlongb(lnbuf, bs->lineNumBase, bl->lineNum, bs->lineNumCharset);
			float nw = (floor(LOGB(bs->lineNumBase, bl->lineNum)) + 1) * tdp->charWidth;

			Color4* lnc = &lineColors[0];
			if(bl->flags & BL_BOOKMARK_FLAG) lnc = &lnc[1];
			if(bl->flags & BL_BREAKPOINT_FLAG) lnc = &lnc[2];

			gm->curZ++;
			drawTextLine(
				gm, 
				tdp, 
				lnc, 
				lnbuf, 
				100, 
				(Vector2){tl.x - nw - bdp->lineNumExtraWidth, yoff}
			);
			gm->curZ--;
		}
		
		float adv = 0;
		
		// highlight current line
		if(bl == CURSOR_LINE(gbe->sel) && gbe->outlineCurLine && !gbe->sel->line[0]) {
			gm->curZ = z + 10;
			GUI_Box(
				V(tl.x - 1, tl.y + bs->outlineCurrentLineYOffset), 
				V(sz.x - gbe->textAreaOffsetX, tdp->lineHeight + bs->outlineCurrentLineYOffset + 2), // +1 to not cover underscores
				1,
				&ts->outlineCurrentLineBorderColor
			);
		}
		
		
		if(bl->length) { // only draw lines with text
			
			size_t styleIndex = 0;
			size_t styleCols = 0;
			TextStyleAtom* atom = NULL;
			if(VEC_LEN(&bl->style)) {
				atom = &VEC_HEAD(&bl->style);
				styleCols = atom->length;
			}
			
			// draw characters
			for(int i = 0; i < bl->length; i++) { 
								
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
				if(inSelection && bs->invertSelection) {
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
					
					if(adv >= gbe->scrollCols * tdp->charWidth && adv < (gbe->scrollCols * tdp->charWidth) + sz.x) {
						gm->curZ = z + 4;
						GUI_CharFont_NoGuard('0', V(tl.x + hsoff + adv, yoff), f, fsize, &pulseColor);
						gm->curZ = z + 2;
						GUI_Rect(V(tl.x + adv + hsoff, yoff - ascender), V(MAX(5, (float)tdp->charWidth), tdp->lineHeight), &pulseColorBg);
					}
					
					adv += tdp->charWidth;	
				}
				else if(c == '\t') {
					if(inSelection) { // tabs inside selection
						gm->curZ = z + 2;
						GUI_Rect(V(tl.x + adv + hsoff, yoff - ascender), V(adv + hsoff + (tdp->charWidth * tdp->tabWidth), tdp->lineHeight), bg);
					}
					
					adv += tdp->charWidth * tdp->tabWidth;
				}
				else { 
					// normal, non-tab text		
					if(adv >= gbe->scrollCols * tdp->charWidth && adv < (gbe->scrollCols * tdp->charWidth) + sz.x) {
						gm->curZ = z + 4;
						GUI_CharFont_NoGuard(c, V(tl.x + hsoff + adv, yoff), f, fsize, fg);
						gm->curZ = z + 2;
						GUI_Rect(V(tl.x + adv + hsoff, yoff - ascender), V(MAX(5, (float)tdp->charWidth), tdp->lineHeight), bg);
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
		else { // empty lines
			// check selection
			if(BufferRangeSet_test(gbe->findSet, bl, 0)) {
				// inside other selection
				inSelection = 1;
				fg = &((Color4){1.0, 0.0, 0.3, 1.0});
				bg = &((Color4){0.0, 1.0, 0.3, 1.0});
			}
			else {
				if(BufferRangeSet_test(gbe->selSet, bl, 0)) {
					inSelection = 1;
					fg = &ts->hl_textColor;
					bg = &ts->hl_bgColor;
				}
				else {
					inSelection = 0;
					fg = &ts->textColor;
					bg = &ts->bgColor;	
				}
			}
		
		}

        // draw a little half-width selection background at the end of selected lines (and empty ones)
		if(inSelection) {
			gm->curZ = z + 2;
			GUI_Rect(V(tl.x + adv + hsoff, yoff - ascender), V(MAX(5, (float)tdp->charWidth / 1.0), tdp->lineHeight), &ts->hl_bgColor);
		}

		if(ytop > edh) break; // end of buffer control

		// advance to the next line
		yoff += tdp->lineHeight;
		ytop += tdp->lineHeight;
		bl = bl->next;
		linesRendered++;
		if(linesRendered > lineTo - lineFrom) break;
		
		
	}
	
	
	
	
	// draw cursors
	VEC_EACH(&gbe->selSet->ranges, i, r) {
		if(r == gbe->sel && (gbe->cursorBlinkPaused || gbe->cursorBlinkTimer > gbe->cursorBlinkOnTime)) continue;
		
		//tl = (Vector2){0,0};//{gbe->header.absTopLeft.x + lineNumWidth, gbe->header.absTopLeft.y}; // TODO IMGUI
		float cursorOff = hsoff + getColOffset(CURSOR_LINE(r)->buf, CURSOR_COL(r), tdp->tabWidth) * tdp->charWidth;
		float cursory = (CURSOR_LINE(r)->lineNum - 1 - gbe->scrollLines) * tdp->lineHeight;
		
		gm->curZ = z + 10;
		GUI_Rect(V(tl.x + cursorOff, tl.y + cursory), V(2, tdp->lineHeight), &ts->cursorColor);
	}

	gm->curZ = z;
	
	
	// autocomplete box
	if(gbe->showAutocomplete) {
		
		
//		size_t popupLines = MIN(VEC_LEN(&gbe->autocompleteOptions), gbe->maxAutocompleteLines);
		
//		float cursory = (gbe->sel->startLine->lineNum - 1 - gbe->scrollLines) * tdp->lineHeight;
		//	.pos = {tl.x + cursorOff + 2, tl.y + cursory + tdp->lineHeight},
		
		
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
	float size = tdp->fontSize; 
	float hoff = 0;//size * f->ascender;
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

			GUI_CharFont_NoGuard(c, V(tl.x + adv, tl.y + hoff), f, size, textColor);
			
			adv += tdp->charWidth; // ci->advance * size; // BUG: needs sdfDataSize added in?
			
			charsDrawn++;
		}
		else {
			adv += tdp->charWidth;
			charsDrawn++;
		}
		
		
	}
	
}


#include "ui/macros_off.h"







