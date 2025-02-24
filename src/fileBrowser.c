
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


#include "ui/gui.h"
#include "ui/gui_internal.h"

#include "fileBrowser.h"




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



void send_open_message(FileBrowser* w, char* path) {
	char* path2 = resolve_path(path);

	MessageFileOpt opt = {0};
	opt = (MessageFileOpt){
		.path = path2,
		.line_num = 1,
		.paneTargeter = -1,
	};
	
	if(w->gs->MainControl_openInPlace) {
		opt.set_focus = 1;
	}
	
	MessagePipe_Send(w->upstream, MSG_OpenFileOpt, &opt, NULL);
	
	free(path2);
	
	if(w->gs->MainControl_openInPlace) {
		MessagePipe_Send(w->upstream, MSG_CloseMe, w, NULL);
	}
}




#include "ui/macros_on.h"



// todo: color mod time by age



int GUI_FileBrowserEntryList_(GUIManager* gm, FileBrowserEntryList* el, vec2 tl, vec2 sz, PassFrameParams* pfp) {
	
	GUI_PushClip(tl, sz);
	GUI_PushFontName("Arial", 16, &gm->defaults.selectedItemTextColor);
	
	el->lineHeight = 20;
	el->headerHeight = 20;
	
	float z = gm->curZ;
	float lh = el->lineHeight;
	float gutter = el->leftMargin + 20;
	
		// draw column header
	float xoff = tl.x + el->leftMargin;
	
	VEC_EACH(&el->columnInfo, i, ci) {
		
		GUI_BoxFilled(V(xoff, tl.y), V(ci.width, el->headerHeight), 1, &gm->defaults.fileBrowserHeaderBorderColor, &gm->defaults.fileBrowserHeaderBgColor);
		
		
		if(col_type_labels[ci.type]) {
			GUI_TextLineAdv(
				V(xoff, tl.y), V(ci.width, el->headerHeight), 
				col_type_labels[ci.type], -1,
				GUI_TEXT_ALIGN_VCENTER,
				gm->curFont,
				gm->curFontSize,
				&gm->defaults.fileBrowserHeaderTextColor
			);
		}
		
		xoff += col_type_widths[ci.type];
	}
	
	// cursor
	gm->curZ = z + 2;
	GUI_BoxFilled(
		V(tl.x + gutter, tl.y + el->headerHeight + (el->cursorIndex - el->scrollOffset/* + !w->isRootDir*/) * lh),
		V(sz.x - gutter, lh),
		1,
//		&gm->defaults.outlineCurrentLineBorderColor,
		&C4H(ffffffff),
		C4(0,0,0,0)
	);
	
	
	int linesDrawn = 0;
	
	if(GUI_VScrollbar(&el->scrollPos, V(tl.x + sz.x - 10, tl.y), V(10, sz.y), VEC_LEN(&el->files) - 1, &el->scrollPos)) {
		el->scrollOffset = el->scrollPos;
	}
	
//	// draw the virtual parent folder line
//	if(!w->isRootDir) {
//		xoff = tl.x + w->leftMargin;
//		VEC_EACH(&w->columnInfo, i, ci) {
//			AABB2 box;
//			box.min.x = xoff;
//			box.min.y = tl.y + (lh * linesDrawn) + w->headerHeight;
//			box.max.x = xoff + col_type_widths[ci.type];
//			box.max.y = tl.y + (lh * (linesDrawn + 1)) + w->headerHeight;
//			
//			switch(ci.type) {
//				case GUIFB_CT_icon:
//					GUI_Image(V(box.min.x, box.min.y), V(20,20), "icon/folder");
//					break;
//				
//				case GUIFB_CT_name:
//					GUI_TextLine(V(box.min.x, box.min.y), "..", strlen(".."));// , "Arial", 16, &gm->defaults.selectedItemTextColor);
//					break;
//			}
//			
//			xoff += col_type_widths[ci.type];
//		}
//		
//		linesDrawn++;
//	} 
	
	
	
	// draw file line items
	for(intptr_t i = el->scrollOffset; i < (intptr_t)VEC_LEN(&el->files); i++) {
		if(lh * linesDrawn > sz.y) break; // stop at the bottom of the window
			
		FileBrowserEntry* e = &VEC_ITEM(&el->files, i);
		
		Vector2 line_tl = V(tl.x + el->leftMargin, tl.y + (lh * linesDrawn) + el->headerHeight);
		Vector2 line_sz = V(sz.x, lh - 1);
		
		gm->curZ = z + 1;
		if(e->isSelected) { // backgrounds for selected items
			GUI_Rect(line_tl, line_sz, &gm->defaults.selectedItemBgColor);
		}
		else if(GUI_MouseInside(line_tl, line_sz)) {
			GUI_Rect(line_tl, V(line_sz.x, line_sz.y + 1), &gm->defaults.selectedItemBgColor);
		}
		
		
		gm->curZ = z + 3;
		xoff = tl.x + el->leftMargin;
		VEC_EACH(&el->columnInfo, i, ci) {
			AABB2 box;
			box.min.x = xoff;
			box.min.y = tl.y + (lh * linesDrawn) + el->headerHeight;
			box.max.x = xoff + col_type_widths[ci.type];
			box.max.y = tl.y + (lh * (linesDrawn + 1)) + el->headerHeight;
			
			switch(ci.type) {
				
				case GUIFB_CT_icon: {
					char* iconame;
					if(e->type == 1) iconame = "icon/document";
					else /*if(e->type == 2)*/ iconame = "icon/folder"; // todo: resolve symlinks
					
					GUI_Image(V(box.min.x, box.min.y), V(20,20), iconame);
					break;
				}
				
				case GUIFB_CT_name:
					GUI_TextLine(box.min, e->name, -1);
					break;
					
				case GUIFB_CT_size:
					if(e->type != 2 && e->humanSize) {
						GUI_TextLine(box.min, e->humanSize, -1);
						// TODO: align right
					}
					break;
					
				case GUIFB_CT_sizeOnDisk:
					if(e->type != 2 && e->humanSizeOnDisk) {
						// TODO: align right
						GUI_TextLine(box.min, e->humanSizeOnDisk, -1);
					}
					break;
				
				case GUIFB_CT_atime:
					if(e->atimeStr)
						GUI_TextLine(box.min, e->atimeStr, -1);
					break;
					
				case GUIFB_CT_mtime:
					if(e->mtimeStr)
						GUI_TextLine(box.min, e->mtimeStr, -1);
					break;
					
				case GUIFB_CT_ctime:
					if(e->ctimeStr)
						GUI_TextLine(box.min, e->ctimeStr, -1);
					break;
				
			}
		
			xoff += col_type_widths[ci.type];
		}
			
		linesDrawn++;
	}
	
	GUI_PopFont();
	GUI_PopClip();
	
	return 0;
}











