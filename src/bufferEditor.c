#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "common_math.h"


#include "buffer.h"
#include "ui/gui.h"
#include "ui/gui_internal.h"
#include "clipboard.h"





extern int g_DisableSave;


    //////////////////////////////////
   //                              //
  //       GUIBufferEditor        //
 //                              //
//////////////////////////////////

static void render(GUIBufferEditor* w, PassFrameParams* pfp) {
// HACK
// 	GUIBufferEditor_Draw(w, w->header.gm, w->scrollLines, + w->scrollLines + w->linesOnScreen + 2, 0, 100);
	
//	if(w->lineNumTypingMode) {
//		GUIHeader_render(&w->lineNumEntryBox->header, pfp); 
//	}
	
//	char* statusLine = "hello world";
//	printf("status line: %s\n", statusLine);
	
	
	GUIHeader_renderChildren(&w->header, pfp);
}


static void keyDown(GUIHeader* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	int needRehighlight = 0;
	
	if(isprint(gev->character) && (gev->modifiers & (~(GUIMODKEY_SHIFT | GUIMODKEY_LSHIFT | GUIMODKEY_RSHIFT))) == 0) {
		GUIBufferEditControl_ProcessCommand(w->ec, &(BufferCmd){
			BufferCmd_InsertChar, gev->character
		}, &needRehighlight);
		
		GUIBufferEditControl_RefreshHighlight(w->ec);
		GBEC_scrollToCursor(w->ec);
	}
	else {
		// special commands
		unsigned int S = GUIMODKEY_SHIFT;
		unsigned int C = GUIMODKEY_CTRL;
		unsigned int A = GUIMODKEY_ALT;
		unsigned int T = GUIMODKEY_TUX;
		
		unsigned int scrollToCursor   = 1 << 0;
		unsigned int rehighlight      = 1 << 1;
		unsigned int resetCursorBlink = 1 << 2;
		unsigned int undoSeqBreak     = 1 << 3;
		unsigned int hideMouse        = 1 << 4;
		unsigned int centerOnCursor   = 1 << 5;
		
		
		Cmd found;
		unsigned int iter = 0;
		while(Commands_ProbeCommand(gev, w->commands, w->inputMode, &found, &iter)) {
			// GUIBufferEditor will pass on commands to the buffer
			GUIBufferEditor_ProcessCommand(w, &(BufferCmd){
				.type = found.cmd, .str = found.str 
			}, &needRehighlight);
			
			
			if(found.flags & scrollToCursor) {
				GBEC_scrollToCursor(w->ec);
			}
			
			if(found.flags & rehighlight) {
				GUIBufferEditControl_RefreshHighlight(w->ec);
			}
			
			if(found.flags & resetCursorBlink) {
				w->ec->cursorBlinkTimer = 0;
			}
			
			if(found.flags & undoSeqBreak) {
				if(!w->ec->sel) {
//					printf("seq break without selection\n");
					Buffer_UndoSequenceBreak(
						w->buffer, 0, 
						w->ec->current->lineNum, w->ec->curCol,
						0, 0, 0
					);
				}
				else {
//					printf("seq break with selection\n");
					Buffer_UndoSequenceBreak(
						w->buffer, 0, 
						w->ec->sel->startLine->lineNum, w->ec->sel->startCol,
						w->ec->sel->endLine->lineNum, w->ec->sel->endCol,
						1 // TODO check pivot locations
					);
				}
			}
			
			if(found.flags & centerOnCursor) {
				GBEC_scrollToCursorOpt(w->ec, 1);
			}
		}
		
	}
	
}

static void parentResize(GUIBufferEditor* w, GUIEvent* gev) {
	w->header.size = gev->size;
	if(w->trayRoot) {
		w->trayRoot->header.size.x = w->header.size.x;
	}
}

static void updatePos(GUIBufferEditor* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	Vector2 wsz = w->header.size; 
	
	float sbHeight = w->statusBarHeight;
	if(!w->showStatusBar) {
		sbHeight = 0;
	}
	
	
	if(w->trayOpen) {
		w->trayRoot->header.size.x = w->header.size.x;
		w->trayRoot->header.z = 500;
		sbHeight += w->trayRoot->header.size.y;
		w->trayRoot->header.topleft.y = wsz.y - sbHeight;
		
		w->trayRoot->color = w->header.gm->defaults.trayBgColor;
		w->trayRoot->padding = (AABB2){{5,5}, {5,5}};
	}
	
	w->statusBar->header.hidden = !w->showStatusBar;
	w->statusBar->header.size = (Vector2){wsz.x, w->statusBarHeight};
	w->statusBar->header.topleft = (Vector2){0, 0};
	
	w->ec->header.size = (Vector2){wsz.x, wsz.y - sbHeight};
	w->ec->header.topleft = (Vector2){0, 0};
	
	gui_defaultUpdatePos(&w->header, grp, pfp);
}


