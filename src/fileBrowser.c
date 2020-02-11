


#include "sti/sti.h"

#include "fileBrowser.h"




static void render(GUIFileBrowser* w, PassFrameParams* pfp) {
// HACK
// 	GUIBufferEditor_Draw(w, w->header.gm, w->scrollLines, + w->scrollLines + w->linesOnScreen + 2, 0, 100);
	
// 	if(w->lineNumTypingMode) {
// 		GUIHeader_render(&w->lineNumEntryBox->header, pfp); 
// 	}
	
// 	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIFileBrowser* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	gui_defaultUpdatePos(w, grp, pfp);
	Buffer* b = w->buffer;
	
	// cursor blink
	float t = w->cursorBlinkOnTime + w->cursorBlinkOffTime;
	w->cursorBlinkTimer = fmod(w->cursorBlinkTimer + pfp->timeElapsed, t);
	
	w->sbMinHeight = 20;
	// scrollbar position calculation
	// calculate scrollbar height
	float wh = w->header.size.y;
	float sbh = fmax(wh / (b->numLines - w->linesOnScreen), w->sbMinHeight);
	
	// calculate scrollbar offset
	float sboff = ((wh - sbh) / b->numLines) * (w->scrollLines);
	
	GUIResize(w->scrollbar, (Vector2){10, sbh});
	w->scrollbar->header.topleft.y = sboff;
}




GUIFileBrowser* GUIFileBrowser_New(GUIManager* gm);

	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
// 		.KeyDown = keyDown,
// 		.Click = click,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
// 		.ParentResize = parentResize,
	};
	
	
	GUIFileBrowser* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	w->header.cursor = GUIMOUSECURSOR_TEXT;
	
	w->scrollbar = GUIWindow_New(gm);
	GUIResize(w->scrollbar, (Vector2){10, 50});
	w->scrollbar->color = (Vector){.9,.9,.9};
	w->scrollbar->header.z = 100;
	w->scrollbar->header.gravity = GUI_GRAV_TOP_RIGHT;
	
	GUIRegisterObject(w->scrollbar, w);
	
	w->curDir = strdup(".");
	
	// HACK
	w->linesPerScrollWheel = 3;
	w->cursorBlinkOnTime = 0.6;
	w->cursorBlinkOffTime = 0.6;
	
	return w;
}

int read_dir_cb(char* fullpath, char* filename, void* _w) {
	GUIFileBrowser* w = (GUIFileBrowser*)_w;
	
	VEC_INC(&w->entries);
	GUIFileBrowserEntry* e = &VEC_TAIL(&w->entries);
	
	e->name = strdup(filename);
	
	
	return 0;
}


void GUIFileBrowser_Refresh(GUIFileBrowser* w) {
	
	for(int i = 0; i < VEC_LEN(&w->entries), i++) {
		GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, i);
		if(e->name) {
			free(e->name);
			e->name = 0;
		}
	}
	
	VEC_TRUNC(&w->entries);
	
	recurseDirs(w->curDir, read_dir_cb, w, 0, 0);
	
}

void GUIFileBrowser_SetDir(GUIFileBrowser* w, char* dir) {
	if(w->curDir) free(w->curDir);
	
	curDir = strdup(dir);
	
	GUIFileBrowser_Refresh(w);
}

