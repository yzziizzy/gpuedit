

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "window.h"
#include "app.h"
#include "texture.h"
#include "sti/sti.h"

#include "gui.h"
#include "gui_internal.h"

#include "utilities.h"

// // FontConfig
// #include "text/fcfg.h"

// for sdf debugging
#include "dumpImage.h"


// VEC(GUIObject*) gui_list; 
// VEC(GUIObject*) gui_reap_queue; 


GUIObject* guiBaseHitTest(GUIObject* go, Vector2 absTestPos);




void gui_default_ParentResize(GUIObject* root, GUIEvent* gev) {
	root->h.size = gev->size;
	
	VEC_EACH(&root->h.children, i, child) {
		gev->currentTarget = child; 
		GUIObject_TriggerEvent(child, gev);
	}
}










void gui_default_Delete(GUIHeader* h) {
	h->deleted = 1;
	
	VEC_PUSH(&h->gm->reapQueue, h);
	
	// delete the children
	VEC_EACH(&h->children, i, ch) {
		GUIObject_Delete_(ch);
	}
	
	
	GUIUnregisterObject_(h);
}





void GUIResize(GUIHeader* gh, Vector2 newSz) {
	if(gh->deleted) return;
	
	if(gh->vt->Resize) {
		gh->vt->Resize((GUIObject*)gh, newSz);
	}
	else {
		gh->size = newSz;
	}
	
	// parents need to resize their children
} 





void gui_headerInit(GUIHeader* gh, GUIManager* gm, struct gui_vtbl* vt, struct GUIEventHandler_vtbl* event_vt) {
	VEC_INIT(&gh->children);
	gh->gm = gm;
	gh->vt = vt;
	gh->event_vt = event_vt;
	
	gh->scale = 1.0;
	gh->alpha = 1.0;
	gh->z = 0.0;
}



// grp is data about the parent's positioning. 
// the child must calculate its own (absolute) position based on the parent's info passed in.
// this info is then passed down to its children
// the default behavior is:
//   position according to gravity
//   no extra clipping
//   add z values together
void gui_defaultUpdatePos(GUIObject* go, GUIRenderParams* grp, PassFrameParams* pfp) {
	
	GUIHeader* h = &go->h;
	
	Vector2 tl = gui_calcPosGrav(h, grp);
	h->absTopLeft = tl;
	h->absClip = grp->clip;
	h->absZ = grp->baseZ + h->z + 0.00001;
	
	// TODO: relTopLeft, absClip
	
	GUIRenderParams grp2 = {
		.size = h->size,
		.offset = tl,
		.clip = h->absClip,
		.baseZ = h->absZ,
	};
	
	VEC_EACH(&h->children, ind, child) {
		GUIHeader_updatePos(child, &grp2, pfp);
	}
	
}


// grp is data about the parent's positioning. 
// the child must calculate its own (absolute) position based on the parent's info passed in.
// this info is then passed down to its children
// the default behavior is:
//   position according to gravity
//   no extra clipping
//   add z values together
void gui_selfUpdatePos(GUIHeader* h, GUIRenderParams* grp, PassFrameParams* pfp) {
	
	Vector2 tl = gui_calcPosGrav(h, grp);
	h->absTopLeft = tl;
	h->absClip = grp->clip;
	h->absZ = grp->baseZ + h->z + 100;
}

void gui_columnUpdatePos(GUIHeader* gh, GUIRenderParams* grp, PassFrameParams* pfp) {
	// TODO: fix fencepost issue with spacing
	// TODO: figure out better phase for size calculation
	float total_h = 0.0;
	float max_w = 0.0;
	VEC_EACH(&gh->children, i, child) { 
		total_h += child->h.size.y;
		max_w = fmax(max_w, child->h.size.x);
	}
	
	gh->size.y = total_h;
	gh->size.x = max_w;
	
	Vector2 tl = gui_calcPosGrav(gh, grp);
	
	// columnlayout works by spoofing the renderparams supplied to each child
	total_h = 0.0;
	VEC_EACH(&gh->children, i, child) { 
		
		GUIRenderParams grp2 = {
			.clip = grp->clip,
			.size = child->h.size, // sized to the child to eliminate gravity 
			.offset = {
				.x = tl.x,
				.y = tl.y + total_h 
			},
			.baseZ = grp->baseZ + gh->z + 100,
		};
		
		GUIHeader_updatePos(child, &grp2, pfp);
		
		total_h += child->h.size.y;
	}
}



