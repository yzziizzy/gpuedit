

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#include "gui.h"
#include "gui_internal.h"







void GUI_SetHot_(GUIManager* gm, void* id, void* data, void (*freeFn)(void*)) {
	if(gm->hotID == id) return;
	
	if(gm->hotID && gm->hotFree) {
		gm->hotFree(gm->hotData);
	}
	
	gm->hotID = id;
	gm->hotData = data;
	gm->hotFree = freeFn;
}

void GUI_SetActive_(GUIManager* gm, void* id, void* data, void (*freeFn)(void*)) {
	if(gm->activeID == id) return;
	
	if(gm->activeID && gm->activeFree) {
		gm->activeFree(gm->activeData);
	}
	
	gm->activeID = id;
	gm->activeData = data;
	gm->activeFree = freeFn;
}


void* GUI_GetData_(GUIManager* gm, void* id) {
	
	VEC_EACHP(&gm->elementData, i, d) {
		if(id == d->id) {
			d->age = 0;
			return d->data;
		}
	}	

	return NULL;
}


void GUI_SetData_(GUIManager* gm, void* id, void* data, void (*freeFn)(void*)) {
	
	void* dd = GUI_GetData_(gm, id);
	if(dd) {
		printf("douplicate data setting\n");
	}
	
	VEC_INC(&gm->elementData);
	GUIElementData* d = &VEC_TAIL(&gm->elementData);
	d->id = id;
	d->data = data;
	d->freeFn = freeFn;
	d->age = 0;
}


void GUI_Triangle_(
	GUIManager* gm, 
	Vector2 centroid, 
	float baseWidth, 
	float height, 
	float rotation,
	Color4* bgColor
) {
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	
	*v++ = (GUIUnifiedVertex){
		.pos = {centroid.x, centroid.y, baseWidth, height},
		.clip = GUI_AABB2_TO_SHADER(gm->curClip),
		
		.guiType = 6, // triangle
		
		.texIndex1 = 0, // borderWidth,
		
		.fg = GUI_COLOR4_TO_SHADER(*bgColor), 
		.bg = GUI_COLOR4_TO_SHADER(*bgColor), 
		.z = gm->curZ,
		.alpha = 1,
		.rot = rotation,
	};
}
/*

void gui_drawTriangleBorder(
	GUIManager* gm, 
	Vector2 centroid, 
	float baseWidth, 
	float height,
	float rotation,
	AABB2* clip, 
	float z, 
	Color4* bgColor,
	float borderWidth,
	Color4* borderColor
) {
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	
	*v++ = (GUIUnifiedVertex){
		.pos = {centroid.x, centroid.y, baseWidth, height},
		.clip = GUI_AABB2_TO_SHADER(*clip),
		
		.guiType = 6, // triangle
		
		.texIndex1 = borderWidth,
		
		.fg = GUI_COLOR4_TO_SHADER(*borderColor), 
		.bg = GUI_COLOR4_TO_SHADER(*bgColor), 
		.z = z,
		.alpha = 1,
		.rot = rotation,
	};
}
*/



void GUI_Image_(GUIManager* gm, Vector2 tl, Vector2 sz, char* name) {
	
	TextureAtlasItem* it;
	if(HT_get(&gm->ta->items, name, &it)) {
		fprintf(stderr, "Could not find gui image '%s' \n", name);
	}
	else {
		// icon
		GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
		*v = (GUIUnifiedVertex){
			.pos = {tl.x, tl.y, tl.x + sz.x, tl.y + sz.y},
			.clip = GUI_AABB2_TO_SHADER(gm->curClip),
			
			.texIndex1 = it->index,
			.texIndex2 = 0,
			.texFade = 0,
			
			.guiType = 2, // simple image
			
			.texOffset1 = { it->offsetNorm.x * 65535, it->offsetNorm.y * 65535 },
			.texOffset2 = 0,
			.texSize1 = { it->sizeNorm.x * 65535, it->sizeNorm.y * 65535 },
			.texSize2 = 0,
			
			.fg = {255,255,255,255},
			.bg = {255,255,255,255},
			
			.z = gm->curZ,
			.alpha = 1,
		};
	}
}


