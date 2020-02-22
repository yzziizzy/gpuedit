
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


#include <sys/types.h> // opendir
#include <dirent.h> // readdir
#include <unistd.h> // pathconf
#include <sys/stat.h>

#include "common_math.h"
#include "common_gl.h"


#include "sti/sti.h"

#include "fileBrowser.h"

#include "gui_internal.h"




static void render(GUIFileBrowser* w, PassFrameParams* pfp) {
	GUIManager* gm = w->header.gm;
	
	Vector2 tl = w->header.absTopLeft;
	
// 	drawTextLine();
	
	float gutter = 35;
	
	for(intptr_t i = w->scrollOffset; i < VEC_LEN(&w->entries); i++) {
		
		GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, i);
		
		AABB2 box;
		box.min.x = tl.x + gutter;
		box.min.y = tl.y + (25 * i);
		box.max.x = tl.x + 800;
		box.max.y = tl.y + (25 * (i + 1));
		
		
		if(e->isSelected) { // backgrounds for selected items
			struct Color4* color = &gm->defaults.tabActiveBgColor;
			
			GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
			*v = (GUIUnifiedVertex){
				.pos = {box.min.x, box.min.y, box.max.x, box.max.y},
				.clip = {0, 0, 800, 800},
				
				.guiType = 0, // window (just a box)
				
				.fg = *color, // TODO: border color
				.bg = *color, // TODO: color
				
				.z = /*w->header.z +*/ 1000,
				.alpha = 1,
			};
		}
		
		
		char* iconame;
		if(e->type == 1) iconame = "icon/document";
		else if(e->type == 2) iconame = "icon/folder";
		
		TextureAtlasItem* it;
		if(HT_get(&gm->ta->items, iconame, &it)) {
			printf("could not find gui image '%s' %p \n", "icon_document");
		}
		else {
			// icon
			GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
			*v = (GUIUnifiedVertex){
				.pos = {tl.x +10, box.min.y, tl.x +10+20, box.min.y + 20},
				.clip = {0, 0, 800, 800},
				
				.guiType = 0, // window (just a box)
				
				.texIndex1 = it->index,
				.texIndex2 = 0,
				.texFade = .5,
				
				.guiType = 2, // simple image
				
				.texOffset1 = { it->offsetNorm.x * 65535, it->offsetNorm.y * 65535 },
		// 		.texOffset1 = { .1 * 65535, .1 * 65535 },
				.texOffset2 = 0,
				.texSize1 = { it->sizeNorm.x * 65535, it->sizeNorm.y * 65535 },
		// 		.texSize1 = { .5 * 65535, .5 * 65535 },
				.texSize2 = 0,
				
				.fg = {255,255,255,255},
				.bg = {255,255,255,255},
				
				.z = /*w->header.z +*/ 1000,
				.alpha = 1,
			};
		}
		
		gui_drawDefaultUITextLine(gm, &box, &gm->defaults.tabTextColor , 10000000, e->name, strlen(e->name));
		
	}

	// cursor
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	*v = (GUIUnifiedVertex){
		.pos = {
			tl.x + gutter, 
			tl.y + w->cursorIndex * 25, 
			tl.x + 800,
			tl.y + (w->cursorIndex + 1) * 25
		},
		.clip = {0, 0, 800, 800},
		.texIndex1 = 1, // order width
		.guiType = 4, // bordered window (just a box)
		.fg = gm->defaults.tabActiveBgColor, // border color
		.bg = {0,0,0,0},
		.z = .75,
		.alpha = 1.0,
	};

	
	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIFileBrowser* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	gui_defaultUpdatePos(w, grp, pfp);
	
// 	// cursor blink
// 	float t = w->cursorBlinkOnTime + w->cursorBlinkOffTime;
// 	w->cursorBlinkTimer = fmod(w->cursorBlinkTimer + pfp->timeElapsed, t);
// 	
// 	w->sbMinHeight = 20;
// 	// scrollbar position calculation
// 	// calculate scrollbar height
// 	float wh = w->header.size.y;
// 	float sbh = fmax(wh / (b->numLines - w->linesOnScreen), w->sbMinHeight);
// 	
// 	// calculate scrollbar offset
// 	float sboff = ((wh - sbh) / b->numLines) * (w->scrollLines);
// 	
// 	GUIResize(w->scrollbar, (Vector2){10, sbh});
// 	w->scrollbar->header.topleft.y = sboff;
}


