
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


#include "../gui.h"
#include "../gui_internal.h"



static void render(GUIFileBrowserControl* w, PassFrameParams* pfp) {
	GUIManager* gm = w->header.gm;
	
	Vector2 tl = w->header.absTopLeft;
	
	
// 	drawTextLine();
	float lh = w->lineHeight;
	float gutter = w->leftMargin + 20;
	
	int linesDrawn = 0;
	
	for(intptr_t i = w->scrollOffset; i < VEC_LEN(&w->entries); i++) {
		if(lh * linesDrawn > w->header.size.y) break; // stop at the bottom of the window
		
		// TODO stop drawing at end of window properly
		
		GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, i);
		
		AABB2 box;
		box.min.x = tl.x + gutter;
		box.min.y = tl.y + (lh * linesDrawn);
		box.max.x = tl.x + 800;
		box.max.y = tl.y + (lh * (linesDrawn + 1));
		
		
		if(e->isSelected) { // backgrounds for selected items
			struct Color4* color = &gm->defaults.tabActiveBgColor;
			
			GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
			*v = (GUIUnifiedVertex){
				.pos = {box.min.x, box.min.y, box.max.x, box.max.y},
				.clip = GUI_AABB2_TO_SHADER(w->header.absClip),
				
				.guiType = 0, // window (just a box)
				
				.fg = GUI_COLOR4_TO_SHADER(*color),
				.bg = GUI_COLOR4_TO_SHADER(*color),
				
				.z = w->header.absZ,
				.alpha = 1,
			};
		}
		
		
		char* iconame;
		if(e->type == 1) iconame = "icon/document";
		else /*if(e->type == 2)*/ iconame = "icon/folder"; // todo: resolve symlinks
		
		TextureAtlasItem* it;
		if(HT_get(&gm->ta->items, iconame, &it)) {
			printf("could not find gui image '%s' %p \n", "icon_document");
		}
		else {
			// icon
			GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
			*v = (GUIUnifiedVertex){
				.pos = {tl.x +10, box.min.y, tl.x +10+20, box.min.y + 20},
				.clip = GUI_AABB2_TO_SHADER(w->header.absClip),
				
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
				
				.z = w->header.absZ,
				.alpha = 1,
			};
		}
		
		// the file name
		gui_drawTextLine(gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x - box.min.x,0}, &w->header.absClip, &gm->defaults.tabTextColor , 10000000, e->name, strlen(e->name));
		
		linesDrawn++;
	}

	// cursor
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, 1);
	*v = (GUIUnifiedVertex){
		.pos = {
			tl.x + gutter, 
			tl.y + (w->cursorIndex - w->scrollOffset) * lh, 
			tl.x + 800,
			tl.y + (w->cursorIndex - w->scrollOffset + 1) * lh
		},
		.clip = GUI_AABB2_TO_SHADER(w->header.absClip),
		.texIndex1 = 1, // order width
		.guiType = 4, // bordered window (just a box)
		.fg = GUI_COLOR4_TO_SHADER(gm->defaults.tabActiveBgColor), // border color
		.bg = {0,0,0,0},
		.z = w->header.absZ + 0.75,
		.alpha = 1.0,
	};

	
	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIFileBrowserControl* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	gui_defaultUpdatePos(w, grp, pfp);
	

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



static char* getParentDir(char* child) {
	intptr_t len = strlen(child);
	
	for(intptr_t i = len - 1; i > 0; i--) {
		if(child[i] == '/') {
			
			if(child[i-1] == '\\') {
				i--; // escaped slash
				continue;
			}
			
			if(i == len - 1) { // this is a trailing slash; ignore it
				continue;
			}
			
			// found the last legitimate slash
			
			return strndup(child, i);
		}
		
	}
	
	return strdup("/");
}

// make sure the cursor never goes off-screen
void GUIFileBrowserControl_Autoscroll(GUIFileBrowserControl* w) {
	float linesOnScreen = floor(w->header.size.y / w->lineHeight);
	
	if(w->cursorIndex < w->scrollOffset) {
		w->scrollOffset = w->cursorIndex;
		return;
	}
	
	if(w->cursorIndex > linesOnScreen + w->scrollOffset - 1) {
		w->scrollOffset = w->cursorIndex - linesOnScreen + 1;
		return;
	}
}

static void scrollUp(GUIObject* w_, GUIEvent* gev) {
	GUIFileBrowserControl* w = (GUIFileBrowserControl*)w_;
	w->scrollOffset -= 60;
}
static void scrollDown(GUIObject* w_, GUIEvent* gev) {
	GUIFileBrowserControl* w = (GUIFileBrowserControl*)w_;
	w->scrollOffset += 60;
}


