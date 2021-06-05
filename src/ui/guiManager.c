

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#include "gui.h"
#include "gui_internal.h"





static void preFrame(PassFrameParams* pfp, void* gm_);
static void draw(void* gm_, GLuint progID, PassDrawParams* pdp);
static void postFrame(void* gm_);



GUIManager* GUIManager_alloc(GUI_GlobalSettings* gs) {
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



// _init is always called before _initGL
void GUIManager_init(GUIManager* gm, GUI_GlobalSettings* gs) {
	
	static struct gui_vtbl root_vt = {
		.UpdatePos = (void*)updatePosRoot,
		.Render = (void*)renderRoot,
// 		.HitTest = hitTestRoot,
	};
		
	static struct GUIEventHandler_vtbl event_vt = {
		.ParentResize = gui_default_ParentResize,
	};
	
	VEC_INIT(&gm->reapQueue);
	RING_INIT(&gm->focusStack, 32);
	
	gm->gs = gs;
	
	GUIManager_StartWorkerThread(gm);
	
//	gm->useSoftCursor = 1;
//	gm->softCursorName = "icon/folder";
//	gm->softCursorSize = (Vector2){50,50};
	
	gm->maxInstances = gs->maxInstances;
	
	gm->elementCount = 0;
	gm->elementAlloc = 2048;
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
	
	RING_PUSH(&gm->focusStack, gm->root);
	
	gm->nextCmdFlagBit = 0;
	HT_init(&gm->cmdElementLookup, 32);
	HT_init(&gm->cmdModeLookup, 32);
//	HT_init(&gm->cmdNameLookup, 256);
	HT_init(&gm->cmdFlagLookup, 32);
	VEC_INIT(&gm->cmdList);
	
	
	// set up commands
	
	char* flag_names[] = {
		"resetCursorBlink",
		NULL,
	};
	
	for(int i = 0; flag_names[i]; i++) 
		GUIManager_AddCommandFlag(gm, flag_names[i]);
	
	
	struct {char* n; uint16_t id;} elem_names[] = {
	#define X(a) { #a, GUIELEMENT_##a },
	GUI_ELEMENT_LIST
		{NULL, 0},
	#undef X
	};
	
	for(int i = 0; elem_names[i].n; i++) 
		GUIManager_AddCommandElement(gm, elem_names[i].n, elem_names[i].id);
	
	
	struct {char* en; char* n; uint32_t id;} cmd_names[] = {
	#define X(a, b) { #a, #b, GUICMD_##a##_##b },
	GUI_COMMAND_LIST
		{NULL, NULL, 0},
	#undef X
	};
	
	for(int i = 0; cmd_names[i].n; i++) 
		GUIManager_AddCommand(gm, cmd_names[i].en, cmd_names[i].n, cmd_names[i].id);
	
	
	
	
	gm->defaults.font = FontManager_findFont(gm->fm, "Arial");
	gm->defaults.fontSize = .45;
	decodeHexColorNorm(gs->textColor, (float*)&(gm->defaults.textColor));

	decodeHexColorNorm(gs->buttonTextColor, (float*)&(gm->defaults.buttonTextColor));
	decodeHexColorNorm(gs->buttonHoverTextColor, (float*)&(gm->defaults.buttonHoverTextColor));
	decodeHexColorNorm(gs->buttonDisTextColor, (float*)&(gm->defaults.buttonDisTextColor));
	decodeHexColorNorm(gs->buttonBgColor, (float*)&(gm->defaults.buttonBgColor));
	decodeHexColorNorm(gs->buttonHoverBgColor, (float*)&(gm->defaults.buttonHoverBgColor));
	decodeHexColorNorm(gs->buttonDisBgColor, (float*)&(gm->defaults.buttonDisBgColor));
	decodeHexColorNorm(gs->buttonBorderColor, (float*)&(gm->defaults.buttonBorderColor));
	decodeHexColorNorm(gs->buttonHoverBorderColor, (float*)&(gm->defaults.buttonHoverBorderColor));
	decodeHexColorNorm(gs->buttonDisBorderColor, (float*)&(gm->defaults.buttonDisBorderColor));

	decodeHexColorNorm(gs->editBorderColor, (float*)&(gm->defaults.editBorderColor));
	decodeHexColorNorm(gs->editBgColor, (float*)&(gm->defaults.editBgColor));
	decodeHexColorNorm(gs->editTextColor, (float*)&(gm->defaults.editTextColor));
	decodeHexColorNorm(gs->editSelBgColor, (float*)&(gm->defaults.editSelBgColor));
	gm->defaults.editWidth = 150;
	gm->defaults.editHeight = 18;

	decodeHexColorNorm(gs->cursorColor, (float*)&(gm->defaults.cursorColor));

	decodeHexColorNorm(gs->tabTextColor, (float*)&(gm->defaults.tabTextColor));
	decodeHexColorNorm(gs->tabBorderColor, (float*)&(gm->defaults.tabBorderColor));
	decodeHexColorNorm(gs->tabActiveBgColor, (float*)&(gm->defaults.tabActiveBgColor));
	decodeHexColorNorm(gs->tabHoverBgColor, (float*)&(gm->defaults.tabHoverBgColor));
	decodeHexColorNorm(gs->tabBgColor, (float*)&(gm->defaults.tabBgColor));

	decodeHexColorNorm(gs->outlineCurrentLineBorderColor, (float*)&(gm->defaults.outlineCurrentLineBorderColor));
	decodeHexColorNorm(gs->selectedItemTextColor, (float*)&(gm->defaults.selectedItemTextColor));
	decodeHexColorNorm(gs->selectedItemBgColor, (float*)&(gm->defaults.selectedItemBgColor));

	decodeHexColorNorm(gs->windowBgBorderColor, (float*)&(gm->defaults.windowBgBorderColor));
	gm->defaults.windowBgBorderWidth = 1;
	decodeHexColorNorm(gs->windowBgColor, (float*)&(gm->defaults.windowBgColor));
	decodeHexColorNorm(gs->windowTitleBorderColor, (float*)&(gm->defaults.windowTitleBorderColor));
	gm->defaults.windowTitleBorderWidth = 1;
	decodeHexColorNorm(gs->windowTitleColor, (float*)&(gm->defaults.windowTitleColor));
	decodeHexColorNorm(gs->windowTitleTextColor, (float*)&(gm->defaults.windowTitleTextColor));
	decodeHexColorNorm(gs->windowCloseBtnBorderColor, (float*)&(gm->defaults.windowCloseBtnBorderColor));
	gm->defaults.windowCloseBtnBorderWidth = 1;
	decodeHexColorNorm(gs->windowCloseBtnColor, (float*)&(gm->defaults.windowCloseBtnColor));
	decodeHexColorNorm(gs->windowScrollbarColor, (float*)&(gm->defaults.windowScrollbarColor));
	decodeHexColorNorm(gs->windowScrollbarBorderColor, (float*)&(gm->defaults.windowScrollbarBorderColor));
	gm->defaults.windowScrollbarBorderWidth = 1;

	decodeHexColorNorm(gs->selectBgColor, (float*)&(gm->defaults.selectBgColor));
	decodeHexColorNorm(gs->selectBorderColor, (float*)&(gm->defaults.selectBorderColor));
	decodeHexColorNorm(gs->selectTextColor, (float*)&(gm->defaults.selectTextColor));
	
	decodeHexColorNorm(gs->trayBgColor, (float*)&(gm->defaults.trayBgColor));
	
	gm->defaults.charWidth_fw = gs->charWidth_fw;
	gm->defaults.lineHeight_fw = gs->lineHeight_fw;
	gm->defaults.font_fw = FontManager_findFont(gm->fm, gs->font_fw);;
	gm->defaults.fontSize_fw = gs->fontSize_fw;
	decodeHexColorNorm(gs->statusBarBgColor, (float*)&(gm->defaults.statusBarBgColor));
	decodeHexColorNorm(gs->statusBarTextColor, (float*)&(gm->defaults.statusBarTextColor));
	decodeHexColorNorm(gs->fileBrowserHeaderTextColor, (float*)&(gm->defaults.fileBrowserHeaderTextColor));
	decodeHexColorNorm(gs->fileBrowserHeaderBorderColor, (float*)&(gm->defaults.fileBrowserHeaderBorderColor));
	decodeHexColorNorm(gs->fileBrowserHeaderBgColor, (float*)&(gm->defaults.fileBrowserHeaderBgColor));
	
	gm->defaults.selectSize = (Vector2){80, 25};
	
	gm->defaultCursor = GUIMOUSECURSOR_ARROW;
	
	json_file_t* jsf = json_load_path("/etc/gpuedit/templates.json");
	// TODO: leaked file structure
	gm->templates = jsf->root;
	
}


void GUIManager_initGL(GUIManager* gm) {
	static VAOConfig vaoConfig[] = {
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // top, left, bottom, right
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // tlbr clipping planes
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_FALSE}, // tex indices 1&2, tex fade, gui type
		{0, 4, GL_UNSIGNED_SHORT, 0, GL_TRUE}, // tex offset 1&2
		{0, 4, GL_UNSIGNED_SHORT, 0, GL_TRUE}, // tex size 1&2
		
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_TRUE}, // fg color
		{0, 4, GL_UNSIGNED_BYTE, 0, GL_TRUE}, // bg color
		
		{0, 4, GL_FLOAT, 0, GL_FALSE}, // z-index, alpha, opts 1-2
		
		{0, 0, 0, 0, 0}
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


GUIHeader* GUIManager_triggerClick(GUIManager* gm, Vector2 testPos) {
	GUIHeader* go = GUIManager_hitTest(gm, testPos);
	
	if(go) GUIHeader_triggerClick(go, testPos);
	
	return go;
}


GUIHeader* GUIManager_hitTest(GUIManager* gm, Vector2 testPos) {
	GUIHeader* go = GUIHeader_hitTest(gm->root, testPos);
	return go == gm->root ? NULL : go;
}


void GUIManager_updatePos(GUIManager* gm, PassFrameParams* pfp) {
	GUIHeader_updatePos(gm->root, NULL, pfp);
}




void GUIManager_Reap(GUIManager* gm) {
	if(VEC_LEN(&gm->reapQueue) == 0) return;
	/*
	printf("\nReap Queue:\n");
	VEC_EACH(&gm->reapQueue, i, h) {
		printf("  %d > %p %s\n", i, h, h->deleted?"del":"BAD");
	}
	*/
	#define check_nullfiy(n) if(gm->n && gm->n->deleted) gm->n = NULL;
	check_nullfiy(lastHoveredObject)
	check_nullfiy(dragStartTarget)
	#undef check_nullify
	
	
	if(gm->modalBackdrop && gm->modalBackdrop->header.deleted) {
		gm->modalBackdrop = NULL;
	}
	
	GUIHeader* head = GUIManager_getFocusedObject(gm);
	
	
/*	printf("\nreap Focus Stack:\n");
	RING_EACH(&gm->focusStack, i, o) {
		printf( "  %d > %p\n", i, o);
	}*/
	
RESTART:
	RING_EACH(&gm->focusStack, i, fh) {
		//printf( "  .. [%d] checking FS: %p", i, fh);
		// BUG modifying the ring causes the next element to be skipped
		// shouldn't be a problem here, but it might be
		if(fh->deleted) {
			//printf(" -> deleting, restart search \n");
			RING_RM(&gm->focusStack, i);
			goto RESTART; // shitty hack for getting around modifying the array
		}
		//printf(" -> fine\n");
	}
	
	
	GUIHeader* head2 = GUIManager_getFocusedObject(gm);
	//if(VEC_LEN(&gm->reapQueue)) printf("head1: %p, new head: %p \n", head, head2);
	
	// GeinedFocus event for the new top of the stack if the old one was deleted
	if(head != head2) {
		GUIEvent gev = {};
		gev.type = GUIEVENT_GainedFocus;
		gev.originalTarget = head2;
		gev.currentTarget = head2;
	
		GUIManager_BubbleEvent(gm, head2, &gev);	
	}
	
	VEC_EACH(&gm->reapQueue, i, h) {
	//	printf("Reaping %p\n", h);
		GUIHeader_Reap(h);
	}
	
	VEC_TRUNC(&gm->reapQueue);
}


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

void GUIManager_copyElements(GUIManager* gm, GUIUnifiedVertex* elems, int count) {
	GUIManager_checkElemBuffer(gm, count);
	memcpy(gm->elemBuffer + gm->elementCount, elems, count * sizeof(*gm->elemBuffer));
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


void GUIManager_HandleMouseMove(GUIManager* gm, InputState* is, InputEvent* iev) {
	Vector2 newPos = {
		iev->intPos.x, iev->intPos.y
	};
	
	gm->lastMousePos = newPos;
	
	// find the deepest target
	GUIHeader* t = GUIManager_hitTest(gm, newPos);
	if(t && t->deleted) {
		printf("Event target is deleted\n");
		return;
	}
	
	GUIEvent gev = {
		.type = GUIEVENT_MouseMove,
		.originalTarget = NULL,
		.currentTarget = NULL,
		.eventTime = 0,
		.pos = newPos,
		.character = 0, // N/A
		.keycode = 0, // N/A
		.modifiers = translateModKeys(gm, iev), 
		.cancelled = 0,
		.requestRedraw = 0,
	};
	
	
	if(!t) { // mouse left the window
		if(gm->lastHoveredObject) {
			gev.type = GUIEVENT_MouseLeave;
			gev.originalTarget = (GUIHeader*)gm->lastHoveredObject;
			gev.currentTarget = (GUIHeader*)gm->lastHoveredObject;
			gev.cancelled = 0;
			GUIManager_BubbleEvent(gm, (GUIHeader*)gm->lastHoveredObject, &gev);
			
			gm->lastHoveredObject = NULL;
		}
		
		// still send move events 
		if(gm->isDragging) {
			gev.type = GUIEVENT_DragMove;
			gev.originalTarget = (GUIHeader*)gm->dragStartTarget;
			gev.currentTarget = (GUIHeader*)gm->dragStartTarget;
			gev.dragStartPos = gm->dragStartPos;
			gev.cancelled = 0;
			GUIManager_BubbleEvent(gm, (GUIHeader*)gm->dragStartTarget, &gev);
		}
		
		return;
	}
	
	
	// set the cursor, maybe
	int cur = t->cursor;
	if(cur == GUIMOUSECURSOR_DYNAMIC) {
		cur = GUIHeader_ChooseCursor(t, newPos);
	}
	GUIManager_SetCursor(gm, cur);

	// mouse enter/leave
	if(!gm->lastHoveredObject) {
		gev.type = GUIEVENT_MouseEnter;
		gev.originalTarget = t;
		gev.currentTarget = t;
		gev.cancelled = 0;
		GUIManager_BubbleEvent(gm, t, &gev);
	}
	else if((GUIHeader*)gm->lastHoveredObject != t) {
		gev.type = GUIEVENT_MouseLeave;
		gev.originalTarget = (GUIHeader*)gm->lastHoveredObject;
		gev.currentTarget = (GUIHeader*)gm->lastHoveredObject;
		gev.cancelled = 0;
		GUIManager_BubbleEvent(gm, (GUIHeader*)gm->lastHoveredObject, &gev);
		
		gev.type = GUIEVENT_MouseEnter;
		gev.originalTarget = t;
		gev.currentTarget = t;
		gev.cancelled = 0;
		GUIManager_BubbleEvent(gm, t, &gev);
	}
	
	gm->lastHoveredObject = (GUIHeader*)t;
	
	// check for dragging
	if(gm->isMouseDown && !gm->isDragging && gm->dragStartTarget) {
		float dragDist = vDist2p(&newPos, &gm->dragStartPos);
		if(dragDist >= gm->minDragDist) {
			gm->isDragging = 1;
			
			gev.button = gm->dragButton;
			gev.type = GUIEVENT_DragStart;
			gev.originalTarget = (GUIHeader*)gm->dragStartTarget;
			gev.currentTarget = t;
			gev.dragStartPos = gm->dragStartPos;
			gev.cancelled = 0;
			GUIManager_BubbleEvent(gm, (GUIHeader*)gm->dragStartTarget, &gev);
		};
	}
	
	// DragMove event
	if(gm->isDragging) {
		gev.type = GUIEVENT_DragMove;
		gev.originalTarget = (GUIHeader*)gm->dragStartTarget;
		gev.currentTarget = t;
		gev.dragStartPos = gm->dragStartPos;
		gev.cancelled = 0;
		GUIManager_BubbleEvent(gm, (GUIHeader*)gm->dragStartTarget, &gev);
	}
	
	gev.type = GUIEVENT_MouseMove;
	gev.originalTarget = t;
	gev.currentTarget = t;
	gev.cancelled = 0;
	GUIManager_BubbleEvent(gm, t, &gev);
	
	// TODO: handle redraw request
}

void GUIManager_HandleMouseClick(GUIManager* gm, InputState* is, InputEvent* iev) {
	
	Vector2 newPos = {
		iev->intPos.x, iev->intPos.y
	};
	
	// find the deepest target
	GUIHeader* t = GUIManager_hitTest(gm, newPos);
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
		if(!gm->isDragging) {
			gm->dragStartPos = newPos;
			gm->dragStartTarget = (GUIHeader*)t;
			gm->dragButton = iev->button;
			gev.dragStartPos = gm->dragStartPos;
		}
	} else if(iev->type == EVENT_MOUSEUP) {
		char suppressClick = 0;
		
		// check for drag end
		if(gm->isDragging && iev->button == gm->dragButton) {
			gev.type = GUIEVENT_DragStop,
			gev.currentTarget = t,
			gev.cancelled = 0;
			gev.button = gm->dragButton;
			gev.dragStartPos = gm->dragStartPos;
			GUIManager_BubbleEvent(gm, t, &gev);
			
			gev.currentTarget = (GUIHeader*)gm->dragStartTarget,
			gev.cancelled = 0;
			GUIManager_BubbleEvent(gm, (GUIHeader*)gm->dragStartTarget, &gev);
			
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
	
	GUIHeader* t = GUIManager_getFocusedObject(gm);
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
void GUIManager_BubbleEvent(GUIManager* gm, GUIHeader* target, GUIEvent* gev) {
	GUIHeader* obj = target;
	if(target->deleted) return;
	
	int bubble = GUIEventBubbleBehavior[gev->type];
	
	if(bubble == 0) {
		// no bubbling, just the target
		GUIHeader_TriggerEvent(obj, gev);
	}
	else if(bubble == 1) {
		// bubble until cancelled
		//printf("\n");
		while(obj && !gev->cancelled) {
			if(obj->deleted) break;
			//printf("bubbling %p, %s\n", obj, obj->header.name);
			gev->currentTarget = obj;
			GUIHeader_TriggerEvent(obj, gev);
			
			obj = obj->parent;
		}
	}
	else if(bubble == 2) {
		// trigger on all parents
		while(obj) {
			gev->currentTarget = obj;
			GUIHeader_TriggerEvent(obj, gev);
			
			obj = obj->parent;
		}
	}
	else {
		fprintf(stderr, "!!! unknown bubbling behavior: %d\n", bubble);
	}
	
}


void GUIManager_BubbleUserEvent(GUIManager* gm, GUIHeader* target, char* ev) {
	GUIEvent gev = {};
	gev.type = GUIEVENT_User;
	gev.eventTime = 0; //gev->eventTime;
	gev.originalTarget = target;
	gev.currentTarget = target;
	gev.cancelled = 0;
	// handlers are responsible for cleanup
	gev.userData = NULL;
	gev.userSize = NULL;
	
	gev.userType = ev;

	GUIManager_BubbleEvent(gm, target, &gev);
}


// lowest level of event triggering
// does not do bubbling
void GUIHeader_TriggerEvent(GUIHeader* o, GUIEvent* gev) {
	
	
	if(o && o->vt && o->vt->HandleCommand && gev->type == GUIEVENT_KeyDown) {
		
		GUI_Cmd* cmd = Commands_ProbeCommand(o, gev);
		
		if(cmd) {	
//			printf("handling command\n");	
			o->vt->HandleCommand(o, cmd);
			gev->cancelled = 1;
			// todo: check no-suppress flag
			return;
		}
	}
	
	
	
	
	if(o && o->event_vt) {
		
		switch(gev->type) {
			#define X(name, b) case GUIEVENT_##name: \
					if(o->event_vt && o->event_vt->name) (*o->event_vt->name)((GUIHeader*)o, gev); \
					break;
				
				GUIEEVENTTYPE_LIST
			#undef X
		}
		
		if(gev->cancelled) return;
		
		// BUG: check for cancelled event?
		if(o->event_vt && o->event_vt->Any) (*o->event_vt->Any)((GUIHeader*)o, gev);
		
		
		VEC_EACH(&o->dynamicHandlers, i, hand) {
			if(gev->cancelled) return;
			if(hand.type == gev->type || hand.type == GUIEVENT_Any) {
				if(hand.cb) hand.cb((GUIHeader*)o, gev);
			}
		}
	}
	
	
	// BUG Definitely the wrong place for this: process tab stops
	if((o->flags & GUI_CHILD_TABBING) && !gev->cancelled) {
		// TODO: unhardcode
		if(gev->type == GUIEVENT_KeyDown) {
			// TODO: normalize the tab key code, or read from file
			if(gev->keycode == XK_Tab || gev->keycode == XK_ISO_Left_Tab) {
				GUIHeader_NextTabStop(o);
				gev->cancelled = 1;
			}
			
		}
	}
}


/*
NOTE:
These functions are for temporal, dynamic event handlers that must be added
and removed at run time.

They are NOT for general event handling in GUI elements. Use the vtable for that.
*/
void GUIHeader_AddHandler(GUIHeader* h, enum GUIEventType type, GUI_EventHandlerFn cb) {
	VEC_INC(&h->dynamicHandlers);
	VEC_TAIL(&h->dynamicHandlers).type = type;
	VEC_TAIL(&h->dynamicHandlers).cb = cb;
}

void GUIHeader_RemoveHandler_(GUIHeader* h, enum GUIEventType type, GUI_EventHandlerFn cb) {
	VEC_EACH(&h->dynamicHandlers, i, hand) {
		if(hand.type == type && hand.cb == cb) {
			VEC_RM(&h->dynamicHandlers, i);
			return;
		}
	}
}


// focus stack functions

GUIHeader* GUIManager_getFocusedObject(GUIManager* gm) {
	if(RING_LEN(&gm->focusStack) == 0) return NULL;
	return RING_TAIL(&gm->focusStack);
}

GUIHeader* GUIManager_popFocusedObject(GUIManager* gm) {
	GUIHeader* old = NULL, *new;
	GUIEvent gev = {};
	
	// can't pop off the root element at the bottom
	if(RING_LEN(&gm->focusStack) <= 1) return RING_TAIL(&gm->focusStack);
	
	RING_POP(&gm->focusStack, old);
	
	//printf("popping object %p from focus stack\n", old);
	
	gev.type = GUIEVENT_LostFocus;
	gev.originalTarget = (GUIHeader*)old;
	gev.currentTarget = (GUIHeader*)old;
	
	GUIManager_BubbleEvent(gm, (GUIHeader*)old, &gev);
	
	new = RING_TAIL(&gm->focusStack);
	//printf("  -> %p now head of stack\n", new);
	
	gev.type = GUIEVENT_GainedFocus;
	gev.originalTarget = (GUIHeader*)new;
	gev.currentTarget = (GUIHeader*)new;
	
	GUIManager_BubbleEvent(gm, (GUIHeader*)new, &gev);
	
	return old;
}


void GUIManager_pushFocusedObject(GUIManager* gm, GUIHeader* h) {
	GUIHeader* old = (GUIHeader*)GUIManager_getFocusedObject(gm);
	GUIEvent gev = {};
	
	if(old == h) {
	//	printf("refocusing existing focused object: %p \n", h);
		return;
	}
	
	//printf("old focused obj: %p\n", old);
	if(old) {
		gev.type = GUIEVENT_LostFocus;
		gev.originalTarget = (GUIHeader*)old;
		gev.currentTarget = (GUIHeader*)old;
	
		GUIManager_BubbleEvent(gm, (GUIHeader*)old, &gev);
	}
	
	RING_PUSH(&gm->focusStack, (GUIHeader*)h);
	/*
	printf("\nFocus Stack:\n");
	RING_EACH(&gm->focusStack, i, o) {
		printf( "  %d > %p\n", i, o);
	}
	*/
	gev.type = GUIEVENT_GainedFocus;
	gev.originalTarget = (GUIHeader*)h;
	gev.currentTarget = (GUIHeader*)h;
	
	GUIManager_BubbleEvent(gm, (GUIHeader*)h, &gev);
}



// Rendering



static int gui_elem_sort_fn(GUIUnifiedVertex* a, GUIUnifiedVertex* b) {
	return a->z == b->z ? 0 : (a->z > b->z ? 1 : -1);
}


static void preFrame(PassFrameParams* pfp, void* gm_) {
	GUIManager* gm = (GUIManager*)gm_;
	
	GUIUnifiedVertex* vmem = PCBuffer_beginWrite(&gm->instVB);
	if(!vmem) {
		printf("attempted to update invalid PCBuffer in GUIManager\n");
		return;
	}

	double sort;
	double time;
	double total = 0.0;
//	printf("\n");
	
	sort = getCurrentTime();
	
	
	gm->elementCount = 0;
	gm->root->size = (Vector2){pfp->dp->targetSize.x, pfp->dp->targetSize.y};
	gm->root->absClip = (AABB2){0,0, pfp->dp->targetSize.x, pfp->dp->targetSize.y};
	
	GUIRenderParams grp = {
		.offset = {0,0}, 
		.size = gm->root->size,
		.clip = gm->root->absClip,
	};
	
#define printf(...)
	
	GUIHeader_updatePos(gm->root, &grp, pfp);
	time = timeSince(sort);
	total += time;
	printf("updatePos time: %fus\n", time  * 1000000.0);
	
	sort = getCurrentTime();
	
	GUIHeader_render(gm->root, pfp);
	time = timeSince(sort);
	total += time;
	printf("render time: %fus\n", time  * 1000000.0);

	if(gm->useSoftCursor) {
		TextureAtlasItem* it;
		if(HT_get(&gm->ta->items, gm->softCursorName, &it)) {
			printf("could not find gui image '%s'\n", name);
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

	
// 	static size_t framecount = 0;
	
	
 	sort = getCurrentTime();
// 	gui_debugFileDumpVertexBuffer(gm, "/tmp/gpuedit/presortdump", framecount);
	qsort(gm->elemBuffer, gm->elementCount, sizeof(*gm->elemBuffer), (void*)gui_elem_sort_fn);
 	time = timeSince(sort);
	total += time;
	printf("qsort time: %fus\n", time  * 1000000.0);
	
// 	gui_debugFileDumpVertexBuffer(gm, "/tmp/gpuedit/framedump", framecount);
	
// 	printf("Elemcount: %ld\n", gm->elementCount);
	
// 	framecount++;
 	sort = getCurrentTime();
	memcpy(vmem, gm->elemBuffer, gm->elementCount * sizeof(*gm->elemBuffer));
	 time = timeSince(sort);
	total += time;
	printf("memcpy time: %fus\n", time  * 1000000.0);
	printf("total time: %fus\n", total  * 1000000.0);
#undef printf
}

static void draw(void* gm_, GLuint progID, PassDrawParams* pdp) {
	GUIManager* gm = (GUIManager*)gm_;
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



static void postFrame(void* gm_) {
	GUIManager* gm = (GUIManager*)gm_;
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
	pd->draw = draw;
	pd->postFrame = postFrame;
	pd->prog = prog;
	
	return pd;;
}



int GUIManager_SpawnModal(GUIManager* gm, GUIHeader* obj) {
	if(gm->modalRoot || gm->modalRoot) return 1;
	
	GUIWindow* bd  = GUIWindow_New(gm);
	bd->color = (struct Color4){0,0,0,.7};
	gm->modalBackdrop = bd;
	bd->header.flags |= GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
	bd->header.z = 999999999; // nine nines
		
	GUIHeader_RegisterObject(gm->root, &gm->modalBackdrop->header);
	GUIHeader_RegisterObject(&gm->modalBackdrop->header, obj);
	
	GUIManager_pushFocusedObject(gm, obj);
	
	return 0;
}


int GUIManager_DestroyModal(GUIManager* gm) {
	GUI_Delete(gm->modalBackdrop);
	return 0;
}



GUIHeader* GUIManager_SpawnTemplate(GUIManager* gm, char* name) {
	json_value_t* v;
	
	json_obj_get_key(gm->templates, name, &v);
	if(!v) return NULL;
	
	return GUICL_CreateFromConfig(gm, v);
}


static void* gui_worker_thread_fn(void* _gm) {
	GUIManager* gm = (GUIManager*)_gm;
	GUIWorkerJob* job;
	
	while(1) {
		job = GUIManager_PopJob(gm);
		if(job) {
			job->fn(job->owner, job->data, &job->pctCompleteHint);
		}
	}
	
	return NULL;
}

void GUIManager_StartWorkerThread(GUIManager* gm) {
	
	pthread_attr_t attr;

	// set up the support stuctures
	pthread_mutex_init(&gm->workerQueueMutex, NULL);
	sem_init(&gm->workerWhip, 0, 0);
	
	// start the thread
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	int ret = pthread_create(&gm->workerThread, &attr, gui_worker_thread_fn, gm);
	if(ret) {
		fprintf(stderr, "Failed to create GUI worker thread\n");
		
		return;
	}
	
}

float* GUIManager_EnqueueJob(GUIManager* gm, GUIHeader* owner, GUI_WorkerFn fn, void* data) {
	
	GUIWorkerJob* job = pcalloc(job);
	job->owner = owner;
	job->fn = fn;
	job->data = data;
	atomic_flag_clear(&job->done);
	
	
	pthread_mutex_lock(&gm->workerQueueMutex);
	
	if(gm->workerQueueHead == NULL) {
		gm->workerQueueHead = job;
		gm->workerQueueTail = job;
	}
	else {
		gm->workerQueueTail->next = job;
		gm->workerQueueTail = job;
	}
	
	pthread_mutex_unlock(&gm->workerQueueMutex);
	
	sem_post(&gm->workerWhip);
	
	return &job->pctCompleteHint;
}


GUIWorkerJob* GUIManager_PopJob(GUIManager* gm) {
	GUIWorkerJob* job = NULL;
	
	sem_wait(&gm->workerWhip);
	
	pthread_mutex_lock(&gm->workerQueueMutex);
	
	if(gm->workerQueueHead != NULL) {
		job = gm->workerQueueHead;
		
		// last item
		if(gm->workerQueueHead == gm->workerQueueTail) {
			gm->workerQueueHead = NULL;
			gm->workerQueueTail = NULL;
		}
		else {
			gm->workerQueueHead = job->next;
		}
	}
	
	pthread_mutex_unlock(&gm->workerQueueMutex);
	
	return job;
}
