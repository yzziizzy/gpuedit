
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


#include <sys/types.h> // opendir
#include <dirent.h> // readdir
#include <unistd.h> // pathconf
#include <sys/stat.h>


#include "gui.h"
#include "gui_internal.h"


//    name, width, label
#define COLUMN_TYPE_LIST \
	X(icon, 20, NULL) \
	X(name, 400, "Name") \
	X(mime, 50, "Type") \
	X(size, 100, "Size") \
	X(sizeOnDisk, 50, "Size On Disk") \
	X(owner, 50, "Owner") \
	X(group, 50, "Group") \
	X(mode, 50, "Mode") \
	X(atime, 140, "Accessed") \
	X(mtime, 140, "Modified") \
	X(ctime, 140, "Created") \
	

enum {
	#define X(a, b, c) GUIFB_CT_##a,
		COLUMN_TYPE_LIST
	#undef X
	GUIFB_CT_MAXVALUE
};

char* col_type_names[] = {
	#define X(a, b, c) [GUIFB_CT_##a] = #a,
		COLUMN_TYPE_LIST
	#undef X
	[GUIFB_CT_MAXVALUE] = NULL,
};

char* col_type_labels[] = {
	#define X(a, b, c) [GUIFB_CT_##a] = c,
		COLUMN_TYPE_LIST
	#undef X
	[GUIFB_CT_MAXVALUE] = NULL,
};

float col_type_widths[] = {
	#define X(a, b, c) [GUIFB_CT_##a] = b,
		COLUMN_TYPE_LIST
	#undef X
	[GUIFB_CT_MAXVALUE] = 0,
};