void GUIObject_triggerClick(GUIObject* go, Vector2 testPos) {
	printf("GUIObject_triggerClick not implemented.");
// 	if(go->vt->Click)
// 		go->vt->Click(go, testPos);
}




// default hit testing handler that only operates on the header
GUIObject* gui_defaultHitTest(GUIHeader* h, Vector2 absTestPos) {
	
	if(!(absTestPos.x >= h->absTopLeft.x && 
		absTestPos.y >= h->absTopLeft.y &&
		absTestPos.x <= (h->absTopLeft.x + h->size.x) && 
		absTestPos.y <= (h->absTopLeft.y  + h->size.y))) {
		
		return NULL;
	}
	
	int i;
	GUIObject* bestKid = NULL;
	for(i = 0; i < VEC_LEN(&h->children); i++) {
		GUIObject* kid = GUIObject_hitTest(VEC_ITEM(&h->children, i), absTestPos);
		if(kid) {
			if(!bestKid) {
				bestKid = kid;
			}
			else {
				if(kid->h.absZ > bestKid->h.absZ) bestKid = kid;
			}
		}
	}
	
	return bestKid ? bestKid : (GUIObject*)h;
}




/*
void GUIObject_triggerClick(GUIObject* go, GUIEvent* e) {
	
	if(!go) return;
	e->currentTarget = go;
	
	if(c->h.onClick)
		c->h.onClick(go, e);
	
// 	e->currentTarget = c->h.parent;
	
// 	guiTriggerClick(e);
}*/


// old

void guiSetClientSize(GUIObject* go, Vector2 cSize) {
	if(go->h.vt && go->h.vt->SetClientSize)
		return go->h.vt->SetClientSize(go, cSize);
}

Vector2 guiGetClientSize(GUIObject* go) {
	if(go->h.vt && go->h.vt->GetClientSize)
		return go->h.vt->GetClientSize(go);
	
	return (Vector2){-1,-1};
} 

Vector2 guiRecalcClientSize(GUIObject* go) {
	if(go->h.vt && go->h.vt->RecalcClientSize)
		return go->h.vt->RecalcClientSize(go);
	
	return (Vector2){-1,-1};
} 


void guiAddClient(GUIObject* parent, GUIObject* child) {
	if(parent->h.vt && parent->h.vt->AddClient)
		parent->h.vt->AddClient(parent, child);
} 

void guiRemoveClient(GUIObject* parent, GUIObject* child) {
	if(parent->h.vt && parent->h.vt->RemoveClient)
		parent->h.vt->RemoveClient(parent, child);
} 







///////////// internals //////////////



