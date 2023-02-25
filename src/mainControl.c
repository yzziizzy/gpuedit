#include <ctype.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "app.h"
#include "mainControl.h"
#include "fuzzyMatch.h"
#include "ui/gui.h"
#include "ui/gui_internal.h"
#include "ui/configLoader.h"
#include "c_json/json.h"


#include "fileBrowser.h"
#include "fuzzyMatchControl.h"
#include "grepOpenControl.h"
//#include "calcControl.h"
//#include "terminal.h"

// temporary, should be separated
#include "window.h"




static float tabscroll_fn_None(MainControlTab* tab, float boxw, PassFrameParams* pfp) { return 0; }
static float tabscroll_fn_Linear(MainControlTab* tab, float boxw, PassFrameParams* pfp) { 
	float max_xoff = tab->titleWidth - boxw + 2;
	float phase = fmodf(pfp->appTime,
		tab->lingerStart + tab->lingerEnd + 2 * tab->scrollSpeed
	);
	
	if(phase > tab->lingerStart + tab->scrollSpeed + tab->lingerEnd) {
		// scroll backwards
		float t = phase - (tab->lingerStart + tab->scrollSpeed + tab->lingerEnd);
		return (1.0 - (t / tab->scrollSpeed)) * max_xoff;
	}
	else if(phase > tab->lingerStart + tab->scrollSpeed) {
		// linger end
		return 1.0 * max_xoff;
	}
	else if(phase > tab->lingerStart) {
		// scroll forwards
		float t = phase - tab->lingerStart;
		return (t / tab->scrollSpeed) * max_xoff;
	}
	else {
		// linger start
		return 0.0  * max_xoff;
	}
}

static float tabscroll_fn_Sinusoidal(MainControlTab* tab, float boxw, PassFrameParams* pfp) { 
	float max_xoff = tab->titleWidth - boxw + 2;
	float phase = fmodf(pfp->appTime,
		tab->lingerStart + tab->lingerEnd + 2 * tab->scrollSpeed
	);
	
	if(phase > tab->lingerStart + tab->scrollSpeed + tab->lingerEnd) {
		// scroll backwards
		float t = phase - (tab->lingerStart + tab->scrollSpeed + tab->lingerEnd);
		return (cos((t / tab->scrollSpeed) * 3.14) * 0.5 + 0.5) * max_xoff;
	}
	else if(phase > tab->lingerStart + tab->scrollSpeed) {
		// linger end
		return 1.0 * max_xoff;
	}
	else if(phase > tab->lingerStart) {
		// scroll forwards
		float t = phase - tab->lingerStart;
		return (1.0 - (cos((t / tab->scrollSpeed) * 3.14) * 0.5 + 0.5)) * max_xoff;
	}
	else {
		// linger start
		return 0.0 * max_xoff;
	}
}

static float tabscroll_fn_Swing(MainControlTab* tab, float boxw, PassFrameParams* pfp) { 
	float max_xoff = tab->titleWidth - boxw + 2;
	float phase = fmodf(pfp->appTime,
		tab->lingerStart + tab->lingerEnd + 2 * tab->scrollSpeed
	);
	
	if(phase > tab->lingerStart + tab->scrollSpeed + tab->lingerEnd) {
		// scroll backwards
		float t = phase - (tab->lingerStart + tab->scrollSpeed + tab->lingerEnd);
		return (1.0 - (1.0 / ( 1.0 + exp((t - .5) * -16)))) * max_xoff;
	}
	else if(phase > tab->lingerStart + tab->scrollSpeed) {
		// linger end
		return 1.0 * max_xoff;
	}
	else if(phase > tab->lingerStart) {
		// scroll forwards
		float t = phase - tab->lingerStart;
		return (1.0 / ( 1.0 + exp((t - .5) * -16))) * max_xoff;
	}
	else {
		// linger start
		return 0.0 * max_xoff;
	}
}

static float tabscroll_fn_Loop(MainControlTab* tab, float boxw, PassFrameParams* pfp) { 
	float sp = tab->scrollSpeed * 3;

	float w = tab->titleWidth + boxw;
	float t = fmodf(pfp->appTime, sp);
	float s = t / sp;
	
	return s * w - boxw;
}


#include "ui/macros_on.h"


void MainControl_Render(MainControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	
	Vector2 psz = {
		x: sz.x / w->xDivisions,
		y: sz.y / w->yDivisions,
	};
	
	for(int y = 0; y < w->yDivisions; y++) {
	for(int x = 0; x < w->xDivisions; x++) {

		MainControlPane* pane = w->paneSet[x + y * w->xDivisions];
		Vector2 ptl = V(tl.x + x * psz.x, tl.y + y * psz.y);

		// set this pane to be focused if any mouse button went up in it,
		//   regardless of if a deeper element traps the event
		// Don't refocus on wheel scrolling
		if(gm->curEvent.type == GUIEVENT_MouseUp && (gm->curEvent.button == 1 || gm->curEvent.button == 2 || gm->curEvent.button == 3)) {
			
			// hover focus
			if(GUI_PointInBoxV_(gm, ptl, psz, gm->lastMousePos)) {
				MainControl_SetFocusedPane(w, pane);
			}
			
		}
	}}
	
	for(int y = 0; y < w->yDivisions; y++) {
	for(int x = 0; x < w->xDivisions; x++) {
		MainControlPane* pane = w->paneSet[x + y * w->xDivisions];
		Vector2 ptl = V(tl.x + x * psz.x, tl.y + y * psz.y);
		MainControlPane_Render(pane, gm, ptl, psz, pfp);
	}}
}

