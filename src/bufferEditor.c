#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "commands.h"
#include "ui/gui.h"
#include "ui/gui_internal.h"
#include "clipboard.h"





extern int g_DisableSave;


#include "ui/macros_on.h"

void GUIBufferEditor_Render(GUIBufferEditor* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	// GUI_BeginWindow()
	float top = 0;
	w->trayHeight = 60;
	
	if(gm->activeID == w) {
		ACTIVE(w->ec);
	}
	
	if(w->gotoLineTrayOpen) top += 40;
	
	GUI_BeginWindow(w, tl, sz, gm->curZ, 0);
	
	float sbh = w->statusBarHeight * w->showStatusBar;
	Vector2 ecsz = V(sz.x, sz.y - sbh - w->trayOpen * 60 - top);
		

	if(w->showStatusBar) {
		StatusBar_Render(w->statusBar, gm, V(0, sz.y - sbh), V(sz.x, sbh), pfp);
	}
	
	if(w->gotoLineTrayOpen) {
		gm->curZ += 20;
		GUI_Rect(V(0, 0), V(sz.x, 40), &gm->defaults.trayBgColor);
		
		DEFAULTS(GUIEditOpts, eo);
		eo.selectAll = 1;
		
		ACTIVE(&w->gotoLineNum);
		if(GUI_IntEdit_(gm, &w->gotoLineNum, V(10,10), sz.x - 20, &w->gotoLineNum, &eo)) {
			BufferLine* bl = Buffer_raw_GetLineByNum(w->ec->b, w->gotoLineNum);
				
			if(bl) {
				GBEC_MoveCursorTo(w->ec, bl, 0);
				GBEC_scrollToCursorOpt(w->ec, 1);
			}
		
		}
		gm->curZ -= 20;
		
//		if(gm->activeID == &w->gotoLineNum) GUI_CancelInput();
	}
	
	
	if(w->trayOpen) {
		gm->curZ += 10;
		int update = 0;
		
		GUI_Rect(V(0, sz.y - sbh - 60), V(sz.x, 60), &gm->defaults.trayBgColor);
		
		DEFAULTS(GUIEditOpts, eopts);
		eopts.selectAll = 1;
		if(GUI_Edit_(gm, &w->findQuery, V(10, sz.y - sbh - 55), sz.x - 10 - 200, &w->findQuery, &eopts)) {
			update = 1;
		}
		if(GUI_Edit_(gm, &w->replaceText, V(10, sz.y - sbh - 25), sz.x - 10 - 200, &w->replaceText, &eopts)) {
			
		}
		
		
		GUI_Notify_(gm, 0, &w->replaceText, GUI_CMD_GUISYM_FindBox);
		GUI_Notify_(gm, 0, &w->findQuery, GUI_CMD_GUISYM_FindBox);
		
		/*
		static int find_was_active = 0;
		int needRehighlight; // useless
		if((gm->activeID == &w->findQuery || gm->activeID == &w->replaceText) && !find_was_active) {
			// find focused
			find_was_active = 1;
			GUI_Cmd cmd = {0};
			cmd.src_type = GUI_CMD_SRC_FOCUS;
			cmd.key = GUICMD_Buffer_FindEditControlFocus;
			GUIBufferEditor_ProcessCommand(w, &cmd, &needRehighlight);
		}
		else if((gm->activeID != &w->findQuery && gm->activeID != &w->replaceText)&& find_was_active) {
			// find blurred
			find_was_active = 0;
			GUI_Cmd cmd = {0};
			cmd.cmd = GUICMD_Buffer_FindEditControlBlur;
			GUIBufferEditor_ProcessCommand(w, &cmd, &needRehighlight);
		}
		*/
		

		
		DEFAULTS(GUIButtonOpts, bo)
		bo.size = V(90, 20);
		bo.fontSize = 12;
		
		if(GUI_Button_(gm, ID(&w->findQuery)+1, V(sz.x - 200 + 10, sz.y - sbh - 55), "Find Next", &bo)) {
			update = 2;
		}
		if(GUI_Button_(gm, ID(&w->replaceText)+1, V(sz.x - 200 + 10, sz.y - sbh - 25), "Replace", &bo)) {
			update = 3;
		}
		if(GUI_Button_(gm, ID(&w->replaceText)+2, V(sz.x - 200 + 10 + 95, sz.y - sbh - 25), "Replace All", &bo)) {
			update = 4;
		}

		gm->curZ -= 10;
		
		
		switch(update) {
			case 1:
			case 2:
				GUIBufferEditor_UpdateFindPattern(w, w->findQuery.data);
				
				GUIBufferEditor_RelativeFindMatch(w, 0, 1, w->findState); // this line probably causes the result cycling when typing
				GUIBufferEditor_scrollToCursor(w);
				break;
			case 3:
				GUIBufferEditor_ReplaceNext(w);
				break;
			case 4:
				char* rtext = w->replaceText.data;
				size_t len = w->replaceText.len;
				
				GUIBufferEditor_ReplaceAll(w, w->findState->findSet, rtext);
			
				// HACK
				VEC_TRUNC(&w->findState->findSet->ranges);
				break;
		}
		
		if(!gm->drawMode && 
			gm->curEvent.type == GUIEVENT_KeyDown && gm->curEvent.modifiers == 0 && 
			(gm->curEvent.keycode == XK_ISO_Left_Tab || gm->curEvent.keycode == XK_Tab)
		) {
			if(gm->activeID == &w->findQuery) {
				ACTIVE(&w->replaceText);
				GUI_CancelInput();
			}
			else if(gm->activeID == &w->replaceText) {
				ACTIVE(&w->findQuery);
				GUI_CancelInput();
			}
		}
	}
	
	if(w->saveTrayOpen) {
		// render the save tray
		// above the find tray
	}
	
	// forward activeness to the edit control
	if(gm->activeID == w) ACTIVE(w->ec);

	GUI_PushClip(V(0,top), ecsz);
	GBEC_Render(w->ec, gm, V(0,top), ecsz, pfp);
	GUI_PopClip();
	
	// command processing
	if(!gm->drawMode && GUI_InputAvailable()) {
		size_t numCmds;
		GUI_Cmd* cmd = Commands_ProbeCommand(gm, GUIELEMENT_Buffer, &gm->curEvent, &w->inputState, &numCmds);
		int needRehighlight = 0;
		for(int j = 0; j < numCmds; j++) {
			int cmd_result = GUIBufferEditor_ProcessCommand(w, cmd+j, &needRehighlight);
			switch(cmd_result) {
				case 0:
					GUI_CancelInput();
					if(needRehighlight || cmd[j].flags & GUICMD_FLAG_rehighlight) {
						GUIBufferEditControl_MarkRefreshHighlight(w->ec);
					}
					break;
				case 1:
					// command not handled
					break;
				case 2: // editor is gone or other reason to process no more commands
					GUI_CancelInput();
					return;
				default:
					dbg("unexpected GUIBufferEditor_ProcessCommand result [%d]", cmd_result);
			}
		}
		
		Commands_UpdateModes(gm, &w->inputState, cmd, numCmds);
		
		w->gotoLineTrayOpen = !!(w->inputState.curFlags & GUICMD_MODE_FLAG_showGoToLineBar);
		w->trayOpen = !!(w->inputState.curFlags & GUICMD_MODE_FLAG_showFindBar);
	}
	
	// --------- drawing code ---------
	if(gm->drawMode) {	
		
		if(w->ec->needsRehighlight) {
			GUIBufferEditControl_RefreshHighlight(w->ec);
			w->ec->needsRehighlight = 0;
		}
		
		// the little red recording circle
		if(w->ec->isRecording) {
			gm->curZ += 100;
			GUI_CircleFilled(V(sz.x - 30, sz.y - (sbh) + 2), (sbh - 4), 0, C4(1,0,0,1), C4(1,0,0,1));
			gm->curZ -= 100;
		}
	}
	
	
	GUI_EndWindow();
}