void FileBrowser_Render(FileBrowser* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	GUISettings* gs = gm->gs;
	
	char buffer[256];
	struct tm tm = {};
	time_t time = 0;
	
	w->entries.linesOnScreen = ((sz.y - w->headerHeight) / w->lineHeight) - w->isRootDir;

	if(gm->drawMode) {
		w->entries.scrollOffset += gm->scrollDist * -3;
		w->entries.scrollOffset = lclamp(w->entries.scrollOffset, 0, w->entries.linesOnScreen);
		if(gm->scrollDist) {
			w->entries.scrollPos = w->entries.scrollOffset;
		}
	}
	

		// 	drawTextLine();
	float lh = w->lineHeight;
	float gutter = w->leftMargin + 20;

	vec2 flsz = V(sz.x, sz.y - lh);
	vec2 fltl = V(tl.x + 0, tl.y + lh);
	
	GUI_FileBrowserEntryList_(gm, &w->entries, fltl, flsz, pfp);

	
//	w->/*scrollOffset*/ = 10/*;*/
	
	w->sbMinHeight = 20;
	float sbh = w->sbMinHeight;
	
	
	// calculate scrollbar offset
	float max_scroll = VEC_LEN(&w->entries.files) - w->entries.linesOnScreen;
	float scroll_pct = w->entries.scrollOffset / max_scroll;
	float sboff = scroll_pct * (sz.y - sbh);
	
	

	
	

	if(GUI_InputAvailable()) {
		GUI_Cmd* cmd = Commands_ProbeCommandMode(gm, GUIELEMENT_FileBrowser, &gm->curEvent, 0, NULL);
		if(cmd) {
			FileBrowser_ProcessCommand(w, cmd);
			GUI_CancelInput();
			return;
		}
		
		if(gm->curEvent.type == GUIEVENT_MouseUp && gm->curEvent.button == 1 && gm->curEvent.multiClick == 2) {
			// determine the clicked line
			Vector2 mp = GUI_MousePos();
			int cline = floor((mp.y - w->headerHeight) / w->lineHeight) + (int)w->entries.scrollOffset - 1 - !w->isRootDir;
			
			if(!w->isRootDir && cline == -1) {
				char* p = getParentDir(w->curDir);
				free(w->curDir);
				w->curDir = p;
				
				FileBrowser_Refresh(w);
				GUI_CancelInput();
			}
			else if(cline >= 0 && cline <= VEC_LEN(&w->entries.files) - 1) {
				FileBrowserEntry* e = &VEC_ITEM(&w->entries.files, cline);
				
				if(e->type == 2) { // enter the directory
					char* p = path_join(w->curDir, e->name);
					free(w->curDir);
					w->curDir = p;
					
					FileBrowser_Refresh(w);
				}
				else send_open_message(w, e->name);
				
				GUI_CancelInput();
			}
		}
		
	}
	
	
	float z = gm->curZ;
	


	
	gm->curZ = z;
	
}