void MainControlPane_Render(MainControlPane* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	float oZ = gm->curZ;
	MainControl* mc = w->mc;
	
	Vector2 tab_sz = {sz.x, sz.y - mc->tabHeight - 1};
	float tabw = (sz.x - (1 + VEC_LEN(&w->tabs))) / (VEC_LEN(&w->tabs));
	
	// layout mode traps all input
	if(!gm->drawMode && mc->inputMode == 10) {
	
		if(GUI_InputAvailable()) {
			GUI_Cmd* cmd = Commands_ProbeCommandMode(gm, GUIELEMENT_Main, &gm->curEvent, mc->inputMode, NULL);	
			if(cmd) MainControl_ProcessCommand(mc, cmd);
		}
		
		// no input escapes to the tabs
		// BUG: does not trap mouse input; need "GUI_VoidInput()" that kills the input type and clears the flags
		GUI_CancelInput();
		gm->curEvent.type = 0;
	}
	
	
	// check for clicks to change focus
	if(!gm->drawMode) {
	
		VEC_EACH(&w->tabs, i, tab) {
		
			if(GUI_MouseInside(V(tl.x + tabw * i + i + 1, tl.y), V(tabw - 1, mc->tabHeight))) {
				if(GUI_MouseWentUp(1)) {
					MainControlPane_GoToTab(w, i);
				}
				else if(GUI_MouseWentUp(2)) {
					MainControlPane_CloseTab(w, i);
				}
			}
		
		}
	}
	
	
	// only render the active tab
	if(w->currentIndex > -1) {
		MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
		
		if(gm->drawMode || w == w->mc->focusedPane) {
			if(a && a->render) a->render(a->client, gm, V(tl.x, tl.y + mc->tabHeight + 1), tab_sz, pfp);
		}
	}
	
	

	
	// handle input
	if(!gm->drawMode) {
		
		if(GUI_InputAvailable()) {
			
			GUI_Cmd* cmd = Commands_ProbeCommandMode(gm, GUIELEMENT_Main, &gm->curEvent, mc->inputMode, NULL);
			
			if(cmd) {
				MainControl_ProcessCommand(mc, cmd);
				GUI_CancelInput();
			}
		}
				
		return;
	}
	
	// ---------- only drawing code after this point ----------
	
	// TEMP
	VEC_EACH(&w->tabs, i, t) {
		if(t->everyFrame) t->everyFrame(t);
	}
	
	struct Color4* bgcolor =  &gm->defaults.tabBorderColor;
	if(w->mc->numPanes > 1 && w == w->mc->focusedPane) {
		bgcolor = &gm->defaults.tabCurrentPaneBorderColor;
	}
	
	// background
	GUI_Rect(tl, V(sz.x, mc->tabHeight), bgcolor);
//	GUI_Rect(tl, V(sz.x, mc->tabHeight), &C4H(00ff00ff));
	

	gm->curZ += 1;
	
	
	// tab backgrounds
	VEC_EACH(&w->tabs, i, tab) {
		Vector2 boxSz = {tabw - 1, mc->tabHeight};
	
		struct Color4* color;
		if(tab->isActive) color = &gm->defaults.tabActiveBgColor;
		else if(GUI_MouseInside(V(tl.x + tabw * i + i + 1, tl.y), boxSz)) color = &gm->defaults.tabHoverBgColor;
		else color = &gm->defaults.tabBgColor;
		
		// TODO: fix pixel widths
		GUI_Rect(V(tl.x + tabw * i + i + 1, tl.y + 1), V(tabw - 1, mc->tabHeight - 2), color);
//		GUI_Rect(V(tl.x + tabw * i + i + 1, tl.y + 1), V(tabw - 1, mc->tabHeight - 2), &C4H(ff0000ff));
		
	}
	
	gm->curZ += 1;
	
	AABB2 oClip = gm->curClip;
	
	GUIFont* titleFont = GUI_FindFont(gm, "Arial",  mc->tabHeight - 5);
	
	// tab titles
	VEC_EACH(&w->tabs, i, tab) {
		float textw = gui_getTextLineWidth(gm, titleFont,  mc->tabHeight - 5, tab->title, strlen(tab->title));
		float xoff = (tabw - 2.0 - textw) / -2.0;
		
		tab->titleWidth = textw;
	
		AABB2 box;
		box.min.x = tl.x + tabw * i + i + 1;
		box.min.y = tl.y + 1;
		box.max.x = tl.x + tabw * (i + 1) + i + 1;
		box.max.y = tl.y + mc->tabHeight - 1;
		gm->curClip = box;
		
			
		if(textw > tabw - 2) {
			switch(tab->scrollType) {
				#define X(x) case TABSC_##x: xoff = tabscroll_fn_##x(tab, tabw, pfp); break;
					TAB_SCROLL_TYPE_LIST
				#undef X
			}
		}
	
		Color4 c = gm->defaults.tabTextColor;
		if(tab->isStarred) {
			c = C4H(ff0000ff);
		}
		
		GUI_TextLine(
			tab->title, strlen(tab->title), 
			V(box.min.x - xoff, box.min.y), /*V(tabw - 2, mc->tabHeight - 2),*/ 
			"Arial", mc->tabHeight - 5, 
			&c
		);
		
	}
	
	gm->curClip = oClip;
	gm->curZ -= 2;
#include "ui/macros_off.h"
}





/*

static void userEvent(GUIHeader* w_, GUIEvent* gev) {
	MainControl* w = (MainControl*)w_;
	
	} else if(0 == strcmp(gev->userType, "SmartBubble")) {
		GUIBubbleOpt* opt = (GUIBubbleOpt*)gev->userData;
		if(0 == strcmp(opt->ev, "FuzzyOpen")) {
			MainControl_FuzzyOpener(w, opt->sel);
			gev->cancelled = 1;
		} else if(0 == strcmp(opt->ev, "GrepOpen")) {
			MainControl_GrepOpen(w, opt->sel);
			gev->cancelled = 1;
		} else {
			printf("MainControl::SmartBubble unknown ev '%s'\n", opt->ev);
		}
	}
}

*/

void* args[4];

