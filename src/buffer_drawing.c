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


size_t drawCharacter(
	GUIManager* gm, 
	TextDrawParams* tdp, 
	struct Color4* fgColor, 
	struct Color4* bgColor, 
	int c, 
	Vector2 tl,
	float z,
	AABB2* clip
) {
// 		printf("'%s'\n", bl->buf);
	GUIFont* f = tdp->font;
	float size = tdp->fontSize; 
	float hoff = size * f->ascender;
		
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
			
			.bg = GUI_COLOR4_TO_SHADER(*bgColor),
			
			.z = z,
			
			.clip = GUI_AABB2_TO_SHADER(*clip),
			.alpha = 1,
		};
	}
	
	// character
	if(c != ' ' && c != '\t') { // TODO: proper printable character check
		v = GUIManager_reserveElements(gm, 1);
		
		*v = (GUIUnifiedVertex){
		
			.pos.t = tl.y + ci->topLeftOffset.y * size,
			.pos.l = tl.x + ci->topLeftOffset.x * size,
			.pos.b = tl.y + ci->bottomRightOffset.y * size,
			.pos.r = tl.x + ci->bottomRightOffset.x * size,
			
//			.pos.t = tl.y + hoff - ci->topLeftOffset.y * size,
//			.pos.l = tl.x + ci->topLeftOffset.x * size,
//			.pos.b = tl.y + hoff + ci->size.y * size - ci->topLeftOffset.y * size,
//			.pos.r = tl.x + ci->size.x * size + ci->topLeftOffset.x * size,
			
			.guiType = 1, // text
			
			.texOffset1.x = ci->texNormOffset.x * 65535.0,
			.texOffset1.y = ci->texNormOffset.y * 65535.0,
			.texSize1.x = ci->texNormSize.x *  65535.0,
			.texSize1.y = ci->texNormSize.y * 65535.0,
			.texIndex1 = ci->texIndex,
			
			.fg = GUI_COLOR4_TO_SHADER(*fgColor),
			
			.z = z+0.001,
			
			.clip = GUI_AABB2_TO_SHADER(*clip),
			.alpha = 1,
		};
	}
	
	return 0;
}



#define V(a,b) ((Vector2){.x = (a), .y = (b)})