static void render(GUIFileBrowserControl* w, PassFrameParams* pfp) {
	GUIManager* gm = w->header.gm;
	GUI_GlobalSettings* gs = gm->gs;
	GUIHeader* gh = &w->header;
	Vector2 tl = gh->absTopLeft;
	char buffer[256];
	struct tm tm = {};
	time_t time = 0;
	
// 	drawTextLine();
	float lh = w->lineHeight;
	float gutter = w->leftMargin + 20;
	
	int linesDrawn = 0;
	
	// draw column header
	float xoff = tl.x + w->leftMargin;
	
	VEC_EACH(&w->columnInfo, i, ci) {
		
		gui_drawBoxBorder(gm, 
			(Vector2){xoff, tl.y},
			(Vector2){ci.width, w->headerHeight},
			&gh->absClip, gh->absZ,
			&gm->defaults.fileBrowserHeaderBgColor,
			1,
			&gm->defaults.fileBrowserHeaderBorderColor
		);
		
		if(col_type_labels[ci.type]) {
			gui_drawVCenteredTextLine(gm,
				(Vector2){xoff + 1, tl.y + 1}, 
				(Vector2){ci.width - 2, w->headerHeight - 2},
				&gh->absClip, 
				&gm->defaults.fileBrowserHeaderTextColor,
				gh->absZ + 0.01,
				col_type_labels[ci.type],
				strlen(col_type_labels[ci.type])
			);
		}
		
		xoff += col_type_widths[ci.type];
	}
	
	
	
	// draw file line items
	for(intptr_t i = w->scrollOffset; i < (intptr_t)VEC_LEN(&w->entries); i++) {
		if(lh * linesDrawn > w->header.size.y) break; // stop at the bottom of the window
			
		GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, i);
		

		
		if(e->isSelected) { // backgrounds for selected items
			gui_drawBox(gm, 
				(Vector2){tl.x + w->leftMargin, tl.y + (lh * linesDrawn) + w->headerHeight}, 
				(Vector2){800, lh}, 
				&gh->absClip, gh->absZ, 
				&gm->defaults.selectedItemBgColor
			);
		}
		
		
		xoff = tl.x + w->leftMargin;
		VEC_EACH(&w->columnInfo, i, ci) {
			AABB2 box;
			box.min.x = xoff;
			box.min.y = tl.y + (lh * linesDrawn) + w->headerHeight;
			box.max.x = xoff + col_type_widths[ci.type];
			box.max.y = tl.y + (lh * (linesDrawn + 1)) + w->headerHeight;
			
			
			switch(ci.type) {
				
				case GUIFB_CT_icon: {
					char* iconame;
					if(e->type == 1) iconame = "icon/document";
					else /*if(e->type == 2)*/ iconame = "icon/folder"; // todo: resolve symlinks
					
					gui_win_drawImg(gm, gh, iconame, (Vector2){tl.x +10, box.min.y}, (Vector2){20,20});
					break;
				}
				
				case GUIFB_CT_name:
					gui_drawTextLine(gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x - box.min.x,0}, &w->header.absClip, &gm->defaults.selectedItemTextColor, gh->absZ+0.1, e->name, strlen(e->name));
					break;
					
				case GUIFB_CT_size:
					if(e->type != 2 && e->humanSize) {
						gui_drawTextLineAdv(gm, 
							(Vector2){box.min.x, box.min.y}, 
							(Vector2){box.max.x - box.min.x - 10,0}, 
							&w->header.absClip, 
							&gm->defaults.selectedItemTextColor, 
							NULL, 0,
							GUI_TEXT_ALIGN_RIGHT,
							gh->absZ + 0.1, 
							e->humanSize, strlen(e->humanSize)
						);
					}
					break;
					
				case GUIFB_CT_sizeOnDisk:
					if(e->type != 2 && e->humanSizeOnDisk) {
						gui_drawTextLineAdv(gm, 
							(Vector2){box.min.x, box.min.y}, 
							(Vector2){box.max.x - box.min.x - 10,0}, 
							&w->header.absClip, 
							&gm->defaults.selectedItemTextColor, 
							NULL, 0,
							GUI_TEXT_ALIGN_RIGHT,
							gh->absZ + 0.1, 
							e->humanSizeOnDisk, strlen(e->humanSizeOnDisk)
						);
					}
					break;
				
				case GUIFB_CT_atime:
					if(e->atimeStr)
						gui_drawTextLine(gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x - box.min.x,0}, &w->header.absClip, &gm->defaults.selectedItemTextColor, gh->absZ+0.1, e->atimeStr, strlen(e->atimeStr));
					break;
					
				case GUIFB_CT_mtime:
					if(e->mtimeStr)
						gui_drawTextLine(gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x - box.min.x,0}, &w->header.absClip, &gm->defaults.selectedItemTextColor, gh->absZ+0.1, e->mtimeStr, strlen(e->mtimeStr));
					break;
					
				case GUIFB_CT_ctime:
					if(e->ctimeStr)
						gui_drawTextLine(gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x - box.min.x,0}, &w->header.absClip, &gm->defaults.selectedItemTextColor, gh->absZ+0.1, e->ctimeStr, strlen(e->ctimeStr));
					break;
				
			}
		
			xoff += col_type_widths[ci.type];
		}
			
		linesDrawn++;
	}
	
	// cursor
	gui_drawBoxBorder(gm, 
		(Vector2){tl.x + gutter, tl.y + w->headerHeight + (w->cursorIndex - w->scrollOffset) * lh},
		(Vector2){800, lh},
		&gh->absClip, gh->absZ,
		&(Color4){0,0,0,0},
		1,
		&gm->defaults.outlineCurrentLineBorderColor
	);

	
	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIFileBrowserControl* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	gui_defaultUpdatePos(&w->header, grp, pfp);
	
	float wh = w->header.size.y;
	int linesOnScreen = wh / w->lineHeight;
	
	w->sbMinHeight = 20;
	float sbh = w->sbMinHeight;
	
	// calculate scrollbar offset
	float max_scroll = VEC_LEN(&w->entries) - linesOnScreen;
	float scroll_pct = w->scrollOffset / max_scroll;
	float sboff = scroll_pct * (wh - sbh);

//	GUIResize(w->scrollbar, (Vector2){10, sbh});
	w->scrollbar->header.topleft.y = sboff;
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

static void scrollUp(GUIHeader* w_, GUIEvent* gev) {
	GUIFileBrowserControl* w = (GUIFileBrowserControl*)w_;
	w->scrollOffset = MAX(0, w->scrollOffset - w->linesPerScrollWheel);
}
static void scrollDown(GUIHeader* w_, GUIEvent* gev) {
	GUIFileBrowserControl* w = (GUIFileBrowserControl*)w_;
	float linesOnScreen = floor(w->header.size.y / w->lineHeight);
	w->scrollOffset = MIN(MAX(0, VEC_LEN(&w->entries) - linesOnScreen), w->scrollOffset + w->linesPerScrollWheel);
}