void MainControl_ProcessCommand(MainControl* w, GUI_Cmd* cmd) {
	
	switch(cmd->cmd) {
	
	case GUICMD_Main_EnterLayoutMode:
		w->inputMode = 10;
		break;
		
	case GUICMD_Main_ExitLayoutMode:
		w->inputMode = 0;
		break;
		
	case GUICMD_Main_ExpandPanesX:
		MainControl_ExpandPanes(w, w->xDivisions + cmd->amt, w->yDivisions);
		break;
	case GUICMD_Main_ExpandPanesY:
		MainControl_ExpandPanes(w, w->xDivisions, w->yDivisions + cmd->amt);
		break;
	
	case GUICMD_Main_FocusPaneRelX:
		MainControl_FocusPane(w, w->focusedPos.x + cmd->amt, w->focusedPos.y);
		break;
	case GUICMD_Main_FocusPaneRelY:
		MainControl_FocusPane(w, w->focusedPos.x, w->focusedPos.y + cmd->amt);
		break;
		
	case GUICMD_Main_OpenFileBrowser:
		MainControl_OpenFileBrowser(w, "./");
		break;

	case GUICMD_Main_FuzzyOpener:
		MainControl_FuzzyOpener(w, NULL);
		break;
	
	case GUICMD_Main_GrepOpen:
		MainControl_GrepOpen(w, NULL);
		break;

	case GUICMD_Main_OpenConjugate:
		MainControl_OpenConjugate(w, VEC_ITEM(&w->focusedPane->tabs, w->focusedPane->currentIndex), cmd->amt);
		break;
		
	case GUICMD_Main_Calculator:
//		MainControl_Calculator(w);
		break;
		
	case GUICMD_Main_Terminal:
//		MainControl_Terminal(w);
		break;

	case GUICMD_Main_MainMenu:
//		MainControl_OpenMainMenu(w);
		break;
		
	case GUICMD_Main_SaveActiveTab:
		printf("NYI\n");
		break;
		
	case GUICMD_Main_SaveAll:
		printf("NYI\n");
		break;
		
	case GUICMD_Main_Quit:
		printf("NYI\n");
		break;
		
	case GUICMD_Main_SaveQuit:
		printf("NYI\n");
		break;
		
	case GUICMD_Main_QuitWithoutSave:
		// TODO: nicer
		exit(0);
		break;
		
	case GUICMD_Main_LoadFile:
		MainControl_LoadFile(w, cmd->str);
		break;
		
	case GUICMD_Main_NewEmptyBuffer:
		MainControl_NewEmptyBuffer(w);
		break;
		
	case GUICMD_Main_CloseTab:
		MainControl_CloseTab(w, w->focusedPane->currentIndex);
		break;
		
	case GUICMD_Main_SaveAndCloseTab:
		printf("NYI\n"); // see BufferCmd_SaveAndClose and BufferCmd_PromptAndClose
		break;
		
	case GUICMD_Main_SortTabs: MainControlPane_SortTabs(w->focusedPane); break;
	case GUICMD_Main_MoveTabR: MainControlPane_SwapTabs(w->focusedPane, w->focusedPane->currentIndex, w->focusedPane->currentIndex + 1); break;
	case GUICMD_Main_MoveTabL: MainControlPane_SwapTabs(w->focusedPane, w->focusedPane->currentIndex, w->focusedPane->currentIndex - 1); break;
	case GUICMD_Main_NextTab: MainControlPane_NextTab(w->focusedPane, 1/*cmd->n*/); break;
	case GUICMD_Main_PrevTab: MainControlPane_PrevTab(w->focusedPane, 1/*cmd->n*/); break;
	case GUICMD_Main_GoToTab: MainControlPane_GoToTab(w->focusedPane, cmd->amt); break;
	
	
	case GUICMD_Main_FontNudgeLow: 
		w->gm->fontClipLow += (float)cmd->amt * 0.01;
		w->gm->fontClipLow = MIN(MAX(w->gm->fontClipLow, 0.0), w->gm->fontClipHigh);
		w->gm->fontClipGap = w->gm->fontClipHigh - w->gm->fontClipLow; 
		printf("low: %f, high: %f [gap: %f]\n", w->gm->fontClipLow, w->gm->fontClipHigh, w->gm->fontClipGap);
		break;
	case GUICMD_Main_FontNudgeHigh:
		w->gm->fontClipHigh += (float)cmd->amt * 0.01;
		w->gm->fontClipHigh = MIN(MAX(w->gm->fontClipHigh, w->gm->fontClipLow), 1.0);
		w->gm->fontClipGap = w->gm->fontClipHigh - w->gm->fontClipLow;
		printf("low: %f, high: %f [gap: %f]\n", w->gm->fontClipLow, w->gm->fontClipHigh, w->gm->fontClipGap);
		break;
	
	case GUICMD_Main_FontNudgeWidth:
		w->gm->fontClipGap += (float)cmd->amt * 0.01;
		w->gm->fontClipGap = MIN(MAX(w->gm->fontClipGap, 0), 1.0);
		
		if(w->gm->fontClipLow + w->gm->fontClipGap > 1.0) w->gm->fontClipLow = 1.0 - w->gm->fontClipGap;
		w->gm->fontClipHigh = w->gm->fontClipLow + w->gm->fontClipGap;
		printf("low: %f, high: %f [gap: %f]\n", w->gm->fontClipLow, w->gm->fontClipHigh, w->gm->fontClipGap);
		break;
		
	case GUICMD_Main_FontNudgeCenter: 
		w->gm->fontClipLow += (float)cmd->amt * 0.01;
		w->gm->fontClipLow = MIN(MAX(w->gm->fontClipLow, 0.0), 1.0 - w->gm->fontClipGap);
		w->gm->fontClipHigh = w->gm->fontClipLow + w->gm->fontClipGap;
		printf("low: %f, high: %f [gap: %f]\n", w->gm->fontClipLow, w->gm->fontClipHigh, w->gm->fontClipGap);
		break;
		
	}
}


static int message_handler(MainControl* w, Message* m) {
	switch(m->type) {
		case MSG_CmdFwd:
			MainControl_ProcessCommand(w, (GUI_Cmd*)m->data);
			break;
			
		case MSG_CloseMe: {
			int i = MainControl_FindTabIndexByClient(w, m->data);
			if(i > -1) MainControl_CloseTab(w, i);
			break;
		}
		
		case MSG_OpenFile:
			MainControl_LoadFile(w, (char*)m->data);
			break;
		
		case MSG_OpenFileOpt:
			MainControl_LoadFileOpt(w, (GUIFileOpt*)m->data);
			break;
			
		case MSG_GrepOpener:
			MainControl_GrepOpen(w, (char*)m->data);
			free(m->data);
			break;
			
		default:
			return 0;
	}
	return 1;
}


MainControlPane* MainControlPane_New(MainControl* mc) {
	MainControlPane* w = pcalloc(w);
	w->mc = mc;
	w->currentIndex = -1;
	mc->numPanes++;
	
	return w;
}


