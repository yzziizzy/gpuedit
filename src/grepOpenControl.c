
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


//#include <sys/types.h> // opendir
//#include <dirent.h> // readdir
//#include <unistd.h> // pathconf
//#include <sys/stat.h>


#include "ui/gui.h"
#include "ui/gui_internal.h"

#include "grepOpenControl.h"
#include "commands.h"
#include "app.h" // for execProcess*


// #define DEBUG(...) printf(__VA_ARGS__)
#define DEBUG(...)


char* split_result(char* grep_result) {
	if(!grep_result) {
		return NULL;
	}

	for(int i=0;;i++) {
		switch(grep_result[i]) {
			case '\0':
				return NULL;
				break;
			case '\\':
				if(grep_result[i+1] != '\0') {
					i++;
				} else {
					return NULL;
				}
				break;
			case ':':
				grep_result[i] = '\0';
				return &grep_result[i+1];
				break;
		}
	}

	return NULL;
}


static void render(GUIHeader* w_, PassFrameParams* pfp) {
	GUIGrepOpenControl* w = (GUIGrepOpenControl*)w_;
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


	// 	drawTextLine();
	float lh = w->lineHeight;
	float gutter = w->leftMargin + 20;

	int linesDrawn = 0;

	for(intptr_t i = 0; w->matches && i < w->matchCnt; i++) {
		if(lh * linesDrawn > w->header.size.y) break; // stop at the bottom of the window

		AABB2 box;
		box.min.x = tl.x + gutter;
		box.min.y = tl.y + 30 + (lh * linesDrawn);
		box.max.x = tl.x + 800;
		box.max.y = tl.y + 30 + (lh * (linesDrawn + 1));


		if(w->cursorIndex == i) { // backgrounds for selected items
			struct Color4* color = &gm->defaults.selectedItemBgColor;

			v = GUIManager_reserveElements(gm, 1);
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
		gui_drawTextLine(gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x - box.min.x,0}, &w->header.absClip, &gm->defaults.selectedItemTextColor , 10000000, w->matches[i].render_line, strlen(w->matches[i].render_line));

		linesDrawn++;
	}



	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIHeader* w_, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIGrepOpenControl* w = (GUIGrepOpenControl*)w_;

	w->searchBox->header.topleft.y = 0;
	w->searchBox->header.topleft.x = 0;
	w->searchBox->header.size.y = 25;


	gui_defaultUpdatePos(&w->header, grp, pfp);
}



static void userEvent(GUIHeader* w_, GUIEvent* gev) {
	GUIGrepOpenControl* w = (GUIGrepOpenControl*)w_;

	if((GUIEdit*)gev->originalTarget == w->searchBox) {
		if(0 == strcmp(gev->userType, "change")) {
			w->cursorIndex = 0;

			if(w->searchTerm) free(w->searchTerm);
			w->searchTerm = strndup(gev->userData, gev->userSize);

			GUIGrepOpenControl_Refresh(w);
		}
		else if(0 == strcmp(gev->userType, "enter")) {
			Cmd found = {.cmd = GrepOpenCmd_Open};
			GUIGrepOpenControl_ProcessCommand(w, &found);
		}
	}
}



static void keyDown(GUIHeader* w_, GUIEvent* gev) {
	GUIGrepOpenControl* w = (GUIGrepOpenControl*)w_;

	Cmd found;
	unsigned int iter = 0;
	while(Commands_ProbeCommand(gev, w->commands, 0, &found, &iter)) {
		// GUIBufferEditor will pass on commands to the buffer
		GUIGrepOpenControl_ProcessCommand(w, &found);

	}

}


static void gainedFocus(GUIHeader* w_, GUIEvent* gev) {
	GUIGrepOpenControl* w = (GUIGrepOpenControl*)w_;

	GUIManager_pushFocusedObject(w->header.gm, &w->searchBox->header);
}


