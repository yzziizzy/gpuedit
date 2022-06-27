

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#include "gui.h"
#include "gui_internal.h"




static void preFrame(PassFrameParams* pfp, void* gm_);
static void draw(void* gm_, GLuint progID, PassDrawParams* pdp);
static void postFrame(void* gm_);



GUIManager* GUIManager_alloc() {
	GUIManager* gm;
	pcalloc(gm);
	
	gm->fm = FontManager_alloc();
	
	return gm;
}

// _init is always called before _initGL
void GUIManager_Init(GUIManager* gm, GUISettings* gs) {
	gm->gs = gs;

	GUIManager_InitCommands(gm);
	
	FontManager_init(gm->fm, gs);
	
	gm->vertCount = 0;
	gm->vertAlloc = 2048;
	gm->vertBuffer = calloc(1, sizeof(*gm->vertBuffer) * gm->vertAlloc);
	
		
	gm->fontClipLow = 0.45;
	gm->fontClipHigh = 0.80;
	gm->fontClipGap = gm->fontClipHigh - gm->fontClipLow;
	
	gm->windowHeap.cnt = 0;
	gm->windowHeap.alloc = 16;
	gm->windowHeap.buf = calloc(1, gm->windowHeap.alloc * sizeof(*gm->windowHeap.buf));
	gm->rootWin = GUIWindow_new(gm, 0);
	
	gm->minDragDist = 2;
	gm->doubleClickTime = 0.500;
	gm->tripleClickTime = 1.000;
	gm->quadClickTime = 1.500;
	gm->multiClickDist = 2.000;
	
}


static void init_pcbuffer(PCBuffer* pcb, GLuint* vao, int instances) {
	static VAOConfig vaoConfig[] = {
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // top, left, bottom, right
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // tlbr clipping planes
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_FALSE}, // tex indices 1&2, tex fade, gui type
		{0, 4, GL_UNSIGNED_SHORT, 0, GL_TRUE}, // tex offset 1&2
		{0, 4, GL_UNSIGNED_SHORT, 0, GL_TRUE}, // tex size 1&2
		
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_TRUE}, // fg color
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_TRUE}, // bg color
		
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // z-index, alpha, rotation, opt 2
		
		{0, 0, 0, 0, 0}
	};

	
	*vao = makeVAO(vaoConfig);
	glBindVertexArray(*vao);
	
	int stride = calcVAOStride(0, vaoConfig);

	PCBuffer_startInit(pcb, instances * stride, GL_ARRAY_BUFFER);
	updateVAO(0, vaoConfig); 
	PCBuffer_finishInit(pcb);
}