static void click(GUIHeader* w_, GUIEvent* gev) {
	GUIFileBrowserControl* w = (GUIFileBrowserControl*)w_;
	GUIManager* gm = w_->gm;
	
	intptr_t line = floor(
		(gev->pos.y - w->header.absTopLeft.y - w->headerHeight) 
		/ w->lineHeight
	) + w->scrollOffset;
	
	
	GUIFileBrowserControl_Autoscroll(w);
	
	GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, w->cursorIndex);
	
	// open the file on doubleclick
	if(0 && gev->multiClick == 2) {
		char* files[2] = { path_join(w->curDir, e->name), NULL };
		
		if(w->onChoose) w->onChoose(w->onChooseData, files, 1);
		
		free(files[0]);
	}
	else {
		
		GUI_Cmd* cmd = Commands_ProbeSubCommand(w_, GUIELEMENT_FileRow, gev);
		
		if(cmd) {
			switch(cmd->cmd) {
				case GUICMD_FileViewer_ToggleSelect:
					//printf("selected\n");
					w->cursorIndex = line;
					e->isSelected = !e->isSelected;
					w->numSelected += e->isSelected ? 1 : -1;
					break;
					
					
				case GUICMD_FileViewer_SmartOpen: {
					//printf("smartopen\n");
					if(e->type == 2) { // enter the directory
						char* p = path_join(w->curDir, e->name);
						free(w->curDir);
						w->curDir = p;
						
						GUIFileBrowserControl_Refresh(w);
					}
					else if(w->numSelected == 0) {
						
						GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, w->cursorIndex);
						e->isSelected = 1;
						w->numSelected++;
		
						size_t sz;						
						GUIEvent gev2 = {};
						gev2.type = GUIEVENT_User;
						gev2.eventTime = 0;//gev->eventTime;
						gev2.originalTarget = &w->header;
						gev2.currentTarget = &w->header;
						gev2.cancelled = 0;
						// handlers are responsible for cleanup
						gev2.userData = GUIFileBrowserControl_CollectSelected(w, &sz);
						gev2.userSize = sz;
						
						gev2.userType = "accepted";
					
						GUIManager_BubbleEvent(w->header.gm, &w->header, &gev2);
				
						//if(w->onChoose) w->onChoose(w->onChooseData, files, n);
						if(gev2.userData && !gev2.cancelled) {
							GUIFileBrowserControl_FreeEntryList(gev2.userData, sz);
						}
			
						e->isSelected = 0;
						w->numSelected = 0;
					}
					else { // open selected files
						size_t sz;						
						GUIEvent gev2 = {};
						gev2.type = GUIEVENT_User;
						gev2.eventTime = 0;//gev->eventTime;
						gev2.originalTarget = &w->header;
						gev2.currentTarget = &w->header;
						gev2.cancelled = 0;
						// handlers are responsible for cleanup
						gev2.userData = GUIFileBrowserControl_CollectSelected(w, &sz);
						gev2.userSize = sz;
						
						gev2.userType = "accepted";
					
						GUIManager_BubbleEvent(w->header.gm, &w->header, &gev2);
				
						//if(w->onChoose) w->onChoose(w->onChooseData, files, n);
						if(gev2.userData && !gev2.cancelled) {
							GUIFileBrowserControl_FreeEntryList(gev2.userData, sz);
						}
					}
				}
			} // switch
			
			
		}
	}
}




static void handleCommand(GUIHeader* w_, GUI_Cmd* cmd) {
	GUIFileBrowserControl* w = (GUIFileBrowserControl*)w_;
	
	
	switch(cmd->cmd) {
		case GUICMD_FileViewer_MoveCursorV:
			w->cursorIndex = (w->cursorIndex + cmd->amt + VEC_LEN(&w->entries)) % (intptr_t)VEC_LEN(&w->entries);
			GUIFileBrowserControl_Autoscroll(w);
			break;

		case GUICMD_FileViewer_ParentDir: {
			char* p = getParentDir(w->curDir);
			free(w->curDir);
			w->curDir = p;
			
			GUIFileBrowserControl_Refresh(w);
			break;
		}
		case GUICMD_FileViewer_ToggleSelect: {
			GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, w->cursorIndex);
			e->isSelected = !e->isSelected;
			w->numSelected += e->isSelected ? 1 : -1;
			break;
		}
		
		case GUICMD_FileViewer_SmartOpen: {
			GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, w->cursorIndex);
			
			if(e->type == 2) { // enter the directory
				char* p = path_join(w->curDir, e->name);
				free(w->curDir);
				w->curDir = p;
				
				GUIFileBrowserControl_Refresh(w);
			}
			else if(w->numSelected == 0) {
				
				GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, w->cursorIndex);
				e->isSelected = 1;
				w->numSelected++;

				size_t sz;						
				GUIEvent gev2 = {};
				gev2.type = GUIEVENT_User;
				gev2.eventTime = 0;//gev->eventTime;
				gev2.originalTarget = &w->header;
				gev2.currentTarget = &w->header;
				gev2.cancelled = 0;
				// handlers are responsible for cleanup
				gev2.userData = GUIFileBrowserControl_CollectSelected(w, &sz);
				gev2.userSize = sz;
				
				gev2.userType = "accepted";
			
				GUIManager_BubbleEvent(w->header.gm, &w->header, &gev2);
		
				//if(w->onChoose) w->onChoose(w->onChooseData, files, n);
				if(gev2.userData && !gev2.cancelled) {
					GUIFileBrowserControl_FreeEntryList(gev2.userData, sz);
				}
	
				e->isSelected = 0;
				w->numSelected = 0;
			}
			else { // open selected files
				size_t sz;						
				GUIEvent gev2 = {};
				gev2.type = GUIEVENT_User;
				gev2.eventTime = 0;//gev->eventTime;
				gev2.originalTarget = &w->header;
				gev2.currentTarget = &w->header;
				gev2.cancelled = 0;
				// handlers are responsible for cleanup
				gev2.userData = GUIFileBrowserControl_CollectSelected(w, &sz);
				gev2.userSize = sz;
				
				gev2.userType = "accepted";
			
				GUIManager_BubbleEvent(w->header.gm, &w->header, &gev2);
		
				//if(w->onChoose) w->onChoose(w->onChooseData, files, n);
				if(gev2.userData && !gev2.cancelled) {
					GUIFileBrowserControl_FreeEntryList(gev2.userData, sz);
				}
	
			}
		}
	}
}