// draws the editor's text area and line numbers
void GUIBufferEditControl_Draw(GUIBufferEditControl* gbe, GUIManager* gm, Vector2 tl, Vector2 sz,
	 int lineFrom, int lineTo, int colFrom, int colTo, PassFrameParams* pfp) {
	Buffer* b = gbe->buffer;
	
	if(!b) return;
	if(!b->first) return;
	
	BufferDrawParams* bdp = gbe->bdp;
	TextDrawParams* tdp = bdp->tdp;
	ThemeDrawParams* theme = bdp->theme;
	float fsize = tdp->fontSize; 
	GUIFont* f = gbe->font;
	int line = 1;
	int linesRendered = 0;
	char lnbuf[32];
	GUIUnifiedVertex* v;
	
	float edh = sz.y;
	
	// draw general background	
	GUI_Rect(V(0,0), gm->curWin->size, &theme->bgColor);
	
	float lineNumWidth = ceil(LOGB(gbe->gs->Buffer_lineNumBase, b->numLines + 1)) * tdp->charWidth + bdp->lineNumExtraWidth;
	float hsoff = -gbe->scrollCols * tdp->charWidth;
	
	if(bdp->showLineNums) tl.x += lineNumWidth;
	
	gbe->textAreaOffsetX = tl.x; // save for other functions

	if(bdp->showLineNums) {
		gm->curZ += 1;
		GUI_Rect(V(0,0), V(lineNumWidth, edh), &theme->lineNumBgColor);
		gm->curZ -= 1;
	}
	
	BufferLine* bl = b->first; // BUG broken 
	
	// scroll down
	// TODO: cache a pointer
	for(intptr_t i = 0; i < gbe->scrollLines && bl->next; i++) bl = bl->next; 
	
	int inSelection = 0;
	int maxCols = 10000;
	
	struct Color4* fg = &theme->textColor; 
	struct Color4* bg = &theme->bgColor;
	
	struct Color4 lineColors[] = {
		theme->lineNumColor,
		theme->lineNumBookmarkColor,
		{.95,.05,.05,1.0}, // breakpoint
	};

	StyleInfo* styles;
	if(gbe->gs->Theme->is_dark) {
		styles = gbe->h->stylesDark;
	} else {
		styles = gbe->h->stylesLight;
	}
	
	
	tl.y += f->ascender * fsize;
//	printf("%f\n", f->ascender);
	
	int xx = 0;
	// draw lines
	while(bl) {
		
		// line numbers
		if(bdp->showLineNums) {
			sprintlongb(lnbuf, gbe->gs->Buffer_lineNumBase, bl->lineNum, gbe->gs->Buffer_lineNumCharset);
			float nw = (floor(LOGB(gbe->gs->Buffer_lineNumBase, bl->lineNum)) + 1) * tdp->charWidth;

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
				(Vector2){tl.x - nw - bdp->lineNumExtraWidth, tl.y}
			);
			gm->curZ--;
		}
		
		float adv = 0;
		
		// highlight current line
		if(bl == gbe->current && gbe->outlineCurLine && !gbe->sel) {
			GUIUnifiedVertex* vv = GUIManager_reserveElements(gm, 1);
			*vv = (GUIUnifiedVertex){
//					v->pos.t = off.y + hoff + ci->topLeftOffset.y * fontsize;
//					v->pos.l = off.x + alignoff + adv + ci->topLeftOffset.x * fontsize;
//					v->pos.b = off.y + hoff + ci->bottomRightOffset.y * fontsize;
//					v->pos.r = off.x + alignoff + adv + ci->bottomRightOffset.x * fontsize;
			
			// TODO IMGUI
//				.pos = {
////					tl.x - 1, 
//					tl.y + gbe->gs->Buffer_outlineCurrentLineYOffset, 
//					tl.x + gbe->header.size.x - gbe->textAreaOffsetX, 
//					tl.y + tdp->lineHeight + gbe->gs->Buffer_outlineCurrentLineYOffset + 2, // +1 to not cover underscores
//				},
//				.clip = GUI_AABB2_TO_SHADER(gbe->header.absClip),// TODO IMGUI
				.texIndex1 = 1, // order width
				.guiType = 4, // bordered window (just a box)
				.fg = GUI_COLOR4_TO_SHADER(theme->outlineCurrentLineBorderColor), // border color 
				.bg = {0,0,0,0},
//				.z = gbe->header.absZ + 0.1, // TODO IMGUI
				.alpha = 1.0,
			};
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
					fg = &theme->hl_textColor;
					bg = &theme->hl_bgColor;
				}
				else {
					if(BufferRangeSet_test(gbe->findSet, bl, i)) {
						// inside other selection
						inSelection = 1;
						fg = &theme->find_textColor;
						bg = &theme->find_bgColor;
					}
					else {
						inSelection = 0;
						fg = &theme->textColor;
						bg = &theme->bgColor;	
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
				if(inSelection && gbe->gs->Buffer_invertSelection) {
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
					
					// TODO IMGUI
//					if(adv >= gbe->scrollCols * tdp->charWidth && adv < (gbe->scrollCols * tdp->charWidth) + gbe->header.size.x) 
//						drawCharacter(gm, tdp, &pulseColor, &pulseColorBg, '0', 
//							(Vector2){tl.x + hsoff + adv, tl.y}, gbe->header.absZ, &gbe->header.absClip);
					
					adv += tdp->charWidth;	
				}
				else if(c == '\t') {
					if(inSelection) { // tabs inside selection
						v = GUIManager_reserveElements(gm, 1);
						*v = (GUIUnifiedVertex){
							.pos.t = tl.y,
							.pos.l = tl.x + adv + hsoff,
							.pos.b = tl.y + tdp->lineHeight,
							.pos.r = tl.x + adv + hsoff + (tdp->charWidth * tdp->tabWidth),
							
							.guiType = 0, // box
							
							.bg = GUI_COLOR4_TO_SHADER(*bg),
//							.z = gbe->header.absZ, // TODO IMGUI
							
							// disabled in the shader right now
//							.clip = GUI_AABB2_TO_SHADER(gbe->header.absClip), // TODO IMGUI
							.alpha = 1,
						};
					}
					
					adv += tdp->charWidth * tdp->tabWidth;
				}
				else { 
				
					GUI_CharFont_NoGuard(c, V(tl.x + hsoff + adv, tl.y), f, fsize, fg);
					// normal, non-tab text		
//					if(adv >= gbe->scrollCols * tdp->charWidth && adv < (gbe->scrollCols * tdp->charWidth) + gbe->header.size.x) // TODO IMGUI
//						drawCharacter(gm, tdp, fg, bg, c, (Vector2){tl.x + hsoff + adv, tl.y}, gbe->header.absZ, &gbe->header.absClip); // TODO IMGUI
					
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
					fg = &theme->hl_textColor;
					bg = &theme->hl_bgColor;
				}
				else {
					inSelection = 0;
					fg = &theme->textColor;
					bg = &theme->bgColor;	
				}
			}
		
		}

		if(inSelection) {
			v = GUIManager_reserveElements(gm, 1);

			*v = (GUIUnifiedVertex){
				.pos.t = tl.y,
				.pos.l = tl.x + adv + hsoff,
				.pos.b = tl.y + tdp->lineHeight,
				.pos.r = tl.x + adv + hsoff + MAX(5, (float)tdp->charWidth / 1.0),

				.guiType = 0, // box

				.bg = GUI_COLOR4_TO_SHADER(theme->hl_bgColor),
//				.z = gbe->header.absZ, // TODO IMGUI

				// disabled in the shader right now
//				.clip = GUI_AABB2_TO_SHADER(gbe->header.absClip), // TODO IMGUI
				.alpha = 1,
			};
		}

		if(tl.y > edh) break; // end of buffer control

		// advance to the next line
		tl.y += tdp->lineHeight;
		bl = bl->next;
		linesRendered++;
		if(linesRendered > lineTo - lineFrom) break;
		
		
	}
	
	
	
	
	// draw cursor
	if(gbe->cursorBlinkPaused || gbe->cursorBlinkTimer <= gbe->cursorBlinkOnTime) {
		tl = (Vector2){0,0};//{gbe->header.absTopLeft.x + lineNumWidth, gbe->header.absTopLeft.y}; // TODO IMGUI
		v = GUIManager_reserveElements(gm, 1);
		float cursorOff = hsoff + getColOffset(gbe->current->buf, gbe->curCol, tdp->tabWidth) * tdp->charWidth;
		float cursory = (gbe->current->lineNum - 1 - gbe->scrollLines) * tdp->lineHeight;
		*v = (GUIUnifiedVertex){
			.pos = {tl.x + cursorOff, tl.y + cursory, tl.x + cursorOff + 2, tl.y + cursory + tdp->lineHeight},
//			.clip = GUI_AABB2_TO_SHADER(gbe->header.absClip), // TODO IMGUI
			.guiType = 0, // window (just a box)
			.fg = {255, 128, 64, 255}, // TODO: border color
			.bg = GUI_COLOR4_TO_SHADER(theme->cursorColor), 
//			.z = gbe->header.absZ, // TODO IMGUI
			.alpha = 1,
		};
	}
	
	
	// autocomplete box
	if(gbe->showAutocomplete) {
		
		
		size_t popupLines = MIN(VEC_LEN(&gbe->autocompleteOptions), gbe->maxAutocompleteLines);
		
		float cursory = (gbe->current->lineNum - 1 - gbe->scrollLines) * tdp->lineHeight;
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