#include "ui/macros_off.h"



// called by the buffer when important changes happen
static void bufferChangeNotify(BufferChangeNotification* note, void* _w) {
	GUIBufferEditor* w = (GUIBufferEditor*)_w;
	
	if(note->action == BCA_DeleteLines) { // bufferline pointers are about to become invalid
		
		if(w->findState) {
			if(BufferLine_IsInRange(w->findState->findLine, &note->sel)) {
				w->findState->findCharS = 0;
				w->findState->findCharE = 0;
				w->findState->findLen = 0;
				w->findState->findLine = note->sel.line[0]->prev;
				if(!w->findState->findLine) {		
					w->findState->findLine = note->sel.line[1]->next;		
				}
			}
			
			if(BufferLine_IsInRange(w->findState->nextFindLine, &note->sel)) {
				w->findState->nextFindChar = 0;
				w->findState->nextFindLine = note->sel.line[0]->prev;
				if(!w->findState->nextFindLine) {		
					w->findState->nextFindLine = note->sel.line[1]->next;		
				}
			}
		
			BufferRangeSet_DeleteLineNotify(w->findState->searchSpace, &note->sel);
			BufferRangeSet_DeleteLineNotify(w->findState->findSet, &note->sel);
		}
		
	}
}



void GUIBufferEditor_SetBuffer(GUIBufferEditor* w, Buffer* b) {
	w->b = b;
	Buffer_RegisterChangeListener(b, bufferChangeNotify, w);
	GUIBufferEditControl_SetBuffer(w->ec, b);
}


GUIBufferEditor* GUIBufferEditor_New(GUIManager* gm, MessagePipe* tx) {

	
	GUIBufferEditor* w = pcalloc(w);
	w->gm = gm;	GUIString_Init(&w->findQuery);
	GUIString_Init(&w->replaceText);
	
	w->ec = GUIBufferEditControl_New(gm);
	
	w->statusBar = StatusBar_New(gm, w);
	w->showStatusBar = !gm->gs->hideStatusBar;
	
	w->tx = tx;
	
//	pcalloc(w->findSet);
//	w->ec->findSet = w->findSet;
	
	

	
	return w;
}



void GUIBufferEditor_Destroy(GUIBufferEditor* w) {
#define SFREE(x) \
do { \
	if(x) { \
		free(x); \
		x = NULL; \
	} \
} while(0)
	
	SFREE(w->sourceFile);
	
	Buffer_DecRef(w->b);
	MessagePipe_Send(w->tx, MSG_BufferRefDec, w, NULL);
	Buffer_Delete(w->b);

	
	// TODO more regex cleanup
	GUIBufferEditor_StopFind(w);
	
//	VEC_FREE(&w->findRanges);
	
	
	free(w);
	
	return;
}