void GUIBufferEditor_SetBuffer(GUIBufferEditor* w, Buffer* b) {
	w->buffer = b;
	GUIBufferEditControl_SetBuffer(w->ec, b);
}

static void userEvent(GUIHeader* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	
	if(w->trayOpen && (GUIEdit*)gev->originalTarget == w->findBox) {
		if(0 == strcmp(gev->userType, "change")) {
			// because userData is not null terminated from the Edit
			if(w->findQuery) {
				free(w->findQuery);
			}
			w->findQuery = strndup(gev->userData, gev->userSize);
			
			GUIBufferEditor_StopFind(w);
			
			w->findSet = GUIBufferEditor_FindAll(w, w->findQuery);
			w->ec->findSet = w->findSet;
			
			GUIBufferEditor_RelativeFindMatch(w, 1, 1);
			GUIBufferEditor_scrollToCursor(w);
		}
	} else if(0 == strcmp(gev->userType, "SaveTray_save_click")) {
		if(!g_DisableSave) {
			Buffer_SaveToFile(w->buffer, w->sourceFile);
		}
		else {
			printf("Buffer saving disabled.\n");
		}
		GUIManager_BubbleUserEvent(w->header.gm, &w->header, "closeMe");
	} else if(0 == strcmp(gev->userType, "SaveTray_discard_click")) {
		GUIManager_BubbleUserEvent(w->header.gm, &w->header, "closeMe");
	} else if(0 == strcmp(gev->userType, "SaveTray_cancel_click")) {
		if(w->trayOpen) {
			GUIBufferEditor_CloseTray(w);
		}
	}
}

static void gainedFocus(GUIHeader* w_, GUIEvent* gev) {
	GUIBufferEditor* w = (GUIBufferEditor*)w_;
	
	if(w->findMode == 1 || w->replaceMode == 1) {
		GUIManager_pushFocusedObject(w->header.gm, &w->findBox->header);
	}
}


GUIBufferEditor* GUIBufferEditor_New(GUIManager* gm) {
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.KeyDown = keyDown,
// 		.Click = click,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
		.ParentResize = (void*)parentResize,
		.User = userEvent,
		.GainedFocus = gainedFocus,
	};
	
	
	GUIBufferEditor* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	w->header.cursor = GUIMOUSECURSOR_TEXT;
	
	
	w->ec = GUIBufferEditControl_New(gm);
// 	w->ec->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
	GUI_RegisterObject(w, w->ec);
	
	
	w->showStatusBar = !gm->gs->hideStatusBar;
	w->statusBar = GUIStatusBar_New(gm);
// 	w->statusBar->header.flags = GUI_MAXIMIZE_X;
	w->statusBar->header.gravity = GUI_GRAV_BOTTOM_LEFT;
	w->statusBar->header.size.y = 30; 
	w->statusBar->header.size.x = w->header.size.x;
	w->statusBar->header.z = 500;
	
	w->statusBar->padding = (AABB2){{5,5}, {5,5}};
	w->statusBar->spacing = 3;
	
	w->statusBar->ec = w->ec;
	
	GUI_RegisterObject(w, w->statusBar);
	
	
	
	pcalloc(w->findSet);
	w->ec->findSet = w->findSet;
	
// 	GUITextF* textf;
	
// 	textf = GUITextF_new(w->header.gm);
// 	textf->header.topleft = (Vector2){4, 0};
// 	GUITextF_setString(textf, "Column %>ld", &w->ec->ec->curCol);
// 	GUI_RegisterObject(w->header.parent, textf);
	
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
	
	// this is internally reference counted
	Buffer_Delete(w->buffer);
	
	// TODO gui cleanup
// 	w->scrollbar;
// 	w->lineNumEntryBox;
// 	w->findBox;
// 	w->loadBox;
// 	w->trayRoot;
	
	// TODO more regex cleanup
	GUIBufferEditor_StopFind(w);
	
//	VEC_FREE(&w->findRanges);
	
	
	// TODO: free gui stuff
	//
	//TODO: free edit control
	
	free(w);
}



void GUIBufferEditor_UpdateSettings(GUIBufferEditor* w, GlobalSettings* s) {
	w->gs = s;
	
	w->statusBarHeight = w->gs->Buffer_statusBarHeight;
	
	GUIBufferEditControl_UpdateSettings(w->ec, s);
}