#include "ui/macros_off.h"








/*
static void userEvent(GUIHeader* w_, GUIEvent* gev) {
	GUIFileBrowser* w = (GUIFileBrowser*)w_;

	if((GUIFileBrowserControl*)gev->originalTarget == w->fbc) {
		if(0 == strcmp(gev->userType, "accepted")) {
			gev->cancelled = 1;
			
			GUIFileBrowserEntry* e = gev->userData;
			while(e->name) {
			
				GUIEvent gev2 = {};
				gev2.type = GUIEVENT_User;
				gev2.originalTarget = &w->header;
				gev2.currentTarget = &w->header;
				// handlers are responsible for cleanup
				gev2.userData = e->fullPath;
				gev2.userSize = strlen(e->fullPath);
				gev2.userType = "openFile";
			
				GUIManager_BubbleEvent(w->header.gm, &w->header, &gev2);
			
				e++;
			}
			
			GUIFileBrowser_UnselectAll(w);
			GUIFileBrowserControl_FreeEntryList(gev->userData, gev->userSize);
		}
	}
}
*/


void FileBrowser_ProcessCommand(FileBrowser* w, GUI_Cmd* cmd) {
	long amt;
	
	switch(cmd->cmd) {
		case GUICMD_FileBrowser_MoveCursorV:
			w->entries.cursorIndex = (w->entries.cursorIndex + cmd->amt + VEC_LEN(&w->entries.files)) % (intptr_t)VEC_LEN(&w->entries.files);
			
				
	
			if(w->entries.cursorIndex < w->entries.scrollOffset) {
				w->entries.scrollOffset = w->entries.cursorIndex;
			}
			else if(w->entries.cursorIndex >= w->entries.scrollOffset + w->entries.linesOnScreen - 1) {
				w->entries.scrollOffset = w->entries.cursorIndex - w->entries.linesOnScreen + 1;
			}
			
			
			break;
			
		case GUICMD_FileBrowser_CursorMoveNoWrap:
			w->entries.cursorIndex += cmd->amt;
			w->entries.cursorIndex = w->entries.cursorIndex >= (intptr_t)VEC_LEN(&w->entries.files) - 1 ? (intptr_t)VEC_LEN(&w->entries.files) - 1: w->entries.cursorIndex;
			w->entries.cursorIndex = w->entries.cursorIndex < 0 ? 0 : w->entries.cursorIndex;
			
				
			
			if(w->entries.cursorIndex < w->entries.scrollOffset) {
				w->entries.scrollOffset = w->entries.cursorIndex;
			}
			else if(w->entries.cursorIndex >= w->entries.scrollOffset + w->entries.linesOnScreen - 1) {
				w->entries.scrollOffset = w->entries.cursorIndex - w->entries.linesOnScreen + 1;
			}
			
			break;
			
		case GUICMD_FileBrowser_UpDir: {
			printf("foobar\n");
			char* p = getParentDir(w->curDir);
			free(w->curDir);
			w->curDir = p;
			
			FileBrowser_Refresh(w);
			break;
		}
		case GUICMD_FileBrowser_ToggleSelect: {
			FileBrowserEntry* e = &VEC_ITEM(&w->entries.files, w->entries.cursorIndex);
			e->isSelected = !e->isSelected;
			w->entries.numSelected += e->isSelected ? 1 : -1;
			break;
		}
		
		case GUICMD_FileBrowser_SmartOpen: {
			FileBrowserEntry* e = &VEC_ITEM(&w->entries.files, w->entries.cursorIndex);
			
			if(e->type == 2) { // enter the directory
				char* p = path_join(w->curDir, e->name);
				free(w->curDir);
				w->curDir = p;
				
				FileBrowser_UnselectAll(w);
				
				FileBrowser_Refresh(w);
			}
			else if(w->entries.numSelected == 0) {
				FileBrowserEntry* e = &VEC_ITEM(&w->entries.files, w->entries.cursorIndex);
				send_open_message(w, e->fullPath);
			}
			else { // open selected files
				for(size_t i = 0; i < VEC_LEN(&w->entries.files); i++) {
					FileBrowserEntry* e = &VEC_ITEM(&w->entries.files, i);
		
					if(!e->isSelected) {
						send_open_message(w, e->fullPath);
					}
				}
				
				FileBrowser_UnselectAll(w);
			}
		}
	}
			
	
}