void GUIGrepOpenControl_ProcessCommand(GUIGrepOpenControl* w, Cmd* cmd) {
	long amt;

	switch(cmd->cmd) {
		case GrepOpenCmd_CursorMove:
			if(w->matchCnt == 0) break;
			w->cursorIndex = (cmd->amt + w->cursorIndex + w->matchCnt) % w->matchCnt;
			break;

		case GrepOpenCmd_Open: {
			char* path_raw = path_join(w->matches[w->cursorIndex].basepath, w->matches[w->cursorIndex].filepath);
			char* path = resolve_path(path_raw);

			GUIEvent gev2 = {};
			gev2.type = GUIEVENT_User;
			gev2.eventTime = 0;//gev->eventTime;
			gev2.originalTarget = &w->header;
			gev2.currentTarget = &w->header;
			gev2.cancelled = 0;
			// handlers are responsible for cleanup
			gev2.userData = path;
			gev2.userSize = strlen(path);

			gev2.userType = "openFile";

			GUIManager_BubbleEvent(w->header.gm, &w->header, &gev2);

			free(path_raw);
			free(path);

			if(w->gs->MainControl_openInPlace) {
				GUIManager_BubbleUserEvent(w->header.gm, &w->header, "closeMe");
			}

			break;
		}

	}

}


GUIGrepOpenControl* GUIGrepOpenControl_New(GUIManager* gm, char* path) {

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


	GUIGrepOpenControl* w = pcalloc(w);

	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	w->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;

	w->lineHeight = 25;
	w->leftMargin = 20;

	w->searchBox = GUIEdit_New(gm, "");
	w->searchBox->header.flags |= GUI_MAXIMIZE_X;
	w->searchBox->header.gravity = GUI_GRAV_TOP_LEFT;

	GUI_RegisterObject(w, w->searchBox);


	return w;
}

void GUIGrepOpenControl_Refresh(GUIGrepOpenControl* w) {
	//printf("seearch term: '%s'\n", w->searchTerm);
	size_t max_candidates = 1024;
	gocandidate* candidates = NULL;
	size_t n_candidates = 0;

	size_t n_filepaths = 0;
	char** contents = NULL;
	char*** stringBuffers = NULL;

	// printf("launching grep opener\n");
	char* cmd = "/usr/bin/git";
	char* args[] = {cmd, "-C", NULL, "grep", "-niI", "--full-name", w->searchTerm, NULL};

	int i = 0;
	int j = 0;

	if(!w->searchTerm || strlen(w->searchTerm) < 3) {
		goto CLEANUP;
	}

	while(w->gs->MainControl_searchPaths[i]) {
		i++;
	}
	if(i == 0) return;

	candidates = malloc(max_candidates*sizeof(*candidates));

	contents = malloc(sizeof(*contents)*i);
	stringBuffers = malloc(sizeof(*stringBuffers)*i);

	i = 0;
	while(w->gs->MainControl_searchPaths[i]) {
		args[2] = w->gs->MainControl_searchPaths[i];
		contents[i] = execProcessPipe_charpp(args, &stringBuffers[i], &n_filepaths);
		DEBUG("result: %ld filepaths\n", n_filepaths);

		if(n_candidates+n_filepaths >= max_candidates) {
			max_candidates *= 2;
			candidates = realloc(candidates, max_candidates*sizeof(*candidates));
		}
		for(j=0;j<n_filepaths;j++) {
			// DEBUG("got filepath: %s\n", stringBuffers[i][j]);
			candidates[n_candidates+j].basepath = w->gs->MainControl_searchPaths[i];
			candidates[n_candidates+j].filepath = stringBuffers[i][j];
			candidates[n_candidates+j].line = split_result(candidates[n_candidates+j].filepath);
			candidates[n_candidates+j].render_line = sprintfdup("%s:%s",
				candidates[n_candidates+j].filepath,
				candidates[n_candidates+j].line
			);
		}

		i++;
		n_candidates += n_filepaths;
	}
	contents[i] = NULL;
	stringBuffers[i] = NULL;

CLEANUP:
	if(w->stringBuffers) {
		i = 0;
		while(w->stringBuffers[i]) {
			free(w->stringBuffers[i]);
			i++;
		}
		free(w->stringBuffers);
	}
	w->stringBuffers = stringBuffers;

	if(w->contents) {
		i = 0;
		while(w->contents[i]) {
			free(w->contents[i]);
			i++;
		}
		free(w->contents);
	}
	w->contents = contents;

	if(w->matches) {
		for(i=0;i<w->matchCnt;i++) {
			free(w->matches[i].render_line);
		}
		free(w->matches);
	}
	w->matches = candidates;
	w->matchCnt = n_candidates;

	//free(matches);
	// free(filepaths);

}

#undef DEBUG

