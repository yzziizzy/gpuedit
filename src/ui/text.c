#include <ctype.h>

#include "gui.h"
#include "gui_internal.h"

#include "macros_on.h"





// draws a single character from its font origin point
void GUI_Char_(GUIManager* gm, int c, Vector2 origin, GUIFont* font, float size, Color4* color) {
	if(!gm->drawMode) return; // this function is only for drawing mode
	
	GUI_Char_NoGuard_(gm, c, origin, font, size, color);
}

// draws a single character from its font origin point
void GUI_Char_NoGuard_(GUIManager* gm, int c, vec2 origin, GUIFont* font, float size, Color4* color) {
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	struct charInfo* ci = &font->regular[c];

	v->pos.t = origin.y + ci->topLeftOffset.y * size;
	v->pos.l = origin.x + ci->topLeftOffset.x * size;
	v->pos.b = origin.y + ci->bottomRightOffset.y * size;
	v->pos.r = origin.x + ci->bottomRightOffset.x * size;
	
	v->guiType = 1; // text
			
	v->texOffset1.x = ci->texNormOffset.x * 65535.0;
	v->texOffset1.y = ci->texNormOffset.y * 65535.0;
	v->texSize1.x =  ci->texNormSize.x *  65535.0;
	v->texSize1.y =  ci->texNormSize.y * 65535.0;
	v->texIndex1 = ci->texIndex;
			
	v->clip = GUI_AABB2_TO_SHADER(gm->curClip);
	v->bg = (struct ShaderColor4){0};
	v->fg = GUI_COLOR4_TO_SHADER(*color);
	v->z = gm->curZ;
	v->rot = 0;
}

// draws a single pixel-aligned bitmap character from its font origin point
void GUI_BitmapChar_NoGuard_(GUIManager* gm, int c, vec2 origin, GUIFont* font, float size, Color4* color) {
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	struct charInfo* ci = &font->regular[c];

	v->pos.t = origin.y + ci->topLeftOffset.y * size;
	v->pos.l = origin.x + ci->topLeftOffset.x * size;
	v->pos.b = origin.y + ci->bottomRightOffset.y * size;
	v->pos.r = origin.x + ci->bottomRightOffset.x * size;
	
	float t = round(v->pos.t);
	float l = round(v->pos.l);
	v->pos.b -= v->pos.t - t;
	v->pos.r -= v->pos.l - l;
	v->pos.t = t;
	v->pos.l = l;
		
	v->guiType = 100; // bitmap text
			
	v->texOffset1.x = ci->texNormOffset.x * 65535.0;
	v->texOffset1.y = ci->texNormOffset.y * 65535.0;
	v->texSize1.x =  ci->texNormSize.x *  65535.0;
	v->texSize1.y =  ci->texNormSize.y * 65535.0;
	v->texIndex1 = ci->texIndex;
			
	v->clip = GUI_AABB2_TO_SHADER(gm->curClip);
	v->bg = (struct ShaderColor4){0};
	v->fg = GUI_COLOR4_TO_SHADER(*color);
	v->z = gm->curZ;
	v->rot = 0;
}


/* older gui_draw version
void GUI_Char_(
	GUIManager* gm,
	Vector2 tl,
	int c,
	float fontSize,
	GUIFont* font,
	struct Color4* color
) {
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	struct charInfo* ci = &font->regular[(int)c];
	float hoff = fontsize * font->ascender;
			
	Vector2 off = tl;
	
	float offx = ci->texNormOffset.x;//TextRes_charTexOffset(gm->font, 'A');
	float offy = ci->texNormOffset.y;//TextRes_charTexOffset(gm->font, 'A');
	float widx = ci->texNormSize.x;//TextRes_charWidth(gm->font, 'A');
	float widy = ci->texNormSize.y;//TextRes_charWidth(gm->font, 'A');
	
	v->pos.t = off.y + hoff - ci->topLeftOffset.y * fontsize;
	v->pos.l = off.x + ci->topLeftOffset.x * fontsize;
	v->pos.b = off.y + hoff - ci->bottomRightOffset.y * fontsize;
	v->pos.r = off.x + ci->bottomRightOffset.x * fontsize;
	
	v->guiType = 1; // text
	
	v->texOffset1.x = offx * 65535.0;
	v->texOffset1.y = offy * 65535.0;
	v->texSize1.x = widx *  65535.0;
	v->texSize1.y = widy * 65535.0;
	v->texIndex1 = ci->texIndex;
	
	v->clip = GUI_AABB2_TO_SHADER(*gm->curClip);
	v->fg = GUI_COLOR4_TO_SHADER(*color);
	v->bg = (struct ShaderColor4){0};
	v->z = gm->curZ;
	v->rot = 0;
}
*/


float GUI_GetTextWidthAdv_(
	GUIManager* gm,
	char* txt, 
	ssize_t charCount,
	GUIFont* font,
	float fontsize
) {
	if(txt == NULL || charCount == 0) return 0;
	
	float adv = 0;
	
	float spaceadv = font->regular[' '].advance * fontsize;
	
	for(size_t n = 0; txt[n] != 0 && n < charCount; n++) {
		char c = txt[n];
		
		struct charInfo* ci = &font->regular[(int)c];
		
		if(c == '\t') {
			adv += spaceadv * 4; // hardcoded to annoy you
		}
		else if(c != ' ') {
			adv += ci->advance * fontsize;
		}
		else {
			adv += spaceadv;
		}
	}
	
	return adv;	
}

