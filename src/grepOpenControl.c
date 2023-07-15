
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


//#include <sys/types.h> // opendir
//#include <dirent.h> // readdir
//#include <unistd.h> // pathconf
//#include <sys/stat.h>


#include "grepOpenControl.h"
#include "app.h" // for execProcess*

// #include "ui/gui.h"
#include "ui/gui_internal.h"


static void open_match(GrepOpenControl* w, int i);


// #define DBG(...) printf(__VA_ARGS__)
#define DBG(...)


#include "ui/macros_on.h"

void GrepOpenControl_Render(GrepOpenControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {

	gm->curZ += 10;
		
	DEFAULTS(GUIEditOpts, eopts);
	eopts.selectAll = 1;
	if(GUI_Edit_(gm, &w->searchTerm, tl, sz.x, &w->searchTerm, &eopts)) {
		GrepOpenControl_Refresh(w);
	}
	
	gm->curZ -= 10;


	if(GUI_InputAvailable()) {
		GUI_Cmd* cmd = Commands_ProbeCommandMode(gm, GUIELEMENT_GrepOpen, &gm->curEvent, 0, NULL);
		
		if(cmd) {
			GrepOpenControl_ProcessCommand(w, cmd);
			GUI_CancelInput();
		}
		
		
		if(GUI_MouseWentUp(1)) {
			// determine the clicked line
			Vector2 mp = GUI_MousePos();
			int cline = floor((mp.y - 20) / w->lineHeight) - 1;
			
			if(cline >= 0 && cline <= (int)w->matchCnt - 1) {
				open_match(w, cline);
				GUI_CancelInput();
			}
		}
	}


	if(!gm->drawMode) return;


	// draw general background
	GUI_Rect(tl, sz, &gm->defaults.windowBgColor);
	
	// 	drawTextLine();
	float lh = w->lineHeight;
	float gutter = w->leftMargin + 20;
	
	int linesDrawn = 0;
	int linesOnScreen = floor((sz.y - 20) / lh); 
	
	if(w->cursorIndex < w->scrollLine + 3) {
		w->scrollLine = MAX(0, w->cursorIndex - 3);
	}
	else if(w->cursorIndex >= w->scrollLine + linesOnScreen - 10) {
		w->scrollLine = w->cursorIndex - linesOnScreen + 10;
	}
	
	gm->curZ++;
	
	for(intptr_t i = w->scrollLine; w->matches && i < w->matchCnt; i++) {
		DBG("rendering match: %ld\n", i);
	
		if(lh * linesDrawn > sz.y) break; // stop at the bottom of the window
		
		Vector2 btl = {tl.x + gutter, tl.y + 20 + (lh * linesDrawn)};
		Vector2 bsz = {sz.x - gutter, (lh)};
		
		if(w->cursorIndex == i) { // backgrounds for selected items
			struct Color4* color = &gm->defaults.selectedItemBgColor;
			GUI_Rect(btl, bsz, color);
		}

		gm->curZ++;
		// the file name
		GUI_TextLine(w->matches[i].render_line, strlen(w->matches[i].render_line), btl, "Arial",  16, &gm->defaults.selectedItemTextColor);
		gm->curZ--;
		
		linesDrawn++;
	}

}

#include "ui/macros_off.h"


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


// sorry about this function
char* cleanup_line(char* line) {
	char* out = line;
	
	if(!out) return out;
	
	for(;;out++) {
		switch(out[0]) {
			case '\0':
				return out;
				break;
			case '\\':
				if(out[1] != '\0') {
					out++;
				} else {
					return out;
				}
				break;
			case ':':
				out++;
				goto COLON_FOUND;
				break;
		}
	}
COLON_FOUND:
	
	for(;;out++) {
		switch(out[0]) {
			case ' ':
			case '\t':
				break;
			default:
				return out;
				break;
		}
	}
	
	return out;
}


void GrepOpenControl_ProcessCommand(GrepOpenControl* w, GUI_Cmd* cmd) {
	long amt;

	switch(cmd->cmd) {
		case GUICMD_GrepOpen_Exit:
			MessagePipe_Send(w->upstream, MSG_CloseMe, w, NULL);
			break;
			
		case GUICMD_GrepOpen_MoveCursorV:
			if(w->matchCnt == 0) break;
			w->cursorIndex = (cmd->amt + w->cursorIndex + w->matchCnt) % w->matchCnt;
			break;

		case GUICMD_GrepOpen_OpenFile: {
			if(w->matchCnt == 0) break;
			
			open_match(w, w->cursorIndex);

			break;
		}

	}

}


GrepOpenControl* GrepOpenControl_New(GUIManager* gm, Settings* s, MessagePipe* mp, char* searchTerm) {

	GrepOpenControl* w = pcalloc(w);
	w->upstream = mp;
	w->bs = Settings_GetSection(s, SETTINGS_Buffer); // lineNumBase, lineNumCharset
	w->gs = Settings_GetSection(s, SETTINGS_General);

	w->lineHeight = 25;
	w->leftMargin = 20;

	if(searchTerm) {
		w->searchTerm.data = strdup(searchTerm);
		w->searchTerm.len = strlen(w->searchTerm.data);
		w->searchTerm.alloc = w->searchTerm.len;
	}
	
	return w;
}

void GrepOpenControl_Refresh(GrepOpenControl* w) {
	//printf("seearch term: '%s'\n", w->searchTerm);
	size_t max_candidates = 1024;
	gocandidate* candidates = NULL;
	size_t n_candidates = 0;

	size_t n_filepaths = 0;
	char** contents = NULL;
	char*** stringBuffers = NULL;

	// printf("launching grep opener\n");
	char* cmd = "/usr/bin/git";
	char* args[] = {cmd, "-C", NULL, "grep", "-niI", "--full-name", w->searchTerm.data, NULL};

	int i = 0;
	int j = 0;
	int n_paths = 0;
	
	char lnbuf[32];

	if(!w->searchTerm.data || w->searchTerm.len < 3) {
		goto CLEANUP;
	}

	while(w->gs->MainControl_searchPaths[n_paths]) {
		n_paths++;
	}
	if(n_paths == 0) return;

	candidates = malloc(max_candidates*sizeof(*candidates));
	contents = malloc(sizeof(*contents)*(n_paths+1));
	stringBuffers = malloc(sizeof(*stringBuffers)*(n_paths+1));

	i = 0;
	while(w->gs->MainControl_searchPaths[i]) {
		args[2] = w->gs->MainControl_searchPaths[i];
		contents[i] = execProcessPipe_charpp(args, &stringBuffers[i], &n_filepaths);
		DBG("result: %ld filepaths\n", n_filepaths);

		if(n_candidates+n_filepaths >= max_candidates) {
			max_candidates = 2 * MAX(n_filepaths, max_candidates);
			candidates = realloc(candidates, max_candidates*sizeof(*candidates));
		}
		for(j=0;j<n_filepaths;j++) {
			// DBG("got filepath: %s\n", stringBuffers[i][j]);
			candidates[n_candidates+j].basepath = w->gs->MainControl_searchPaths[i];
			candidates[n_candidates+j].filepath = stringBuffers[i][j];
			candidates[n_candidates+j].line = split_result(candidates[n_candidates+j].filepath);
			candidates[n_candidates+j].line_num = atol(candidates[n_candidates+j].line);
			candidates[n_candidates+j].line = cleanup_line(candidates[n_candidates+j].line);
			sprintlongb(lnbuf, w->bs->lineNumBase, candidates[n_candidates+j].line_num, w->bs->lineNumCharset);
			candidates[n_candidates+j].render_line = sprintfdup("%s:%s  %s",
				candidates[n_candidates+j].filepath,
				lnbuf,
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
		DBG("freeing %lu matches\n", w->matchCnt);
		for(i=0;i<w->matchCnt;i++) {
			DBG("i: %d, line: %s, render: %s\n", i, w->matches[i].line, w->matches[i].render_line);
			free(w->matches[i].render_line);
			w->matches[i].render_line = NULL;
		}
		free(w->matches);
	}
	w->matches = candidates;
	w->matchCnt = n_candidates;

	//free(matches);
	// free(filepaths);

}


static void open_match(GrepOpenControl* w, int i) {

	char* path_raw = path_join(w->matches[i].basepath, w->matches[i].filepath);
	char* path = resolve_path(path_raw);
	intptr_t line_num = w->matches[i].line_num;
	
	MessageFileOpt opt = {0};
	opt = (MessageFileOpt){
		.path = path,
		.line_num = line_num,
		.set_focus = 0,
		.scroll_existing = 1,
	};
	
	if(w->gs->MainControl_openInPlace) {
		opt.set_focus = 1;
	}
	
	MessagePipe_Send(w->upstream, MSG_OpenFileOpt, &opt, NULL);

	free(path_raw);
	free(path);
	
	if(w->gs->MainControl_openInPlace) {
		MessagePipe_Send(w->upstream, MSG_CloseMe, w, NULL);
	}
}


#undef DBG