// makes sure the cursor is on screen, with minimal necessary movement
void GUIBufferEditor_scrollToCursor(GUIBufferEditor* gbe) {
	GBEC_scrollToCursor(gbe->ec);
}




static void SaveTray_save_click(GUIHeader* w_, GUIEvent* gev) {
	GUIManager_BubbleUserEvent(w_->gm, w_, "SaveTray_save_click");
	gev->cancelled = 1;
}

static void SaveTray_discard_click(GUIHeader* w_, GUIEvent* gev) {
	GUIManager_BubbleUserEvent(w_->gm, w_, "SaveTray_discard_click");
	gev->cancelled = 1;
}

static void SaveTray_cancel_click(GUIHeader* w_, GUIEvent* gev) {
	GUIManager_BubbleUserEvent(w_->gm, w_, "SaveTray_cancel_click");
	gev->cancelled = 1;
}



int GUIBufferEditor_StartFind(GUIBufferEditor* w, char* pattern) {
	
	int errno;
	PCRE2_SIZE erroff;
	PCRE2_UCHAR errbuf[256];
	//printf("starting RE find: '%s'\n", pattern);
	
	// free previous regex 
	if(w->findRE) {
		pcre2_code_free(w->findRE);
		pcre2_match_data_free(w->findMatch);
		w->findMatch = 0;
	}

	uint32_t options = PCRE2_CASELESS;
	
	w->findRE = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, options, &errno, &erroff, NULL);
	if(!w->findRE) {
		pcre2_get_error_message(errno, errbuf, sizeof(errbuf));
		w->findREError = strdup(errbuf);
		w->findREErrorChar = erroff;
		printf("PCRE find error #1: '%s' \n", errbuf);
		
		return 1;
	}
	
	// compilation was successful. clear any old errors.
	if(w->findREError) {
		free(w->findREError);
		w->findREError = 0;
		w->findREErrorChar = -1;
		printf("PCRE find error #2\n");
	}
	
	w->findMatch = pcre2_match_data_create_from_pattern(w->findRE, NULL);
	
	w->nextFindLine = w->ec->current;
	w->nextFindChar = w->ec->curCol;
	
	
	return 0;
}


int GUIBufferEditor_SmartFind(GUIBufferEditor* w, char* charSet, FindMask_t mask) {
	if(w->trayOpen) {
		GUIBufferEditor_CloseTray(w);
	}
	
	w->findMode = 1;
	w->trayOpen = 1;
	w->inputMode = 1;
	
	w->trayRoot = (GUIWindow*)GUIManager_SpawnTemplate(w->header.gm, "find_tray");
	GUI_RegisterObject(w, w->trayRoot);
	w->findBox = (GUIEdit*)GUI_FindChild(w->trayRoot, "find");
	w->findResultsText = (GUIText*)GUI_FindChild(w->trayRoot, "results");
	
	BufferRange sel = {};
	Buffer* b = w->ec->buffer;
	char* str = NULL;
	if(!str
		&& (mask & FM_SELECTION)
		&& w->ec->sel
		&& (w->ec->sel->startLine == w->ec->sel->endLine)
		&& (w->ec->sel->endCol - w->ec->sel->startCol > 0)
	) {
		str = Buffer_StringFromSelection(b, w->ec->sel, NULL);
	}
	if(!str && (mask & FM_SEQUENCE)) {
		Buffer_GetSequenceUnder(b, w->ec->current, w->ec->curCol, charSet, &sel);
		if((sel.startLine == sel.endLine) && (sel.endCol - sel.startCol > 0)) {
			str = Buffer_StringFromSelection(b, &sel, NULL);
		}
	}
					
	if(str) {
		if(w->findQuery) {
			free(w->findQuery);
		}
		w->findQuery = str;
	} else if(!w->findQuery) {
		w->findQuery = strdup("");
	}
	GUIEdit_SetText(w->findBox, w->findQuery);
	
	w->findIndex = -1;
	
	w->findSet = GUIBufferEditor_FindAll(w, w->findQuery);
	w->ec->findSet = w->findSet;

	// locate the match at/after the cursor
	GUIBufferEditor_RelativeFindMatch(w, 1, 1);
		
//	GUIBufferEditor_scrollToCursor(w);
	
	w->ec->cursorBlinkPaused = 1;
	GUIManager_pushFocusedObject(w->header.gm, &w->findBox->header);
	
	return 0;
}


