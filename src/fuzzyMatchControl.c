
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>


//#include <sys/types.h> // opendir
//#include <dirent.h> // readdir
//#include <unistd.h> // pathconf
//#include <sys/stat.h>




#include "fuzzyMatch.h"
#include "fuzzyMatchControl.h"
#include "app.h" // for execProcess*

#include "ui/gui_internal.h"


static void open_match(GUIFuzzyMatchControl* w, int i);

//#define DBG printf
#define DBG(...) 


#include "ui/macros_on.h"

void GUIFuzzyMatchControl_Render(GUIFuzzyMatchControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	
//	if(GUI_MouseInside(tl, sz)) {
//		ACTIVE(&w->searchTerm);
//	}

	if(gm->activeID == w) {
		ACTIVE(&w->searchTerm);
	}
	

	gm->curZ += 10;
	
	DEFAULTS(GUIEditOpts, eopts);
	for(int i = 0; i < 3; i++) {
		eopts[i].selectAll = 1;
	}
	if(GUI_Edit_(gm, &w->searchTerm, tl, sz.x, &w->searchTerm, eopts)) {
		GUIFuzzyMatchControl_Refresh(w);
	}
	gm->curZ -= 10;


	if(GUI_InputAvailable()) {
		GUI_Cmd* cmd = Commands_ProbeCommandMode(gm, GUIELEMENT_FuzzyMatcher, &gm->curEvent, 0, NULL);
		
		if(cmd) {
			int cmd_result = GUIFuzzyMatchControl_ProcessCommand(w, cmd);
			switch(cmd_result) {
				case 0:
					GUI_CancelInput();
					break;
				case 1:
					// command not handled
					break;
				case 2: // editor is gone or other reason to process no more commands
					GUI_CancelInput();
					return;
				default:
					dbg("unexpected GUIFuzzyMatchControl_ProcessCommand result [%d]", cmd_result);
			}
		}
		
		if(GUI_MouseWentUp(1)) {
			if(GUI_MouseInside(tl, sz)) {
				ACTIVE(&w->searchTerm);
			}
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
	
	gm->curZ++;
	
	for(intptr_t i = 0; w->matches && i < w->matchCnt; i++) {
		if(w->matches[i].excluded) {
//			DBG("Match <%s> excluded [%d]\n", w->matches[i].filepath, w->matches[i].excluded);
			continue;
		}
		
//		DBG("rendering match: %ld\n", i);
	
		if(lh * linesDrawn > sz.y) break; // stop at the bottom of the window
		
		Vector2 btl_proj = {tl.x + gutter, tl.y + 35 + (lh * linesDrawn)};
		Vector2 btl_file = {btl_proj.x + w->proj_gutter + gutter, tl.y + 35 + (lh * linesDrawn)};
		Vector2 bsz = {sz.x - gutter, (lh)};
		
		if(w->cursorIndex == i) { // backgrounds for selected items
			struct Color4* color = &gm->defaults.selectedItemBgColor;
			GUI_Rect(btl_proj, bsz, color);
		}

		gm->curZ++;
		
		// the project name
		GUI_TextLineAdv(btl_proj, bsz, w->matches[i].projname, strlen(w->matches[i].projname), GUI_TEXT_ALIGN_VCENTER, w->font, w->fontsize, &gm->defaults.selectedItemTextColor);
		// the file name
		GUI_TextLineAdv(btl_file, bsz, w->matches[i].filepath, strlen(w->matches[i].filepath), GUI_TEXT_ALIGN_VCENTER, w->font, w->fontsize, &gm->defaults.selectedItemTextColor);
		
		gm->curZ--;
		
		linesDrawn++;
	}

}

#include "ui/macros_off.h"




/*

static void userEvent(GUIHeader* w_, GUIEvent* gev) {
	GUIFuzzyMatchControl* w = (GUIFuzzyMatchControl*)w_;
	
	if((GUIEdit*)gev->originalTarget == w->searchBox) {
		if(0 == strcmp(gev->userType, "change")) {
			w->cursorIndex = 0;
						
			if(w->searchTerm) free(w->searchTerm);
			w->searchTerm = strndup(gev->userData, gev->userSize);
			
			GUIFuzzyMatchControl_Refresh(w);
		}
	}
}



*/



int GUIFuzzyMatchControl_ProcessCommand(GUIFuzzyMatchControl* w, GUI_Cmd* cmd) {

	switch(cmd->cmd) {
		case GUICMD_FuzzyMatcher_Exit:
			MessagePipe_Send(w->upstream, MSG_CloseMe, w, NULL);
			return 2;
			
		case GUICMD_FuzzyMatcher_MoveCursorV:
			if(w->matchCnt == 0) break;
			w->cursorIndex = (cmd->amt + w->cursorIndex + w->matchCnt) % w->matchCnt;
			break;
			
		case GUICMD_FuzzyMatcher_OpenFile: {
			if(w->matchCnt == 0) break;
			
			int openinplace = w->gs->MainControl_openInPlace;
			open_match(w, w->cursorIndex);
			if(openinplace) return 2;
			break;
		}
		
		default:
			return 1; // command not handled
	}
	
	return 0;
}


GUIFuzzyMatchControl* GUIFuzzyMatchControl_New(GUIManager* gm, Settings* s, MessagePipe* mp, char* path, char* searchTerm) {

	
	
	GUIFuzzyMatchControl* w = pcalloc(w);
	w->upstream = mp;
	w->gs = Settings_GetSection(s, SETTINGS_General);

//	w->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
//	w->header.cmdElementType = CUSTOM_ELEM_TYPE_FuzzyMatcher;
	
	w->fontsize = 16;
	#define MAGIC_UI_FONTNAME "Arial"
	GUIFont* font = GUI_FindFont(gm, MAGIC_UI_FONTNAME);
	if(!font) font = gm->defaults.font;
	w->font = font;
	
	
	
	w->lineHeight = 25;
	w->leftMargin = 20;
	
	w->proj_gutter = 20;
	
	int n_paths = 0;
	char** projnames;
	while(w->gs->MainControl_searchPaths[n_paths]) {
		n_paths++;
	}
	projnames = pcallocn(projnames, n_paths + 1);
	
	char** parts;
	long out_len = 0;
	for(int i=0;i<n_paths;i++) {
		parts = strsplit(w->gs->MainControl_searchPaths[i], '/', &out_len);
		if(!parts || !out_len) continue;
		projnames[i] = strdup(parts[out_len-1]);
		float txtw = gui_getTextLineWidth(gm, w->font, w->fontsize, projnames[i], strlen(projnames[i]));
		w->proj_gutter = MAX(w->proj_gutter, txtw);
		for(int j=0;j<out_len;j++) free(parts[j]);
		free(parts);
	}
	projnames[n_paths] = NULL;
	w->projnames = projnames;
	
	
	if(searchTerm) {
		GUIString_Set(&w->searchTerm, searchTerm);
	}
	else {
		GUIString_Init(&w->searchTerm);
	}
	
	
	return w;
}

void GUIFuzzyMatchControl_Refresh(GUIFuzzyMatchControl* w) {
//	printf("seearch term: '%s'\n", w->searchTerm.data);
	size_t max_candidates = 1024;
	fcandidate* candidates = NULL;
	size_t n_candidates = 0;

	size_t n_filepaths = 0;
	char** contents = NULL;
	char*** stringBuffers = NULL;
	
	w->cursorIndex = 0;
	
	w->searchTerm.data[w->searchTerm.len] = '\0';
	
	DBG("~~ begin fuzzy opener <search: %s>\n", w->searchTerm.data);
	char* cmd = "/usr/bin/git";
	char* args[] = {cmd, "-C", NULL, "ls-files", "-co", "--exclude-standard", NULL};
	
	int i = 0;
	int j = 0;
	int n_paths = 0;
	
	if(!w->searchTerm.data || !w->searchTerm.len) {
		goto CLEANUP;
	}
	
	while(w->gs->MainControl_searchPaths[n_paths]) {
		n_paths++;
	}
	if(n_paths == 0) return;

	candidates = pcallocn(candidates, max_candidates);
	contents = pcallocn(contents, n_paths + 1);
	stringBuffers = pcallocn(stringBuffers, n_paths + 1);
	
	i = 0;
	while(w->gs->MainControl_searchPaths[i]) {
		args[2] = w->gs->MainControl_searchPaths[i];
		contents[i] = execProcessPipe_charpp(args, &stringBuffers[i], &n_filepaths);
		DBG("result: %ld filepaths\n", n_filepaths);

		if(n_candidates+n_filepaths >= max_candidates) {
			DBG("fps: %lu, mc: %lu\n", n_filepaths, max_candidates);
			max_candidates = 2*MAX(n_filepaths, max_candidates);
			DBG("mc: %lu, size: %lu\n", max_candidates, sizeof(*candidates));
			candidates = realloc(candidates, max_candidates*sizeof(*candidates));
		}
		
		for(j = 0; j < n_filepaths; j++) {
//			DBG("got filepath: %s\n", stringBuffers[i][j]);
			
			candidates[n_candidates + j].excluded = 0;
			for(int k = 0; w->gs->MainControl_excludePatterns[k]; k++) {
				if(strstr(stringBuffers[i][j], w->gs->MainControl_excludePatterns[k])) {
//					DBG("Excluded: %s [pattern: %s]\n", stringBuffers[i][j], w->gs->MainControl_excludePatterns[k]);
					candidates[n_candidates+j].excluded = 1;
					break;
				}
			}
			
			candidates[n_candidates + j].basepath = w->gs->MainControl_searchPaths[i];
			candidates[n_candidates + j].filepath = stringBuffers[i][j];
			candidates[n_candidates + j].projname = w->projnames[i];
//			candidates[n_candidates + j].render_line = sprintfdup("%s %s"
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

	if(w->candidates) {
		free(w->candidates);
	}
	w->candidates = candidates;

	char* input = w->searchTerm.data;

	fcandidate* matches = NULL;
	int n_matches = 0;
	int err = -1;

	if(input) {
		err = fuzzy_match_fcandidate(candidates, n_candidates, &matches, &n_matches, input, 0);
		DBG("fuzzy match exit code: %d\n", err);
	}

	if(w->matches) free(w->matches);
	if(!err) {
		w->matches = matches;
		w->matchCnt = n_matches;
		DBG("match count at end: %ld\n", w->matchCnt);
	//	for(i=0;i<n_matches;i++) {
	//		printf("ordered match [%s]\n", matches[i]);
	//	}
	}
	else {
		w->matches = NULL;
		w->matchCnt = 0;
	}
	
	
	//free(matches);
	// free(filepaths);

}


void GUIFuzzyMatchControl_SaveSessionState(GUIFuzzyMatchControl* w, json_value_t* out) {
	if(!w->searchTerm.len) return;
	
	w->searchTerm.data[w->searchTerm.len] = '\0';
	json_obj_set_key(out, "path", json_new_str(w->searchTerm.data));
}


void GUIFuzzyMatchControl_Destroy(GUIFuzzyMatchControl* w) {
	int i;
	for(i=0; w->projnames[i]; i++) {
		free(w->projnames[i]);
	}
	free(w->projnames);
	
	if(w->stringBuffers) {
		i = 0;
		while(w->stringBuffers[i]) {
			free(w->stringBuffers[i]);
			i++;
		}
		free(w->stringBuffers);
	}

	if(w->contents) {
		i = 0;
		while(w->contents[i]) {
			free(w->contents[i]);
			i++;
		}
		free(w->contents);
	}

	if(w->matches) {
		free(w->matches);
	}

	free(w);
}



static void open_match(GUIFuzzyMatchControl* w, int i) {

	char* path_raw = path_join(w->matches[i].basepath, w->matches[i].filepath);
	char* path = resolve_path(path_raw);
	
	MessageFileOpt opt = {0};
	opt = (MessageFileOpt){
		.path = path,
		.line_num = 1,
		.paneTargeter = -1,
	};
	
	if(w->gs->MainControl_openInPlace) {
		opt.set_focus = 1;
	}
	
	MessagePipe_Send(w->upstream, MSG_OpenFileOpt, &opt, NULL);

	if(w->gs->MainControl_openInPlace) {
		MessagePipe_Send(w->upstream, MSG_CloseMe, w, NULL);
	}

	free(path_raw);
	free(path);
	
}


#undef DBG

