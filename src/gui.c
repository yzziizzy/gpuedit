

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


static void preFrame(PassFrameParams* pfp, GUIManager* gm);
static void draw(GUIManager* gm, GLuint progID, PassDrawParams* pdp);
static void postFrame(GUIManager* gm);



GUIManager* GUIManager_alloc(GlobalSettings* gs) {
	GUIManager* gm;
	pcalloc(gm);
	
	GUIManager_init(gm, gs);
	
	return gm;
}



static void updatePosRoot(GUIHeader* gh, GUIRenderParams* always_null, PassFrameParams* pfp) {
	GUIRenderParams grp = {
		.size = gh->size,
		.offset = {0,0},
		.clip = {{0,0}, gh->size},
		.baseZ = 0,
	};
	
	VEC_EACH(&gh->children, ind, child) {
		GUIHeader_updatePos(child, &grp, pfp);
	}
}

static void renderRoot(GUIHeader* gh, PassFrameParams* pfp) {
	GUIHeader_renderChildren(gh, pfp);
}

// static GUIObject* hitTestRoot(GUIObject* go, Vector2 absTestPos) {
// 	GUIHeader* gh = &go->header;
// 	float highestZ = 0;
// 	int i;
// 	for(i = 0; i < VEC_LEN(&gh->children); i++) {
// 		GUIObject* kid = GUIObject_hitTest(VEC_ITEM(&gh->children, i), testPos);
// 		if(kid) {
// 			printf("hit: %p\n", kid);
// 			return kid;
// 		}
// 	}
// 	
// 	// the root window does not respond to hits itself
// 	return NULL;
// }




void gui_default_ParentResize(GUIObject* root, GUIEvent* gev) {
	root->h.size = gev->size;
	
	VEC_EACH(&root->h.children, i, child) {
		gev->currentTarget = child; 
		GUITriggerEvent(child, gev);
	}
}


// _init is always called before _initGL
void GUIManager_init(GUIManager* gm, GlobalSettings* gs) {
	
	static struct gui_vtbl root_vt = {
		.UpdatePos = updatePosRoot,
		.Render = renderRoot,
// 		.HitTest = hitTestRoot,
	};
		
	static struct GUIEventHandler_vtbl event_vt = {
		.ParentResize = gui_default_ParentResize,
	};
	
	VEC_INIT(&gm->reapQueue);
	
	gm->gs = gs;
	
	gm->maxInstances = gs->GUIManager_maxInstances;
	
	gm->elementCount = 0;
	gm->elementAlloc = 64;
	gm->elemBuffer = calloc(1, sizeof(*gm->elemBuffer) * gm->elementAlloc);
	
	gm->fm = FontManager_alloc(gs);
	
// 	gm->ta = TextureAtlas_alloc(gs);
// 	gm->ta->width = 256;
// 	TextureAtlas_addFolder(gm->ta, "pre", "assets/ui/icons", 0);
// 	TextureAtlas_finalize(gm->ta);
	
	gm->minDragDist = 2;
	gm->doubleClickTime = 0.300;
	
	gm->root = calloc(1, sizeof(GUIHeader));
	gui_headerInit(gm->root, NULL, &root_vt, &event_vt); 
	
	VEC_INIT(&gm->focusStack);
	VEC_PUSH(&gm->focusStack, gm->root);
	
	gm->defaults.font = FontManager_findFont(gm->fm, "Arial");
	gm->defaults.fontSize = .45;
	gm->defaults.textColor = (struct Color4){200,200,200,255};
	gm->defaults.windowBgColor = (struct Color4){10,10,10,255};
	gm->defaults.editBgColor = (struct Color4){20,50,25,255};
	gm->defaults.cursorColor = (struct Color4){240,240,240,255};
	
	gm->defaultCursor = GUIMOUSECURSOR_ARROW;
}


