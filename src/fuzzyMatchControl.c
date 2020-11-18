
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


//#include <sys/types.h> // opendir
//#include <dirent.h> // readdir
//#include <unistd.h> // pathconf
//#include <sys/stat.h>


#include "gui.h"
#include "gui_internal.h"

#include "fuzzyMatch.h"
#include "fuzzyMatchControl.h"
#include "commands.h"
#include "app.h" // for execProcess*



static void render(GUIObject* w_, PassFrameParams* pfp) {
	GUIFuzzyMatchControl* w = (GUIFuzzyMatchControl*)w_;
	GUIManager* gm = w->header.gm;
	
	Vector2 tl = w->header.absTopLeft;

	
	// 	drawTextLine();
	float lh = w->lineHeight;
	float gutter = w->leftMargin + 20;
	
	int linesDrawn = 0;
	
	for(intptr_t i = 0; w->files && i < w->fileCnt; i++) {
		if(lh * linesDrawn > w->header.size.y) break; // stop at the bottom of the window

		AABB2 box;
		box.min.x = tl.x + gutter;
		box.min.y = tl.y + 30 + (lh * linesDrawn);
		box.max.x = tl.x + 800;
		box.max.y = tl.y + 30 + (lh * (linesDrawn + 1));


		if(w->cursorIndex == i) { // backgrounds for selected items
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



		// the file name
		gui_drawTextLine(gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x - box.min.x,0}, &w->header.absClip, &gm->defaults.tabTextColor , 10000000, w->files[i], strlen(w->files[i]));
		
		linesDrawn++;
	}


	
	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIObject* w_, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIFuzzyMatchControl* w = (GUIFuzzyMatchControl*)w_;
	
	w->searchBox->header.topleft.y = 0;
	w->searchBox->header.topleft.x = 0;
	w->searchBox->header.size.y = 25;


	gui_defaultUpdatePos(&w->header, grp, pfp);
}



static void userEvent(GUIObject* w_, GUIEvent* gev) {
	GUIFuzzyMatchControl* w = (GUIFuzzyMatchControl*)w_;
	
	if((GUIEdit*)gev->originalTarget == w->searchBox) {
		if(0 == strcmp(gev->userType, "change")) {
			w->cursorIndex = 0;
						
			if(w->searchTerm) free(w->searchTerm);
			w->searchTerm = strndup(gev->userData, gev->userSize);
			
			GUIFuzzyMatchControl_Refresh(w);
		}
		else if(0 == strcmp(gev->userType, "enter")) {
			Cmd found = {.cmd = FuzzyMatcherCmd_Open};
			GUIFuzzyMatchControl_ProcessCommand(w, &found);				
		}
	}
}



static void keyDown(GUIObject* w_, GUIEvent* gev) {
	GUIFuzzyMatchControl* w = (GUIFuzzyMatchControl*)w_;

	Cmd found;
	unsigned int iter = 0;
	while(Commands_ProbeCommand(gev, w->commands, 0, &found, &iter)) {
		// GUIBufferEditor will pass on commands to the buffer
		GUIFuzzyMatchControl_ProcessCommand(w, &found);		
		
	}

}


static void gainedFocus(GUIObject* w_, GUIEvent* gev) {
	GUIFuzzyMatchControl* w = (GUIFuzzyMatchControl*)w_;
	
	GUIManager_pushFocusedObject(w->header.gm, w->searchBox);
}


void GUIFuzzyMatchControl_ProcessCommand(GUIFuzzyMatchControl* w, Cmd* cmd) {
	long amt;

	switch(cmd->cmd) {
		case FuzzyMatcherCmd_CursorMove:
			w->cursorIndex = (cmd->amt + w->cursorIndex + w->fileCnt) % w->fileCnt;
			break;
			
		case FuzzyMatcherCmd_Open: {
			GUIEvent gev2 = {};
			gev2.type = GUIEVENT_User;
			gev2.eventTime = 0;//gev->eventTime;
			gev2.originalTarget = w;
			gev2.currentTarget = w;
			gev2.cancelled = 0;
			// handlers are responsible for cleanup
			gev2.userData = w->files[w->cursorIndex];
			gev2.userSize = strlen(w->files[w->cursorIndex]);
			
			gev2.userType = "openFile";
		
			GUIManager_BubbleEvent(w->header.gm, w, &gev2);
			break;
		}
		
	}
	
}


GUIFuzzyMatchControl* GUIFuzzyMatchControl_New(GUIManager* gm, char* path) {

	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.KeyDown = keyDown,
		.GainedFocus = gainedFocus,
		//.Click = click,
		//.DoubleClick = click,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
// 		.ParentResize = parentResize,
		.User = userEvent,
	};
	
	
	GUIFuzzyMatchControl* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	w->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
	
	w->lineHeight = 25;
	w->leftMargin = 20;
	
	w->searchBox = GUIEdit_New(gm, "");
	w->searchBox->header.flags |= GUI_MAXIMIZE_X;
	w->searchBox->header.gravity = GUI_GRAV_TOP_LEFT;
	
	GUIRegisterObject(w, w->searchBox);
	
	
	return w;
}

void GUIFuzzyMatchControl_Refresh(GUIFuzzyMatchControl* w) {
	//printf("seearch term: '%s'\n", w->searchTerm);

	if(!w->searchTerm) return;
	
	
	
	// printf("launching fuzzy opener\n");
	char* cmd = "/usr/bin/git";
	char* base_args[] = {cmd, "-C", NULL, "ls-files", NULL};
	size_t elem = sizeof(base_args);
	size_t base_n = elem/sizeof(char*);
	char*** args;
	char** arg_lists;
	
	int i = 0;
	while(w->header.gm->gs->MainControl_searchPaths[i]) {
		 i++;
	}
	args = malloc((i+1)*sizeof(*args));
	arg_lists = malloc(i*elem);

	i = 0;
	while(w->header.gm->gs->MainControl_searchPaths[i]) {
		memcpy(&(arg_lists[i*base_n]), base_args, elem);
		args[i] = &(arg_lists[i*base_n]);
		args[i][2] = w->header.gm->gs->MainControl_searchPaths[i];

		i++;
	}
	args[i] = NULL;

	char** filepaths;
	size_t n_filepaths;
	if(w->stringBuffer) free(w->stringBuffer);
	w->stringBuffer = execProcessPipe_charppv(args, &filepaths, &n_filepaths);

	free(args);
	free(arg_lists);
	
	// for(i=0;i<n_filepaths;i++) {
	// 	printf("got filepath: %s\n", filepaths[i]);
	// }

	char* input = w->searchTerm;

	char** matches;
	int n_matches = 0;
	int err = 0;

	err = fuzzy_match_charpp(filepaths, n_filepaths, &matches, &n_matches, input, 0);
	// printf("fuzzy match exit code: %d\n", err);

	if(w->files) free(w->files);	
	if(!err) {
		w->files = matches;
		w->fileCnt = n_matches;
		
	//	for(i=0;i<n_matches;i++) {
	//		printf("ordered match [%s]\n", matches[i]);
	//	}
	}
	else {
		w->files = NULL;
		w->fileCnt = 0;
	}
	
	
	//free(matches);
	free(filepaths);

}
