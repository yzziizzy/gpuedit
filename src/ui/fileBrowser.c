
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
	
	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIFileBrowser* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	gui_defaultUpdatePos(w, grp, pfp);
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


static void keyUp(GUIObject* w_, GUIEvent* gev) {
	GUIFileBrowser* w = (GUIFileBrowser*)w_;
/*	
	if(gev->keycode == XK_Down) {
		w->cursorIndex = (w->cursorIndex + 1) % VEC_LEN(&w->entries);
		autoscroll(w);
	}
	else if(gev->keycode == XK_Up) {
		w->cursorIndex = (w->cursorIndex - 1 + VEC_LEN(&w->entries)) % (intptr_t)VEC_LEN(&w->entries);
		autoscroll(w);
	}
	else if(gev->keycode == XK_BackSpace) { // navigate to parent dir
		char* p = getParentDir(w->curDir);
		free(w->curDir);
		w->curDir = p;
		
		GUIFileBrowser_Refresh(w);
	}
	else if(gev->keycode == XK_Return) {
		GUIFileBrowserEntry* e = &VEC_ITEM(&w->entries, w->cursorIndex);
		
		if(e->type == 2) { // enter the directory
			char* p = pathJoin(w->curDir, e->name);
			free(w->curDir);
			w->curDir = p;
			
			GUIFileBrowser_Refresh(w);
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
	*/
}


static void click(GUIObject* w_, GUIEvent* gev) {
	GUIFileBrowser* w = (GUIFileBrowser*)w_;
	
	
}



GUIFileBrowser* GUIFileBrowser_New(GUIManager* gm, char* path) {

	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.KeyUp = keyUp,
		.Click = click,
		.DoubleClick = click,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
// 		.ParentResize = parentResize,
	};
	
	
	GUIFileBrowser* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	w->fbc = GUIFileBrowserControl_New(gm, path);
	w->fbc->header.flags |= GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
	w->filenameBar = GUIEdit_New(gm, "");
	w->acceptBtn = GUIButton_New(gm, "Accept");
	w->cancelBtn = GUIButton_New(gm, "Cancel");
	
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	
	w->curDir = realpath(path, NULL);
	
	GUIFileBrowser_Refresh(w);
	
	return w;
}

void GUIFileBrowser_Destroy(GUIFileBrowser* w) {
	
	// TODO:free stuff inside entries
	
	if(w->curDir) free(w->curDir);
	
	// TODO: free gui stuff
	
}




void GUIFileBrowser_Refresh(GUIFileBrowser* w) {
	GUIFileBrowserControl_Refresh(w->fbc);
}

void GUIFileBrowser_SetDir(GUIFileBrowser* w, char* dir) {
	if(w->curDir) free(w->curDir);
	
	w->curDir = strdup(dir);
	
	GUIFileBrowser_Refresh(w);
}