int GUIBufferEditor_RelativeFindMatch(GUIBufferEditor* w, int offset, int continueFromCursor) {
	if(VEC_LEN(&w->findSet->ranges) == 0) {
		GUIText_setString(w->findResultsText, "No results");
		return 1;
	}
	
	if(w->findSet->changeCounter != w->buffer->changeCounter) {
		GUIBufferEditor_FindAll(w, w->findQuery);
		w->findIndex = -1;
		printf("reset find index\n");
	}
	
	BufferRange* r = NULL;
//	GUIBufferControl* ec = w->ec;
	intptr_t line = w->ec->current->lineNum;
	intptr_t col = w->ec->curCol;
	
	if(continueFromCursor && (offset > 0)) {
		VEC_EACH(&w->findSet->ranges, i, range) {
			if(range->startLine->lineNum < line) {
				continue;
			} else if((range->startLine->lineNum == line) && (range->startCol <= col)) {
				continue;
			}
			
			w->findIndex = i;
			r = range;
			break;
		}
	} else if(continueFromCursor && (offset < 0)) {
		VEC_R_EACH(&w->findSet->ranges, i, range) {
			if(range->startLine->lineNum > line) {
				continue;
			} else if((range->startLine->lineNum == line) && (range->startCol >= col)) {
				continue;
			}
			
			w->findIndex = i;
			r = range;
			break;
		}
	}
	
	if(!r) {
		w->findIndex = (w->findIndex + offset + VEC_LEN(&w->findSet->ranges)) % VEC_LEN(&w->findSet->ranges);
		
		r = VEC_ITEM(&w->findSet->ranges, w->findIndex);
	}
	char fmt_buffer[420];
	snprintf(fmt_buffer, 420, "%ld of %ld", w->findIndex + 1, VEC_LEN(&w->findSet->ranges));
	GUIText_setString(w->findResultsText, fmt_buffer);
	
	GBEC_MoveCursorTo(w->ec, r->startLine, r->startCol);
	GBEC_SetCurrentSelection(w->ec, r->startLine, r->startCol, r->endLine, r->endCol);
	
	
	return 0;
};


void GUIBufferEditor_StopFind(GUIBufferEditor* w) {
	
	// clear errors
	/*if(w->findREError) {
		free(w->findREError);
		w->findREError = NULL;
		w->findREErrorChar = -1;
	}
	*/
	
	w->findIndex = -1;
	BufferRangeSet_FreeAll(w->findSet);
}


int GUIBufferEditor_FindWord(GUIBufferEditor* w, char* word) {
	Buffer* b = w->buffer;
	
	if(!b->first) return 1;
	
	BufferLine* bl = b->first;
	while(bl) {
		if(bl->buf) {
			char* r = strstr(bl->buf, word);//, bl->length); 
			if(r != NULL) {
				intptr_t dist = r - bl->buf;
				
// 				printf("found: %d, %d\n", bl->lineNum, dist);
				
				w->ec->current = bl;
				w->ec->curCol = dist;
				
				if(!w->ec->sel) w->ec->sel = calloc(1, sizeof(*w->ec->sel));
				w->ec->sel->startLine = bl; 
				w->ec->sel->endLine = bl; 
				w->ec->sel->startCol = dist;
				w->ec->sel->endCol = dist + strlen(word);
				
				w->ec->selectPivotLine = bl;
				w->ec->selectPivotCol = w->ec->sel->endCol;
				
				return 0;
			}
		}
		bl = bl->next;
	}
	
	return 1;
// 	printf("not found: '%s'\n", word);
}



BufferRangeSet* GUIBufferEditor_FindAll(GUIBufferEditor* w, char* pattern) {

	BufferRangeSet* set = pcalloc(set);
	set->changeCounter = w->buffer->changeCounter;
	
	

	pcre2_code* findRE;
	pcre2_match_data* findMatch;
	BufferLine* findLine;
	intptr_t findCharS;
	intptr_t findCharE;
	intptr_t findLen;
	char* findREError;
	int findREErrorChar;
	

	int errno;
	PCRE2_SIZE erroff;
	PCRE2_UCHAR errbuf[256];
	//printf("starting RE find: '%s'\n", pattern);
	
	
	
	uint32_t options = PCRE2_CASELESS;
	
	findRE = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, options, &errno, &erroff, NULL);
	if(!findRE) {
		pcre2_get_error_message(errno, errbuf, sizeof(errbuf));
		printf("PCRE find error #1: '%s' \n", errbuf);
		
		return NULL;
	}
	
	// compilation was successful.
	
	findMatch = pcre2_match_data_create_from_pattern(findRE, NULL);


	BufferLine* nextFindLine;
	intptr_t nextFindChar = 0;
	
	
	BufferLine* bl = w->buffer->first;
	int off = 0; // this is for partial matches
	uint32_t opts = PCRE2_NOTEMPTY | PCRE2_NOTEMPTY_ATSTART;
	int res;
	int wraps = 0;
	
	while(bl) {
		res = pcre2_match(findRE, bl->buf + nextFindChar, bl->length, off, opts, findMatch, NULL);
		
		if(res > 0) {
			// found a match
		
			PCRE2_SIZE* ovec = pcre2_get_ovector_pointer(findMatch);
			
			BufferRange* r  = pcalloc(r);
			r->startLine = bl;
			r->startCol = (int)ovec[0] + nextFindChar;
			r->endLine = bl;
			r->endCol = (int)ovec[1] + nextFindChar;
			
			VEC_PUSH(&set->ranges, r);
			
			nextFindChar += (int)ovec[1];  
			
//			printf("match found at: %d:%d\n", bl->lineNum, (int)ovec[0] + nextFindChar);
			
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
				
				printf("PCRE real error: %p %p %ld '%s'\n", findRE, bl->buf, bl->lineNum, errbuf);
				
				return NULL;
			}
			
			bl = bl->next;
			nextFindChar = 0;
		}
		
		if(!bl) { // end of file
			break;
		}
	}
	

	// clean up regex structures
	if(findRE) {
		pcre2_code_free(findRE);
		pcre2_match_data_free(findMatch);
	}
		
	return set;
}