void GUIFileBrowserControl_FreeEntryList(GUIFileBrowserEntry* e, intptr_t sz) {
	GUIFileBrowserEntry* p = e;
	
	#define safe_free(x) if(e->x) { free(e->x); e->x = NULL; }
	for(intptr_t i = 0; i < sz && e->name; i++) {
		if(e->name) free(e->name);
		if(e->fullPath) free(e->fullPath);
		e->name = NULL;
		e->fullPath = NULL;
		
		safe_free(name);
		safe_free(fullPath);
		safe_free(atimeStr);
		safe_free(mtimeStr);
		safe_free(ctimeStr);
		safe_free(humanSize);
		safe_free(humanSizeOnDisk);
		
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
		files[n].fullPath = path_join(w->curDir, e->name);
		
		files[n].atimeStr = strdup(e->atimeStr);
		files[n].mtimeStr = strdup(e->mtimeStr);
		files[n].ctimeStr = strdup(e->ctimeStr);
		files[n].humanSize = strdup(e->humanSize);
		files[n].humanSizeOnDisk = strdup(e->humanSizeOnDisk);
		
		n++;
	}
	files[n] = (GUIFileBrowserEntry){};
	
	
	if(szOut) *szOut = n;
	return files;
}

static int chooseCursor(GUIHeader* _w, Vector2 testPos) {
	GUIFileBrowserControl* w = (GUIFileBrowserControl*)_w;
	/*
	if(testPos.y > _w->absTopLeft.y + w->headerHeight) {
		return GUIMOUSECURSOR_ARROW;
	}	
	
	
	// draw column header
	float xoff = tl.x + w->leftMargin;
	
	VEC_EACH(&w->columnInfo, i, ci) {
		xoff += col_type_widths[ci.type];
		
		if() {
			
		}
			
		
	}
	
		*/
//		return GUIMOUSECURSOR_H_MOVE;
	
	return GUIMOUSECURSOR_ARROW;
}