void GUIManager_initGL(GUIManager* gm, GlobalSettings* gs) {
	static VAOConfig vaoConfig[] = {
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // top, left, bottom, right
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // tlbr clipping planes
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_FALSE}, // tex indices 1&2, tex fade, gui type
		{0, 4, GL_UNSIGNED_SHORT, 0, GL_TRUE}, // tex offset 1&2
		{0, 4, GL_UNSIGNED_SHORT, 0, GL_TRUE}, // tex size 1&2
		
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_TRUE}, // fg color
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_TRUE}, // bg color
		
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // z-index, alpha, opts 1-2
		
		{0, 0, 0}
	};

	
	gm->vao = makeVAO(vaoConfig);
	glBindVertexArray(gm->vao);
	
	int stride = calcVAOStride(0, vaoConfig);

	PCBuffer_startInit(&gm->instVB, gm->maxInstances * stride, GL_ARRAY_BUFFER);
	updateVAO(0, vaoConfig); 
	PCBuffer_finishInit(&gm->instVB);
	
	///////////////////////////////
	// font texture
	
	glGenTextures(1, &gm->fontAtlasID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, gm->fontAtlasID);
	glerr("bind font tex");
	
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0); // no mipmaps for this; it'll get fucked up
	glerr("param font tex");
	
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R8, gm->fm->atlasSize, gm->fm->atlasSize, VEC_LEN(&gm->fm->atlas));
	
	VEC_EACH(&gm->fm->atlas, ind, at) {
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 
			0, 0, ind, // offsets
			gm->fm->atlasSize, gm->fm->atlasSize, 1, 
			GL_RED, GL_UNSIGNED_BYTE, at);
		glerr("load font tex");
	}
	
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	
	
	///////////////////////////////
	// regular texture atlas
	
	glGenTextures(1, &gm->atlasID);
	glBindTexture(GL_TEXTURE_2D_ARRAY, gm->atlasID);
	glerr("bind font tex");
	
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0); // no mipmaps for this; it'll get fucked up
	glerr("param font tex");
	
	glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, gm->ta->width, gm->ta->width, VEC_LEN(&gm->ta->atlas));
	
	char buf [50];
	
	VEC_EACH(&gm->ta->atlas, ind, at2) {
		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 
			0, 0, ind, // offsets
			gm->ta->width, gm->ta->width, 1, 
			GL_RGBA, GL_UNSIGNED_BYTE, at2);
		glerr("load font tex");	
	}
	
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	
	//////////////////////////////////
	
}


void GUIManager_SetMainWindowTitle(GUIManager* gm, char* title) {
	if(gm->windowTitleSetFn) gm->windowTitleSetFn(gm->windowTitleSetData, title);
}

void GUIManager_SetCursor(GUIManager* gm, int cursor) {
	if(gm->currentCursor == cursor) return;
	gm->currentCursor = cursor;
	if(gm->mouseCursorSetFn) gm->mouseCursorSetFn(gm->mouseCursorSetData, cursor);
}


static int gui_elem_sort_fn(GUIUnifiedVertex* a, GUIUnifiedVertex* b) {
	return a->z == b->z ? 0 : (a->z > b->z ? 1 : -1);
}