void GUIBufferEditor_ReplaceAll(GUIBufferEditor* w, BufferRangeSet* rset, char* text) {
	if(!VEC_LEN(&rset->ranges)) return;
	
	Buffer* b = w->ec->buffer;
	GUIBufferEditControl* ec = w->ec;
	size_t len = strlen(text);
			
	VEC_EACH(&rset->ranges, i, r) {		
		if(r) {
			Buffer_DeleteSelectionContents(b, r);
			Buffer_LineInsertChars(b, r->startLine, text, r->startCol, len);
		}
	}
	
}


void GUIBufferEditor_MoveCursorTo(GUIBufferEditor* gbe, intptr_t line, intptr_t col) {
	if(line < 1) line = 1;
	if(col < 0) col = 0;
	gbe->ec->current = Buffer_raw_GetLine(gbe->buffer, line);
	gbe->ec->curCol = MIN(col, gbe->ec->current->length); // TODO: check for bounds
}


void GUIBufferEditor_ProcessCommand(GUIBufferEditor* w, BufferCmd* cmd, int* needRehighlight) {
	GUIEdit* e;
	struct json_file* jsf;
	
	switch(cmd->type){
		case BufferCmd_ToggleMenu:
				
			jsf = json_load_path("/etc/gpuedit/buffer_menu.json");
			w->menu = (GUISimpleWindow*)GUICL_CreateFromConfig(w->header.gm, jsf->root);
			
			// TODO: free json 
			
			GUI_RegisterObject(w, w->menu);
			
			
			GUISelectBox* hlsel = (GUISelectBox*)GUI_FindChild(w->menu, "highlighter");
			GUISelectBoxOption opts[] = {
				{.label = "C", .data = "c"},
				{.label = "C++", .data = "cpp"},
				{.label = "JavaScript", .data = "javascript"},
			};
			
			if(hlsel) GUISelectBox_SetOptions(hlsel, opts, 3);
			
			
			
			
			break;

		case BufferCmd_ToggleGDBBreakpoint: {
			w->ec->current->flags ^= BL_BREAKPOINT_FLAG;
			
			//if(w->ec->current->flags & BL_BREAKPOINT_FLAG) {
			// actually toggles breakpoint
				if(w->setBreakpoint) 
					w->setBreakpoint(w->sourceFile, w->ec->current->lineNum, w->setBreakpointData);
			//}
			
			break;
		}
		case BufferCmd_MovePage:
			GBEC_MoveCursorV(w->ec, cmd->amt * w->ec->linesOnScreen);
			
			w->ec->scrollLines = MAX(0, MIN(w->ec->scrollLines + cmd->amt * w->ec->linesOnScreen, w->buffer->numLines - 1));
			break;
		
			
			// TODO: init selectoin and pivots if no selection active
		case BufferCmd_GrowSelectionH:
// 			if(!w->selectPivotLine) {
			if(!w->ec->sel) {
				w->ec->selectPivotLine = w->ec->current;
				w->ec->selectPivotCol = w->ec->curCol;
			}
// 			printf("pivot: %d, %d\n", w->selectPivotLine->lineNum, w->selectPivotCol);
			GBEC_MoveCursorH(w->ec, cmd->amt);
			GUIBufferEditControl_SetSelectionFromPivot(w->ec);
			break;
		
		case BufferCmd_GrowSelectionToNextSequence:
			if(!w->ec->sel) {
				w->ec->selectPivotLine = w->ec->current;
				w->ec->selectPivotCol = w->ec->curCol;
			}
			GBEC_MoveToNextSequence(w->ec, w->ec->current, w->ec->curCol, cmd->str);
			GUIBufferEditControl_SetSelectionFromPivot(w->ec);
			break;
		
		case BufferCmd_GrowSelectionToPrevSequence:
			if(!w->ec->sel) {
				w->ec->selectPivotLine = w->ec->current;
				w->ec->selectPivotCol = w->ec->curCol;
			}
			GBEC_MoveToPrevSequence(w->ec, w->ec->current, w->ec->curCol, cmd->str);
			GUIBufferEditControl_SetSelectionFromPivot(w->ec);
			break;
		
		case BufferCmd_GrowSelectionToSOL:
			if(!w->ec->sel) {
				w->ec->selectPivotLine = w->ec->current;
				w->ec->selectPivotCol = w->ec->curCol;
			}
			GBEC_MoveToFirstCharOfLine(w->ec, w->ec->current);
			GUIBufferEditControl_SetSelectionFromPivot(w->ec);
			break;
		
		case BufferCmd_GrowSelectionToEOL:
			if(!w->ec->sel) {
				w->ec->selectPivotLine = w->ec->current;
				w->ec->selectPivotCol = w->ec->curCol;
			}
			GBEC_MoveToLastCharOfLine(w->ec, w->ec->current);
			GUIBufferEditControl_SetSelectionFromPivot(w->ec);
			break;
			
		
		case BufferCmd_GrowSelectionV:
			if(!w->ec->sel) {
				w->ec->selectPivotLine = w->ec->current;
				w->ec->selectPivotCol = w->ec->curCol;
			}
// 			printf("pivot: %d, %d\n", w->selectPivotLine->lineNum, w->selectPivotCol);
			GBEC_MoveCursorV(w->ec, cmd->amt);
			GUIBufferEditControl_SetSelectionFromPivot(w->ec);
			break;
		
		case BufferCmd_ClearSelection:
			if(w->ec->sel) GBEC_ClearAllSelections(w->ec);
			break;
			
		case BufferCmd_SelectSequenceUnder:
			GBEC_SelectSequenceUnder(w->ec, w->ec->current, w->ec->curCol, cmd->str); 
			w->ec->selectPivotLine = w->ec->sel->startLine;
			w->ec->selectPivotCol = w->ec->sel->startCol;
			break;
			
		case BufferCmd_MoveToNextSequence:
			GBEC_MoveToNextSequence(w->ec, w->ec->current, w->ec->curCol, cmd->str);
			break;
			
		case BufferCmd_MoveToPrevSequence:
			GBEC_MoveToPrevSequence(w->ec, w->ec->current, w->ec->curCol, cmd->str);
			break;
			
		case BufferCmd_DeleteToNextSequence:
			GBEC_DeleteToNextSequence(w->ec, w->ec->current, w->ec->curCol, cmd->str);
			break;
			
		case BufferCmd_DeleteToPrevSequence:
			GBEC_DeleteToPrevSequence(w->ec, w->ec->current, w->ec->curCol, cmd->str);
			break;
			
		case BufferCmd_GoToEOL:
			if(w->ec->sel) GBEC_ClearAllSelections(w->ec);
			w->ec->curCol = w->ec->current->length;
			break;
			
		case BufferCmd_GoToSOL:
			if(w->ec->sel) GBEC_ClearAllSelections(w->ec);
			w->ec->curCol = 0;
			break;
		
		case BufferCmd_GoToAfterIndent:
			if(w->ec->sel) GBEC_ClearAllSelections(w->ec);
			GBEC_MoveCursorTo(w->ec, w->ec->current, BufferLine_GetIndentCol(w->ec->current));
			break;
			
		case BufferCmd_GoToLineLaunch:
			if(!w->lineNumTypingMode) {
				if(w->trayOpen) {
					GUIBufferEditor_CloseTray(w);
				}
				
				w->lineNumTypingMode = 1;
				w->trayOpen = 1;
				w->inputMode = 3;
				
				w->trayRoot = (GUIWindow*)GUIManager_SpawnTemplate(w->header.gm, "goto_tray");
				GUI_RegisterObject(w, w->trayRoot);
				w->lineNumEntryBox = (GUIEdit*)GUI_FindChild(w->trayRoot, "goto_line");
				
				w->ec->cursorBlinkPaused = 1;
				GUIManager_pushFocusedObject(w->header.gm, &w->lineNumEntryBox->header);
			}
			else {
				GUIBufferEditor_CloseTray(w);
				w->ec->cursorBlinkPaused = 0;
				GUIManager_popFocusedObject(w->header.gm);
			}
		// TODO: change hooks
			break;
		
		case BufferCmd_GoToLineSubmit:
			if(w->lineNumTypingMode) {
				intptr_t line_num = strtol(GUIEdit_GetText(w->lineNumEntryBox), NULL, w->gs->Buffer_lineNumBase);
				BufferLine* bl = Buffer_raw_GetLine(w->ec->buffer, line_num);
				
				if(bl) {
					GBEC_MoveCursorTo(w->ec, bl, 0);
					GBEC_scrollToCursorOpt(w->ec, 1);
				}
				
				GUIBufferEditor_CloseTray(w);
				w->lineNumTypingMode = 0;
				GUIManager_popFocusedObject(w->header.gm);
			}
			break;
		
		case BufferCmd_ReplaceNext: { // TODO put this all in a better spot
			Buffer* b = w->ec->buffer;
			GUIBufferEditControl* ec = w->ec;
			
			if(!VEC_LEN(&w->findSet->ranges)) break;
			
			BufferRange* r = VEC_ITEM(&w->findSet->ranges, w->findIndex);
			
			if(r) {
				Buffer_DeleteSelectionContents(b, r);
				
				char* rtext = GUIEdit_GetText(w->replaceBox);
				size_t len = strlen(rtext);
				
				Buffer_LineInsertChars(b, r->startLine, rtext, r->startCol, len);
				GBEC_MoveCursorTo(ec, r->startLine, r->startCol);
				GBEC_MoveCursorH(ec, len);
			}
			
			GUIBufferEditor_RelativeFindMatch(w, 1, 1);
			break;
		}
		
		case BufferCmd_ReplaceAll: {
			char* rtext = GUIEdit_GetText(w->replaceBox);
//			size_t len = strlen(rtext);
			
			GUIBufferEditor_ReplaceAll(w, w->findSet, rtext);
		
			// HACK
			VEC_TRUNC(&w->findSet->ranges);
			 
			break;
		}
		
		case BufferCmd_ReplaceStart:
			if(!w->replaceMode) {
				char* preserved = NULL;
				if(w->trayOpen) {
					GUIBufferEditor_CloseTray(w);
				}
				
				w->replaceMode = 1;
				w->trayOpen = 1;
				w->inputMode = 2;
		
				
				w->trayRoot = (GUIWindow*)GUIManager_SpawnTemplate(w->header.gm, "replace_tray");
				GUI_RegisterObject(w, w->trayRoot);
				w->findBox = (GUIEdit*)GUI_FindChild(w->trayRoot, "find");
				w->replaceBox = (GUIEdit*)GUI_FindChild(w->trayRoot, "replace");
				if(w->findQuery) {
					GUIEdit_SetText(w->findBox, w->findQuery);
				}
				
				w->ec->cursorBlinkPaused = 1;
				GUIManager_pushFocusedObject(w->header.gm, &w->findBox->header);
			}
			else {
				GUIBufferEditor_CloseTray(w);
				w->ec->cursorBlinkPaused = 0;
				GUIManager_popFocusedObject(w->header.gm);
			}
			break;
			
		case BufferCmd_FindStartSequenceUnderCursor:
			GUIBufferEditor_SmartFind(w, cmd->str, FM_SEQUENCE);
			break;
			
		case BufferCmd_FindStartFromSelection:
			GUIBufferEditor_SmartFind(w, cmd->str, FM_SELECTION);
			break;
			
		case BufferCmd_FindStart:			
		case BufferCmd_FindResume:
			GUIBufferEditor_SmartFind(w, cmd->str, FM_NONE);
			break;
		case BufferCmd_SmartFind:
			GUIBufferEditor_SmartFind(w, cmd->str, FM_SELECTION|FM_SEQUENCE);
			break;
		
		case BufferCmd_FindNext:
			GUIBufferEditor_RelativeFindMatch(w, 1, 1);
			break;
			
		case BufferCmd_FindPrev:
			GUIBufferEditor_RelativeFindMatch(w, -1, 1);
			break;
			
		case BufferCmd_CollapseWhitespace:
			Buffer_CollapseWhitespace(w->buffer, w->ec->current, w->ec->curCol);
			break;
			
		case BufferCmd_CloseTray:
			if(w->trayOpen) {
				GUIBufferEditor_CloseTray(w);
				w->lineNumTypingMode = 0;
				w->findMode = 0;
				w->loadTypingMode = 0;
				w->ec->cursorBlinkPaused = 0;
				GUIManager_popFocusedObject(w->header.gm);
				
				// HACK
				VEC_TRUNC(&w->findSet->ranges);
			}
			break;
			
// 		case BufferCmd_CloseBuffer: {
// 			GUIHeader* oo = GUIManager_SpawnTemplate(w->header.gm, "save_changes");
// 			GUI_RegisterObject(NULL, oo); // register to root window
			
			
// 			break;
// 		}
		case BufferCmd_SmartBubbleSelection: {
			GUIBubbleOpt opt = {};
			GUIEvent gev2 = {};
			
			opt.ev = strdup(cmd->str);
			if(w->ec->sel) {
				opt.sel = Buffer_StringFromSelection(w->buffer, w->ec->sel, NULL);
			}
			
			gev2.type = GUIEVENT_User;
			gev2.eventTime = 0;//gev->eventTime;
			gev2.originalTarget = &w->header;
			gev2.currentTarget = &w->header;
			gev2.cancelled = 0;
			// handlers are responsible for cleanup
			gev2.userData = &opt;
			gev2.userSize = sizeof(opt);
			
			gev2.userType = "SmartBubble";
		
			GUIManager_BubbleEvent(w->header.gm, &w->header, &gev2);
			free(opt.ev);
			free(opt.sel);
			break;
		}
			

		case BufferCmd_Save: 
			if(!g_DisableSave) {
				Buffer_SaveToFile(w->buffer, w->sourceFile);
			}
			else {
				printf("Buffer saving disabled.\n");
			}
			break;
		
		case BufferCmd_SaveAndClose:
			if(!g_DisableSave) {
				Buffer_SaveToFile(w->buffer, w->sourceFile);
			}
			else {
				printf("Buffer saving disabled.\n");
			}
			GUIManager_BubbleUserEvent(w->header.gm, &w->header, "closeMe");
			break;
		
		case BufferCmd_PromptAndClose:
			// launch save_tray
			if(w->trayOpen) {
				GUIBufferEditor_CloseTray(w);
			}
			
			if(w->buffer->undoSaveIndex == w->buffer->undoCurrent) {
				// changes are saved, so just close
				GUIManager_BubbleUserEvent(w->header.gm, &w->header, "closeMe");
				break;
			}
			
			w->trayOpen = 1;
			
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
			
			w->ec->cursorBlinkPaused = 1;
			
			break;
		
		case BufferCmd_Reload:
		{
			struct hlinfo* hl = w->buffer->hl; // preserve the meta info
			EditorParams* ep = w->buffer->ep;
			
			Buffer_Delete(w->buffer);
			w->ec->selectPivotLine = NULL;
			w->ec->selectPivotCol = 0;
			// keep the scroll lines
			w->buffer = Buffer_New();
			w->ec->buffer = w->buffer;
			Buffer_LoadFromFile(w->buffer, w->sourceFile);
			
			w->buffer->hl = hl;
			w->buffer->ep = ep;
			w->ec->scrollLines = MIN(w->ec->scrollLines, w->buffer->numLines);
		}
		break;
		
		default:
			GUIBufferEditControl_ProcessCommand(w->ec, cmd, needRehighlight);
			
	}
	
// 	printf("line/col %d:%d %d\n", b->current->lineNum, b->curCol, b->current->length);
}