void GUIBufferEditor_SaveSessionState(GUIBufferEditor* w, json_value_t* out) {
	
	json_obj_set_key(out, "path", json_new_str(w->sourceFile));
	json_obj_set_key(out, "line", json_new_int(CURSOR_LINE(w->ec->sel)->lineNum));
	json_obj_set_key(out, "col", json_new_int(CURSOR_COL(w->ec->sel)));
	
	json_value_t* bookmarks = json_new_array();
	int n_bookmarks = 0;
	BufferLine* bl = w->b->first;
	while(bl) {
		if(bl->flags & BL_BOOKMARK_FLAG) {
			json_array_push_tail(bookmarks, json_new_int(bl->lineNum));
			n_bookmarks++;
		}
		
		bl = bl->next;
	}
	if(n_bookmarks) {
		json_obj_set_key(out, "bookmarks", bookmarks);
	}
}


void GUIBufferEditor_LoadSessionState(GUIBufferEditor* w, json_value_t* state) {
	int line = json_obj_get_int(state, "line", 1);
	int col = json_obj_get_int(state, "col", 0);
	
//	GUIBufferEditControl_SetScroll(w->ec, line, col);
	GBEC_MoveCursorToNum(w->ec, line, col);
	GBEC_SetScrollCentered(w->ec, line, 0); // GBEC_scrollToCursorOpt didn't seem to be working here
	
	json_value_t* bookmarks = json_obj_get_val(state, "bookmarks");
	if(bookmarks) {
		json_link_t* blink = bookmarks->arr.head;
		BufferLine* bl = w->b->first;
		while(blink && bl) {
			if(blink->v->type == JSON_TYPE_INT) {
				int64_t ln = blink->v->n;
				if(ln == bl->lineNum) {
					bl->flags |= BL_BOOKMARK_FLAG;
					bl = bl->next;
					blink = blink->next;
				} else if(ln > bl->lineNum) {
					bl = bl->next;
				} else {
					blink = blink->next;
				}
			} else {
				blink = blink->next;
			}
		}
	}
}


void GUIBufferEditor_UpdateSettings(GUIBufferEditor* w, Settings* s) {
	w->s = s;
	w->gs = Settings_GetSection(s, SETTINGS_General);
	w->bs = Settings_GetSection(s, SETTINGS_Buffer);
	w->ts = Settings_GetSection(s, SETTINGS_Theme);
	
	w->statusBarHeight = w->bs->statusBarHeight;
	
	GUIBufferEditControl_UpdateSettings(w->ec, s);
}


// makes sure the cursor is on screen, with minimal necessary movement
void GUIBufferEditor_scrollToCursor(GUIBufferEditor* gbe) {
	GBEC_scrollToCursor(gbe->ec);
}



BufferFindState* BufferFindState_Create(Buffer* b, char* pattern, GUIFindOpt* opt, BufferRange* searchSpace) {
	BufferFindState* st = pcalloc(st);
	st->b = b;
	
	// TODO: findmask
	if(pattern) st->pattern = strdup(pattern);
	if(opt) st->opts = *opt;
	
	
	pcalloc(st->searchSpace);
	if(searchSpace) {
		VEC_PUSH(&st->searchSpace->ranges, BufferRange_Copy(searchSpace));
	}
	else {
		// search the whole file
		BufferRange* r = pcalloc(r);
		r->line[0] = b->first;
		r->col[0] = 0;
		r->line[1] = b->last;
		r->col[1] = b->last->length;
		VEC_PUSH(&st->searchSpace->ranges, r);	
	}
	
	return st;
}

// regenerates all PCRE internal structures
int BufferFindState_CompileRE(BufferFindState* st) {
	int errno;
	PCRE2_SIZE erroff;
	PCRE2_UCHAR errbuf[256];
	//printf("starting RE find: '%s'\n", pattern);
	
	// free previous regex 
	if(st->findRE) {
		pcre2_code_free(st->findRE);
		pcre2_match_data_free(st->findMatch);
		st->findMatch = 0;
	}
	
		
	uint32_t options = 0;
	if(!st->opts.case_cmp) {
		options |= PCRE2_CASELESS;
	}
	
	if(st->opts.match_mode == GFMM_PLAIN) {
		options |= PCRE2_LITERAL;
	}

	st->findRE = pcre2_compile((PCRE2_SPTR)st->pattern, PCRE2_ZERO_TERMINATED, options, &errno, &erroff, NULL);
	if(!st->findRE) {
		pcre2_get_error_message(errno, errbuf, sizeof(errbuf));
		st->findREError = strdup(errbuf);
		st->findREErrorChar = erroff;
		L1("PCRE find error #1: '%s' \n", errbuf);
		
		return 1;
	}
	
	// compilation was successful. clear any old errors.
	if(st->findREError) {
		free(st->findREError);
		st->findREError = 0;
		st->findREErrorChar = -1;
		L1("PCRE find error #2\n");
	}
	
	st->findMatch = pcre2_match_data_create_from_pattern(st->findRE, NULL);
		
	return 0;
}


// used to change the search query without changing any other parameters or find state
int GUIBufferEditor_UpdateFindPattern(GUIBufferEditor* w, char* s) {
	int ret;
	if(!w->findState) return 1;
	
	if(w->findState->pattern) free(w->findState->pattern);
	w->findState->pattern = strdup(s);
	
	if(ret = BufferFindState_CompileRE(w->findState)) return ret;
	
	return BufferFindState_FindAll(w->findState);
}