void MainControlPane_Free(MainControlPane* w, int freeTabContent) {
	w->mc->numPanes = MAX(0, w->mc->numPanes - 1);
	
	if(freeTabContent) {
		VEC_EACH(&w->tabs, i, t) {
			if(t->onDestroy) t->onDestroy(t);
			if(t->title) free(t->title);
			free(t);
		}
	}
	
	VEC_FREE(&w->tabs);
	free(w);
}


MainControl* MainControl_New(GUIManager* gm, Settings* s) {

		
	MainControl* w = pcalloc(w);
	w->s = s;
	w->gs = Settings_GetSection(s, SETTINGS_General);
	MessagePipe_Listen(&w->rx, (void*)message_handler, w);
	
	w->tabHeight = w->gs->MainControl_tabHeight;
	
	// TEMP HACK
	HT_init(&w->breakpoints, 64);
	// ----
	
	HighlighterManager_Init(&w->hm, s);
	Highlighter_ScanDirForModules(&w->hm, w->gs->highlightersPath);
	
	w->xDivisions = 1;
	w->yDivisions = 1;
	int numPanes = w->xDivisions * w->yDivisions;
	
	w->paneSet = malloc(numPanes * sizeof(*w->paneSet));
	for(int i = 0; i < numPanes; i++) {
		w->paneSet[i] = MainControlPane_New(w);
	}
	
	w->focusedPane = w->paneSet[0];
	
	return w;
}

void MainControl_UpdateSettings(MainControl* w, Settings* s) {
	
	w->s = s;
	w->gs = Settings_GetSection(s, SETTINGS_General);
	
	
	VEC_EACH(&w->editors, i, e) {
		GUIBufferEditor_UpdateSettings(e, s);
	}
}


void MainControl_SetFocusedPane(MainControl* w, MainControlPane* p) {
	
	if(w->focusedPane == p) return;

	int x, y;
	for(y = 0; y < w->yDivisions; y++) {
	for(x = 0; x < w->xDivisions; x++) {
		if(p == w->paneSet[x + w->xDivisions * y]) {
			w->focusedPane = p;
			w->focusedPos.x = x; 
			w->focusedPos.y = y;
			
			MainControlTab* tab = VEC_ITEM(&p->tabs, p->currentIndex);
			
			if(!tab->isActive) {
				tab->isActive = 1;
			}
			GUI_SetActive_(w->gm, tab->client, NULL, NULL);
			
			MainControl_OnTabChange(w);
			
			return; 
		}
	}}
	
	fprintf(stderr, "Tried to focus a pane not part of the set (%p).\n", p);
}


void MainControl_ExpandPanes(MainControl* w, int newX, int newY) {
	newX = MAX(1, MIN(newX, 6));
	newY = MAX(1, MIN(newY, 6));
	
	if(newX == w->xDivisions && newY == w->yDivisions) return;
	 
	int newTotal = newX * newY;
	MainControlPane** newPanes = malloc(newTotal * sizeof(*newPanes));
	
	int biggerX = MAX(w->xDivisions, newX);
	int biggerY = MAX(w->yDivisions, newY);
	int smallerX = MIN(w->xDivisions, newX);
	int smallerY = MIN(w->yDivisions, newY);

	// There are 3 possible situations for each pane:
	// 1) initial overlap
	// 2) old size is larger; combine to highest level
	// 3) new size is larger; spawn empty pane

	// 1: copy the overlap region panes
	for(int y = 0; y < smallerY; y++) { 
	for(int x = 0; x < smallerX; x++) { 
		newPanes[x + newX * y] = w->paneSet[x + w->xDivisions * y];
	}}
	
	
	// 2: squash old pane tabs into the highest new pane, on new existing rows
	for(int y = 0; y < smallerY; y++) { 
	for(int x = newX; x < w->xDivisions; x++) { 
		VEC_CAT(&newPanes[newX - 1 + newX * y]->tabs, &w->paneSet[x + w->xDivisions * y]->tabs);
		MainControlPane_Free(w->paneSet[x + w->xDivisions * y], 0);
	}}
	
	// 2: squash old pane tabs into the highest new pane, on new existing columns
	for(int x = 0; x < smallerX; x++) { 
	for(int y = newY; y < w->yDivisions; y++) {
		VEC_CAT(&newPanes[x + newX * (newY - 1)]->tabs, &w->paneSet[x + w->xDivisions * y]->tabs);
		MainControlPane_Free(w->paneSet[x + w->xDivisions * y], 0);
	}}
	
	// 2: squash old pane tabs into the highest new pane, in the clipped-off corner
	for(int x = newX; x < w->xDivisions; x++) {
	for(int y = newY; y < w->yDivisions; y++) {
		VEC_CAT(&newPanes[newX - 1 + newX * (newY - 1)]->tabs, &w->paneSet[x + w->xDivisions * y]->tabs);
		MainControlPane_Free(w->paneSet[x + w->xDivisions * y], 0);
	}}
	
	// 3: create empty panes in newly created space, on the ends of all the rows
	for(int y = 0; y < newY; y++) {
	for(int x = w->xDivisions; x < newX; x++) {
		newPanes[x + newX * y] = MainControlPane_New(w);
		MainControlPane_EmptyTab(newPanes[x + newX * y]);
	}}
	
	// 3: create empty panes in newly created space, below the existing data
	for(int y = w->yDivisions; y < newY; y++) {
	for(int x = 0; x < w->xDivisions; x++) {
		newPanes[x + newX * y] = MainControlPane_New(w);
		MainControlPane_EmptyTab(newPanes[x + newX * y]);
	}}
	
	// todo: clear out any empty tabs that have been squashed into lower panes
	
	free(w->paneSet);
	w->paneSet = newPanes;
	w->xDivisions = newX;
	w->yDivisions = newY;
	
	// focus the closest pane to the previously focused pane
	MainControl_FocusPane(w, w->focusedPos.x, w->focusedPos.y);
}


void MainControl_FocusPane(MainControl* w, int x, int y) {
	
	x = MAX(0, MIN(x, w->xDivisions - 1));
	y = MAX(0, MIN(y, w->yDivisions - 1));
	
	MainControlPane* p = w->paneSet[x + y * w->xDivisions];
	
	if(w->focusedPane != p) {
		
		w->focusedPane = p;
		w->focusedPos.x = x;
		w->focusedPos.y = y;
		
		w->gm->activeID = (void*)VEC_ITEM(&p->tabs, p->currentIndex)->client;
		
		MainControl_OnTabChange(w);
	}
}


