
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


#include "ui/gui.h"
#include "ui/gui_internal.h"

#include "fileBrowser.h"
#include "commands.h"



static void render(GUIFileBrowser* w, PassFrameParams* pfp) {
	GUIManager* gm = w->header.gm;
	GUIUnifiedVertex* v;

	Vector2 tl = w->header.absTopLeft;

	// draw general background
	v = GUIManager_reserveElements(gm, 1);
	*v = (GUIUnifiedVertex){
		.pos = {
			tl.x,
			tl.y,
			tl.x + w->header.size.x,
			tl.y + w->header.size.y
		},
		.clip = GUI_AABB2_TO_SHADER(w->header.absClip),
		.guiType = 0, // window (just a box)
		.fg = {0, 0, 255, 255},
		.bg = GUI_COLOR4_TO_SHADER(gm->defaults.windowBgColor),
		.z = w->header.absZ,
		.alpha = 1,
	};
	
	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIHeader* w_, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIFileBrowser* w = (GUIFileBrowser*)w_;
	
	float padding = 4;
	
	w->fbc->header.size.x = w->header.size.x;
	w->fbc->header.size.y = w->header.size.y - 65;
// 	w->fbc->header.topleft.y = 20;
	
	w->tray->header.size.y = 65;
	
	w->newDirBtn->header.topleft.y = 5;
	w->newFileBtn->header.topleft.y = 5;
	w->newFileBtn->header.topleft.x = 120;
	
	w->acceptBtn->header.topleft.y = 5;
	w->cancelBtn->header.topleft.y = 5;
	w->cancelBtn->header.topleft.x = 120;
	
	w->newDirBtn->header.size = (Vector2){100, 25};
	w->newFileBtn->header.size = (Vector2){100, 25};
	w->acceptBtn->header.size = (Vector2){100, 25};
	w->cancelBtn->header.size = (Vector2){100, 25};
	
	w->filenameBar->header.topleft.y = 35;
	w->filenameBar->header.topleft.x = 0;
	w->filenameBar->header.size.y = 25;
	
	
	gui_defaultUpdatePos(&w->header, grp, pfp);
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

static void handleCommand(GUIHeader* w_, GUI_Cmd* cmd) {
	GUIFileBrowser* w = (GUIFileBrowser*)w_;
	
	GUIFileBrowser_ProcessCommand(w, cmd);
}


void GUIFileBrowser_ProcessCommand(GUIFileBrowser* w, GUI_Cmd* cmd) {
	long amt;
	
	switch(cmd->cmd) {
//		case FileBrowserCmd_Exit:
//			GUIManager_BubbleUserEvent(w->header.gm, &w->header, "closeMe");
//			break;

/*	if(w->gs->MainControl_openInPlace) {
				GUIManager_BubbleUserEvent(w->header.gm, &w->header, "closeMe");
			}
	*/
			
	}
}


static void click(GUIHeader* w_, GUIEvent* gev) {
	GUIFileBrowser* w = (GUIFileBrowser*)w_;
	
	
	intptr_t sz;
	
	GUIEvent gev2 = {};
	gev2.type = GUIEVENT_User;
	gev2.eventTime = gev->eventTime;
	gev2.originalTarget = w_;
	gev2.currentTarget = w_;
	gev2.cancelled = 0;
	
	
	int userClose = 0;
	int userEvent = 0;
	if(gev->originalTarget == (void*)w->cancelBtn) {
		gev2.userType = "cancelled";
		userEvent = 1;
	}
	else if(gev->originalTarget == (void*)w->acceptBtn) {
		
		// handlers are responsible for cleanup
		gev2.userData = GUIFileBrowserControl_CollectSelected(w->fbc, &sz);
		gev2.userSize = sz;
		
		gev2.userType = "accepted";
		userEvent = 1;

		userClose = w->gs->MainControl_openInPlace ? 1 : 0;
	}
	else if(gev->originalTarget == (void*)w->newFileBtn) {
		gev2.userType = "new_file";
		userEvent = 1;
	}
	else if(gev->originalTarget == (void*)w->newDirBtn) {
		gev2.userType = "new_dir";
		userEvent = 1;
	}
	
	if(userEvent) {
		GUIManager_BubbleEvent(w->header.gm, &w->header, &gev2);
		
		// clean up the entry list if nothing caught the event
		if(gev2.userData && !gev2.cancelled) {
			GUIFileBrowserControl_FreeEntryList(gev2.userData, sz);
		}
	}
	if(userClose) {
		GUIManager_BubbleUserEvent(w->header.gm, &w->header, "closeMe");
	}
	
	gev->cancelled = 1;
}

static void gainedFocus(GUIHeader* w_, GUIEvent* gev) {
	GUIFileBrowser* w = (GUIFileBrowser*)w_;
	GUIManager_pushFocusedObject(w_->gm, &w->fbc->header);
}


GUIFileBrowser* GUIFileBrowser_New(GUIManager* gm, char* path) {

	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
		.HandleCommand = (void*)handleCommand,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.Click = click,
		.DoubleClick = click,
		.GainedFocus = gainedFocus,
		.User = userEvent,
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
	w->header.cmdElementType = CUSTOM_ELEM_TYPE_FileBrowser;
		
	w->fbc = GUIFileBrowserControl_New(gm, path);
	w->fbc->header.flags |= GUI_MAXIMIZE_X;

	w->fbc->linesPerScrollWheel = gm->gs->linesPerScrollWheel;
	
	w->tray = GUIWindow_New(gm);
	w->tray->header.flags |= GUI_MAXIMIZE_X;
	w->tray->header.gravity = GUI_GRAV_BOTTOM_LEFT;
	w->tray->color = gm->defaults.trayBgColor;
	
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
	
	GUI_RegisterObject(w, w->fbc);
	GUI_RegisterObject(w, w->tray);
	GUI_RegisterObject(w, w->filenameBar);
	GUI_RegisterObject(w, w->acceptBtn);
	GUI_RegisterObject(w, w->cancelBtn);
	GUI_RegisterObject(w, w->newDirBtn);
	GUI_RegisterObject(w, w->newFileBtn);
	
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
	for(size_t i = 0; i < VEC_LEN(&w->fbc->entries); i++) {
		VEC_ITEM(&w->fbc->entries, i).isSelected = 0;
	}
	
	w->fbc->numSelected = 0;
}