void FileBrowserControl_FreeEntryList(FileBrowserEntryList* el, intptr_t sz) {
	
	#define safe_free(x) if(e->x) { free(e->x); e->x = NULL; }
	
	VEC_EACHP(&el->files, i, e) {
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
	}
	
	VEC_FREE(&el->files);
	VEC_FREE(&el->columnInfo);
}


FileBrowserEntry* FileBrowserEntryList_CollectSelected(FileBrowser* w, intptr_t* szOut) {
	
//	
//	// collect a list of files
//	intptr_t n = 0;
//	FileBrowserEntry* files = malloc(sizeof(*files) * (w->numSelected + 1));
//	
//	for(size_t i = 0; i < VEC_LEN(&w->entries); i++) {
//		FileBrowserEntry* e = &VEC_ITEM(&w->entries, i);
//		
//		if(!e->isSelected) continue;
//		
//		files[n] = *e;
//		files[n].name = strdup(e->name);
//		files[n].fullPath = path_join(w->curDir, e->name);
//		
//		files[n].atimeStr = strdup(e->atimeStr);
//		files[n].mtimeStr = strdup(e->mtimeStr);
//		files[n].ctimeStr = strdup(e->ctimeStr);
//		files[n].humanSize = strdup(e->humanSize);
//		files[n].humanSizeOnDisk = strdup(e->humanSizeOnDisk);
//		
//		n++;
//	}
//	files[n] = (FileBrowserEntry){};
//	
//	
//	if(szOut) *szOut = n;
//	return files;
//	
	return NULL;
}