// prepares and initializes internal BufferEditor data with the provided findstate struct
// takes ownership of st
int GUIBufferEditor_StartFind(GUIBufferEditor* w, BufferFindState* st) {
	
	if(w->findState) {
		BufferFindState_FreeAll(w->findState);
		free(w->findState);
	}
	
	BufferFindState_CompileRE(st);
	w->findState = st;
	
	st->findIndex = -1;
	
	st->nextFindLine = CURSOR_LINE(w->ec->sel);
	st->nextFindChar = CURSOR_COL(w->ec->sel);
	
	return 0;
}


int GUIBufferEditor_SmartFind(GUIBufferEditor* w, char* charSet, FindMask_t mask) {
//	if(w->trayOpen) {
//		GUIBufferEditor_CloseTray(w);
//	}
	
//	w->inputMode = BIM_Find;
//	w->trayOpen = 1;
	
	BufferRange sel = {};
	Buffer* b = w->ec->b;
	char* str = NULL;
	int fix_cursor = 0;
	
	if((mask & FM_SELECTION) && HAS_SELECTION(w->ec->sel) && w->ec->sel->line[0] == w->ec->sel->line[1]) {
		GUIBufferEditor_StopFind(w);
		str = Buffer_StringFromSelection(b, w->ec->sel, NULL);
		fix_cursor = 1;
		printf("Stopping find and setting search from selection string <%s>\n", str);
	}
	else if(!str && (mask & FM_SEQUENCE) && charSet) {
		Buffer_GetSequenceUnder(b, CURSOR_LINE(w->ec->sel), CURSOR_COL(w->ec->sel), charSet, &sel);
		if((sel.line[0] == sel.line[1]) && (sel.col[1] - sel.col[0] > 0)) {
			GUIBufferEditor_StopFind(w);
			str = Buffer_StringFromSelection(b, &sel, NULL);
			fix_cursor = 1;
			printf("Stopping find and setting search from sequence string <%s>\n", str);
		}
		else if(w->findState) {
			printf("Continuing with existing find state\n");
			w->findState->findSet->changeCounter++;
			return GUIBufferEditor_RelativeFindMatch(w, 1, 1, w->findState);
		}
		// unhandled / uninitialized else case?
		else {
			printf("unhandled / uninitialized else case. First search or findstate was cleared?\n");
			str = strdup("");
			fix_cursor = 1;
		}
	}
	else {
		str = strdup("");
		fix_cursor = 1;
	}
	
	if(str) {
		GUIString_Set(&w->findQuery, str);
	}
	if(fix_cursor) {
		// GUI_Edit_Trigger_(w->gm, void * id, &w->findQuery, XK_End);
		// move cursor to end of findQuery 
	}
	
	BufferRange* searchSpace = NULL;
	if(mask & FM_WITHIN_SELECTION && HAS_SELECTION(w->ec->sel) && w->ec->sel->line[0] != w->ec->sel->line[1]) { 
		searchSpace = BufferRange_Copy(w->ec->sel);
	}
	
	BufferFindState* st = BufferFindState_Create(w->b, str, NULL, searchSpace);
	if(searchSpace) {
		w->ec->findSearchSpace = st->searchSpace;
		GBEC_ClearAllSelections(w->ec);
	}
	GUIBufferEditor_StartFind(w, st);
	
	
	st->opts.match_mode = GFMM_PCRE;
	BufferFindState_FindAll(st);
	w->ec->findSet = st->findSet;
	
	// locate the match at/after the cursor
	GUIBufferEditor_RelativeFindMatch(w, 1, 1, st);
		
//	GUIBufferEditor_scrollToCursor(w);
	
//	GUIManager_pushFocusedObject(w->header.gm, &w->findBox->header);
	
	
	if(str) free(str);
	
	return 0;
}


int GUIBufferEditor_RelativeFindMatch(GUIBufferEditor* w, int offset, int continueFromCursor, BufferFindState* st) {
	if(!st || !st->findSet) {
//		printf("no findset\n");
		return 1;
	}
	
	if(!VEC_LEN(&st->findSet->ranges)) {
//		GUIText_setString(w->findResultsText, "No results");
//		printf("find set empty\n");
		return 2;
	}
	
	if(st->findSet->changeCounter != w->b->changeCounter) {
		BufferFindState_FindAll(st); // used to pass in the find query box text
		st->findIndex = -1;
//		printf("reset find index\n");
	}
	
	BufferRange* r = NULL;
//	GUIBufferControl* ec = w->ec;
	intptr_t line = CURSOR_LINE(w->ec->sel)->lineNum;
	intptr_t col = CURSOR_COL(w->ec->sel);
	
	if(st->findIndex == -1) {
		if(continueFromCursor && (offset > 0)) {
			VEC_EACH(&st->findSet->ranges, i, range) {
				if(range->line[0]->lineNum < line) {
					continue;
				} 
				else if((range->line[0]->lineNum == line) && (range->col[0] < col)) {
					continue;
				}
				
				st->findIndex = i;
				r = range;
				break;
			}
		} 
		else if(continueFromCursor && (offset < 0)) {
			VEC_R_EACH(&st->findSet->ranges, i, range) {
				if(range->line[0]->lineNum > line) {
					continue;
				} 
				else if((range->line[0]->lineNum == line) && (range->col[0] >= col)) {
					continue;
				}
				
				st->findIndex = i;
				r = range;
				break;
			}
		}
	}
	
	if(!r && VEC_LEN(&st->findSet->ranges)) {
		st->findIndex = (st->findIndex + offset + VEC_LEN(&st->findSet->ranges)) % VEC_LEN(&st->findSet->ranges);
		
		r = VEC_ITEM(&st->findSet->ranges, st->findIndex);
	}
	else if(!r) {
		return 3;
	}
//	char fmt_buffer[420];
//	snprintf(fmt_buffer, 420, "%ld of %ld", st->findIndex + 1, VEC_LEN(&st->findSet->ranges));
//	GUIText_setString(w->findResultsText, fmt_buffer);
	
	GBEC_MoveCursorTo(w->ec, r->line[0], r->col[0]);
	GBEC_SetCurrentSelection(w->ec, r->line[0], r->col[0], r->line[1], r->col[1]);
	
	
	return 0;
};