// returns absolute top/left offset coordinates for a gui object based on the RP from the parent
Vector2 gui_calcPosGrav(GUIHeader* h, GUIRenderParams* grp) {
	/*
	printf("grav: %d - %f,%f, %f,%f, %f,%f, %f,%f, \n",
		   h->gravity,
			grp->offset.x,
			grp->offset.y,
			h->topleft.x,
			h->topleft.y,
			grp->size.x,
			grp->size.y,
			h->size.x,
			h->size.y
	);
	*/
	switch(h->gravity) {
		default:
		case GUI_GRAV_TOP_LEFT:
			return (Vector2){grp->offset.x + h->topleft.x, grp->offset.y + h->topleft.y}; 
		case GUI_GRAV_CENTER_LEFT:
			return (Vector2){
				grp->offset.x + h->topleft.x, 
				grp->offset.y + h->topleft.y + (grp->size.y / 2) - (h->size.y / 2)
			}; 
		case GUI_GRAV_BOTTOM_LEFT:
			return (Vector2){
				grp->offset.x + h->topleft.x, 
				grp->offset.y + h->topleft.y + (grp->size.y) - (h->size.y)
			};
		case GUI_GRAV_CENTER_BOTTOM:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x / 2) - (h->size.x / 2), 
				grp->offset.y + h->topleft.y + (grp->size.y) - (h->size.y)
			};
		case GUI_GRAV_BOTTOM_RIGHT:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x) - (h->size.x), 
				grp->offset.y + h->topleft.y + (grp->size.y) - (h->size.y)
			};
		case GUI_GRAV_CENTER_RIGHT:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x) - (h->size.x), 
				grp->offset.y + h->topleft.y + (grp->size.y / 2) - (h->size.y / 2)
			};
		case GUI_GRAV_TOP_RIGHT:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x) - (h->size.x), 
				grp->offset.y + h->topleft.y
			};
		case GUI_GRAV_CENTER_TOP:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x / 2) - (h->size.x / 2), 
				grp->offset.y + h->topleft.y
			};
		case GUI_GRAV_CENTER:
			return (Vector2){
				grp->offset.x + h->topleft.x + (grp->size.x / 2) - (h->size.x / 2), 
				grp->offset.y + h->topleft.y + (grp->size.y / 2) - (h->size.y / 2)
			};
	}
}




// returns child coordinates from parent coordinates
// pt is in parent coordinates
Vector2 gui_parent2ChildGrav(GUIHeader* child, GUIHeader* parent, Vector2 pt) {
	
	// pretend the parent is a root element
	GUIRenderParams grp = {
		.size = parent->size, // parent
		.offset = {0, 0},
	};

	// child's top left in parent coordinates
	Vector2 ctl = gui_calcPosGrav(child, &grp);
	printf("ctl: %f, %f | %f, %f\n", pt.x, pt.y, ctl.x, ctl.y);
	return (Vector2) {
		.x = pt.x - ctl.x,
		.y = pt.y - ctl.y
	};
}








GUIObject* GUIObject_findChild(GUIObject* obj, char* childName) {
	if(!obj) return NULL;
	VEC_EACH(&obj->h.children, i, child) {
		if(child->h.name && 0 == strcmp(child->h.name, childName)) return child;
	}
	return NULL;
}






/*

// uses default font type and size
draw_ui_text(pos, color, aabb, str, len) {


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
*/