// stops on linebreak
float GUI_TextLineAdv_(
	GUIManager* gm,
	vec2 tl,   
	vec2 sz, // used for alignment options   
	char* txt, 
	ssize_t charCount,
	int align,
	GUIFont* font,
	float fontSize,
	struct Color4* color
) {
	
// 		printf("'%s'\n", bl->buf);
	if(txt == NULL || charCount == 0) return 0;
	if(charCount < 0) charCount = strlen(txt);
	
	assert(font != NULL);
	assert(fontSize > 0);
	assert(color != NULL);
	
	int isBitmap = 0;
	if(fontSize >= 4 && fontSize + .4999f <= 36) {
		int b = floor(fontSize + .4999f) - 4;
		if(font->bitmapFonts[b]) {
			font = font->bitmapFonts[b];
			isBitmap = 1;
			fontSize = floor(fontSize + .4999f);
		}
	}
	
	float hoff = fontSize * font->ascender;
	float adv = 0;
	
	float alignoff = 0;
	if((align & 0x3) == GUI_TEXT_ALIGN_RIGHT) {
		assert(sz.x > 0);
		
		float txtw = GUI_GetTextWidthAdv_(gm, txt, charCount, font, fontSize);
		alignoff = -(txtw - sz.x);
	}
	else if((align & 0x3) == GUI_TEXT_ALIGN_CENTER) {
//		assert(sz.x > 0);
	
		float txtw = GUI_GetTextWidthAdv_(gm, txt, charCount, font, fontSize);
		alignoff = ((sz.x - txtw) / 2.0);
	}
	
	
	if((align & (0x3 << 2)) == GUI_TEXT_ALIGN_VCENTER) {
		hoff += (sz.y/2.f) - (font->ascender * fontSize) / 1.5f;
	}
	
	
	float maxAdv = sz.x;
	if(maxAdv == 0) maxAdv = 99999999999999; // HACK: the problem is (Vector2){0,0} here
	
	
	float spaceadv = font->regular[' '].advance * fontSize;
	
	
	for(size_t n = 0; txt[n] != 0 && adv < maxAdv && n < charCount; n++) {
		char c = txt[n];
		
		struct charInfo* ci = &font->regular[(int)c];
		
		if(c == '\t') {
			adv += spaceadv * 4; // hardcoded to annoy you
			if(isBitmap) adv = ceil(adv); // align to pixel boundaries
		}
		else if(c != ' ') {
			GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
			
			Vector2 off = tl;
			
			float offx = ci->texNormOffset.x;
			float offy = ci->texNormOffset.y;
			float widx = ci->texNormSize.x;
			float widy = ci->texNormSize.y;
			
			v->pos.t = off.y + hoff + ci->topLeftOffset.y * fontSize;
			v->pos.l = off.x + alignoff + adv + ci->topLeftOffset.x * fontSize;
			v->pos.b = off.y + hoff + ci->bottomRightOffset.y * fontSize;
			v->pos.r = off.x + alignoff + adv + ci->bottomRightOffset.x * fontSize;
			
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
			v->fg = GUI_COLOR4_TO_SHADER(*color);
			v->bg = GUI_COLOR4_TO_SHADER(*color);
			v->z = gm->curZ;
			
			adv += ci->advance * fontSize;
			if(isBitmap) adv = ceil(adv); // align to pixel boundaries
		}
		else {
			adv += spaceadv;
			if(isBitmap) adv = ceil(adv); // align to pixel boundaries
		}
		
	}
	
	return adv;
}



// no wrapping
float GUI_TextLine_(GUIManager* gm, vec2 tl, char* txt, ssize_t charCount) {
	return GUI_TextLineAdv_(gm, tl, V2(0,0), txt, charCount, GUI_TEXT_ALIGN_LEFT, gm->curFont, gm->curFontSize, &gm->curFontColor);
}


// no wrapping
float GUI_Printf_(
	GUIManager* gm,  
	Vector2 tl, 
	char* fmt,
	...
) {
	va_list ap;
	
	// BUG: it also returns the width, which in theory could be used in event handlers
	
	va_start(ap, fmt);
	int sz = vsnprintf(NULL, 0, fmt, ap) + 1;
	va_end(ap);
	
	char* tmp = malloc(sz);
	va_start(ap, fmt);
	vsnprintf(tmp, sz, fmt, ap);
	va_end(ap);

	float w;
	if(gm->drawMode) {
		w = GUI_TextLine_(gm, tl, tmp, sz);
	}
	else {
		w = GUI_GetTextWidth(tmp, sz);
	}
	
	free(tmp);
	
	return w;
}

// V and H centering
// no wrapping
float GUI_TextLineCentered_(
	GUIManager* gm, 
	vec2 tl, 
	vec2 sz,
	char* text, 
	ssize_t textLen 
) {
	return GUI_TextLineAdv(tl, sz, text, textLen, GUI_TEXT_ALIGN_CENTER | GUI_TEXT_ALIGN_VCENTER, gm->curFont, gm->curFontSize, &gm->curFontColor);
}

// V and H centering
// no wrapping
float GUI_TextLineCenteredAdv_(
	GUIManager* gm, 
	vec2 tl, 
	vec2 sz,
	char* text, 
	ssize_t textLen,
	GUIFont* font,
	float fontSize,
	struct Color4* color
) {
	return GUI_TextLineAdv(tl, sz, text, textLen, GUI_TEXT_ALIGN_CENTER | GUI_TEXT_ALIGN_VCENTER, font, fontSize, color);
}



void GUI_Double_(GUIManager* gm, vec2 tl, double d, int precision) {
	char buf[64];
	int n = snprintf(buf, 64, "%.*f", precision, d);
	GUI_TextLine(tl, buf, n);
}

void GUI_Integer_(GUIManager* gm, vec2 tl, int64_t i) {
	char buf[64];
	int n = snprintf(buf, 64, "%ld", i);
	GUI_TextLine(tl, buf, n);
}



