

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gui.h"
#include "gui_internal.h"




static void render(GUIText* gt, PassFrameParams* pfp);
static GUIHeader* hitTest(GUIHeader* go, Vector2 testPos);
static void guiTextDelete(GUIText* gt);


static void writeCharacterGeom(GUIUnifiedVertex* v, struct charInfo* ci, Vector2 off, float sz, float adv, float line);


GUIText* GUIText_new(GUIManager* gm, char* str, char* fontname, float fontSize) {
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.Delete = (void*)guiTextDelete,
	//	.HitTest = hitTest,
	};
	
	
	GUIText* gt;
	pcalloc(gt);
	
// 	gt->header.gm = gm;
	gui_headerInit(&gt->header, gm, &static_vt, NULL);
// 	gt->header.vt = &static_vt; 
	gt->font = gm->defaults.font;
	
// 	gt->fontSize = fontSize;
// 	gt->font = FontManager_findFont(gm->fm, fontname);

	if(str) {
		gt->currentStr = strdup(str);
	}

	// TODO: x size, fix y size
	gt->header.size = (Vector2){guiTextGetTextWidth(gt, 999999)+ 5, gt->font->height}; 

	
	return gt;
}


static void updatePos(GUIText* gt, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &gt->header; 

	if(h->flags & GUI_AUTO_SIZE) {
		h->size = (Vector2){
			guiTextGetTextWidth(gt, 999999)+5,
			gt->font->height 
		}; 
	}
	
	gui_defaultUpdatePos(&gt->header, grp, pfp);
}


static void render(GUIText* gt, PassFrameParams* pfp) {
	char* txt = gt->currentStr;
	GUIManager* gm = gt->header.gm;
	GUIFont* f = gt->font;
	
	Vector2 tl = gt->header.absTopLeft;
	
	float size = gm->defaults.fontSize; // HACK
	float hoff = size * f->ascender;//gt->header.size.y * .75; // HACK
	float adv = 0;
	
	float spaceadv = f->regular[' '].advance;
	
// 	gui_drawDefaultUITextLine(gm, &box, &gm->defaults.tabTextColor , 10000000, e->label, strlen(e->label));

	
	// this algorithm needs to be kept in sync with the width calculation algorithm below
	for(int n = 0; txt[n] != 0; n++) {
		char c = txt[n];
		
		struct charInfo* ci = &f->regular[(int)c];
		
		
		if(c != ' ') {
			GUIUnifiedVertex* v = GUIManager_checkElemBuffer(gm, 1);
			writeCharacterGeom(v, ci, gt->header.absTopLeft, size, adv, hoff);
			
			v->clip.t = gt->header.absClip.min.y;
			v->clip.l = gt->header.absClip.min.x;
			v->clip.b = gt->header.absClip.max.y;
			v->clip.r = gt->header.absClip.max.x;
			v->z = gt->header.absZ;
			
			adv += ci->advance * size; // BUG: needs sdfDataSize added in?
			v->fg = GUI_COLOR4_TO_SHADER(gm->defaults.textColor),
			//v++;
			gm->elementCount++;
		}
		else {
			adv += spaceadv;
		}
		
		
		
	}
	
}

static GUIHeader* hitTest(GUIHeader* go, Vector2 testPos) {
	
	printf("text hit test, %f, %f \n", testPos.x, testPos.y);
	
	return go;
}

void guiTextDelete(GUIText* gt) { // TODO: implement reap functions 
// 	if(gt->currentStr) free(gt->currentStr);
// 	gt->currentStr = NULL;
}


// this algorithm needs to be kept in sync with the rendering algorithm above
float guiTextGetTextWidth(GUIText* gt, int numChars) {
	char* txt = gt->currentStr;
	GUIManager* gm = gt->header.gm;
	GUIFont* f = /*gt->font;*/gm->defaults.font;
	
	float size = gm->defaults.fontSize; // HACK
	float adv = 0;
	
	float spaceadv = f->regular[' '].advance;
	
	for(int n = 0; txt[n] != 0 && n < numChars; n++) {
		char c = txt[n];
		struct charInfo* ci = &f->regular[(int)c];
		
		if(c != ' ') {
			adv += ci->advance * size; // BUG: needs sdfDataSize added in?
		}
		else {
			adv += spaceadv;
		}
	}
	
	return adv;
}


void GUIText_setString(GUIText* gt, char* newval) {
	if(0 != strcmp(newval, gt->currentStr)) {
		if(gt->currentStr) free(gt->currentStr);
		gt->currentStr = strdup(newval);
	}
}









static void writeCharacterGeom(GUIUnifiedVertex* v, struct charInfo* ci, Vector2 off, float sz, float adv, float line) {
	float offx = ci->texNormOffset.x;//TextRes_charTexOffset(gm->font, 'A');
	float offy = ci->texNormOffset.y;//TextRes_charTexOffset(gm->font, 'A');
	float widx = ci->texNormSize.x;//TextRes_charWidth(gm->font, 'A');
	float widy = ci->texNormSize.y;//TextRes_charWidth(gm->font, 'A');
	
	v->pos.t = off.y + line - ci->topLeftOffset.y * sz;
	v->pos.l = off.x + adv + ci->topLeftOffset.x * sz;
	v->pos.b = off.y + line + ci->size.y * sz - ci->topLeftOffset.y * sz;
	v->pos.r = off.x + adv + ci->size.x * sz + ci->topLeftOffset.x * sz;
	
	v->guiType = 1; // text
	
	v->texOffset1.x = offx * 65535.0;
	v->texOffset1.y = offy * 65535.0;
	v->texSize1.x = widx *  65535.0;
	v->texSize1.y = widy * 65535.0;
	v->texIndex1 = ci->texIndex;
}