// stops on linebreak
void gui_drawDefaultUITextLine(
	GUIManager* gm,
	AABB2* box,
	AABB2* clip,
	struct Color4* color, 
	float zIndex,
	char* txt, 
	size_t charCount
) {
	
	
	
// 		printf("'%s'\n", bl->buf);
	if(txt == NULL || charCount == 0) return;
	
	Vector2 tl = {box->min.x, box->min.y};
	
	int charsDrawn = 0;
	GUIFont* f = gm->defaults.font;
	float size = gm->defaults.fontSize; // HACK
	float hoff = size * f->ascender;//gt->header.size.y * .75; // HACK
	float adv = 0;
	if(!color) color = &gm->defaults.textColor;
	
	float maxAdv = box->max.x - box->min.x;
	
	
	float spaceadv = f->regular[' '].advance;
	
	for(int n = 0; txt[n] != 0 && adv < maxAdv && n < charCount; n++) {
		char c = txt[n];
		
		struct charInfo* ci = &f->regular[c];
		
		if(c == '\t') {
			adv += spaceadv * 4; // hardcoded to annoy you
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
			
			v->clip.t = clip->min.y;
			v->clip.l = clip->min.x;
			v->clip.b = clip->max.y;
			v->clip.r = clip->max.x;
			v->fg = GUI_COLOR4_TO_SHADER(*color);
			v->z = zIndex;
			
			adv += ci->advance * size; // BUG: needs sdfDataSize added in?
			//v++;
			gm->elementCount++;
			charsDrawn++;
		}
		else {
			adv += spaceadv;
			charsDrawn++;
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
	
	
	float spaceadv = f->regular[' '].advance;
	
	for(int n = 0; txt[n] != 0 && n < maxChars; n++) {
		char c = txt[n];
		
		if(c == '\t') {
			adv += spaceadv * 4; // hardcoded to annoy you
		}
		else if(c != ' ') {
			struct charInfo* ci = &f->regular[c];
			adv += ci->advance * size; // BUG: needs sdfDataSize added in?
		}
		else {
			adv += spaceadv;
		}
	}
	
	return adv;
}




// 


void GUIHeader_renderChildren(GUIHeader* gh, PassFrameParams* pfp) {
// 	if(gh->hidden || gh->deleted) return;

	VEC_EACH(&gh->children, i, obj) {
		GUIHeader_render(&obj->h, pfp);
	}
}


void GUIRegisterObject_(GUIHeader* parent, GUIHeader* o) {
	int i;
	
	if(!parent) {
		parent = o->gm->root;
	}
	o->parent = parent;
	i = VEC_FIND(&parent->children, &o);
	if(i < 0) {
		VEC_PUSH(&parent->children, o);
	}
}


void GUIUnregisterObject_(GUIHeader* o) {
	GUIHeader* parent = o->parent;
	
	if(!parent) {
		return;
	}
	
	VEC_RM_VAL(&parent->children, &o);
	
	o->parent = NULL;
}




// virtual methods


void GUIObject_Delete_(GUIHeader* h) {
	if(h->vt && h->vt->Delete)
		h->vt->Delete(h);
	else 
		gui_default_Delete(h);
}


void GUIObject_Reap_(GUIHeader* h) {
	if(!h->deleted) {
		printf("Attempting to reap non-deleted GUIObject\n");
// 		Log("Attempting to reap non-deleted GUI Object");
		return;
	}
	
	if(h->vt && h->vt->Reap)
		h->vt->Reap(h);
	else {
		free(h);
	}
}


GUIObject* GUIObject_hitTest(GUIObject* go, Vector2 absTestPos) {
	if(go->h.vt && go->h.vt->HitTest)
		return go->h.vt->HitTest(go, absTestPos);
	
	return gui_defaultHitTest(&go->h, absTestPos);
}

void GUIHeader_updatePos(GUIObject* go, GUIRenderParams* grp, PassFrameParams* pfp) {
	if(go->h.vt && go->h.vt->UpdatePos)
		go->h.vt->UpdatePos(go, grp, pfp);
	else
		gui_defaultUpdatePos(&go->h, grp, pfp);
}


void GUIHeader_render(GUIHeader* gh, PassFrameParams* pfp) {
	if(gh == NULL) return;
	if(gh->hidden || gh->deleted) return;
	
	if(gh->vt && gh->vt->Render)
		gh->vt->Render((GUIObject*)gh, pfp);
	else
		GUIHeader_renderChildren(gh, pfp);
}


void GUIObject_AddClient_(GUIHeader* parent, GUIHeader* client) {
	if(parent->vt && parent->vt->AddClient)
		parent->vt->AddClient(parent, client);
	else
		printf("Object does not have an AddClient function.");
}


void GUIObject_RemoveClient_(GUIHeader* parent, GUIHeader* client) {
	if(parent->vt && parent->vt->RemoveClient)
		parent->vt->RemoveClient(parent, client);
	else
		printf("Object does not have a RemoveClient function.");
}


Vector2 GUIObject_SetScrollPct_(GUIHeader* go, Vector2 pct) {
	if(go->vt && go->vt->SetScrollPct)
		return go->vt->SetScrollPct(go, pct);
	else
		printf("Object does not have a SetScrollPct function.");
	
	return (Vector2){0, 0};
}


Vector2 GUIObject_SetScrollAbs_(GUIHeader* go, Vector2 absPos) {
	if(go->vt && go->vt->SetScrollAbs)
		return go->vt->SetScrollAbs(go, absPos);
	else
		printf("Object does not have a SetScrollAbs function.");
	
	return (Vector2){0, 0};
}