static void preFrame(PassFrameParams* pfp, GUIManager* gm) {
	GUIUnifiedVertex* vmem = PCBuffer_beginWrite(&gm->instVB);
	if(!vmem) {
		printf("attempted to update invalid PCBuffer in GUIManager\n");
		return;
	}
	
	
	gm->elementCount = 0;
	
	
	GUIRenderParams grp = {
		.offset = {0,0}, 
		.size = {800,800},
		.clip = {(0,0),{800,800}},
	};
	gm->root->h.size = (Vector2){800, 800};
	
	GUIHeader_updatePos(gm->root, &grp, pfp);
	
	GUIHeader_render(gm->root, pfp);
	
	
	// test element 
	/*
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
		
	*v = (GUIUnifiedVertex){
// 		.pos = {gw->header.topleft.x, gw->header.topleft.y,
// 			gw->header.topleft.x + gw->header.size.x, gw->header.topleft.y + gw->header.size.y},
		.pos = { 250, 250, 700, 700},
		.clip = {0, 0, 800, 800},
		
		.texIndex1 = 2,
		.texIndex2 = 0,
		.texFade = .5,
		.guiType = 4, // bordered window (just a box)
		
		.texOffset1 = 0,
		.texOffset2 = 0,
		.texSize1 = 0,
		.texSize2 = 0,
		
		.fg = {255, 255, 255, 255}, // TODO: border color
		.bg = {128, 0, 0, 0}, // TODO: color
		
		.z = 99999,
		.alpha = 1.0,
	};
	/* */ 
	
	
	
	double sort;
	
// 	sort = getCurrentTime();
	qsort(gm->elemBuffer, gm->elementCount, sizeof(*gm->elemBuffer), (void*)gui_elem_sort_fn);
// 	printf("qsort time: [%d elem] %f\n", gm->elementCount, timeSince(sort)  * 1000.0);
	
// 	sort = getCurrentTime();
	memcpy(vmem, gm->elemBuffer, gm->elementCount * sizeof(*gm->elemBuffer));
// printf("memcpy time: %f\n", timeSince(sort) * 1000.0);
	
}

static void draw(GUIManager* gm, GLuint progID, PassDrawParams* pdp) {
	size_t offset;
	

// 	if(mdi->uniformSetup) {
// 		(*mdi->uniformSetup)(mdi->data, progID);
// 	}
	GLuint ts_ul = glGetUniformLocation(progID, "fontTex");
	GLuint ta_ul = glGetUniformLocation(progID, "atlasTex");
	
	glActiveTexture(GL_TEXTURE0 + 29);
	glUniform1i(ts_ul, 29);
	glexit("text sampler uniform");
 	glBindTexture(GL_TEXTURE_2D_ARRAY, gm->fontAtlasID);
//  	glBindTexture(GL_TEXTURE_2D, gt->font->textureID); // TODO check null ptr
 	glexit("bind texture");



	glActiveTexture(GL_TEXTURE0 + 28);
	glUniform1i(ta_ul, 28);
	glexit("text sampler uniform");
 	glBindTexture(GL_TEXTURE_2D_ARRAY, gm->atlasID);
//  	glBindTexture(GL_TEXTURE_2D, gt->font->textureID); // TODO check null ptr
 	glexit("bind texture");
	
	// ------- draw --------
	
	glBindVertexArray(gm->vao);
	
	PCBuffer_bind(&gm->instVB);
	offset = PCBuffer_getOffset(&gm->instVB);
	glDrawArrays(GL_POINTS, offset / sizeof(GUIUnifiedVertex), gm->elementCount);
	
	glexit("");
}



static void postFrame(GUIManager* gm) {
	PCBuffer_afterDraw(&gm->instVB);
}






RenderPass* GUIManager_CreateRenderPass(GUIManager* gm) {
	
	RenderPass* rp;
	PassDrawable* pd;

	pd = GUIManager_CreateDrawable(gm);

	rp = calloc(1, sizeof(*rp));
	RenderPass_init(rp);
	RenderPass_addDrawable(rp, pd);
	//rp->fboIndex = LIGHTING;
	
	return rp;
}


PassDrawable* GUIManager_CreateDrawable(GUIManager* gm) {
	
	PassDrawable* pd;
	static ShaderProgram* prog = NULL;
	
	if(!prog) {
		prog = loadCombinedProgram("guiUnified");
		glexit("");
	}
	
	
	pd = Pass_allocDrawable("GUIManager");
	pd->data = gm;
	pd->preFrame = preFrame;
	pd->draw = (PassDrawFn)draw;
	pd->postFrame = postFrame;
	pd->prog = prog;
	
	return pd;;
}