MainControlPane* MainControl_GetPane(MainControl* w, int x, int y) {
	
	x = MAX(0, MIN(x, w->xDivisions - 1));
	y = MAX(0, MIN(y, w->yDivisions - 1));
	
	return w->paneSet[x + y * w->xDivisions];
}


MainControlTab* MainControl_AddGenericTab(MainControl* w, void* client, char* title) {
	return MainControlPane_AddGenericTab(w->focusedPane, client, title);
}

MainControlTab* MainControlPane_AddGenericTab(MainControlPane* w, void* client, char* title) {
	
	GeneralSettings* gs = w->mc->gs;
	
	MainControlTab* t = pcalloc(t);
	t->client = client;
	t->title = strdup(title);
	
	// temp
	t->scrollType = gs->MainControl_tabNameScrollFn;
	t->scrollSpeed = gs->MainControl_tabNameScrollAnimTime;
	t->lingerStart = gs->MainControl_tabNameScrollStartLinger;
	t->lingerEnd = gs->MainControl_tabNameScrollEndLinger;
	
	VEC_PUSH(&w->tabs, t);
	
	if(w->currentIndex == -1) {
		w->currentIndex = 0;
		t->isActive = 1;
//		GUIManager_SetMainWindowTitle(w->header.gm, title);
	}
	
	MainControlTab* t0 = VEC_ITEM(&w->tabs, 0);
	if(t0->type == MCTAB_Empty && VEC_LEN(&w->tabs) > 1) {
		MainControlPane_CloseTab(w, 0);
	}
	
	w->tabAutoSortDirty = 1;
	
	MainControl_OnTabChange(w->mc);
	
	return t;
}


void MainControl_CloseTab(MainControl* w, int index) {
	MainControlPane_CloseTab(w->focusedPane, index);
}

void MainControlPane_CloseTab(MainControlPane* w, int index) {
	
	MainControlTab* t = VEC_ITEM(&w->tabs, index);
	
	VEC_RM_SAFE(&w->tabs, index);
	
	
	if(t->onDestroy) t->onDestroy(t);
	if(t->title) free(t->title);
	free(t);
	
	// TODO: check active
	
	
	// update the current tab index
	size_t n_tabs = VEC_LEN(&w->tabs);
	if(!n_tabs) {
		// closing the last real tab will no longer destroy a pane or exit
		MainControlPane_EmptyTab(w);\
		n_tabs = 1;
	}
	if(w->currentIndex > index || w->currentIndex > (n_tabs - 1)) {
		w->currentIndex--;
	}
	w->currentIndex %= n_tabs;
	
	t = VEC_ITEM(&w->tabs, w->currentIndex);
	t->isActive = 1;
	
	MainControl_OnTabChange(w->mc);
}



int MainControl_FindTabIndexByClient(MainControl* w, void* client) {
	return MainControlPane_FindTabIndexByClient(w->focusedPane, client);
}

int MainControlPane_FindTabIndexByClient(MainControlPane* w, void* client) {
	VEC_EACH(&w->tabs, i, tab) {
		if(tab->client == client) {
			return i;
		}
	}

	return -1;
}


int MainControl_FindTabIndexByBufferPath(MainControl* w, char* path) {
	return MainControlPane_FindTabIndexByBufferPath(w->focusedPane, path);
}

int MainControlPane_FindTabIndexByBufferPath(MainControlPane* w, char* path) {
	GUIBufferEditor* be;
	VEC_EACH(&w->tabs, i, tab) {
		if(tab->type == MCTAB_Buffer) {
			be = (GUIBufferEditor*)tab->client;
			
			if(0 == strcmp(path, be->sourceFile)) {
				return i;
			}
		}
	}

	return -1;
}


static int tab_sort_priorities[] = {
	[MCTAB_None] = 0,
	[MCTAB_Buffer] = 6,
	[MCTAB_FileOpener] = 2,
	[MCTAB_FuzzyOpener] = 1,
	[MCTAB_GrepOpener] = 3,
	[MCTAB_Calculator] = 4,
	[MCTAB_Empty] = 5,
};


static int tab_sort_fn(void* _a, void* _b) {
	MainControlTab* a = *((MainControlTab**)_a);
	MainControlTab* b = *((MainControlTab**)_b);
	
	if(a->type != b->type) {
		return tab_sort_priorities[a->type] - tab_sort_priorities[b->type]; 
	}
	
	// types are the same
	switch(a->type) {
		default:
		case MCTAB_None:
		case MCTAB_FuzzyOpener:
		case MCTAB_GrepOpener:
		case MCTAB_Calculator:
		case MCTAB_Empty:
			return (intptr_t)b - (intptr_t)a; // order has no meaning, just be consistent
		
		case MCTAB_Buffer:
		case MCTAB_FileOpener:
			return strcmp(a->title, b->title);
	}
}


void MainControlPane_SortTabs(MainControlPane* w) {
	int len = VEC_LEN(&w->tabs);
	if(len < 2) return; // already sorted
	
	// cache the value of the current index since the indices will change
	MainControlTab* cur = VEC_ITEM(&w->tabs, w->currentIndex);

	VEC_SORT(&w->tabs, tab_sort_fn);
	
	// fix currentIndex
	w->currentIndex = VEC_FIND(&w->tabs, &cur);
	
	MainControl_OnTabChange(w->mc);
}


void MainControlPane_SwapTabs(MainControlPane* w, int ind_a, int ind_b) {
	int len = VEC_LEN(&w->tabs);
	if(len < 2) return; // not enough tabs to swap

	// normalize indices
	ind_a = (ind_a + len) % len;
	ind_b = (ind_b + len) % len;
	
	// swap pointers
	MainControlTab* a = VEC_ITEM(&w->tabs, ind_a);
	VEC_ITEM(&w->tabs, ind_a) = VEC_ITEM(&w->tabs, ind_b);
	VEC_ITEM(&w->tabs, ind_b) = a;
	
	// check and fix currentIndex
	if(w->currentIndex == ind_a) w->currentIndex = ind_b;
	else if(w->currentIndex == ind_b) w->currentIndex = ind_a;
	
	MainControl_OnTabChange(w->mc);
}