float gui_getTextLineWidth(
	GUIManager* gm,
	GUIFont* font,
	float fontsize,
	char* txt, 
	size_t charCount
) {
	if(txt == NULL || charCount == 0) return 0;
	
	if(!font) font = gm->defaults.font; // HACK
	if(!fontsize) fontsize = gm->defaults.fontSize; // HACK
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


int gui_charFromPixel (
	GUIManager* gm,
	GUIFont* font,
	float fontsize,
	char* txt, 
	float pixelOffset
) {
	if(txt == NULL || pixelOffset <= 0) return 0;
	
	if(!font) font = gm->defaults.font; // HACK
	if(!fontsize) fontsize = gm->defaults.fontSize; // HACK
	float adv = 0;
	
	float spaceadv = font->regular[' '].advance * fontsize;
	
	size_t n;
	for(n = 0; txt[n] != 0; n++) {
		char c = txt[n];
		
		struct charInfo* ci = &font->regular[(int)c];
		
		float a;
		if(c == '\t') {
			a = spaceadv * 4; // hardcoded to annoy you
		}
		else if(c == ' ') {
			a = spaceadv;
		}
		else {
			a = ci->advance * fontsize;
		}
		
		if(adv + a >= pixelOffset) {
			return (adv + (a / 2.0)) < pixelOffset ? n + 1 : n; 
		}
		
		adv += a;
	}
	
	return n;
}




float GUI_CharFromPixelF_(
	GUIManager* gm,
	GUIFont* font,
	float fontsize,
	char* txt,
	ssize_t maxChars,
	float pixelOffset
) {
	if(maxChars == 0 || txt == NULL || pixelOffset <= 0) return 0;
	
	if(!font) font = gm->defaults.font; // HACK
	if(!fontsize) fontsize = gm->defaults.fontSize; // HACK
	if(maxChars < 0) maxChars = strlen(txt);
	
//	int isBitmap = 0;
//	if(fontsize >= 4 && fontsize + .4999f <= 36) {
//		int b = floor(fontsize + .4999f) - 4;
//		if(font->bitmapFonts[b]) {
//			font = font->bitmapFonts[b];
//			isBitmap = 1;
//			fontsize = floor(fontsize + .4999f);
//		}
//	}
	
	float adv = 0;
	
	float spaceadv = font->regular[' '].advance * fontsize;
	
	size_t n;
	for(n = 0; txt[n] != 0 && n < maxChars; n++) {
		char c = txt[n];
		f32 oldAdv = adv;
		
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
		
//		if(isBitmap) adv = ceil(adv); // align to pixel boundaries
		
		if(adv >= pixelOffset) {
			f32 delta = adv - oldAdv;
			return (f32)n + ((pixelOffset - oldAdv) / delta);
		}
	}
	
	return n;
}


void gui_drawCharacter(
	GUIManager* gm,
	Vector2 tl,
	Vector2 sz,
	AABB2* clip,
	float z,
	int c,
	struct Color4* color,
	GUIFont* font,
	float fontsize
) {

	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	struct charInfo* ci = &font->regular[(int)c];
	float hoff = fontsize * font->ascender;
			
	Vector2 off = tl;
	
	float offx = ci->texNormOffset.x;//TextRes_charTexOffset(gm->font, 'A');
	float offy = ci->texNormOffset.y;//TextRes_charTexOffset(gm->font, 'A');
	float widx = ci->texNormSize.x;//TextRes_charWidth(gm->font, 'A');
	float widy = ci->texNormSize.y;//TextRes_charWidth(gm->font, 'A');
	
	v->pos.t = off.y + hoff + ci->topLeftOffset.y * fontsize;
	v->pos.l = off.x + ci->topLeftOffset.x * fontsize;
	v->pos.b = off.y + hoff + ci->topLeftOffset.y * fontsize;
	v->pos.r = off.x + ci->topLeftOffset.x * fontsize;
	
	v->guiType = font->bitmapSize > 0 ? 20 : 1; // text
	
	v->texOffset1.x = offx * 65535.0;
	v->texOffset1.y = offy * 65535.0;
	v->texSize1.x = widx *  65535.0;
	v->texSize1.y = widy * 65535.0;
	v->texIndex1 = ci->texIndex;
	
	v->clip = GUI_AABB2_TO_SHADER(*clip);
	v->fg = GUI_COLOR4_TO_SHADER(*color);
	v->bg = (struct ShaderColor4){0};
	v->z = z;
	v->rot = 0;
}

void gui_drawTextLine(
	GUIManager* gm,
	Vector2 tl,
	Vector2 sz,
	AABB2* clip,
	struct Color4* color, 
	float z,
	char* txt, 
	size_t charCount
) {
	gui_drawTextLineAdv(gm, tl, sz, clip, color, NULL, 0, GUI_TEXT_ALIGN_LEFT, z, txt, charCount);
}


// stops on linebreak
void gui_drawTextLineAdv(
	GUIManager* gm,
	Vector2 tl,
	Vector2 sz,
	AABB2* clip,
	struct Color4* color,
	GUIFont* font,
	float fontsize,
	int align,
	float z,
	char* txt, 
	size_t charCount
) {
	
// 		printf("'%s'\n", bl->buf);
	if(txt == NULL || charCount == 0) return;
	if(charCount == -1) charCount = strlen(txt);
	
	if(!font) font = gm->defaults.font;
	if(!fontsize) fontsize = gm->defaults.fontSize; // HACK
	float hoff = fontsize * font->ascender;
	float adv = 0;
	if(!color) color = &gm->defaults.textColor;
	
	float alignoff = 0;
	if(align == GUI_TEXT_ALIGN_RIGHT) {
		float txtw = gui_getTextLineWidth(gm, font, fontsize, txt, charCount);
		alignoff = -(txtw - sz.x);
	}
	else if(align == GUI_TEXT_ALIGN_CENTER) {
	
		float txtw = gui_getTextLineWidth(gm, font, fontsize, txt, charCount);
		alignoff = ((sz.x - txtw) / 2.0);
	}
	
	// BUG: the problem is (Vector2){0,0} here
	float maxAdv = sz.x;
	
	
	float spaceadv = font->regular[' '].advance * fontsize;
	
	for(size_t n = 0; txt[n] != 0 && adv < maxAdv && n < charCount; n++) {
		char c = txt[n];
		
		struct charInfo* ci = &font->regular[(int)c];
		
		if(c == '\t') {
			adv += spaceadv * 4; // hardcoded to annoy you
		}
		else if(c != ' ') {
			GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
			
			Vector2 off = tl;
			
			float offx = ci->texNormOffset.x;//TextRes_charTexOffset(gm->font, 'A');
			float offy = ci->texNormOffset.y;//TextRes_charTexOffset(gm->font, 'A');
			float widx = ci->texNormSize.x;//TextRes_charWidth(gm->font, 'A');
			float widy = ci->texNormSize.y;//TextRes_charWidth(gm->font, 'A');
			
			v->pos.t = off.y + hoff + ci->topLeftOffset.y * fontsize;
			v->pos.l = off.x + alignoff + adv + ci->topLeftOffset.x * fontsize;
			v->pos.b = off.y + hoff + ci->bottomRightOffset.y * fontsize;
			v->pos.r = off.x + alignoff + adv + ci->bottomRightOffset.x * fontsize;
			
		//	printf("huh... %f,%f, %f\n", ci->quadSize.x, ci->quadSize.y, fontsize);
			//printf("huh... %f\n", off.x + alignoff + adv + ci->topLeftOffset.x * fontsize);
			//printf("adv:%f  tlo: [%f,%f]  off: [%f,%f, %f,%f]  pos: [%f,%f,%f,%f]\n", 
			//	adv, ci->topLeftOffset.x, ci->topLeftOffset.x, off.x, off.y, hoff, alignoff, v->pos.t,v->pos.l,v->pos.b,v->pos.r);
			
			v->guiType = font->bitmapSize > 0 ? 20 : 1; // text
			
			v->texOffset1.x = offx * 65535.0;
			v->texOffset1.y = offy * 65535.0;
			v->texSize1.x = widx *  65535.0;
			v->texSize1.y = widy * 65535.0;
			v->texIndex1 = ci->texIndex;
			
			v->clip.t = clip->min.y;
			v->clip.l = clip->min.x;
			v->clip.b = clip->max.y;
			v->clip.r = clip->max.x;
			v->fg = GUI_COLOR4_TO_SHADER(*color);
			v->bg = (struct ShaderColor4){0};
			v->z = z;
			
			adv += ci->advance * fontsize; // BUG: needs sdfDataSize added in?
			//v++;
// 			gm->elementCount++;
		}
		else {
			adv += spaceadv;
		}
		
		
	}

}


float gui_getDefaultUITextWidth(
	GUIManager* gm,
	char* txt, 
	size_t maxChars
) {
	
	if(txt == NULL || maxChars == 0) return 0;
	
	GUIFont* f = gm->defaults.font;
	float size = gm->defaults.fontSize; // HACK
	float adv = 0;
	
	
	float spaceadv = f->regular[' '].advance * size;
	
	for(size_t n = 0; txt[n] != 0 && n < maxChars; n++) {
		char c = txt[n];
		
		if(c == '\t') {
			adv += spaceadv * 4; // hardcoded to annoy you
		}
		else if(c != ' ') {
			struct charInfo* ci = &f->regular[(int)c];
			adv += ci->advance * size; // BUG: needs sdfDataSize added in?
		}
		else {
			adv += spaceadv;
		}
	}
	
	return adv;
}




void gui_drawVCenteredTextLine(
	GUIManager* gm,
	Vector2 tl,  
	Vector2 sz,  
	AABB2* clip,  
	struct Color4* color,
	float z,
	char* txt, 
	size_t charCount
) {
	
	GUIFont* f = gm->defaults.font;
	float size = gm->defaults.fontSize; // HACK
	float hoff = size * f->height;
	
	float a = sz.y - hoff;
	float b = fmax(a / 2.0, 0);
	
	gui_drawTextLine(gm, (Vector2){tl.x, tl.y + b}, sz, clip, color, z, txt, charCount);
}






GUIFont* GUI_FindFont(GUIManager* gm, char* name, float size) {
	// the master font for this face
	GUIFont* mf = FontManager_findFont(gm->fm, name);
	
	// check to see if a bitmap font is available
	int isz = floor(size + 0.5); 
	if(isz < 0 || isz > 63) return mf;
	
//	printf("checking size %f, %d\n", size, isz);
	
	uint64_t mask = 1ul << (isz - 4);
	// check the bitfield
	if(!(mf->bitmapSizes & mask)) {
//		printf("  ---missing from bitfield: %s/%d/%f\n", name, isz, size);
		return mf;
	}
	
//	printf("found bmp font %s/%d\n", name, isz);
	
	// find and return the correct bitmap font object
	VEC_EACH(&mf->bitmapFonts, i, bf) {
		if(bf->bitmapSize == isz) return bf;
	}
	
	// just in case the bitmap font wasn't found for some reason.
	return mf;
}





void gui_debugPrintVertex(GUIUnifiedVertex* v, FILE* of) {
	if(v->guiType == 0) {
		fprintf(of, "Box:  % 3.1f,% 3.1f / % 2.1f,% 2.1f | clip [%.2f,%.2f,%.2f,%.2f] | z %.4f \n", 
			   v->pos.l, v->pos.t, v->pos.r, v->pos.b, 
			   v->clip.l, v->clip.t, v->clip.r, v->clip.b, 
			   v->z
		);
	}
	else if(v->guiType == 1) {
		fprintf(of, "Char: % 3.1f,% 3.1f / % 2.1f,% 2.1f | clip [%.2f,%.2f,%.2f,%.2f] | z %.4f \n", 
			   v->pos.l, v->pos.t, v->pos.r, v->pos.b, 
			   v->clip.l, v->clip.t, v->clip.r, v->clip.b, 
			   v->z
		);
	}
	else if(v->guiType == 2) {
		fprintf(of, "Img:  % 3.1f,% 3.1f / % 2.1f,% 2.1f | clip [%.2f,%.2f,%.2f,%.2f] | z %.4f \n", 
			   v->pos.l, v->pos.t, v->pos.r, v->pos.b, 
			   v->clip.l, v->clip.t, v->clip.r, v->clip.b, 
			   v->z
		);
	}
	else if(v->guiType == 3) {
		fprintf(of, "Render Target: % 3.1f,% 3.1f / % 2.1f,% 2.1f | clip [%.2f,%.2f,%.2f,%.2f] | z %.4f \n", 
			   v->pos.l, v->pos.t, v->pos.r, v->pos.b, 
			   v->clip.l, v->clip.t, v->clip.r, v->clip.b, 
			   v->z
		);
	}
	else if(v->guiType == 4) {
		fprintf(of, "BBox: % 3.1f,% 3.1f / % 2.1f,% 2.1f | clip [%.2f,%.2f,%.2f,%.2f] | z %.4f \n", 
			   v->pos.l, v->pos.t, v->pos.r, v->pos.b, 
			   v->clip.l, v->clip.t, v->clip.r, v->clip.b, 
			   v->z
		);
	}
	else if(v->guiType == 6) {
		fprintf(of, "Tri:  % 3.1f,% 3.1f / % 2.1f,% 2.1f | clip [%.2f,%.2f,%.2f,%.2f] | z %.4f \n", 
			   v->pos.l, v->pos.t, v->pos.r, v->pos.b, 
			   v->clip.l, v->clip.t, v->clip.r, v->clip.b, 
			   v->z
		);
	}
	else if(v->guiType == 7) {
		fprintf(of, "Ellipse:  %.2f,%.2f / %.2f,%.2f | clip [%.2f,%.2f,%.2f,%.2f] | z %.4f \n", 
			   v->pos.l, v->pos.t, v->pos.r, v->pos.b, 
			   v->clip.l, v->clip.t, v->clip.r, v->clip.b, 
			   v->z
		);
	}
}

void gui_debugDumpVertexBuffer(GUIUnifiedVertex* buf, size_t cnt, FILE* of) {
	for(size_t i = 0; i < cnt; i++) {
		fprintf(of, "% 5ld ", i);
		gui_debugPrintVertex(buf + i, of);
	}
}

void gui_debugFileDumpVertexBuffer(GUIManager* gm, char* filePrefix, int fileNum) {
	FILE* f;
	char fname[512];
	
	snprintf(fname, 512, "%s-%05d.txt", filePrefix, fileNum);
	
	
	f = fopen(fname, "wb");
	gui_debugDumpVertexBuffer(gm->vertBuffer, gm->vertCount, f);
	fclose(f);
}