static void keyUp(GUIObject* w_, GUIEvent* gev) {
	GUIFileBrowser* w = (GUIFileBrowser*)w_;
	
	if(gev->keycode == XK_Down) {
		w->cursorIndex = (w->cursorIndex + 1) % VEC_LEN(&w->entries);
	}
	else if(gev->keycode == XK_Up) {
		w->cursorIndex = (w->cursorIndex - 1) % VEC_LEN(&w->entries);
	}
	else if(gev->keycode == XK_Return) {
		
		// collect a list of files
		intptr_t n = 0;
		char** files = malloc(sizeof(*files) * (w->numSelected + 1));
		for(size_t i = 0; i < VEC_LEN(&w->entries); i++) {
			GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, i);
			if(!e->isSelected) continue;
			files[n++] = pathJoin(w->curDir, e->name);
			
			e->isSelected = 0; // unselect them 
		}
		files[n] = 0;
		
		
		if(w->onChoose) w->onChoose(w->onChooseData, files, n);
		
		// clean up
		char** ff = files;
		while(*ff) {
			free(*ff);
			ff++;
		}
		free(files);
		
	}
	else if(gev->keycode == XK_space) {
		GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, w->cursorIndex);
		if(e->isSelected) {
			e->isSelected = 0;
			w->numSelected--;
		}
		else {
			e->isSelected = 1;
			w->numSelected++;
		}
	}
	
}


GUIFileBrowser* GUIFileBrowser_New(GUIManager* gm, char* path) {

	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.KeyUp = keyUp,
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
	
	w->curDir = strdup(path);
	
	GUIFileBrowser_Refresh(w);
	
	return w;
}

void GUIFileBrowser_Destroy(GUIFileBrowser* w) {
	VEC_FREE(&w->entries);
	
	// TODO:free stuff inside entries
	
	if(w->curDir) free(w->curDir);
	
	// TODO: free gui stuff
	
}

int read_dir_cb(char* fullpath, char* filename, void* _w) {
	GUIFileBrowser* w = (GUIFileBrowser*)_w;
	
	VEC_INC(&w->entries);
	GUIFileBrowserEntry* e = &VEC_TAIL(&w->entries);
	*e = (GUIFileBrowserEntry){};
	
	e->name = strdup(filename);
	
// 	printf("file: %s\n", e->name);
	
	return 0;
}


// folders on top, then by name
static int entry_cmp_fn(void* a_, void* b_) {
	GUIFileBrowserEntry* a = (GUIFileBrowserEntry*)a_;
	GUIFileBrowserEntry* b = (GUIFileBrowserEntry*)b_;
	
	if(a->type == 2 && b->type == 1) return -1;
	if(a->type == 1 && b->type == 2) return 1;
	
	return strcmp(a->name, b->name);
}


void GUIFileBrowser_Refresh(GUIFileBrowser* w) {
	
	for(int i = 0; i < VEC_LEN(&w->entries); i++) {
		GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, i);
		
		if(e->name) {
			free(e->name);
			e->name = 0;
		}
	}
	
	VEC_TRUNC(&w->entries);
	
// 	recurseDirs(w->curDir, read_dir_cb, w, 0, 0);
	
	
	DIR* derp;
	struct dirent* result;
	
	derp = opendir(w->curDir);
	if(derp == NULL) {
		fprintf(stderr, "Error opening directory '%s': %s\n", w->curDir, strerror(errno));
		return;
	}
	
	while(result = readdir(derp)) {
		char* n = result->d_name;
		unsigned char type = DT_UNKNOWN;
		
		// skip self and parent dir entries
		if(n[0] == '.') {
			if(n[1] == '.' && n[2] == 0) continue;
			if(n[1] == 0) continue;
			
// 			if(flags & FSU_EXCLUDE_HIDDEN) continue;
		}
		
		switch(result->d_type) {
			case DT_REG: type = 1; break;
			case DT_DIR: type = 2; break;
			case DT_LNK: type = 3; break;
			default: type = 0;
		}
		
		VEC_INC(&w->entries);
		GUIFileBrowserEntry* e = &VEC_TAIL(&w->entries);
		*e = (GUIFileBrowserEntry){};
		
		e->type = type;
		
		// TODO: async stat
// 	struct stat st;
// 	lstat(e->name, &st);
	
		
// 		e->name = pathJoin(w->curDir, result->d_name);
		e->name = strdup(result->d_name);
		
	}
	
	VEC_SORT(&w->entries, entry_cmp_fn);
	
}

void GUIFileBrowser_SetDir(GUIFileBrowser* w, char* dir) {
	if(w->curDir) free(w->curDir);
	
	w->curDir = strdup(dir);
	
	GUIFileBrowser_Refresh(w);
}