void* MainControlPane_NextTab(MainControlPane* w, char cyclic) {
	int len = VEC_LEN(&w->tabs);
	MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
	a->isActive = 0;
	
	if(cyclic) {
		w->currentIndex = (w->currentIndex + 1) % len;
	}
	else {
		w->currentIndex = MIN(w->currentIndex + 1, len - 1);
	}
	
	a = VEC_ITEM(&w->tabs, w->currentIndex);
	if(!a->isActive) {
		a->isActive = 1;
		GUI_SetActive_(w->mc->gm, a->client, NULL, NULL);
	}
	
	MainControl_OnTabChange(w->mc);
	
	return a->client;
}


void* MainControlPane_PrevTab(MainControlPane* w, char cyclic) {
	int len = VEC_LEN(&w->tabs);
	MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
	a->isActive = 0;
	
	if(cyclic) {
		w->currentIndex = (w->currentIndex - 1 + len) % len;
	}
	else {
		w->currentIndex = MAX(w->currentIndex - 1, 0);
	}
	
	a = VEC_ITEM(&w->tabs, w->currentIndex);
	if(!a->isActive) {
		a->isActive = 1;
		GUI_SetActive_(w->mc->gm, a->client, NULL, NULL);
	}
	
	MainControl_OnTabChange(w->mc);
	
	return a->client;
}


void* MainControlPane_GoToTab(MainControlPane* w, int i) {
	int len = VEC_LEN(&w->tabs);
	MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
	a->isActive = 0;
	
	w->currentIndex = MAX(0, MIN(len - 1, i));
	
	a = VEC_ITEM(&w->tabs, w->currentIndex);
	
	if(!a->isActive) {
		a->isActive = 1;
		GUI_SetActive_(w->mc->gm, a->client, NULL, NULL);
	}
	
	MainControl_OnTabChange(w->mc);
	
	return a->client;
}


void* MainControlPane_nthTabOfType(MainControlPane* w, TabType_t type, int n) {
	int n_match = 0;
	VEC_EACH(&w->tabs, i, tab) {
		if(tab->type == type) {
			n_match++;
		}
		if(n_match == n) {
			if(w->currentIndex > -1) { // deactivate old tab
				MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
				if(a) {
					a->isActive = 0;
				}
			}
				
			w->currentIndex = i;
			
//			GUIManager_SetMainWindowTitle(w->header.gm, tab->title);
			tab->isActive = 1;
			GUI_SetActive_(w->mc->gm, tab->client, NULL, NULL);
			
			MainControl_OnTabChange(w->mc);
			
			return tab->client;
		}
	}
	
	return NULL;
}


void MainControl_OpenConjugate(MainControl* w, MainControlTab* tab, char** exts) {
	
	if(tab->type != MCTAB_Buffer) return;
	
	char* orig = ((GUIBufferEditor*)tab->client)->sourceFile;
	if(!orig) return;
	
	char* ext = path_ext(orig);
	char* cext = NULL;
	
	// find the conjugate's extension
	for(char** s = exts; *s; s++) {
		if(0 == strcmp(ext, *s)) continue;
		cext = *s;
		break;
	}
	
	if(!cext) return;
	
	// construct a new file name
	char* new = alloca(strlen(orig) + strlen(cext) + 2);
	strncpy(new, orig, ext - orig);
	strcat(new, cext);
	
	MainControl_LoadFile(w, new);
	MainControlPane_GoToTab(w->focusedPane, MainControl_FindTabIndexByBufferPath(w, new));
}




/*

static int mmBeforeClose(MainControlTab* t) {
	
	return 0;
}

static void mmAfterClose(MainControlTab* t) {
	GUIMainMenu* mm = (GUIMainMenu*)t->client;
	
	GUIMainMenu_Destroy(mm);
}
*/
static int fbBeforeClose(MainControlTab* t) {
	
	return 0;
}

static void fbAfterClose(MainControlTab* t) {
	FileBrowser* fb = (FileBrowser*)t->client;
	
	FileBrowser_Destroy(fb);
}

static void fbEveryFrame(MainControlTab* t) {
	FileBrowser* fb = (FileBrowser*)t->client;
	
	if(0 != strcmp(t->title, fb->curDir)) {
		if(t->title) free(t->title);
		t->title = strdup(fb->curDir);
		
// 		GUIManager_SetMainWindowTitle(fb->header.gm, t->title);
	}
}


void MainControl_OpenFileBrowser(MainControl* w, char* path) {
//	GUIHeader* o = MainControl_nthTabOfType(w, MCTAB_FILEOPEN, 1);
//	if(o != NULL) {
//		return;
//	}

	FileBrowser* fb = FileBrowser_New(w->gm, &w->rx, path);
//	fb->gs = w->gs;

	MainControlTab* tab = MainControl_AddGenericTab(w, fb, path);
	tab->type = MCTAB_FileOpener;
	tab->client = fb;
	tab->render = (void*)FileBrowser_Render;
	tab->beforeClose = fbBeforeClose;
	tab->afterClose = fbAfterClose;
	// 	tab->everyFrame = fbEveryFrame;
	
	// very important, since normal registration is not used

	MainControlPane_nthTabOfType(w->focusedPane, MCTAB_FileOpener, 1);
	
	MainControl_OnTabChange(w);
}


void MainControl_FuzzyOpener(MainControl* w, char* searchTerm) {
	MainControlPane_FuzzyOpener(w->focusedPane, searchTerm);
}

void MainControlPane_FuzzyOpener(MainControlPane* w, char* searchTerm) {
	void* o = MainControlPane_nthTabOfType(w, MCTAB_FuzzyOpener, 1);
	if(o != NULL) {
		return;
	}

	GUIFuzzyMatchControl* fmc = GUIFuzzyMatchControl_New(w->mc->gm, w->mc->s, &w->mc->rx, "./", searchTerm);
	fmc->gs = w->mc->gs;
//	fmc->commands = w->commands;
	MainControlTab* tab = MainControlPane_AddGenericTab(w, fmc, "fuzzy matcher");
	tab->type = MCTAB_FuzzyOpener;
	tab->render = (void*)GUIFuzzyMatchControl_Render;
	tab->client = fmc;
	//tab->beforeClose = gbeBeforeClose;
	//tab->beforeClose = gbeAfterClose;
	//tab->everyFrame = gbeEveryFrame;
	
	MainControlPane_nthTabOfType(w, MCTAB_FuzzyOpener, 1);

	GUIFuzzyMatchControl_Refresh(fmc);
	
	MainControl_OnTabChange(w->mc);
}