void GUIBufferEditor_StopFind(GUIBufferEditor* w) {
	printf("Find state cleared by StopFind\n");
	
	w->ec->findSearchSpace = NULL;
	BufferFindState_FreeAll(w->findState);
	free(w->findState);
	w->findState = NULL;
}

void BufferFindState_FreeAll(BufferFindState* st) {
	printf("Requested to free find state\n");
	if(!st) return;
	printf("Freeing find state\n");
	// clean up errors
	if(st->findREError) {
		free(st->findREError);
		st->findREError = NULL;
		st->findREErrorChar = -1;
	}

	// clean up the find set
	BufferRangeSet_FreeAll(st->findSet);
	st->findIndex = -1;
	
	// cached strings
	if(st->pattern) free(st->pattern), st->pattern = NULL;
	if(st->replaceText) free(st->replaceText), st->replaceText = NULL;
}

// very broken after multicursor
int GUIBufferEditor_FindWord(GUIBufferEditor* w, char* word) {
	Buffer* b = w->b;
	
	fprintf(stderr, "%s is very broken", __func__);
	
	if(!b->first) return 1;
	
	BufferLine* bl = b->first;
	while(bl) {
		if(bl->buf) {
			char* r = strstr(bl->buf, word);//, bl->length); 
			if(r != NULL) {
				intptr_t dist = r - bl->buf;
				
// 				printf("found: %d, %d\n", bl->lineNum, dist);
				
				
				if(!w->ec->sel->line[1]) w->ec->sel = calloc(1, sizeof(*w->ec->sel));
				w->ec->sel->selecting = 1; 
				w->ec->sel->line[0] = bl; 
				w->ec->sel->line[1] = bl; 
				w->ec->sel->col[0] = dist;
				w->ec->sel->col[1] = dist + strlen(word);
				
				return 0;
			}
		}
		bl = bl->next;
	}
	
	return 1;
// 	printf("not found: '%s'\n", word);
}



int BufferFindState_FindAll(BufferFindState* st) {

	switch(st->opts.match_mode) {
		case GFMM_PLAIN:
		case GFMM_PCRE:
			return BufferFindState_FindAll_PCRE(st);
			break;
		case GFMM_FUZZY:
			return BufferFindState_FindAll_Fuzzy(st);
			break;
		default:
	}

	return 1;
}


int BufferFindState_FindAll_Fuzzy(BufferFindState* st) {
	return 1;
}


int BufferFindState_FindAll_PCRE(BufferFindState* st) {
	
	
	
	if(!st->findSet) {
		st->findSet = pcalloc(st->findSet);
		st->findSet->changeCounter = st->b->changeCounter;
	}
	else {
		// TODO: free ranges internally
		VEC_TRUNC(&st->findSet->ranges);
	}
	

	BufferLine* findLine;
	intptr_t findCharS;
	intptr_t findCharE;
	intptr_t findLen;
	char* findREError;
	int findREErrorChar;
	
	if(BufferFindState_CompileRE(st)) {
		return 1;
	}
		

	BufferLine* nextFindLine;
	intptr_t nextFindChar = 0;
	
	
	VEC_EACH(&st->searchSpace->ranges, ssri, ssr) {
		BufferLine* bl = ssr->line[0];
		int off = 0; // this is for partial matches
		uint32_t opts = PCRE2_NOTEMPTY | PCRE2_NOTEMPTY_ATSTART;
		int res;
		int wraps = 0;
		
		while(bl && bl->lineNum <= ssr->line[1]->lineNum) {
			
			colnum_t slen = bl->length;
			if(bl == ssr->line[1]) slen = MIN(ssr->col[1], slen);
			if(bl == ssr->line[0]) slen -= ssr->col[0];
			
//			if(bl->lineNum > 700 && bl->lineNum < 900) {
//				dbg("off %d, slen: %d, nfc: %ld, ssrline1 %d, ssrline0 %d", off, slen, nextFindChar, ssr->col[1], ssr->col[0]);
//				dbg("matching line %d at <%s>", bl->lineNum, bl->buf + nextFindChar);
//			}
			res = pcre2_match(st->findRE, bl->buf + nextFindChar, slen - nextFindChar, off, opts, st->findMatch, NULL);
//, slen, off, opts, st->findMatch, NULL);
			if(res > 0) {
				// found a match
			
				PCRE2_SIZE* ovec = pcre2_get_ovector_pointer(st->findMatch);
				
				BufferRange* r  = pcalloc(r);
				r->line[0] = bl;
				r->col[0] = (int)ovec[0] + nextFindChar;
				r->line[1] = bl;
				r->col[1] = (int)ovec[1] + nextFindChar;
				
				VEC_PUSH(&st->findSet->ranges, r);
				
				nextFindChar += (int)ovec[1];  
				
//				printf("match found at: %d:%ld\n", bl->lineNum, (int)ovec[0] + nextFindChar);
				
				if(nextFindChar >= bl->length) {
					bl = bl->next;
					nextFindChar = 0;
				}
			
			}
			else {
				// no match
				
				if(res != PCRE2_ERROR_NOMATCH) {
					// real error of some sort	
					char errbuf[256];
					pcre2_get_error_message(res, errbuf, sizeof(errbuf));
					
					printf("PCRE real error: %p %p %ld '%s'\n", st->findRE, bl->buf, (uint64_t)bl->lineNum, errbuf);
					
					return NULL;
				}
				
				bl = bl->next;
				nextFindChar = 0;
			}
			
			if(!bl) { // end of file
				break;
			}
		}
	}

		
	return 0;
}