// add root objects to the root list, record the parent otherwise
void GUIRegisterObject_(GUIHeader* o, GUIHeader* parent) {
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




void guiDelete(GUIObject* go) {
	go->h.deleted = 1;
	
// 	VEC_PUSH(&gui_reap_queue, go);
	
	for(int i = 0; i < VEC_LEN(&go->h.children); i++) {
		//guiTextRender(VEC_DATA(&gui_list)[i], gs);
		guiDelete(VEC_ITEM(&go->h.children, i));
	}
	

	
	if(go->h.vt->Delete)
		go->h.vt->Delete(go);
} 

void guiReap(GUIObject* go) {
	if(!go->h.deleted) {
// 		Log("Attempting to reap non-deleted GUI Object");
		return;
	}
	
	// remove from parent
	guiRemoveChild(go->h.parent, go);
	
	if(go->h.vt->Reap)
		return go->h.vt->Reap(go);
	
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

// NOT SMT SAFE; NO LOCKS
int guiRemoveChild(GUIObject* parent, GUIObject* child) {
	
	if(!parent || !child) return 0;
	
	int i = VEC_FIND(&parent->h.children, child);
	if(i < 0) return 1;
	
	VEC_RM(&parent->h.children, i);
	
	return 0;
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




void GUIHeader_updatePos(GUIObject* go, GUIRenderParams* grp, PassFrameParams* pfp) {
	if(go->h.vt->UpdatePos)
		go->h.vt->UpdatePos(go, grp, pfp);
	else
		gui_defaultUpdatePos(&go->h, grp, pfp);
}

void GUIManager_updatePos(GUIManager* gm, PassFrameParams* pfp) {
	GUIHeader_updatePos(gm->root, NULL, pfp);
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
	h->absZ = grp->baseZ + h->z;
	
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
			.baseZ = grp->baseZ + gh->z,
		};
		
		GUIHeader_updatePos(child, &grp2, pfp);
		
		total_h += child->h.size.y;
	}
}



void GUIObject_triggerClick(GUIObject* go, Vector2 testPos) {
	if(go->h.onClick)
		go->h.onClick(go, testPos);
}


GUIObject* GUIManager_triggerClick(GUIManager* gm, Vector2 testPos) {
	GUIObject* go = GUIManager_hitTest(gm, testPos);
	
	if(go) GUIObject_triggerClick(go, testPos);
	
	return go;
}


GUIObject* GUIManager_hitTest(GUIManager* gm, Vector2 testPos) {
	GUIObject* go = GUIObject_hitTest(gm->root, testPos);
	return go == gm->root ? NULL : go;
}


GUIObject* GUIObject_hitTest(GUIObject* go, Vector2 absTestPos) {
	if(go->h.vt->HitTest)
		return go->h.vt->HitTest(go, absTestPos);
	
	return gui_defaultHitTest(&go->h, absTestPos);
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
	if(go->h.vt->SetClientSize)
		return go->h.vt->SetClientSize(go, cSize);
}

Vector2 guiGetClientSize(GUIObject* go) {
	if(go->h.vt->GetClientSize)
		return go->h.vt->GetClientSize(go);
	
	return (Vector2){-1,-1};
} 

Vector2 guiRecalcClientSize(GUIObject* go) {
	if(go->h.vt->RecalcClientSize)
		return go->h.vt->RecalcClientSize(go);
	
	return (Vector2){-1,-1};
} 


void guiAddClient(GUIObject* parent, GUIObject* child) {
	if(parent->h.vt->AddClient)
		parent->h.vt->AddClient(parent, child);
} 

void guiRemoveClient(GUIObject* parent, GUIObject* child) {
	if(parent->h.vt->RemoveClient)
		parent->h.vt->RemoveClient(parent, child);
} 







///////////// internals //////////////


GUIUnifiedVertex* GUIManager_checkElemBuffer(GUIManager* gm, int count) {
	if(gm->elementAlloc < gm->elementCount + count) {
		gm->elementAlloc = MAX(gm->elementAlloc * 2, gm->elementAlloc + count);
		gm->elemBuffer = realloc(gm->elemBuffer, sizeof(*gm->elemBuffer) * gm->elementAlloc);
	}
	
	return gm->elemBuffer + gm->elementCount;
}

GUIUnifiedVertex* GUIManager_reserveElements(GUIManager* gm, int count) {
	GUIUnifiedVertex* v = GUIManager_checkElemBuffer(gm, count);
	gm->elementCount += count;
	return v;
}


void GUIHeader_render(GUIHeader* gh, PassFrameParams* pfp) {
	if(gh == NULL) return;
	if(gh->hidden || gh->deleted) return;
	
	if(gh->vt->Render)
		gh->vt->Render((GUIObject*)gh, pfp);
	else
		GUIHeader_renderChildren(gh, pfp);
} 

void GUIHeader_renderChildren(GUIHeader* gh, PassFrameParams* pfp) {
// 	if(gh->hidden || gh->deleted) return;

	VEC_EACH(&gh->children, i, obj) {
		GUIHeader_render(&obj->h, pfp);
	}
}


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



static unsigned int translateModKeys(GUIManager* gm, InputEvent* iev) {
	unsigned int m = 0;
	
	if(iev->kbmods & IS_CONTROL) {
		m |= GUIMODKEY_CTRL;
		// check which one
	}
	if(iev->kbmods & IS_ALT) {
		m |= GUIMODKEY_ALT;
		// check which one
	}
	if(iev->kbmods & IS_SHIFT) {
		m |= GUIMODKEY_SHIFT;
		// check which one
	}
	if(iev->kbmods & IS_TUX) {
		m |= GUIMODKEY_TUX;
		// check which one
	}
	
	return m;
}


void GUIManager_HandleMouseMove(GUIManager* gm, InputState* is, InputEvent* iev) {
	Vector2 newPos = {
		iev->intPos.x, iev->intPos.y
	};
	
	// find the deepest target
	GUIObject* t = GUIManager_hitTest(gm, newPos);
	if(!t) return; // TODO handle mouse leaves;
	
	
	// set the cursor, maybe
	int cur = t->h.cursor; 
	GUIManager_SetCursor(gm, cur);
	
	
	GUIEvent gev = {
		.type = GUIEVENT_MouseMove,
		.originalTarget = t,
		.currentTarget = t,
		.eventTime = 0,
		.pos = newPos,
		.character = 0, // N/A
		.keycode = 0, // N/A
		.modifiers = translateModKeys(gm, iev), 
		.cancelled = 0,
		.requestRedraw = 0,
	};
	
	
	// check for dragging
	if(gm->isMouseDown && !gm->isDragging && gm->dragStartTarget) {
		float dragDist = vDist2(&newPos, &gm->dragStartPos);
		if(dragDist >= gm->minDragDist) {
			gm->isDragging = 1;
			gm->dragStartTarget = t;
			
			gev.type = GUIEVENT_DragStart;
			gev.currentTarget = t;
			gev.dragStartPos = gm->dragStartPos;
			gev.cancelled = 0;
			GUIManager_BubbleEvent(gm, gm->dragStartTarget, &gev);
		};
	}
	
	// DragMove event
	if(gm->isDragging) {
		gev.type = GUIEVENT_DragMove;
		gev.originalTarget = gm->dragStartTarget;
		gev.currentTarget = t;
		gev.dragStartPos = gm->dragStartPos;
		gev.cancelled = 0;
		GUIManager_BubbleEvent(gm, gm->dragStartTarget, &gev);
	}
	
	
	GUIManager_BubbleEvent(gm, t, &gev);
	
	// TODO: handle redraw request
}

void GUIManager_HandleMouseClick(GUIManager* gm, InputState* is, InputEvent* iev) {
	
	Vector2 newPos = {
		iev->intPos.x, iev->intPos.y
	};
	
	// find the deepest target
	GUIObject* t = GUIManager_hitTest(gm, newPos);
	if(!t) return; // TODO handle mouse leaves;
	
	// buttons: 
	// 1 - left
	// 2 - mid
	// 3 - right
	// 4 - scroll up
	// 5 - scroll down
	// 6 - scroll left
	// 7 - scroll right
	// 8 - front left side (on my mouse)
	// 9 - rear left side (on my mouse)
	
	GUIEvent gev = {
		.type = GUIEVENT_MouseUp,
		.originalTarget = t,
		.currentTarget = t,
		.eventTime = iev->time,
		.pos = newPos,
		.button = iev->button, 
		.keycode = 0, // N/A
		.modifiers = translateModKeys(gm, iev),
		.multiClick = 1,
		.cancelled = 0,
		.requestRedraw = 0,
	};
	
	
	// TODO doubleclick
	
	
	// TODO: check requestRedraw between each dispatch
	if(iev->type == EVENT_MOUSEDOWN) {
		gev.type = GUIEVENT_MouseDown;
		gev.currentTarget = t,
		gev.cancelled = 0;
		GUIManager_BubbleEvent(gm, t, &gev);
		
		gm->isMouseDown = 1;
		gm->dragStartPos = newPos;
		gm->dragStartTarget = t;
		gev.dragStartPos = gm->dragStartPos;
		
	} else if(iev->type == EVENT_MOUSEUP) {
		char suppressClick = 0;
		
		// check for drag end
		if(gm->isDragging) {
			gev.type = GUIEVENT_DragStop,
			gev.currentTarget = t,
			gev.cancelled = 0;
			gev.dragStartPos = gm->dragStartPos;
			GUIManager_BubbleEvent(gm, t, &gev);
			
			// end dragging
			gm->dragStartTarget = NULL;
			gm->isDragging = 0;
			gm->dragButton = 0;
			
			suppressClick = 1;
		}
		
		gm->isMouseDown = 0;
		
		gev.type = GUIEVENT_MouseUp,
		gev.currentTarget = t,
		gev.cancelled = 0;
		GUIManager_BubbleEvent(gm, t, &gev);
		
		// scroll wheel
		if(iev->button == 4 || iev->button == 5) {
			
			if(iev->button == 4) {
				gev.type = GUIEVENT_ScrollUp;
				gev.currentTarget = t;
				gev.cancelled = 0;
				GUIManager_BubbleEvent(gm, t, &gev);
			}
			else {
				gev.type = GUIEVENT_ScrollDown;
				gev.currentTarget = t;
				gev.cancelled = 0;
				GUIManager_BubbleEvent(gm, t, &gev);
			}
			
			gev.type = GUIEVENT_Scroll;
			gev.currentTarget = t;
			gev.cancelled = 0;
			GUIManager_BubbleEvent(gm, t, &gev);
			
			return;
		}
		
		// aux click; hscroll, etc
		if(iev->button > 5) {
			gev.type = GUIEVENT_AuxClick;
			gev.currentTarget = t;
			gev.cancelled = 0;
			GUIManager_BubbleEvent(gm, t, &gev);
			
			return;
		}
		
		
		// normal clicks
		if(iev->button == 1) {
			
			if(!suppressClick) {
				if(gm->clickHistory[0].button == 1 && 
					iev->time < gm->clickHistory[0].time + gm->doubleClickTime) {
					
					gev.multiClick = 2; 
				}
				
				gev.type = GUIEVENT_Click;
				gev.currentTarget = t;
				gev.cancelled = 0;
				GUIManager_BubbleEvent(gm, t, &gev);
			
			}
		}
		else if(iev->button == 2) {
			// TODO: replace when better input management exists
			gev.type = GUIEVENT_MiddleClick;
			gev.currentTarget = t;
			gev.cancelled = 0;
			GUIManager_BubbleEvent(gm, t, &gev);
		}
		else if(iev->button == 3) {
			// TODO: replace when better input management exists
			gev.type = GUIEVENT_RightClick;
			gev.currentTarget = t;
			gev.cancelled = 0;
			GUIManager_BubbleEvent(gm, t, &gev);
		}
		
		
		
		// push the latest click onto the stack
		gm->clickHistory[2] = gm->clickHistory[1];
		gm->clickHistory[1] = gm->clickHistory[0];
		gm->clickHistory[0].time = iev->time;
		gm->clickHistory[0].button = iev->button;
		
	}
	
}

void GUIManager_HandleKeyInput(GUIManager* gm, InputState* is, InputEvent* iev) {
	
	int type;
	
	// translate event type
	switch(iev->type) {
		case EVENT_KEYDOWN: type = GUIEVENT_KeyDown; break; 
		case EVENT_KEYUP: type = GUIEVENT_KeyUp; break; 
		case EVENT_TEXT: type = GUIEVENT_KeyUp; break; 
		default:
			fprintf(stderr, "!!! Non-keyboard event in GUIManager_HandleKeyInput: %d\n", iev->type);
			return; // not actually a kb event
	}
	
	GUIObject* t = GUIManager_getFocusedObject(gm);
	if(!t) {
		fprintf(stderr, "key input with no focused object\n");
		return; // TODO ???
	}
	
	GUIEvent gev = {
		.type = type,
		.originalTarget = t,
		.currentTarget = t,
		.eventTime = 0,
		.pos = {0,0}, // N/A
		.character = iev->character, 
		.keycode = iev->keysym, 
		.modifiers = translateModKeys(gm, iev),
		.cancelled = 0,
		.requestRedraw = 0,
	};
	
	GUIManager_BubbleEvent(gm, t, &gev);
}


// handles event bubbling and logic
void GUIManager_BubbleEvent(GUIManager* gm, GUIObject* target, GUIEvent* gev) {
	
	GUIObject* obj = target;
	
	int bubble = GUIEventBubbleBehavior[gev->type];
	
	if(bubble == 0) {
		// no bubbling, just the target
		GUITriggerEvent(obj, gev);
	}
	else if(bubble == 1) {
		// bubble until cancelled
		while(obj && !gev->cancelled) {
			gev->currentTarget = obj;
			GUITriggerEvent(obj, gev);
			
			obj = obj->h.parent;
		}
	}
	else if(bubble = 2) {
		// trigger on all parents
		while(obj) {
			gev->currentTarget = obj;
			GUITriggerEvent(obj, gev);
			
			obj = obj->h.parent;
		}
	}
	else {
		fprintf(stderr, "!!! unknown bubbling behavior: %d\n", bubble);
	}
	
}


// lowest level of event triggering
// does not do bubbling
void GUITriggerEvent_(GUIHeader* o, GUIEvent* gev) {
	
	if(!o || !o->event_vt) return;
	
	switch(gev->type) {
		#define X(name, b) case GUIEVENT_##name: \
				if(o->event_vt->name) (*o->event_vt->name)((GUIObject*)o, gev); \
				break;
			
			GUIEEVENTTYPE_LIST
		#undef X
	}
	
	if(o->event_vt->Any) (*o->event_vt->Any)((GUIObject*)o, gev);
}



// focus stack functions

GUIObject* GUIManager_getFocusedObject(GUIManager* gm) {
	if(VEC_LEN(&gm->focusStack) == 0) return NULL;
	return VEC_TAIL(&gm->focusStack);
}

GUIObject* GUIManager_popFocusedObject(GUIManager* gm) {
	GUIObject* o;
	
	// can't pop off the root element at the bottom
	if(VEC_LEN(&gm->focusStack) <= 1) return VEC_TAIL(&gm->focusStack);
	
	VEC_POP(&gm->focusStack, o);
	return o;
}


void GUIManager_pushFocusedObject_(GUIManager* gm, GUIHeader* h) {
	VEC_PUSH(&gm->focusStack, (GUIObject*)h);
	
	GUIEvent gev = {};
	gev.type = GUIEVENT_GainedFocus;
	gev.originalTarget = h;
	gev.currentTarget = h;
	
	GUIManager_BubbleEvent(gm, h, &gev);
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
	
	for(int n = 0; txt[n] != 0 && adv < maxAdv; n++) {
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
			
			v->clip.t = 0; // disabled in the shader right now
			v->clip.l = 0;
			v->clip.b = 1000000;
			v->clip.r = 1000000;
			v->fg = *color;
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