void MainControl_GrepOpen(MainControl* w, char* searchTerm) {
	void* o = MainControlPane_nthTabOfType(w->focusedPane, MCTAB_GrepOpener, 1);
	if(o != NULL) {
		return;
	}

	GrepOpenControl* goc = GrepOpenControl_New(w->gm, w->s, &w->rx, searchTerm);
	goc->gs = w->gs;
//	goc->commands = w->commands;
	MainControlTab* tab = MainControl_AddGenericTab(w, goc, "grep opener");
	tab->type = MCTAB_GrepOpener;
	tab->render = (void*)GrepOpenControl_Render;
	tab->client = goc;
	//tab->beforeClose = gbeBeforeClose;
	//tab->beforeClose = gbeAfterClose;
	//tab->everyFrame = gbeEveryFrame;
	
	// goc->header.parent = (GUIHeader*)w;

	MainControlPane_nthTabOfType(w->focusedPane, MCTAB_GrepOpener, 1);
	
	GrepOpenControl_Refresh(goc);
	
	MainControl_OnTabChange(w);
}


void MainControlPane_EmptyTab(MainControlPane* w) {
	void* o = MainControlPane_nthTabOfType(w, MCTAB_Empty, 1);
	if(o != NULL) {
		return;
	}
	
	MainControlTab* tab = MainControlPane_AddGenericTab(w, NULL, "this tab left blank intentionally");
	tab->type = MCTAB_Empty;

	MainControlPane_nthTabOfType(w, MCTAB_Empty, 1);
}

/*
void MainControl_Calculator(MainControl* w) {
	

	GUICalculatorControl* c = GUICalculatorControl_New(w->header.gm);
	c->gs = w->gs;
	c->commands = w->commands;
	
	MainControlTab* tab = MainControl_AddGenericTab(w, &c->header, "calculator");
	tab->type = MCTAB_CALCULATOR;
	
	c->header.parent = (GUIHeader*)w;

	MainControl_nthTabOfType(w, MCTAB_CALCULATOR, 1);
}


void MainControl_Terminal(MainControl* w) {
	

	GUITerminal* c = GUITerminal_New(w->header.gm);
	c->gs = w->gs;
	c->commands = w->commands;
	
	MainControlTab* tab = MainControl_AddGenericTab(w, &c->header, "terminal");
	tab->type = MCTAB_CALCULATOR;
	
	c->header.parent = (GUIHeader*)w;

	MainControl_nthTabOfType(w, MCTAB_CALCULATOR, 1);
}


void MainControl_OpenMainMenu(MainControl* w) {
	
	if(w->menu) {
		// TODO set the tab to active
		return;
	}
	
	w->menu = GUIMainMenu_New(w->header.gm, w->as);
	
	MainControlTab* tab = MainControl_AddGenericTab(w, &w->menu->header, "Main Menu");
	tab->beforeClose = mmBeforeClose;
	tab->afterClose = mmAfterClose;
}

*/

static int gbeBeforeClose(MainControlTab* t) {
	
	return 0;
}

static int gbeAfterClose(MainControlTab* t) {
	GUIBufferEditor* gbe = (GUIBufferEditor*)t->client;
	
	Settings_Free(gbe->s);
	GUIBufferEditor_Destroy(gbe);
	return 0;
}


static void gbeEveryFrame(MainControlTab* t) {
	GUIBufferEditor* gbe = (GUIBufferEditor*)t->client;
	
	t->isStarred = gbe->b->undoSaveIndex != gbe->b->undoCurrent;
	
}

static void loadBreakpoints(MainControl* w) {

}

static void writeBreakpoints(MainControl* w) {
	
	FILE* f = fopen(".gdbinit", "wb");
	
	HT_EACH(&w->breakpoints, key, char*, val) {
		fwrite(key, 1, strlen(key), f);
	}
	
	fclose(f);
}

static void setBreakpoint(char* file, intptr_t line, MainControl* w) {
	size_t len = snprintf(NULL, 0, "b %s:%ld\n", file, line);
	char* loc = malloc(sizeof(*loc) * (len + 1));
	snprintf(loc, len + 1, "b %s:%ld\n", file, line);
	
	char* foo;
	if(!HT_get(&w->breakpoints, loc, &foo)) {
		// key exists
		//printf("deleting");
		HT_delete(&w->breakpoints, loc);
	}
	else {
		HT_set(&w->breakpoints, loc, loc);
	}
	
	
	writeBreakpoints(w);
}


void MainControl_OnTabChange(MainControl* w) {
	
	if(!w->gs->enableSessions) return;
	
	json_write_context_t jwc = {0};
	
	json_value_t* root = json_new_object(8);
	
	json_value_t* pl = json_new_object(8);
	json_obj_set_key(pl, "x", json_new_int(w->xDivisions));
	json_obj_set_key(pl, "y", json_new_int(w->yDivisions));
	json_obj_set_key(root, "paneLayout", pl);
	
	json_value_t* jpanes = json_new_array();
	for(int y = 0; y < w->yDivisions; y++)
	for(int x = 0; x < w->xDivisions; x++) {
		MainControlPane* p = w->paneSet[x + y * w->xDivisions];
		
		json_value_t* jp = json_new_object(16);
		json_obj_set_key(jp, "x", json_new_int(x));
		json_obj_set_key(jp, "y", json_new_int(y));
		json_obj_set_key(jp, "type", json_new_str("tabs"));
		json_obj_set_key(jp, "active_tab", json_new_int(p->currentIndex));
		if(p == w->focusedPane) {
			json_obj_set_key(jp, "is_focused_pane", json_new_int(1));
		}
		
		json_value_t* jtabs = json_new_array();
		VEC_EACH(&p->tabs, ti, t) {
			
			json_value_t* jt = json_new_object(16);
			json_obj_set_key(jt, "type", json_new_str(mctab_type_names[t->type]));
			
			json_value_t* jtc = json_new_object(32);
			if(t->saveSessionState) t->saveSessionState(t->client, jtc);
			json_obj_set_key(jt, "data", jtc);
			
			json_array_push_tail(jtabs, jt);
		}
		
		json_obj_set_key(jp, "tabs", jtabs);
		json_array_push_tail(jpanes, jp);
	}
	
	json_obj_set_key(root, "panes", jpanes);
	
	jwc.sb = json_string_buffer_create(8192);
	jwc.fmt.indentChar = ' ';
	jwc.fmt.indentAmt = 4;
	jwc.fmt.objColonSpace = 1;
	jwc.fmt.noQuoteKeys = 1;
	jwc.fmt.useSingleQuotes = 1;
//	jwc.fmt.minObjSzExpand = 30;
	
	json_stringify(&jwc, root);
//	printf("%.*s\n", (int)jwc.sb->length, jwc.sb->buf);
	write_whole_file("./.gpuedit.session", jwc.sb->buf, jwc.sb->length);
	
	// TODO: write file
	json_string_buffer_free(jwc.sb);
	
	
}


