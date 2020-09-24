
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



static void updatePos(GUIObject* w_, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIFileBrowser* w = (GUIFileBrowser*)w_;
	
	float padding = 4;
	
	w->fbc->header.size.x = w->header.size.x;
	w->fbc->header.size.y = w->header.size.y - 60;
// 	w->fbc->header.topleft.y = 20;
	
	
	w->newDirBtn->header.topleft.y = 0;
	w->newFileBtn->header.topleft.y = 0;
	w->newFileBtn->header.topleft.x = 120;
	
	w->acceptBtn->header.topleft.y = 0;
	w->cancelBtn->header.topleft.y = 0;
	w->cancelBtn->header.topleft.x = 120;
	
	w->newDirBtn->header.size = (Vector2){100, 25};
	w->newFileBtn->header.size = (Vector2){100, 25};
	w->acceptBtn->header.size = (Vector2){100, 25};
	w->cancelBtn->header.size = (Vector2){100, 25};
	
	w->filenameBar->header.topleft.y = 30;
	w->filenameBar->header.topleft.x = 0;
	w->filenameBar->header.size.y = 25;
	
	
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
	
	
	intptr_t sz;
	
	GUIEvent gev2 = {};
	gev2.type = GUIEVENT_User;
	gev2.eventTime = gev->eventTime;
	gev2.originalTarget = w_;
	gev2.currentTarget = w_;
	gev2.cancelled = 0;
	
	
	int userEvent = 0;
	if(gev->originalTarget == w->cancelBtn) {
		gev2.userType = "cancelled";
		userEvent = 1;
	}
	else if(gev->originalTarget == w->acceptBtn) {
		
		// handlers are responsible for cleanup
		gev2.userData = GUIFileBrowserControl_CollectSelected(w->fbc, &sz);
		gev2.userSize = sz;
		
		gev2.userType = "accepted";
		userEvent = 1;
	}
	else if(gev->originalTarget == w->newFileBtn) {
		gev2.userType = "new_file";
		userEvent = 1;
	}
	else if(gev->originalTarget == w->newDirBtn) {
		gev2.userType = "new_dir";
		userEvent = 1;
	}
	
	if(userEvent) {
		GUIManager_BubbleEvent(w->header.gm, w, &gev2);
		
		// clean up the entry list if nothing caught the event
		if(gev2.userData && !gev2.cancelled) {
			GUIFileBrowserControl_FreeEntryList(gev2.userData, sz);
		}
	}
	
	gev->cancelled = 1;
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
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	
	w->fbc = GUIFileBrowserControl_New(gm, path);
	w->fbc->header.flags |= GUI_MAXIMIZE_X;
	
	w->filenameBar = GUIEdit_New(gm, "");
	w->filenameBar->header.flags |= GUI_MAXIMIZE_X;
	w->filenameBar->header.gravity = GUI_GRAV_BOTTOM_LEFT;
	
	w->acceptBtn = GUIButton_New(gm, "Accept");
	w->cancelBtn = GUIButton_New(gm, "Cancel");
	w->newFileBtn = GUIButton_New(gm, "New File");
	w->newDirBtn = GUIButton_New(gm, "New Dir");
	
	w->acceptBtn->header.gravity = GUI_GRAV_BOTTOM_RIGHT;
	w->cancelBtn->header.gravity = GUI_GRAV_BOTTOM_RIGHT;
	w->newFileBtn->header.gravity = GUI_GRAV_BOTTOM_LEFT;
	w->newDirBtn->header.gravity = GUI_GRAV_BOTTOM_LEFT;
	w->fbc->header.gravity = GUI_GRAV_TOP_LEFT;
	
	GUIRegisterObject(w, w->fbc);
	GUIRegisterObject(w, w->filenameBar);
	GUIRegisterObject(w, w->acceptBtn);
	GUIRegisterObject(w, w->cancelBtn);
	GUIRegisterObject(w, w->newDirBtn);
	GUIRegisterObject(w, w->newFileBtn);
	
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

void GUIFileBrowser_UnselectAll(GUIFileBrowser* w) {
	
}