int GUIBufferEditor_ReplaceNext(GUIBufferEditor* w) {
		Buffer* b = w->ec->b;
		GUIBufferEditControl* ec = w->ec;
		
		if(!w->findState->findSet || w->findState->findSet && !VEC_LEN(&w->findState->findSet->ranges)) return 1;
		
		BufferRange* r = VEC_ITEM(&w->findState->findSet->ranges, w->findState->findIndex);
		
		if(r) {
			Buffer_DeleteSelectionContents(b, r);
			
			char* rtext = w->replaceText.data;
			size_t len = w->replaceText.len;
			
			Buffer_LineInsertChars(b, r->line[0], rtext, r->col[0], len);
			GBEC_MoveCursorTo(ec, r->line[0], r->col[0]);
			GBEC_MoveCursorH(ec, ec->sel, len);
		}
		
		GUIBufferEditor_RelativeFindMatch(w, 1, 1, w->findState);
		
		return 0;
}


int GUIBufferEditor_ReplaceAll(GUIBufferEditor* w, BufferRangeSet* rset, char* text) {
	if(!VEC_LEN(&rset->ranges)) return 1;
	
	Buffer* b = w->ec->b;
	GUIBufferEditControl* ec = w->ec;
	size_t len = strlen(text);
			
	VEC_R_EACH(&rset->ranges, i, r) {
		if(r) {
			Buffer_DeleteSelectionContents(b, r);
			Buffer_LineInsertChars(b, r->line[0], text, r->col[0], len);
		}
	}
	return 0;
}


void GUIBufferEditor_MoveCursorTo(GUIBufferEditor* gbe, intptr_t line, intptr_t col) {
	if(line < 1) line = 1;
	if(col < 0) col = 0;
	CURSOR_LINE(gbe->ec->sel) = Buffer_raw_GetLineByNum(gbe->b, line);
	CURSOR_COL(gbe->ec->sel) = MIN(col, CURSOR_LINE(gbe->ec->sel)->length); // TODO: check for bounds
}