MainControlTab* MainControl_NewEmptyBuffer(MainControl* w) {
	GUIFileOpt opt = {
		.path = NULL,
		.line_num = 1,
		.set_focus = 1,
	};
	return MainControl_LoadFileOpt(w, &opt);
}

MainControlTab* MainControl_LoadFile(MainControl* w, char* path) {
	GUIFileOpt opt = {
		.path = path,
		.line_num = 1,
	};
	return MainControlPane_LoadFileOpt(w->focusedPane, &opt);
}

MainControlTab* MainControlPane_LoadFile(MainControlPane* p, char* path) {
	GUIFileOpt opt = {
		.path = path,
		.line_num = 1,
	};
	return MainControlPane_LoadFileOpt(p, &opt);
}

MainControlTab* MainControl_LoadFileOpt(MainControl* w, GUIFileOpt* opt) {
	return MainControlPane_LoadFileOpt(w->focusedPane, opt);
}

MainControlTab* MainControlPane_LoadFileOpt(MainControlPane* p, GUIFileOpt* opt) {
	
	MainControl* w = p->mc;
	
	if(opt->path) {
		int index = MainControl_FindTabIndexByBufferPath(w, opt->path);
		if(index > -1) {
			GUIBufferEditor* gbe = MainControlPane_GoToTab(w->focusedPane, index);
			
			BufferLine* bl = Buffer_raw_GetLineByNum(gbe->b, opt->line_num);
			if(bl) {
				GBEC_MoveCursorTo(gbe->ec, bl, 0);
				GBEC_scrollToCursorOpt(gbe->ec, 1);
	//			GUIBufferEditControl_SetScroll(gbe->ec, opt->line_num - 11, 0);
			}
			gbe->ec->inputState.modeInfo = Commands_GetModeInfo(w->gm, gbe->ec->inputState.mode);
			return VEC_ITEM(&w->focusedPane->tabs, index);
		}
	}
	
	
	
	
	
	Buffer* buf = BufferCache_GetPath(w->bufferCache, opt->path);
	
	Settings* ls = Settings_Copy(w->s, SETTINGS_ALL);
	
	// read local settings files
	if(opt->path) {
		char* dir = strdup(opt->path);
		dirname(dir);
		Settings_ReadDefaultFilesAt(ls, dir, SETTINGS_ALL);
		free(dir);
	}
	
	BufferSettings* bs = Settings_GetSection(ls, SETTINGS_Buffer);
	ThemeSettings* ts = Settings_GetSection(ls, SETTINGS_Theme);
	GeneralSettings* gs = Settings_GetSection(ls, SETTINGS_General);
	

	// buffer and editor creation
	
	GUIBufferEditor* gbe = GUIBufferEditor_New(w->gm, &w->rx);
	GUIBufferEditor_UpdateSettings(gbe, ls);

	gbe->ec->font = GUI_FindFont(w->gm, bs->font, bs->fontSize);
	gbe->ec->scrollLines = 0;
	gbe->hm = &w->hm;
	gbe->sourceFile = opt->path ? strdup(opt->path) : NULL;
	gbe->setBreakpoint = (void*)setBreakpoint;
	gbe->setBreakpointData = w;
	StatusBar_SetItems(gbe->statusBar, w->gs->MainControl_statusWidgets);
	
	// highlighter
	
	if(VEC_LEN(&w->hm.plugins)) {
		gbe->h = VEC_ITEM(&w->hm.plugins, 0);
		gbe->ec->h = gbe->h;
	}
	else {
		fprintf(stderr, "Error: no highlighter modules loaded.\n");
	}
	// 	initCStyles(gbe->h);
//	char* homedir = getenv("HOME");
//	char* tmp = pathJoin(homedir, ".gpuedit/c_colors.txt");
//
//	Highlighter_LoadStyles(gbe->h, tmp);
//	free(tmp);

	
	GUIBufferEditor_SetBuffer(gbe, buf);
//	GUIBufferEditControl_RefreshHighlight(gbe->ec);
	if(opt->path) GUIBufferEditor_ProbeHighlighter(gbe);
	
	VEC_PUSH(&w->editors, gbe);
	VEC_PUSH(&w->buffers, buf);
	
	// prolly leaks
	char* shortname = opt->path ? basename(strdup(opt->path)) : strdup("<New File>"); 
	
	MainControlTab* tab = MainControlPane_AddGenericTab(p, gbe, shortname);
	tab->type = MCTAB_Buffer;
	tab->beforeClose = gbeBeforeClose;
	tab->beforeClose = gbeAfterClose;
	tab->everyFrame = gbeEveryFrame;
	tab->render = (void*)GUIBufferEditor_Render;
	tab->saveSessionState = (void*)GUIBufferEditor_SaveSessionState;
	tab->client = gbe;
	

/*
	if(opt->set_focus) {
		int i = MainControl_FindTabIndexByHeaderP(w, tab->client);
		MainControl_GoToTab(w, i);
	}
*/	

	GBEC_MoveCursorToNum(gbe->ec, opt->line_num, 0);
	GBEC_SetScrollCentered(gbe->ec, opt->line_num, 0);
	
	gbe->ec->inputState.modeInfo = Commands_GetModeInfo(w->gm, gbe->ec->inputState.mode);
	
	MainControl_OnTabChange(w);
	
	return tab;
}

