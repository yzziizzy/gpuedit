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
//#include "grepOpenControl.h"
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


void GUIMainControl_Render(GUIMainControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp) {
	float oZ = gm->curZ;
	
	Vector2 tab_sz = {sz.x, sz.y - w->tabHeight - 1};
	
	// only render the active tab
	if(w->currentIndex > -1) {
		MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
		if(a && a->render) a->render(a->client, gm, V(tl.x, tl.y + w->tabHeight + 1), tab_sz, pfp);
	}
	
	
	float tabw = (sz.x - (1 + VEC_LEN(&w->tabs))) / (VEC_LEN(&w->tabs));
	
	// handle input
	if(!gm->drawMode) {
		VEC_EACH(&w->tabs, i, tab) {
		
			if(GUI_MouseInside(V(tl.x + tabw * i + i + 1, tl.y), V(tabw - 1, w->tabHeight))) {
				if(GUI_MouseWentUp(1)) {
					GUIMainControl_GoToTab(w, i);
				}
				else if(GUI_MouseWentUp(1)) {
					GUIMainControl_CloseTab(w, i);
				}
			}
		
		}
		
		if(GUI_InputAvailable()) {
			
			GUI_Cmd* cmd = Commands_ProbeCommand(gm, GUIELEMENT_Main, &gm->curEvent, 0);
			
			if(cmd) {
				GUIMainControl_ProcessCommand(w, cmd);
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
	
	// background
	GUI_Rect(tl, V(sz.x, w->tabHeight), &gm->defaults.tabBorderColor);
	

	gm->curZ += 1;
	
	// tab backgrounds
	VEC_EACH(&w->tabs, i, tab) {
		Vector2 boxSz = {tabw - 1, w->tabHeight};
	
		struct Color4* color;
		if(tab->isActive) color = &gm->defaults.tabActiveBgColor;
		else if(GUI_MouseInside(V(tl.x + tabw * i + i + 1, tl.y), boxSz)) color = &gm->defaults.tabHoverBgColor;
		else color = &gm->defaults.tabBgColor;
		
		
		// TODO: fix pixel widths
		GUI_Rect(V(tl.x + tabw * i + i + 1, tl.y + 1), V(tabw - 1, w->tabHeight - 2), color);
		
		
	}
	
	AABB2 oClip = gm->curClip;
	
	// tab titles
	VEC_EACH(&w->tabs, i, tab) {
		float textw = gui_getTextLineWidth(gm, NULL, 0, tab->title, strlen(tab->title));
		float xoff = 0;
		
		tab->titleWidth = textw;
	
		AABB2 box;
		box.min.x = tl.x + tabw * i + i + 1;
		box.min.y = tl.y + 1;
		box.max.x = tl.x + tabw * (i + 1) + i + 1;
		box.max.y = tl.y + w->tabHeight - 1;
		gm->curClip = box;
		
			
		if(textw > tabw - 2) {
			switch(tab->scrollType) {
				#define X(x) case TABSC_##x: xoff = tabscroll_fn_##x(tab, tabw, pfp); break;
					TAB_SCROLL_TYPE_LIST
				#undef X
			}
		}
	
		
		GUI_TextLineCentered(tab->title, strlen(tab->title), V(box.min.x, box.min.y - 2), V(tabw - 2, w->tabHeight - 2), "Arial", w->tabHeight - 5, &gm->defaults.tabTextColor);
//		AABB2 clip = gui_clipTo(w->header.absClip, box);
		
//		gui_drawTextLine(gm, (Vector2){box.min.x - xoff, box.min.y}, (Vector2){textw+1,0}, &box, &gm->defaults.tabTextColor , w->header.absZ + 0.2, tab->title, strlen(tab->title));
		
//		if(tab->isStarred) {
//			box.min.x = box.max.x - 10; // TODO magic number
//			gui_drawTextLine(gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x,0}, &box, &gm->defaults.tabTextColor , w->header.absZ + 0.2, "*", 1);
//		}
	}
	
	
#include "ui/macros_off.h"
}





/*

static void userEvent(GUIHeader* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	
	} else if(0 == strcmp(gev->userType, "SmartBubble")) {
		GUIBubbleOpt* opt = (GUIBubbleOpt*)gev->userData;
		if(0 == strcmp(opt->ev, "FuzzyOpen")) {
			GUIMainControl_FuzzyOpener(w, opt->sel);
			gev->cancelled = 1;
		} else if(0 == strcmp(opt->ev, "GrepOpen")) {
			GUIMainControl_GrepOpen(w, opt->sel);
			gev->cancelled = 1;
		} else {
			printf("MainControl::SmartBubble unknown ev '%s'\n", opt->ev);
		}
	}
}

*/

void* args[4];

void GUIMainControl_ProcessCommand(GUIMainControl* w, GUI_Cmd* cmd) {
	
	switch(cmd->cmd) {
	
	case GUICMD_Main_OpenFileBrowser:
		GUIMainControl_OpenFileBrowser(w, "./");
		break;

	case GUICMD_Main_FuzzyOpener:
		GUIMainControl_FuzzyOpener(w, NULL);
		break;
	
	case GUICMD_Main_GrepOpen:
//		GUIMainControl_GrepOpen(w, NULL);
		break;

	case GUICMD_Main_Calculator:
//		GUIMainControl_Calculator(w);
		break;
		
	case GUICMD_Main_Terminal:
//		GUIMainControl_Terminal(w);
		break;

	case GUICMD_Main_MainMenu:
//		GUIMainControl_OpenMainMenu(w);
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
		GUIMainControl_LoadFile(w, cmd->str);
		break;
		
	case GUICMD_Main_NewEmptyBuffer:
		GUIMainControl_NewEmptyBuffer(w);
		break;
		
	case GUICMD_Main_CloseTab:
		GUIMainControl_CloseTab(w, w->currentIndex);
		break;
		
	case GUICMD_Main_SaveAndCloseTab:
		printf("NYI\n"); // see BufferCmd_SaveAndClose and BufferCmd_PromptAndClose
		break;
		
	case GUICMD_Main_SortTabs: GUIMainControl_SortTabs(w); break;
	case GUICMD_Main_MoveTabR: GUIMainControl_SwapTabs(w, w->currentIndex, w->currentIndex + 1); break;
	case GUICMD_Main_MoveTabL: GUIMainControl_SwapTabs(w, w->currentIndex, w->currentIndex - 1); break;
	case GUICMD_Main_NextTab: GUIMainControl_NextTab(w, 1/*cmd->n*/); break;
	case GUICMD_Main_PrevTab: GUIMainControl_PrevTab(w, 1/*cmd->n*/); break;
	case GUICMD_Main_GoToTab: GUIMainControl_GoToTab(w, cmd->amt); break;
	
	
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


static int message_handler(GUIMainControl* w, Message* m) {
	switch(m->type) {
		case MSG_CmdFwd:
			GUIMainControl_ProcessCommand(w, (GUI_Cmd*)m->data);
			break;
			
		case MSG_CloseMe: {
			int i = GUIMainControl_FindTabIndexByClient(w, m->data);
			if(i > -1) GUIMainControl_CloseTab(w, i);
			break;
		}
		
		case MSG_OpenFile:
			GUIMainControl_LoadFile(w, (char*)m->data);
			break;
		
		case MSG_OpenFileOpt:
			GUIMainControl_LoadFileOpt(w, (GUIFileOpt*)m->data);
			break;
			
		default:
			return 0;
	}
	return 1;
}


GUIMainControl* GUIMainControl_New(GUIManager* gm, Settings* s) {

		
	GUIMainControl* w = pcalloc(w);
	w->s = s;
	w->gs = Settings_GetSection(s, SETTINGS_General);
	MessagePipe_Listen(&w->rx, (void*)message_handler, w);
	
	w->tabHeight = w->gs->MainControl_tabHeight;
	
	// TEMP HACK
	HT_init(&w->breakpoints, 64);
	w->projectPath =  "/home/steve/projects/gpuedit";
	// ----
	
	HighlighterManager_Init(&w->hm, s);
	Highlighter_ScanDirForModules(&w->hm, w->gs->highlightersPath);
	
	
	return w;
}

void GUIMainControl_UpdateSettings(GUIMainControl* w, Settings* s) {
	
	w->s = s;
	w->gs = Settings_GetSection(s, SETTINGS_General);
	
	
	VEC_EACH(&w->editors, i, e) {
		GUIBufferEditor_UpdateSettings(e, s);
	}
}



MainControlTab* GUIMainControl_AddGenericTab(GUIMainControl* w, void* client, char* title) {
	
	GeneralSettings* gs = w->gs;
	
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
	
	w->tabAutoSortDirty = 1;
	
	return t;
}


void GUIMainControl_CloseTab(GUIMainControl* w, int index) {
	
	MainControlTab* t = VEC_ITEM(&w->tabs, index);
	
	VEC_RM_SAFE(&w->tabs, index);
	
	
	if(t->onDestroy) t->onDestroy(t);
	if(t->title) free(t->title);
	free(t);
	
	// TODO: check active
	
	
	// update the current tab index
	size_t n_tabs = VEC_LEN(&w->tabs);
	if(!n_tabs) {
		// todo: exit more fancily
		exit(0);
	}
	if(w->currentIndex > index || w->currentIndex > (n_tabs - 1)) {
		w->currentIndex--;
	}
	w->currentIndex %= n_tabs;
	
	t = VEC_ITEM(&w->tabs, w->currentIndex);
	t->isActive = 1;
}



int GUIMainControl_FindTabIndexByClient(GUIMainControl* w, void* client) {
	VEC_EACH(&w->tabs, i, tab) {
		if(tab->client == client) {
			return i;
		}
	}

	return -1;
}


int GUIMainControl_FindTabIndexByBufferPath(GUIMainControl* w, char* path) {
	GUIBufferEditor* be;
	VEC_EACH(&w->tabs, i, tab) {
		if(tab->type == MCTAB_EDIT) {
			be = (GUIBufferEditor*)tab->client;
			
			if(0 == strcmp(path, be->sourceFile)) {
				return i;
			}
		}
	}

	return -1;
}


static int tab_sort_priorities[] = {
	[MCTAB_NONE] = 0,
	[MCTAB_EDIT] = 4,
	[MCTAB_FILEOPEN] = 2,
	[MCTAB_FUZZYOPEN] = 1,
	[MCTAB_GREPOPEN] = 3,
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
		case MCTAB_NONE:
		case MCTAB_FUZZYOPEN:
		case MCTAB_GREPOPEN:
			return (intptr_t)b - (intptr_t)a; // order has no meaning, just be consistent
		
		case MCTAB_EDIT:
		case MCTAB_FILEOPEN:
			return strcmp(a->title, b->title);
	}
}


void GUIMainControl_SortTabs(GUIMainControl* w) {
	int len = VEC_LEN(&w->tabs);
	if(len < 2) return; // already sorted
	
	// cache the value of the current index since the indices will change
	MainControlTab* cur = VEC_ITEM(&w->tabs, w->currentIndex);

	VEC_SORT(&w->tabs, tab_sort_fn);
	
	// fix currentIndex
	w->currentIndex = VEC_FIND(&w->tabs, &cur);
}


void GUIMainControl_SwapTabs(GUIMainControl* w, int ind_a, int ind_b) {
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
}


GUIHeader* GUIMainControl_NextTab(GUIMainControl* w, char cyclic) {
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
	a->isActive = 1;
	
	GUI_SetActive_(w->gm, a->client, NULL, NULL);
	
	return a->client;
}


GUIHeader* GUIMainControl_PrevTab(GUIMainControl* w, char cyclic) {
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
	a->isActive = 1;
	
	GUI_SetActive_(w->gm, a->client, NULL, NULL);
	
	return a->client;
}


GUIHeader* GUIMainControl_GoToTab(GUIMainControl* w, int i) {
	int len = VEC_LEN(&w->tabs);
	MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
	a->isActive = 0;
	
	w->currentIndex = MAX(0, MIN(len - 1, i));
	
	a = VEC_ITEM(&w->tabs, w->currentIndex);
	a->isActive = 1;
	
	GUI_SetActive_(w->gm, a->client, NULL, NULL);
	
	return a->client;
}


GUIHeader* GUIMainControl_nthTabOfType(GUIMainControl* w, TabType_t type, int n) {
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

			return tab->client;
		}
	}
	return NULL;
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


void GUIMainControl_OpenFileBrowser(GUIMainControl* w, char* path) {
//	GUIHeader* o = GUIMainControl_nthTabOfType(w, MCTAB_FILEOPEN, 1);
//	if(o != NULL) {
//		return;
//	}

	FileBrowser* fb = FileBrowser_New(w->gm, &w->rx, path);
//	fb->gs = w->gs;

	MainControlTab* tab = GUIMainControl_AddGenericTab(w, fb, path);
	tab->type = MCTAB_FILEOPEN;
	tab->client = fb;
	tab->render = (void*)FileBrowser_Render;
	tab->beforeClose = fbBeforeClose;
	tab->afterClose = fbAfterClose;
	// 	tab->everyFrame = fbEveryFrame;
	
	// very important, since normal registration is not used

	GUIMainControl_nthTabOfType(w, MCTAB_FILEOPEN, 1);
}


void GUIMainControl_FuzzyOpener(GUIMainControl* w, char* searchTerm) {
	GUIHeader* o = GUIMainControl_nthTabOfType(w, MCTAB_FUZZYOPEN, 1);
	if(o != NULL) {
		return;
	}

	GUIFuzzyMatchControl* fmc = GUIFuzzyMatchControl_New(w->gm, w->s, &w->rx, "./", searchTerm);
	fmc->gs = w->gs;
//	fmc->commands = w->commands;
	MainControlTab* tab = GUIMainControl_AddGenericTab(w, fmc, "fuzzy matcher");
	tab->type = MCTAB_FUZZYOPEN;
	tab->render = (void*)GUIFuzzyMatchControl_Render;
	tab->client = fmc;
	//tab->beforeClose = gbeBeforeClose;
	//tab->beforeClose = gbeAfterClose;
	//tab->everyFrame = gbeEveryFrame;
	
	GUIMainControl_nthTabOfType(w, MCTAB_FUZZYOPEN, 1);

	GUIFuzzyMatchControl_Refresh(fmc);
}

/*
void GUIMainControl_GrepOpen(GUIMainControl* w, char* searchTerm) {
	GUIHeader* o = GUIMainControl_nthTabOfType(w, MCTAB_GREPOPEN, 1);
	if(o != NULL) {
		return;
	}

	GUIGrepOpenControl* goc = GUIGrepOpenControl_New(w->header.gm, searchTerm);
	goc->gs = w->gs;
	goc->commands = w->commands;
	MainControlTab* tab = GUIMainControl_AddGenericTab(w, &goc->header, "grep opener");
	tab->type = MCTAB_GREPOPEN;
	//tab->beforeClose = gbeBeforeClose;
	//tab->beforeClose = gbeAfterClose;
	//tab->everyFrame = gbeEveryFrame;
	
	goc->header.parent = (GUIHeader*)w;

	GUIMainControl_nthTabOfType(w, MCTAB_GREPOPEN, 1);
	
	GUIGrepOpenControl_Refresh(goc);
}


void GUIMainControl_Calculator(GUIMainControl* w) {
	

	GUICalculatorControl* c = GUICalculatorControl_New(w->header.gm);
	c->gs = w->gs;
	c->commands = w->commands;
	
	MainControlTab* tab = GUIMainControl_AddGenericTab(w, &c->header, "calculator");
	tab->type = MCTAB_CALCULATOR;
	
	c->header.parent = (GUIHeader*)w;

	GUIMainControl_nthTabOfType(w, MCTAB_CALCULATOR, 1);
}


void GUIMainControl_Terminal(GUIMainControl* w) {
	

	GUITerminal* c = GUITerminal_New(w->header.gm);
	c->gs = w->gs;
	c->commands = w->commands;
	
	MainControlTab* tab = GUIMainControl_AddGenericTab(w, &c->header, "terminal");
	tab->type = MCTAB_CALCULATOR;
	
	c->header.parent = (GUIHeader*)w;

	GUIMainControl_nthTabOfType(w, MCTAB_CALCULATOR, 1);
}


void GUIMainControl_OpenMainMenu(GUIMainControl* w) {
	
	if(w->menu) {
		// TODO set the tab to active
		return;
	}
	
	w->menu = GUIMainMenu_New(w->header.gm, w->as);
	
	MainControlTab* tab = GUIMainControl_AddGenericTab(w, &w->menu->header, "Main Menu");
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
	
	t->isStarred = gbe->buffer->undoSaveIndex != gbe->buffer->undoCurrent;
	
}

static void loadBreakpoints(GUIMainControl* w) {

}

static void writeBreakpoints(GUIMainControl* w) {
	
	FILE* f = fopen(".gdbinit", "wb");
	
	HT_EACH(&w->breakpoints, key, char*, val) {
		fwrite(key, 1, strlen(key), f);
	}
	
	fclose(f);
}

static void setBreakpoint(char* file, intptr_t line, GUIMainControl* w) {
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


void GUIMainControl_NewEmptyBuffer(GUIMainControl* w) {
	GUIFileOpt opt = {
		.path = NULL,
		.line_num = 1,
		.set_focus = 1,
	};
	GUIMainControl_LoadFileOpt(w, &opt);
}

void GUIMainControl_LoadFile(GUIMainControl* w, char* path) {
	GUIFileOpt opt = {
		.path = path,
		.line_num = 1,
	};
	GUIMainControl_LoadFileOpt(w, &opt);
}

void GUIMainControl_LoadFileOpt(GUIMainControl* w, GUIFileOpt* opt) {
	
	/* // TODO IMGUI
	if(opt->path) {
		int index = GUIMainControl_FindTabIndexByBufferPath(w, opt->path);
		if(index > -1) {
			GUIHeader* header = GUIMainControl_GoToTab(w, index);
			GUIBufferEditor* gbe = (GUIBufferEditor*)header;
			BufferLine* bl = Buffer_raw_GetLine(gbe->buffer, opt->line_num);
			if(bl) {
				GBEC_MoveCursorTo(gbe->ec, bl, 0);
				GBEC_scrollToCursorOpt(gbe->ec, 1);
	//			GUIBufferEditControl_SetScroll(gbe->ec, opt->line_num - 11, 0);
			}
			return;
		}
	}
	*/
	
	Buffer* buf = Buffer_New();
	
	if(opt->path) {
		int status = Buffer_LoadFromFile(buf, opt->path);
		if(status) {
			Buffer_Delete(buf);
			return;
		}
	}
	
	if(buf->numLines == 0) {
		Buffer_InitEmpty(buf);
	}
	
	
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
	
	// HACK: these structures should be looked up from elsewhere
	EditorParams* ep = pcalloc(ep);
	ep->lineCommentPrefix = "// ";
	ep->selectionCommentPrefix = "/*";
	ep->selectionCommentPostfix= "* /";
	ep->tabWidth = bs->tabWidth;
	
	
	TextDrawParams* tdp = pcalloc(tdp);
	tdp->font = FontManager_findFont(w->gm->fm, bs->font);
	tdp->fontSize = bs->fontSize;
	tdp->charWidth = bs->charWidth;
	tdp->lineHeight = bs->lineHeight;
	tdp->tabWidth = bs->tabWidth;
		
		
	BufferDrawParams* bdp = pcalloc(bdp);
	bdp->tdp = tdp;
	bdp->theme = ts;
	bdp->showLineNums = bs->showLineNums;
	bdp->lineNumExtraWidth = bs->lineNumExtraWidth;
	
	
	
	// buffer and editor creation
	buf->ep = ep;
	
	GUIBufferEditor* gbe = GUIBufferEditor_New(w->gm);
	GUIBufferEditor_UpdateSettings(gbe, ls);

//	gbe->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
// 	gbe->header.size = (Vector2){800, 800}; // doesn't matter
	gbe->ec->font = tdp->font;
	gbe->ec->scrollLines = 0;
	gbe->bdp = bdp;
	gbe->ec->bdp = bdp;
	gbe->hm = &w->hm;
//	gbe->header.name = opt->path ? strdup(opt->path) : strdup("<new buffer>");
//	gbe->header.parent = (GUIHeader*)w; // important for bubbling
	gbe->sourceFile = opt->path ? strdup(opt->path) : NULL;
	gbe->commands = w->commands;
	gbe->setBreakpoint = (void*)setBreakpoint;
	gbe->setBreakpointData = w;
	StatusBar_SetItems(gbe->statusBar, w->gs->MainControl_statusWidgets);
	
	// highlighter
	gbe->h = VEC_ITEM(&w->hm.plugins, 0);
	gbe->ec->h = gbe->h;
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
	
	MainControlTab* tab = GUIMainControl_AddGenericTab(w, gbe, shortname);
	tab->type = MCTAB_EDIT;
	tab->beforeClose = gbeBeforeClose;
	tab->beforeClose = gbeAfterClose;
	tab->everyFrame = gbeEveryFrame;
	tab->render = (void*)GUIBufferEditor_Render;
	tab->client = gbe;

/*
	if(opt->set_focus) {
		int i = GUIMainControl_FindTabIndexByHeaderP(w, tab->client);
		GUIMainControl_GoToTab(w, i);
	}
*/
	
	BufferLine* bl = Buffer_raw_GetLine(gbe->buffer, opt->line_num);
	if(bl) {
		GBEC_MoveCursorTo(gbe->ec, bl, 0);
		GUIBufferEditControl_SetScroll(gbe->ec, opt->line_num - 11, 0);
	}
}