void GUIManager_InitGL(GUIManager* gm) {


	gm->maxInstances = gm->gs->maxInstances;
	

	
// 	gm->ta = TextureAtlas_alloc(gs);
// 	gm->ta->width = 256;
// 	TextureAtlas_addFolder(gm->ta, "pre", "assets/ui/icons", 0);
// 	TextureAtlas_finalize(gm->ta);
	
	GUISettings_LoadDefaults(gm, &gm->defaults);
		
	// TEMP HACK 
	gm->defaults.font = FontManager_findFont(gm->fm, gm->defaults.fontName);
	gm->defaults.font_fw = FontManager_findFont(gm->fm, gm->defaults.fontName_fw);

	
	gm->defaults.charWidth_fw = gm->gs->charWidth_fw;
	gm->defaults.lineHeight_fw = gm->gs->lineHeight_fw;
	gm->defaults.fontSize_fw = gm->gs->fontSize_fw;
	
	
	gm->defaultCursor = GUIMOUSECURSOR_ARROW;
	
	
	init_pcbuffer(&gm->instVB, &gm->vao, gm->maxInstances);

	
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



GUIWindow* GUIWindow_new(GUIManager* gm, GUIWindow* parent) {
	if(gm->windowHeap.cnt >= gm->windowHeap.alloc) {
		gm->windowHeap.alloc *= 2;
		gm->windowHeap.buf = realloc(gm->windowHeap.buf, gm->windowHeap.alloc * sizeof(*gm->windowHeap.buf));
		memset(gm->windowHeap.buf + gm->windowHeap.cnt, 0, (gm->windowHeap.alloc - gm->windowHeap.cnt) * sizeof(*gm->windowHeap.buf)); 
	}
	
	
	GUIWindow* w = &gm->windowHeap.buf[gm->windowHeap.cnt++];
	
	w->parent = parent;
	w->vertCount = 0;
		
	if(w->vertAlloc == 0) {
		w->vertAlloc = 32;
		w->vertBuffer = calloc(1, w->vertAlloc * sizeof(*w->vertBuffer));
	}
	
	VEC_TRUNC(&w->children);
		
	return w;
}



void GUIManager_SetMainWindowTitle(GUIManager* gm, char* title) {
	if(gm->windowTitleSetFn) gm->windowTitleSetFn(gm->windowTitleSetData, title);
}

void GUIManager_SetCursor(GUIManager* gm, int cursor) {
	if(gm->currentCursor == cursor) return;
	gm->currentCursor = cursor;
	if(gm->mouseCursorSetFn) gm->mouseCursorSetFn(gm->mouseCursorSetData, cursor);
}






GUIUnifiedVertex* GUIWindow_checkElemBuffer(GUIWindow* w, int count) {
	if(w->vertAlloc < w->vertCount + count) {
		w->vertAlloc = MAX(w->vertAlloc * 2, w->vertAlloc + count);
		w->vertBuffer = realloc(w->vertBuffer, sizeof(*w->vertBuffer) * w->vertAlloc);
	}
	
	return w->vertBuffer + w->vertCount;
}

GUIUnifiedVertex* GUIManager_reserveElements(GUIManager* gm, int count) {
	GUIUnifiedVertex* v = GUIWindow_checkElemBuffer(gm->curWin, count);
	gm->curWin->vertCount += count;
	gm->totalVerts += count;
	return v;
}

void GUIManager_copyElements(GUIManager* gm, GUIUnifiedVertex* verts, int count) {
	GUIWindow_checkElemBuffer(gm->curWin, count);
	memcpy(gm->curWin->vertBuffer + gm->vertCount, verts, count * sizeof(*gm->curWin->vertBuffer));
	gm->curWin->vertCount += count;
	gm->totalVerts += count;
}






// Event Handling


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


void GUIManager_HandleMouseMove(GUIManager* gm, InputState* is, InputEvent* iev, PassFrameParams* pfp) {
	Vector2 epos = {iev->intPos.x, iev->intPos.y};
	gm->lastMousePos = epos;
//	printf("mdd: %f, %d, %d\n", gm->minDragDist, (int)gm->mouseIsDown[b], (int)gm->mouseIsDragging[b]);
	
	// check for dragging
	for(int b = 0; b < 16; b++) {
		if(gm->mouseIsDown[b] && !gm->mouseIsDragging[b]) {
			
			float dragDist = vDist2(epos, gm->mouseDragStartPos[b]);
			if(dragDist >= gm->minDragDist) {
				gm->mouseIsDragging[b] = 1;
//			printf("mdd[%d]: %f ~ %f (%f,%f)\n", b, dragDist, gm->minDragDist, gm->mouseDragStartPos[b].x, gm->mouseDragStartPos[b].y);
				
				// fire the start event
				gm->curEvent = (GUIEvent){0};
				gm->curEvent = (GUIEvent){
					.type = GUIEVENT_DragStart,
					.button = b, 
					.pos = gm->mouseDragStartPos[b], 
					.modifiers = translateModKeys(gm, iev),
				};
					
				GUIManager_RunRenderPass(gm, pfp, 0);
			};
		}
		
			
		if(gm->mouseIsDragging[b]) {
			// drag move event
			gm->curEvent = (GUIEvent){0};
			gm->curEvent = (GUIEvent){
				.type = GUIEVENT_DragMove,
				.button = b, 
				.pos = epos, 
				.modifiers = translateModKeys(gm, iev),
			};
				
			GUIManager_RunRenderPass(gm, pfp, 0);
		}
	}
	
}

void GUIManager_HandleMouseClick(GUIManager* gm, InputState* is, InputEvent* iev, PassFrameParams* pfp) {
	int type;
	int multiClick = 1;
	int b = iev->button;
	Vector2 epos = {iev->intPos.x, iev->intPos.y};
	

	
	if(iev->type == EVENT_MOUSEDOWN) {
		type = GUIEVENT_MouseDown;
		gm->mouseWentDown[b] = 1;
		
		gm->mouseIsDown[b] = 1;
		gm->mouseDragStartPos[b] = epos;
	}
	else if(iev->type == EVENT_MOUSEUP) {
		type = GUIEVENT_MouseUp;
		gm->mouseWentUp[b] = 1;
		gm->mouseIsDown[b] = 0;
		
		if(b == 4) gm->scrollDist += 1.0;
		else if(b == 5) gm->scrollDist -= 1.0;
		
		// handle dragging
		if(gm->mouseIsDragging[b]) {
		
			gm->curEvent = (GUIEvent){0};
			gm->curEvent = (GUIEvent){
				.type = GUIEVENT_DragStop,
				.button = iev->button, 
				.pos = epos, 
				.modifiers = translateModKeys(gm, iev),
			};
				
			GUIManager_RunRenderPass(gm, pfp, 0);
		
			
			gm->mouseIsDragging[b] = 0;
			gm->mouseDragStartPos[b] = (Vector2){0,0};
			
			return;
		}
		
		// double, triple, and quad-click handling
		// position is tracked because it's not a multiclick if the mouse moves too much
		if(iev->time < gm->clickHistory[b][0].time + gm->doubleClickTime && gm->multiClickDist >= vDist2(gm->clickHistory[b][0].pos, epos)) {	
			if(iev->time < gm->clickHistory[b][1].time + gm->tripleClickTime && gm->multiClickDist >= vDist2(gm->clickHistory[b][1].pos, epos)) {
				if(iev->time < gm->clickHistory[b][2].time + gm->quadClickTime && gm->multiClickDist >= vDist2(gm->clickHistory[b][2].pos, epos)) 
					multiClick = 4;
				else 
					multiClick = 3;
			}
			else multiClick = 2;
		}
		
		// shift the click history
		gm->clickHistory[b][2] = gm->clickHistory[b][1];
		gm->clickHistory[b][1] = gm->clickHistory[b][0];
		gm->clickHistory[b][0].time = iev->time;
		gm->clickHistory[b][0].pos = epos;
	}
	else {
		fprintf(stderr, "!!! Non-mouse event in GUIManager_HandleMouseClick: %d\n", iev->type);
		return; // not actually a mouse event
	}
	

	gm->curEvent = (GUIEvent){0};
	gm->curEvent = (GUIEvent){
		.type = type,
		.button = iev->button, 
		.pos = epos, 
		.multiClick = multiClick,
		.modifiers = translateModKeys(gm, iev),
	};
		
	
	GUIManager_RunRenderPass(gm, pfp, 0);

	
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

}

void GUIManager_HandleKeyInput(GUIManager* gm, InputState* is, InputEvent* iev, PassFrameParams* pfp) {
	
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

	gm->curEvent = (GUIEvent){0};
	gm->curEvent = (GUIEvent){
		.type = type,
		.character = iev->character, 
		.keycode = iev->keysym, 
		.modifiers = translateModKeys(gm, iev),
	};
		
	
	GUIManager_RunRenderPass(gm, pfp, 0);
}



// Rendering
static int gui_vert_sort_fn(GUIUnifiedVertex* a, GUIUnifiedVertex* b) {
	return a->z == b->z ? 0 : (a->z > b->z ? 1 : -1);
}
static int gui_win_sort_fn(GUIWindow* a, GUIWindow* b) {
	return a->z == b->z ? 0 : (a->z > b->z ? 1 : -1);
}



void GUIManager_appendWindowVerts(GUIManager* gm, GUIWindow* w) {
	
//	double sort = getCurrentTime();
	qsort(w->vertBuffer, w->vertCount, sizeof(*w->vertBuffer), (void*)gui_vert_sort_fn);
//	printf("qsort time: %fus\n", timeSince(sort) * 1000000.0);
	
	if(0&&VEC_LEN(&w->children) == 0) {
		// simple memcpy
		memcpy(gm->vertBuffer + gm->vertCount, w->vertBuffer, w->vertCount * sizeof(*gm->vertBuffer));
		gm->vertCount += w->vertCount;
		
		return;
	}
	
	// zipper the elements and windows together based on z
	
	VEC_SORT(&w->children, gui_win_sort_fn);
	
	int ci = 0;
	GUIWindow** cw = VEC_DATA(&w->children);
	
	int vi = 0;
	GUIUnifiedVertex* v = w->vertBuffer;
	
	GUIUnifiedVertex* tv = gm->vertBuffer + gm->vertCount;
	while(1) {
		
		if(ci < VEC_LEN(&w->children) && ((*cw)->z < v->z || vi >= w->vertCount)) {
			// append the window first
			GUIManager_appendWindowVerts(gm, *cw);
			ci++;
			cw++;
			
			tv = gm->vertBuffer + gm->vertCount;
		}
		else if(vi < w->vertCount) {
			*tv = *v;
			tv->pos.l += w->absClip.min.x;
			tv->pos.r += w->absClip.min.x;
			tv->pos.t += w->absClip.min.y;
			tv->pos.b += w->absClip.min.y;
			tv++;
			vi++;
			v++;
			gm->vertCount++;
		}
		else {
			break; // no more vertices or child windows
		}
	}
	

}


void GUIManager_RunRenderPass(GUIManager* gm, PassFrameParams* pfp, int isDraw) {

	// clean up the windows from last frame
	gm->windowHeap.cnt = 0;
	gm->rootWin = GUIWindow_new(gm, 0);
	VEC_TRUNC(&gm->windowStack);
	VEC_PUSH(&gm->windowStack, gm->rootWin);
	gm->curWin = gm->rootWin;
	
	gm->rootWin->absClip = (AABB2){min: {0,0}, max: {pfp->dp->targetSize.x, pfp->dp->targetSize.y}};
	gm->rootWin->clip = gm->rootWin->absClip;
	VEC_TRUNC(&gm->clipStack);
	gm->curClip = gm->rootWin->clip;
	
	gm->time = pfp->appTime;
	gm->timeElapsed = pfp->timeElapsed;
	
	gm->curZ = 1.0;
	gm->fontSize = 20.0f;
	
	if(isDraw) {
		gm->drawMode = isDraw;
		
		// reset the IM gui cache
		gm->totalVerts = 0;
		gm->vertCount = 0;
		
		gm->curEvent = (GUIEvent){0};
	}
	
	gm->renderRootFn(gm->renderRootData, gm, (Vector2){0,0}, gm->screenSizef, pfp);
	gm->drawMode = 0;
	
	if(!isDraw) {
		// walk last pass' gui info to set window ids
		float highestZ = -99999999.9;
		GUIWindow* highestW = gm->windowHeap.buf; // incidentally the root window
		GUIWindow* w = gm->windowHeap.buf;
		for(int i = 0; i < gm->windowHeap.cnt; i++, w++) {
			if(w->z >= highestZ && boxContainsPoint2p(&w->absClip, &gm->lastMousePos)) {
				highestZ = w->z;
				highestW = w;
			}
		}
		
		gm->mouseWinID = highestW->id;
	}
	
}


static void preFrame(PassFrameParams* pfp, void* gm_) {
	GUIManager* gm = (GUIManager*)gm_;
	

	double sort;
	double time;
	double total = 0.0;
//	printf("\n");
	
	sort = getCurrentTime();

	
	GUIManager_RunRenderPass(gm, pfp, 1);
	
	/* soft cursor needs special handling
	time = timeSince(sort);
	total += time;
//	printf("render time: %fus\n", time  * 1000000.0);

	if(gm->useSoftCursor) {
		TextureAtlasItem* it;
		if(HT_get(&gm->ta->items, gm->softCursorName, &it)) {
			printf("could not find gui image '%s'\n", gm->softCursorName);
		}
		else { 
			
			GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
			*v++ = (GUIUnifiedVertex){
				.pos = {
					gm->lastMousePos.x, 
					gm->lastMousePos.y, 
					gm->lastMousePos.x + gm->softCursorSize.x, 
					gm->lastMousePos.y + gm->softCursorSize.y
				},
				.clip = {0,0,999999,999999},
				
				.guiType = 2, // image
				
				.texIndex1 = it->index,
				.texIndex2 = 0,
	
				.texOffset1 = { it->offsetNorm.x * 65535, it->offsetNorm.y * 65535 },
				.texOffset2 = 0,
				.texSize1 = { it->sizeNorm.x * 65535, it->sizeNorm.y * 65535 },
				.texSize2 = 0,
	
				
				.z = 9999999,
				.alpha = 1,
				.rot = 0,
			};
		}
	}

	*/

}

static void draw(void* gm_, GLuint progID, PassDrawParams* pdp) {
	GUIManager* gm = (GUIManager*)gm_;
	size_t offset;

	if(gm->totalVerts >= gm->vertAlloc) {
		gm->vertAlloc = nextPOT(gm->totalVerts);
		gm->vertBuffer = realloc(gm->vertBuffer, gm->vertAlloc * sizeof(*gm->vertBuffer));
	}

	GUIManager_appendWindowVerts(gm, gm->rootWin);
	
	if(gm->vertCount > gm->maxInstances) {
		GLuint vao;
		PCBuffer pcb;
		
		int new = nextPOT(gm->vertCount);
		gm->maxInstances = new;
		
		init_pcbuffer(&pcb, &vao, new);
		
		PCBuffer_free(&gm->instVB);
		glDeleteVertexArrays(1, &gm->vao);
		
		gm->vao = vao;
		gm->instVB = pcb;
	}
	

	GUIUnifiedVertex* vmem = PCBuffer_beginWrite(&gm->instVB);
	if(!vmem) {
		printf("attempted to update invalid PCBuffer in GUIManager\n");
		return;
	}

	size_t maxCnt = MIN(gm->vertCount, gm->maxInstances); 
	memcpy(vmem, gm->vertBuffer, maxCnt * sizeof(*gm->vertBuffer));


// 	if(mdi->uniformSetup) {
// 		(*mdi->uniformSetup)(mdi->data, progID);
// 	}
	
	glUniform1f(glGetUniformLocation(progID, "fontClipLow"), gm->fontClipLow);
	glUniform1f(glGetUniformLocation(progID, "fontClipHigh"), gm->fontClipHigh);
//printf("ul: %d\n", glGetUniformLocation(progID, "fontClipLow"));
//	glUniform1f(glGetUniformLocation(progID, "fontClipLow"), 0.45f);
//	glUniform1f(glGetUniformLocation(progID, "fontClipHigh"), 0.8f);
	glexit("");
	
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
	glDrawArrays(GL_POINTS, offset / sizeof(GUIUnifiedVertex), gm->vertCount);
	
	glexit("");
}



static void postFrame(void* gm_) {
	GUIManager* gm = (GUIManager*)gm_;
	PCBuffer_afterDraw(&gm->instVB);
	
	// reset the inter-frame event accumulators
//	VEC_TRUNC(&gm->keysReleased);
//	for(int i = 0; i < 16; i++) {
//		gm->mouseWentUp[i] = 0;
//		gm->mouseWentDown[i] = 0;
//	}
	gm->scrollDist = 0;
	gm->hotID = 0;
	
	
	// gc the element data
	for(int i = 0; i < VEC_LEN(&gm->elementData); i++) {
		GUIElementData* d;
	RESTART:
		d = &VEC_ITEM(&gm->elementData, i);
		
		if(d->age < 10) {
			d->age++;
			continue;
		}
		
		// free this entry
					
		if(d->freeFn) {
			d->freeFn(d->data);
			d->freeFn = 0;
		}
		
		if(i < VEC_LEN(&gm->elementData) - 1)  {
			*d = VEC_TAIL(&gm->elementData);
			VEC_LEN(&gm->elementData)--;
			goto RESTART;
		}
		else {
			VEC_LEN(&gm->elementData)--;
			break;
		}
	}
	
	
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
		prog = loadCombinedProgram(gm->gs->shaderPath);
		glexit("");
	}
	
	
	pd = Pass_allocDrawable("GUIManager");
	pd->data = gm;
	pd->preFrame = preFrame;
	pd->draw = draw;
	pd->postFrame = postFrame;
	pd->prog = prog;
	
	return pd;;
}