static void keyUp(GUIObject* w_, GUIEvent* gev) {
	GUIFileBrowserControl* w = (GUIFileBrowserControl*)w_;
	
	if(gev->keycode == XK_Down) {
		w->cursorIndex = (w->cursorIndex + 1) % VEC_LEN(&w->entries);
		GUIFileBrowserControl_Autoscroll(w);
	}
	else if(gev->keycode == XK_Up) {
		w->cursorIndex = (w->cursorIndex - 1 + VEC_LEN(&w->entries)) % (intptr_t)VEC_LEN(&w->entries);
		GUIFileBrowserControl_Autoscroll(w);
	}
	else if(gev->keycode == XK_BackSpace) { // navigate to parent dir
		char* p = getParentDir(w->curDir);
		free(w->curDir);
		w->curDir = p;
		
		GUIFileBrowserControl_Refresh(w);
	}
	else if(gev->keycode == XK_Return) {
		GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, w->cursorIndex);
		
		if(e->type == 2) { // enter the directory
			char* p = pathJoin(w->curDir, e->name);
			free(w->curDir);
			w->curDir = p;
			
			GUIFileBrowserControl_Refresh(w);
		}
		else { // open selected files
			
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
	}
	else if(gev->keycode == XK_space) {
		GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, w->cursorIndex);
		e->isSelected = !e->isSelected;
		w->numSelected += e->isSelected ? 1 : -1;
	}
	
}


static void click(GUIObject* w_, GUIEvent* gev) {
	GUIFileBrowserControl* w = (GUIFileBrowserControl*)w_;
	
	intptr_t line = floor((gev->pos.y - w->header.absTopLeft.y) / w->lineHeight) + w->scrollOffset;
	
	w->cursorIndex = line;
	GUIFileBrowserControl_Autoscroll(w);
	
	GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, w->cursorIndex);
	
	// open the file on doubleclick
	if(gev->multiClick == 2) {
		char* files[2] = { pathJoin(w->curDir, e->name), NULL };
		
		if(w->onChoose) w->onChoose(w->onChooseData, files, 1);
		
		free(files[0]);
	}
	else {
		e->isSelected = !e->isSelected;
		w->numSelected += e->isSelected ? 1 : -1;
	}
}


void GUIFileBrowserControl_FreeEntryList(GUIFileBrowserEntry* e, intptr_t sz) {
	GUIFileBrowserEntry* p = e;
	
	for(intptr_t i = 0; i < sz && e->name; i++) {
		if(e->name) free(e->name);
		if(e->fullPath) free(e->fullPath);
		e->name = NULL;
		e->fullPath = NULL;
		p++;
	}
	
	free(e);
}

GUIFileBrowserEntry* GUIFileBrowserControl_CollectSelected(GUIFileBrowserControl* w, intptr_t* szOut) {
	
	
	// collect a list of files
	intptr_t n = 0;
	GUIFileBrowserEntry* files = malloc(sizeof(*files) * (w->numSelected + 1));
	
	for(size_t i = 0; i < VEC_LEN(&w->entries); i++) {
		GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, i);
		
		if(!e->isSelected) continue;
		
		files[n] = *e;
		files[n].name = strdup(e->name);
		files[n].fullPath = pathJoin(w->curDir, e->name);
		
		n++;
	}
	files[n] = (GUIFileBrowserEntry){};
	
	
	if(szOut) *szOut = n;
	return files;
}


GUIFileBrowserControl* GUIFileBrowserControl_New(GUIManager* gm, char* path) {

	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.KeyUp = keyUp,
		.Click = click,
		.DoubleClick = click,
		.ScrollUp = scrollUp,
		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
// 		.ParentResize = parentResize,
	};
	
	
	GUIFileBrowserControl* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	// TODO: from config
	w->lineHeight = 22;
	w->leftMargin = 10;
	
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	
	w->scrollbar = GUIWindow_New(gm);
	GUIResize(w->scrollbar, (Vector2){10, 50});
	w->scrollbar->color = (Color4){.9,.9,.9, 1};
	w->scrollbar->header.z = 100;
	w->scrollbar->header.gravity = GUI_GRAV_TOP_RIGHT;
	
	GUIRegisterObject(w, w->scrollbar);
	
	w->curDir = realpath(path, NULL);
	
	GUIFileBrowserControl_Refresh(w);
	
	return w;
}

void GUIFileBrowserControl_Destroy(GUIFileBrowserControl* w) {
	VEC_FREE(&w->entries);
	
	// TODO:free stuff inside entries
	
	if(w->curDir) free(w->curDir);
	
	// TODO: free gui stuff
	
}

int read_dir_cb(char* fullpath, char* filename, void* _w) {
	GUIFileBrowserControl* w = (GUIFileBrowserControl*)_w;
	
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
	
	return strcasecmp(a->name, b->name);
}


void GUIFileBrowserControl_Refresh(GUIFileBrowserControl* w) {
	
	w->cursorIndex = 0;
	w->numSelected = 0;
	w->scrollOffset = 0;
	
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
	
	closedir(derp);
	
	VEC_SORT(&w->entries, entry_cmp_fn);
	
}

void GUIFileBrowserControl_SetDir(GUIFileBrowserControl* w, char* dir) {
	if(w->curDir) free(w->curDir);
	
	w->curDir = strdup(dir);
	
	GUIFileBrowserControl_Refresh(w);
}