FileBrowser* FileBrowser_New(GUIManager* gm, Settings* s, MessagePipe* mp, char* path) {

	FileBrowser* w = pcalloc(w);
	
	w->gm = gm;
	w->upstream = mp;
	w->headerHeight = 20;
	w->lineHeight = 20;
	w->leftMargin = 20;
	w->curDir = realpath(path, NULL);
	
	w->gs = Settings_GetSection(s, SETTINGS_General);
	
	for(int i = 0; gm->gs->fileBrowserColumnOrder[i]; i++) {
		char* name = gm->gs->fileBrowserColumnOrder[i];
		
		for(int id = 0; col_type_names[id]; id++) {
			if(0 == strcasecmp(col_type_names[id], name)) {
				VEC_PUSH(&w->entries.columnInfo, ((FileBrowserColumnInfo){
					.type = id,
					.width = col_type_widths[id],
				}));
			}
		}
		
	}
	
	FileBrowser_Refresh(w);
	
	return w;
}

void FileBrowser_Destroy(FileBrowser* w) {
	
	// TODO:free stuff inside entries
	
	if(w->curDir) free(w->curDir);
	
	// TODO: free gui stuff
	
}





int read_dir_cb(char* fullpath, char* filename, void* _w) {
	FileBrowser* w = (FileBrowser*)_w;
	
	FileBrowserEntry* e = VEC_INC(&w->entries.files);
	*e = (FileBrowserEntry){0};
	
	e->name = strdup(filename);
	
// 	printf("file: %s\n", e->name);
	
	return 0;
}


// folders on top, then by name
static int entry_cmp_fn(void* a_, void* b_) {
	FileBrowserEntry* a = (FileBrowserEntry*)a_;
	FileBrowserEntry* b = (FileBrowserEntry*)b_;
	
	if(a->type == 2 && b->type == 1) return -1;
	if(a->type == 1 && b->type == 2) return 1;
	
	return strcasecmp(a->name, b->name);
}



static void fill_info_job(FileBrowser* w,  FileBrowserEntry* e, float* pctDone);

void FileBrowser_Refresh(FileBrowser* w) {
	GUIManager* gm = w->gm;
	
	w->entries.cursorIndex = 0;
	w->entries.numSelected = 0;
	w->entries.scrollOffset = 0;
	
	for(size_t i = 0; i < VEC_LEN(&w->entries.files); i++) {
		FileBrowserEntry* e = &VEC_ITEM(&w->entries.files, i);
		
		if(e->name) {
			free(e->name);
			e->name = 0;
		}
	}
	
	VEC_TRUNC(&w->entries.files);
	
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
		
		FileBrowserEntry* e = VEC_INC(&w->entries.files);
		*e = (FileBrowserEntry){};
		
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

	VEC_EACHP(&w->entries.files, i, e) {
		float done;
		fill_info_job(w, e, &done);
//		GUIManager_EnqueueJob(gm, &w->header, fill_info_job, e);
	}
	
	closedir(derp);
	
	VEC_SORT(&w->entries.files, entry_cmp_fn);
	
}

void FileBrowserControl_SetDir(FileBrowser* w, char* dir) {
	if(w->curDir) free(w->curDir);
	
	w->curDir = strdup(dir);
	
	FileBrowser_Refresh(w);
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



static void fill_info_job(FileBrowser* w, FileBrowserEntry* e, float* pctDone) {
	
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
	strftime(buffer, 256, w->gm->gs->fileBrowserATimeFmt, &tm); 
	e->atimeStr = strdup(buffer);

	time = e->mtime;
	localtime_r(&time, &tm);
	strftime(buffer, 256, w->gm->gs->fileBrowserMTimeFmt, &tm); 
	e->mtimeStr = strdup(buffer);

	time = e->ctime;
	localtime_r(&time, &tm);
	strftime(buffer, 256, w->gm->gs->fileBrowserCTimeFmt, &tm); 
	e->ctimeStr = strdup(buffer);

	*pctDone = 1.0;
}




void FileBrowser_SetDir(FileBrowser* w, char* dir) {
	if(w->curDir) free(w->curDir);
	
	w->curDir = strdup(dir);
	
	FileBrowser_Refresh(w);
}


void FileBrowser_UnselectAll(FileBrowser* w) {
	VEC_EACHP(&w->entries.files, i, e) {
		e->isSelected = 0;
	}
	
	w->entries.numSelected = 0;
}