void GUIBufferEditor_CloseTray(GUIBufferEditor* w) {
	if(!w->trayOpen) return;
// 	
	w->trayOpen = 0;
	w->findMode = 0;
	w->replaceMode = 0;
	
	w->inputMode = 0;
	
	GUI_Delete(w->trayRoot);
	w->trayRoot = NULL;
	w->findBox = NULL;
	w->replaceBox = NULL;
}

void GUIBufferEditor_ToggleTray(GUIBufferEditor* w, float height) {
	if(w->trayOpen) GUIBufferEditor_CloseTray(w);
	else GUIBufferEditor_OpenTray(w, height);
}

void GUIBufferEditor_OpenTray(GUIBufferEditor* w, float height) {
	/*if(w->trayOpen) {
		if(w->trayHeight != height) {
			w->trayHeight = height;
			return;
		}
	}
	
	w->trayOpen = 1; 
	w->trayHeight = height;
	w->trayRoot = GUIWindow_New(w->header.gm);
	
	GUI_RegisterObject(w, w->trayRoot);
	*/
}


void GUIBufferEditor_ProbeHighlighter(GUIBufferEditor* w) {
	Highlighter* h;
	
	h = HighlighterManager_ProbeExt(w->hm, w->sourceFile);
	
	if(h) {
		w->ec->h = h;
		
		GUIBufferEditControl_RefreshHighlight(w->ec);
	}

}