GUIFileBrowserControl* GUIFileBrowserControl_New(GUIManager* gm, char* path) {
	GUI_GlobalSettings* gs = gm->gs;

	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
		.ChooseCursor = chooseCursor,
		.HandleCommand = (void*)handleCommand,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
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
	w->header.cmdElementType = GUIELEMENT_FileViewer;
	
	// TODO: from config
	w->lineHeight = 22;
	w->leftMargin = 10;
	w->headerHeight = gm->gs->fileBrowserHeaderHeight;
	
	w->header.cursor = GUIMOUSECURSOR_DYNAMIC;
	
	w->scrollbar = GUIWindow_New(gm);
	GUIResize(&w->scrollbar->header, (Vector2){10, 50});
	w->scrollbar->color = (Color4){.9,.9,.9, 1};
	w->scrollbar->header.z = 100;
	w->scrollbar->header.gravity = GUI_GRAV_TOP_RIGHT;
	
	GUI_RegisterObject(w, w->scrollbar);
	
	w->curDir = realpath(path, NULL);
	
	for(int i = 0; gs->fileBrowserColumnOrder[i]; i++) {
		char* name = gs->fileBrowserColumnOrder[i];
		
		for(int id = 0; col_type_names[id]; id++) {
			if(0 == strcasecmp(col_type_names[id], name)) {
				VEC_PUSH(&w->columnInfo, ((GUIFileBrowserColumnInfo){
					.type = id,
					.width = col_type_widths[id],
				}));
			}
		}
		
	}
	
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



static void fill_info_job(GUIHeader* h, void* _e, float* pctDone);

void GUIFileBrowserControl_Refresh(GUIFileBrowserControl* w) {
	GUIManager* gm = w->header.gm;
	
	w->cursorIndex = 0;
	w->numSelected = 0;
	w->scrollOffset = 0;
	
	for(size_t i = 0; i < VEC_LEN(&w->entries); i++) {
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
		
		
		char* tmp = path_join(w->curDir, result->d_name);
		
		VEC_INC(&w->entries);
		GUIFileBrowserEntry* e = &VEC_TAIL(&w->entries);
		*e = (GUIFileBrowserEntry){};
		
		e->type = type;
		e->isSymlink = 0; 
		
		e->name = strdup(result->d_name);
		e->fullPath = realpath(tmp, NULL);
		
		switch(result->d_type) {
			case DT_REG: e->type = 1; break;
			case DT_DIR: e->type = 2; break;
			case DT_LNK: 
				e->isSymlink = 1; 
				
				struct stat sb;
				
				stat(tmp, &sb);
				switch(sb.st_mode & S_IFMT) {
					case S_IFREG: e->type = 1; break;
					case S_IFDIR: e->type = 2; break;
					default: e->type = 0;
				}
				
				break;
			default: type = 0;
		}
		
		
		free(tmp);
		
		
		// TODO: async stat
// 	struct stat st;
// 	lstat(e->name, &st);
	
		
// 		e->name = path_join(w->curDir, result->d_name);		
	}

	VEC_EACHP(&w->entries, i, e) {
		GUIManager_EnqueueJob(gm, &w->header, fill_info_job, e);
	}
	
	closedir(derp);
	
	VEC_SORT(&w->entries, entry_cmp_fn);
	
}

void GUIFileBrowserControl_SetDir(GUIFileBrowserControl* w, char* dir) {
	if(w->curDir) free(w->curDir);
	
	w->curDir = strdup(dir);
	
	GUIFileBrowserControl_Refresh(w);
}




static char* format_byte_amt(uint64_t sz) {
	
	if(sz < 1024l) {
		return sprintfdup("%ld B", sz);
	}
	else if(sz < 1024l*1024l) {
		return sprintfdup("%ld KB", sz / 1024l);
	}
	else if(sz < 1024l*1024l*1024l) {
		return sprintfdup("%ld MB", sz / (1024l*1024l));
	}
	else if(sz < 1024l*1024l*1024l*1024l) {
		return sprintfdup("%ld GB", sz / (1024l*1024l*1024l));
	}
	else /* if(sz < 1024*1024*1024*1024*1024) */ { 
		return sprintfdup("%ld TB", sz / (1024l*1024l*1024l*1024l));
	}
	
}



static void fill_info_job(GUIHeader* h, void* _e, float* pctDone) {
	GUIFileBrowserControl* w = (GUIFileBrowserControl*)h;
	GUIFileBrowserEntry* e = _e;
	
	struct stat sb;
	
	char buffer[256];
	struct tm tm = {};
	time_t time = 0;
	
	
	stat(e->fullPath, &sb);
	
	e->perms = sb.st_mode;
	
	e->size = sb.st_size;	
	e->sizeOnDisk = sb.st_blocks * 512;
	e->ownerID = sb.st_uid;	
	e->groupID = sb.st_gid;	
	
	e->atime = sb.st_atim.tv_sec;
	e->mtime = sb.st_mtim.tv_sec;
	e->ctime = sb.st_ctim.tv_sec;

	e->humanSize = format_byte_amt(e->size);
	e->humanSizeOnDisk = format_byte_amt(e->sizeOnDisk);

	time = e->atime;
	localtime_r(&time, &tm);
	strftime(buffer, 256, h->gm->gs->fileBrowserATimeFmt, &tm); 
	e->atimeStr = strdup(buffer);

	time = e->mtime;
	localtime_r(&time, &tm);
	strftime(buffer, 256, h->gm->gs->fileBrowserMTimeFmt, &tm); 
	e->mtimeStr = strdup(buffer);

	time = e->ctime;
	localtime_r(&time, &tm);
	strftime(buffer, 256, h->gm->gs->fileBrowserCTimeFmt, &tm); 
	e->ctimeStr = strdup(buffer);

	*pctDone = 1.0;
}