int GUIBufferEditor_ProcessCommand(GUIBufferEditor* w, GUI_Cmd* cmd, int* needRehighlight) {
//	GUIEdit* e;
	struct json_file* jsf;
	GUIManager* gm = w->gm;
	
//	// keep this command around if a macro is being recorded
//	if(w->isRecording && cmd->cmd != GUICMD_Buffer_MacroToggleRecording) {
//		BufferEditorMacro* m = &RING_HEAD(&w->macros);
//		VEC_PUSH(&m->cmds, *cmd);
//		printf("command pushed\n");
//	}
	
	switch(cmd->cmd){
		case GUICMD_Buffer_SetMode:
//			w->inputMode = cmd->amt;
			break;
			
		case GUICMD_Buffer_PushMode:
//			VEC_PUSH(&w->inputModeStack, w->inputMode);
//			w->inputMode = cmd->amt;
			break;
			
		case GUICMD_Buffer_PopMode:
//			if(VEC_LEN(&w->inputModeStack)) {
//				VEC_POP(&w->inputModeStack, w->inputMode);
//			}
//			else
//				w->inputMode = 0;
			break;

		/*
		case GUICMD_Buffer_MacroToggleRecording: 
			w->isRecording = !w->isRecording;
			if(w->isRecording) {
				RING_PUSH(&w->macros, (BufferEditorMacro){});
				BufferEditorMacro* m = &RING_HEAD(&w->macros);
				VEC_TRUNC(&m->cmds);
			}
			break;
			
		case GUICMD_Buffer_MacroReplay: 
			GUIBufferEditor_ReplayMacro(w, cmd->amt + w->isRecording);
			break;
		*/
			
		case GUICMD_Buffer_ToggleGDBBreakpoint: {
			CURSOR_LINE(w->ec->sel)->flags ^= BL_BREAKPOINT_FLAG;
			
			//if(w->ec->current->flags & BL_BREAKPOINT_FLAG) {
			// actually toggles breakpoint
				if(w->setBreakpoint) 
					w->setBreakpoint(w->sourceFile, CURSOR_LINE(w->ec->sel)->lineNum, w->setBreakpointData);
			//}
			
			break;
		}

		
			
			// TODO: init selectoin and pivots if no selection active
	
			
		case GUICMD_Buffer_GoToLineLaunch: 
//			w->gotoLineTrayOpen = 1;
//			w->inputMode = 3;
			GUI_SetActive(&w->gotoLineNum);
		/*
			if(w->inputMode != BIM_GoTo) {
				if(w->trayOpen) {
					GUIBufferEditor_CloseTray(w);
				}
				
				w->inputMode = BIM_GoTo;
				w->trayOpen = 1;
				w->header.cmdMode = BIM_GoTo;
				
				w->trayRoot = (GUIWindow*)GUIManager_SpawnTemplate(w->header.gm, "goto_tray");
				GUI_RegisterObject(w, w->trayRoot);
				w->lineNumEntryBox = (GUIEdit*)GUI_FindChild(w->trayRoot, "goto_line");
				
				GUIManager_pushFocusedObject(w->header.gm, &w->lineNumEntryBox->header);
			}
			else {
				GUIBufferEditor_CloseTray(w);
				GUIManager_popFocusedObject(w->header.gm);
			}*/
		// TODO: change hooks
			break;
		
		case GUICMD_Buffer_GoToLineCancel:
//			w->gotoLineTrayOpen = 0;
//			w->inputMode = 0;
			break;		
		
		case GUICMD_Buffer_GoToLineSubmit: 
//			w->gotoLineTrayOpen = 0;
//			w->inputMode = 0;
			break;
		
		
	///////////////////////////////////
	// START FIND & REPLACE COMMANDS //
	///////////////////////////////////
		
		case GUICMD_Buffer_ReplaceNext:
			GUIBufferEditor_ReplaceNext(w);
			break;
		
		case GUICMD_Buffer_ReplaceAll: {
			char* rtext = w->replaceText.data;
			size_t len = w->replaceText.len;
			
			GUIBufferEditor_ReplaceAll(w, w->findState->findSet, rtext);
		
			// HACK
			VEC_TRUNC(&w->findState->findSet->ranges);
			 
			break;
		}
		
		case GUICMD_Buffer_ReplaceStart:
		/*
			if(w->inputMode != BIM_Replace) {
				char* preserved = NULL;
				if(w->trayOpen) {
					GUIBufferEditor_CloseTray(w);
				}
				
				w->inputMode = BIM_Replace;
				w->trayOpen = 1;
				w->header.cmdMode = BIM_Replace;
		
				
				w->trayRoot = (GUIWindow*)GUIManager_SpawnTemplate(w->header.gm, "replace_tray");
				GUI_RegisterObject(w, w->trayRoot);
				w->findBox = (GUIEdit*)GUI_FindChild(w->trayRoot, "find");
				w->findResultsText = (GUIText*)GUI_FindChild(w->trayRoot, "results");
				w->replaceBox = (GUIEdit*)GUI_FindChild(w->trayRoot, "replace");
				if(w->findQuery) {
					GUIEdit_SetText(w->findBox, w->findQuery);
				}
				
				GUIManager_pushFocusedObject(w->header.gm, &w->findBox->header);

				GUIBufferEditor_RelativeFindMatch(w, 1, 1);
			}
			else {
				GUIBufferEditor_CloseTray(w);
				GUIManager_popFocusedObject(w->header.gm);
			}*/
			break;
			
		case GUICMD_Buffer_FindStartSequenceUnderCursor:
			GUIBufferEditor_SmartFind(w, cmd->str, FM_SEQUENCE);
			break;
			
		case GUICMD_Buffer_FindStartFromSelection:
			GUIBufferEditor_SmartFind(w, cmd->str, FM_SELECTION);
			break;
			
		case GUICMD_Buffer_FindStop:
			GUIBufferEditor_StopFind(w);
			break;
						
		case GUICMD_Buffer_FindStart:
			GUIBufferEditor_SmartFind(w, NULL, FM_SELECTION);
			GUI_SetActive(&w->findQuery);
			break;
				
		case GUICMD_Buffer_FindResume:
			if(!w->findState) {
				GUIBufferEditor_SmartFind(w, NULL, FM_SELECTION);
			}
			GUI_SetActive(&w->findQuery);
			
			break;
		case GUICMD_Buffer_SmartFind:
			GUIBufferEditor_SmartFind(w, cmd->str, FM_SEQUENCE | FM_SELECTION | FM_WITHIN_SELECTION);
			GUI_SetActive(&w->findQuery);
			break;
		
		case GUICMD_Buffer_FindNext:
			GUIBufferEditor_RelativeFindMatch(w, 1, 1, w->findState);
			break;
			
		case GUICMD_Buffer_FindPrev:
			GUIBufferEditor_RelativeFindMatch(w, -1, 1, w->findState);
			break;
		
		/////////////////////////////////
		// END FIND & REPLACE COMMANDS //
		/////////////////////////////////
		
		
		
		case GUICMD_Buffer_CollapseWhitespace:
			Buffer_CollapseWhitespace(w->b, CURSOR_LINE(w->ec->sel), w->ec->sel->col[0]);
			break;
			
// 		case GUICMD_Buffer_CloseBuffer: {
// 			GUIHeader* oo = GUIManager_SpawnTemplate(w->header.gm, "save_changes");
// 			GUI_RegisterObject(NULL, oo); // register to root window
			
			
// 			break;
// 		}
		case GUICMD_Buffer_SmartBubbleSelection: {
			BufferRange r = *w->ec->sel;
			
			if(!HAS_SELECTION(&r)) {
				Buffer_GetSequenceUnder(w->b, CURSOR_LINE(&r), CURSOR_COL(&r), cmd->pstr[1], &r);
			}
			
			char* s = Buffer_StringFromSelection(w->b, &r, NULL);
			char* s_msg = sprintfdup(cmd->pstr[0], s);
			free(s);
			MessagePipe_Send(w->tx, MSG_GrepOpener, s_msg, free);
			break;
			
		}
		

		case GUICMD_Buffer_Save: 
			if(!g_DisableSave) {
				Buffer_SaveToFile(w->b, w->sourceFile);
			}
			else {
				printf("Buffer saving disabled.\n");
			}
			break;
		
		case GUICMD_Buffer_SaveAndClose:
			if(!g_DisableSave) {
				Buffer_SaveToFile(w->b, w->sourceFile);
			}
			else {
				printf("Buffer saving disabled.\n");
			}
			MessagePipe_Send(w->tx, MSG_CloseMe, w, NULL);
			return 2; // no more commands, bufferEditor and tab are gone
			
			break;
		
		case GUICMD_Buffer_PromptAndClose:
			// launch save_tray
//			if(w->trayOpen) {
//				GUIBufferEditor_CloseTray(w);
//			}
			
			if(w->b->undoSaveIndex == w->b->undoCurrent) {
				// changes are saved, so just close
				MessagePipe_Send(w->tx, MSG_CloseMe, w, NULL);
				return 2; // no more commands, bufferEditor and tab are gone
				break;
			}
			
//			w->trayOpen = 1;
			/*
			w->trayRoot = (GUIWindow*)GUIManager_SpawnTemplate(w->header.gm, "save_tray");
			GUI_RegisterObject(w, w->trayRoot);
			GUIText* saveTrayFilename = (GUIText*)GUI_FindChild(w->trayRoot, "filename");
			GUIText_setString(saveTrayFilename, w->sourceFile);
			
			GUIEdit* btn_save = (GUIEdit*)GUI_FindChild(w->trayRoot, "save");
			GUIHeader_AddHandler(&btn_save->header, GUIEVENT_Click, SaveTray_save_click);
			
			GUIEdit* btn_discard = (GUIEdit*)GUI_FindChild(w->trayRoot, "discard");
			GUIHeader_AddHandler(&btn_discard->header, GUIEVENT_Click, SaveTray_discard_click);
			
			GUIEdit* btn_cancel = (GUIEdit*)GUI_FindChild(w->trayRoot, "cancel");
			GUIHeader_AddHandler(&btn_cancel->header, GUIEVENT_Click, SaveTray_cancel_click);
			*/
			
			break;
		
		// currently crashes:
		// 0x00005555555a10f5 in getColOffset (txt=0x1 <error: Cannot access memory at address 0x1>, col=3, tabWidth=4) at src/buffer_drawing.c:24
		case GUICMD_Buffer_Reload:
		{
			struct hlinfo* hl = w->b->hl; // preserve the meta info
			
			Buffer_DecRef(w->b);
			MessagePipe_Send(w->tx, MSG_BufferRefDec, w->b, NULL);
			Buffer_Delete(w->b);
			
			// TODO: check if there are other references and do something sensible
//			w->ec->selectPivotLine = NULL;
//			w->ec->selectPivotCol = 0;

			// TODO: wipe out selections
			// keep the scroll lines
			w->b = Buffer_New(w->bs);
			w->ec->b = w->b;
			Buffer_LoadFromFile(w->b, w->sourceFile);
			
			w->b->hl = hl;
			w->ec->scrollLines = MIN(w->ec->scrollLines, w->b->numLines);
		}
		break;
		
		
		default:
			return 1; // command not handled
	}
	
	

	
	if(cmd->flags & GUICMD_FLAG_scrollToCursor) {
		GBEC_scrollToCursor(w->ec);
	}
	
	if(cmd->flags & GUICMD_FLAG_rehighlight) {
		GUIBufferEditControl_MarkRefreshHighlight(w->ec);
	}
	
	if(cmd->flags & GUICMD_FLAG_resetCursorBlink) {
		w->ec->cursorBlinkTimer = 0;
	}
	
	if(cmd->flags & GUICMD_FLAG_undoSeqBreak) {
		if(!HAS_SELECTION(w->ec->sel)) {
//					printf("seq break without selection\n");
			Buffer_UndoSequenceBreak(
				w->b, 0, 
				CURSOR_LINE(w->ec->sel)->lineNum, CURSOR_COL(w->ec->sel),
				0, 0, 0
			);
		}
		else {
//					printf("seq break with selection\n");
			Buffer_UndoSequenceBreak(
				w->b, 0, 
				CURSOR_LINE(w->ec->sel)->lineNum, CURSOR_COL(w->ec->sel),
				PIVOT_LINE(w->ec->sel)->lineNum, PIVOT_COL(w->ec->sel),
				1 // TODO check pivot locations
			);
		}			
	}
	
	if(cmd->flags & GUICMD_FLAG_centerOnCursor) {
		GBEC_scrollToCursorOpt(w->ec, 1);
	}
		

	return 0;
	
}




void GUIBufferEditor_ProbeHighlighter(GUIBufferEditor* w) {
	Highlighter* h;
	
	h = HighlighterManager_ProbeExt(w->hm, w->sourceFile);
	
	if(h) {
		w->ec->h = h;
		
		GUIBufferEditControl_MarkRefreshHighlight(w->ec);
	}

}

