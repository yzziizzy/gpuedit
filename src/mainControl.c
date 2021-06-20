#include <ctype.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "mainControl.h"
#include "app.h"
#include "fuzzyMatch.h"
#include "ui/gui.h"
#include "ui/gui_internal.h"
#include "ui/configLoader.h"
#include "c_json/json.h"


#include "fileBrowser.h"
#include "fuzzyMatchControl.h"
#include "grepOpenControl.h"
#include "calcControl.h"
#include "terminal.h"

// temporary, should be separated
#include "window.h"



static void render(GUIMainControl* w, PassFrameParams* pfp);
static void updatePos(GUIMainControl* w, GUIRenderParams* grp, PassFrameParams* pfp);



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



static void renderTabs(GUIMainControl* w, PassFrameParams* pfp) {
	GUIManager* gm = w->header.gm;
	GUIUnifiedVertex* v = GUIManager_reserveElements(gm, VEC_LEN(&w->tabs) + 1);
	
	Vector2 tl = w->header.absTopLeft;
	
	// bg
	*v++ = (GUIUnifiedVertex){
		.pos = {tl.x, tl.y, tl.x + w->header.size.x, tl.y + w->tabHeight},
		.clip = GUI_AABB2_TO_SHADER(w->header.absClip),
		
		.guiType = 0, // window (just a box)
		
		.fg = GUI_COLOR4_TO_SHADER(gm->defaults.tabBorderColor), // TODO: border color
		.bg = GUI_COLOR4_TO_SHADER(gm->defaults.tabBorderColor), // TODO: color
		
		.z = w->header.absZ + 0.05,
		.alpha = 1,
	};
	
	float tabw = (w->header.size.x - (1 + VEC_LEN(&w->tabs))) / (VEC_LEN(&w->tabs));
	
	
	// tab backgrounds
	VEC_EACH(&w->tabs, i, tab) {
		struct Color4* color;
		if(tab->isActive) color = &gm->defaults.tabActiveBgColor;
		else if(tab->isHovered) color = &gm->defaults.tabHoverBgColor;
		else color = &gm->defaults.tabBgColor;
		
		*v++ = (GUIUnifiedVertex){
			.pos = {tl.x + tabw * i + i + 1, tl.y + 1, tl.x + tabw * (i + 1) + i + 1, tl.y + w->tabHeight - 1},
			.clip = GUI_AABB2_TO_SHADER(w->header.absClip),
			
			.guiType = 0, // window (just a box)
			
			.fg = GUI_COLOR4_TO_SHADER(*color),
			.bg = GUI_COLOR4_TO_SHADER(*color),
			
			.z = w->header.absZ + 0.1,
			.alpha = 1,
		};
		
	}
	
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
		
			
		if(textw > tabw - 2) {
			switch(tab->scrollType) {
				#define X(x) case TABSC_##x: xoff = tabscroll_fn_##x(tab, tabw, pfp); break;
					TAB_SCROLL_TYPE_LIST
				#undef X
			}
		}
	
		
		AABB2 clip = gui_clipTo(w->header.absClip, box);
		
		gui_drawTextLine(gm, (Vector2){box.min.x - xoff, box.min.y}, (Vector2){textw+1,0}, &clip, &gm->defaults.tabTextColor , w->header.absZ + 0.2, tab->title, strlen(tab->title));
		
		if(tab->isStarred) {
			box.min.x = box.max.x - 10; // TODO magic number
			gui_drawTextLine(gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x,0}, &w->header.absClip, &gm->defaults.tabTextColor , w->header.absZ + 0.2, "*", 1);
		}
	}
}



static void render(GUIMainControl* w, PassFrameParams* pfp) {
	
	renderTabs(w, pfp);
	
	// only render the active tab
	if(w->currentIndex > -1) {
		MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
		if(a && a->client) GUIHeader_render(a->client, pfp);
	}
	
	GUIHeader_renderChildren(&w->header, pfp);
	
	/* debugging and development code
	
	static float spinner = 0;
	
	GUIUnifiedVertex* v = GUIManager_reserveElements(w->header.gm, 3);
	spinner += 0.1;
	spinner = fmod(spinner, 500);
	
	*v++ = (GUIUnifiedVertex){
		.pos = {100, 100, spinner, 200},
		.clip = {0,0, 1000, 1000},
		
		.guiType = 11, // line
		
		.texIndex1 = 10,
		
		.fg = {255, 255, 255, 255}, 
		.bg = {120, 50, 20, 0}, 
		.z = 9999999999999999,
		.alpha = 1,
		.rot = spinner,
	};
	
	float x = 100;
	float y = 100;
	
	*v++ = (GUIUnifiedVertex){
		.pos = {x-100, y - 100, x + 100, y + 100},
		.clip = {0,0, 1000, 1000},
		
		.guiType = 0, // box
		
		.fg = {255, 200, 255, 255}, 
		.bg = {255, 200, 255, 255}, 
		.z = 9999999,
		.alpha = 1,
	};
	
	*v++ = (GUIUnifiedVertex){
		.pos = {x-2, y - 2, x + 2, y + 2},
		.clip = {0,0, 1000, 1000},
		
		.guiType = 0, // box
		
		.fg = {0, 0, 255, 255}, 
		.bg = {0, 0, 255, 255}, 
		.z = 9999999999,
		.alpha = 1,
	};
	//*/
}


static void updatePos(GUIMainControl* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	
	
	if(w->gs->MainControl_autoSortTabs && w->tabAutoSortDirty) {
		GUIMainControl_SortTabs(w);
		w->tabAutoSortDirty = 0;
	} 
	
	
	Vector2 tl = gui_calcPosGrav(h, grp);
	
	h->absTopLeft = tl;
	h->absClip = grp->clip;
	h->absZ = grp->baseZ + h->z;
	
	// update the client of the current tab
	if(w->currentIndex > -1) {
		MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
		
		GUIRenderParams grp2 = {
			.clip = grp->clip, // TODO: update the clip
			// TODO check showXXX flags for height calculation
			.size = {.x = h->size.x, .y = h->size.y - w->tabHeight}, // maximized
			.offset = {
				.x = tl.x,
				.y = tl.y + w->tabHeight,
			},
			.baseZ = grp->baseZ + w->header.z,
		};
		
		if(a) GUIHeader_updatePos(a->client, &grp2, pfp);
	}

	
	// update all the tabs
	/*
	VEC_EACH(&w->tabs, i, child) { 
		
		GUIRenderParams grp2 = {
			.clip = grp->clip, // TODO: update the clip
			.size = child->header.size, // sized to the child to eliminate gravity 
			.offset = {
				.x = tl.x,
				.y = tl.y + tabHeight,
			},
			.baseZ = grp->baseZ + w->header.z,
		};
		
		GUIHeader_updatePos(child, &grp2, pfp);
	}
	*/
	
	// as good a place as any, I suppose
	VEC_EACH(&w->tabs, i, t) {
		if(t->everyFrame) t->everyFrame(t);
	}
}


static GUIHeader* hitTest(GUIMainControl* w, Vector2 absTestPos) {
// 	printf("tab tes pos %f,%f %p\n", absTestPos.x, absTestPos.y, w);
	GUIHeader* o = NULL;
	
	if(w->currentIndex > -1) {
		MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
		if(a) o = gui_defaultHitTest(a->client, absTestPos);
		if(o) return o;
	}
	
	/*
	// check the tabs
	Vector2 tl = w->header.absTopLeft;
	float tabw = (w->header.size.x - (1 + VEC_LEN(&w->tabs))) / (VEC_LEN(&w->tabs));
	
	VEC_EACH(&w->tabs, i, tab) {
		AABB2 box;
		box.min.x = tl.x + tabw * i + i + 1;
		box.min.y = tl.y + 1;
		box.max.x = tl.x + tabw * (i + 1) + i + 1;
		box.max.y = tl.y + w->header.size.y - 1;
		
		if(boxContainsPoint2p(&box, &gev->pos)) {
			if(tab->onClick) tab->onClick(i, gev->button, tab->onClickData);
			if(tab->onActivate) tab->onActivate(i, tab->onActivateData);
			
			return NULL;
		}
		
	}
	*/
	
	return gui_defaultHitTest(&w->header, absTestPos);
}


static void parentResize(GUIHeader* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	gui_default_ParentResize(w_, gev);
}

static void gainedFocus(GUIHeader* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	
	MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
	if(a) { 
		GUIManager_pushFocusedObject(w->header.gm, a->client);
	}
}


static int hitTestTabs(GUIMainControl* w, GUIEvent* gev) {
	Vector2 tl = w->header.absTopLeft;
	float tabw = (w->header.size.x - (1 + VEC_LEN(&w->tabs))) / (VEC_LEN(&w->tabs));
	
	VEC_EACH(&w->tabs, i, tab) {
		AABB2 box;
		box.min.x = tl.x + tabw * i + i + 1;
		box.min.y = tl.y + 1;
		box.max.x = tl.x + tabw * (i + 1) + i + 1;
		box.max.y = tl.y + w->tabHeight - 1;
		
		if(boxContainsPoint2p(&box, &gev->pos)) {
			return i;
		}
	}
	
	return -1;
}



static void mouseMove(GUIHeader* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	if(VEC_LEN(&w->tabs) == 0) return;
	
	int index = hitTestTabs(w, gev);
// 	if(index < 0) return;
	
	// highlight hovered tab
	VEC_EACH(&w->tabs, i, t) {
		t->isHovered = (i == index); 
	}
}


static void click(GUIHeader* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	if(VEC_LEN(&w->tabs) == 0) return;
	
	int index = hitTestTabs(w, gev);
	if(index < 0) return;
	
	if(gev->button == 1) { // left click
		if(w->currentIndex > -1) { // deactivate old tab
			MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
			if(a) {
				a->isActive = 0;
			}
		}
		
		w->currentIndex = index;
		
		// activate new tab
		MainControlTab* t = VEC_ITEM(&w->tabs, index);
		t->isActive = 1;
		
		//GUIManager_popFocusedObject(w->header.gm);
		GUIManager_pushFocusedObject(w->header.gm, t->client);
		GUIManager_SetMainWindowTitle(w->header.gm, t->title);
	}
	else if(gev->button == 2) { // middle click
		// close the tab
		GUIMainControl_CloseTab(w, index);
	}
	
}





static void userEvent(GUIHeader* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	
	if(0 == strcmp(gev->userType, "openFile")) {
		GUIMainControl_LoadFile(w, gev->userData);
		gev->cancelled = 1;
	}
	else if(0 == strcmp(gev->userType, "openFileOpt")) {
		GUIMainControl_LoadFileOpt(w, gev->userData);
		gev->cancelled = 1;
	}
	else if(0 == strcmp(gev->userType, "closeMe")) {
		int i = GUIMainControl_FindTabIndexByHeaderP(w, gev->originalTarget);
		if(i > -1) {
			GUIMainControl_CloseTab(w, i);
			gev->cancelled = 1;
		}
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



void* args[4];

void GUIMainControl_ProcessCommand(GUIMainControl* w, GUI_Cmd* cmd) {
	GUISimpleWindow* sw;
	GUITextF* textf;
	
	//GUIBufferEditor* bb = VEC_ITEM(&w->editors, 0);
// 	printf("eds: %d, lines: %ld\n", VEC_LEN(&w->editors), bb->buffer->numLines);

	//args[0] = &bb->buffer->numLines;
	
	switch(cmd->cmd) {
	case MainCmd_SimpleWindowTest:
	{
/* 		sw = GUISimpleWindow_New(w->header.gm);
		sw->header.topleft = (Vector2){20, 20};
		sw->header.size = (Vector2){400, 400};
		sw->title = "foobar";
// 		sw->absScrollPos.x = 50;
		GUI_RegisterObject(w->header.parent, sw);
		*/
		
		GUIHeader* oo = GUIManager_SpawnTemplate(w->header.gm, "save_changes");
		GUIHeader_RegisterObject(w->header.parent, oo);
// 		GUI_RegisterObject(sw, oo);
	}
		
		
		/*
		textf = GUITextF_new(w->header.gm);
		textf->header.topleft = (Vector2){20, 50};
		GUITextF_setString(textf, "----%>ld--", args);
		GUI_RegisterObject(w->header.parent, textf);
		
		
		for(int i = 0; i < 1; i++) {
			/*GUIFormControl* ww = GUIFormControl_New(w->header.gm, (i%3) +1, "Foo");
			ww->header.topleft.y = i * 35;
			ww->header.size = (Vector2){370, 35};
			GUIHeader_AddClient(sw, ww);* /
			
			GUISelectBox* ww = GUISelectBox_New(w->header.gm);
			ww->header.topleft.y = i * 45;
			ww->header.size = (Vector2){370, 35};
			GUIHeader_AddClient(sw, ww);
			
			GUISelectBoxOption opts[] = {
				{.label = "> One"},
				{.label = "> Two"},
				{.label = "> Three"},
				{.label = "> Four"},
				{.label = "> Five"},
			};
			
			GUISelectBox_SetOptions(ww, opts, 5);
			
		}
		*/
		
		break;
	
	case MainCmd_OpenFileBrowser:
		GUIMainControl_OpenFileBrowser(w, "./");
		break;

	case MainCmd_FuzzyOpener:
		GUIMainControl_FuzzyOpener(w, NULL);
		break;
	
	case MainCmd_GrepOpen:
		GUIMainControl_GrepOpen(w, NULL);
		break;

	case MainCmd_Calculator:
		GUIMainControl_Calculator(w);
		break;
		
	case MainCmd_Terminal:
		GUIMainControl_Terminal(w);
		break;

	case MainCmd_MainMenu:
		GUIMainControl_OpenMainMenu(w);
		break;
		
	case MainCmd_SaveActiveTab:
		printf("NYI\n");
		break;
		
	case MainCmd_SaveAll:
		printf("NYI\n");
		break;
		
	case MainCmd_Quit:
		printf("NYI\n");
		break;
		
	case MainCmd_SaveQuit:
		printf("NYI\n");
		break;
		
	case MainCmd_QuitWithoutSave:
		// TODO: nicer
		exit(0);
		break;
		
	case MainCmd_LoadFile:
		GUIMainControl_LoadFile(w, cmd->str);
		break;
		
	case MainCmd_NewEmptyBuffer:
		GUIMainControl_NewEmptyBuffer(w);
		break;
		
	case MainCmd_CloseTab:
		GUIMainControl_CloseTab(w, w->currentIndex);
		break;
		
	case MainCmd_SaveAndCloseTab:
		printf("NYI\n"); // see BufferCmd_SaveAndClose and BufferCmd_PromptAndClose
		break;
		
	case MainCmd_SortTabs: GUIMainControl_SortTabs(w); break;
	case MainCmd_MoveTabR: GUIMainControl_SwapTabs(w, w->currentIndex, w->currentIndex + 1); break;
	case MainCmd_MoveTabL: GUIMainControl_SwapTabs(w, w->currentIndex, w->currentIndex - 1); break;
	case MainCmd_NextTab: GUIMainControl_NextTab(w, 1/*cmd->n*/); break;
	case MainCmd_PrevTab: GUIMainControl_PrevTab(w, 1/*cmd->n*/); break;
	case MainCmd_GoToTab: GUIMainControl_GoToTab(w, cmd->amt); break;
	
	}
}



static void handleCommand(GUIHeader* w_, GUI_Cmd* cmd) {
	GUIMainControl* w = (GUIMainControl*)w_;
	int needRehighlight = 0;
	
	GUIMainControl_ProcessCommand(w, cmd);
}



GUIMainControl* GUIMainControl_New(GUIManager* gm, GlobalSettings* gs) {
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
		.HitTest = (void*)hitTest,
		.HandleCommand = (void*)handleCommand,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
//		.KeyDown = keyDown,
// 		.Click = click,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
		.Click = click,
		.MiddleClick = click,
		.MouseEnter = mouseMove,
		.MouseLeave = mouseMove,
		.MouseMove = mouseMove,
		.ParentResize = parentResize,
		.GainedFocus = gainedFocus,
		.User = userEvent,
	};
		
	
	GUIMainControl* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	w->header.cmdElementType = CUSTOM_ELEM_TYPE_Main;
	w->gs = gs;
	w->tabHeight = gs->MainControl_tabHeight;
	
	// TEMP HACK
	HT_init(&w->breakpoints, 64);
	w->projectPath =  "/home/steve/projects/gpuedit";
	// ----
	
	HighlighterManager_Init(&w->hm);
	Highlighter_ScanDirForModules(&w->hm, "/usr/lib64/gpuedit/highlighters/");
	Highlighter_ScanDirForModules(&w->hm, "~/.gpuedit/highlighters/");
	
	/*
	GUIWindow* test = GUIWindow_New(gm);
	test->borderColor = (struct Color4){1,0,0,1};
	test->color = (struct Color4){0,1,0,1};
	test->header.size = (Vector2){200, 100};
	GUIManager_SpawnModal(gm, &test->header);
	*/
	
	// TODO: resize
	
	
	return w;
}

void GUIMainControl_UpdateSettings(GUIMainControl* w, GlobalSettings* s) {
	
	*w->gs = *s;
	
	VEC_EACH(&w->editors, i, e) {
		GUIBufferEditor_UpdateSettings(e, s);
	}
}



static void switchtab(int index, int btn, GUITabBarTab* t) {
	GUIMainControl* w = (GUIMainControl*)t->userData1;
	GUIMainControl_GoToTab(w, index);
	//GUIManager_popFocusedObject(w->header.gm);
	
	MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
	if(!a) return; 
	
	GUIManager_pushFocusedObject(w->header.gm, a->client);
	
	if(btn == 2) { // close on middle click
		GUIMainControl_CloseTab(w, index);
		return;
	}
	
	// HACK
	GUIManager_SetMainWindowTitle(w->header.gm, a->title);
}


MainControlTab* GUIMainControl_AddGenericTab(GUIMainControl* w, GUIHeader* client, char* title) {
	
	GlobalSettings* gs = w->gs;
	
	MainControlTab* t = pcalloc(t);
	t->client = (GUIHeader*)client;
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
		GUIManager_SetMainWindowTitle(w->header.gm, title);
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
	
	//GUIManager_popFocusedObject(w->header.gm);
	GUIManager_pushFocusedObject(w->header.gm, t->client);
	GUIManager_SetMainWindowTitle(w->header.gm, t->title);
}



int GUIMainControl_FindTabIndexByHeaderP(GUIMainControl* w, GUIHeader* h) {
	VEC_EACH(&w->tabs, i, tab) {
		if(tab->client == h) {
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
	
	GUIManager_SetMainWindowTitle(w->header.gm, a->title);
//	GUIManager_popFocusedObject(w->header.gm);
	GUIManager_pushFocusedObject(w->header.gm, a->client);
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
	
	GUIManager_SetMainWindowTitle(w->header.gm, a->title);
	GUIManager_pushFocusedObject(w->header.gm, a->client);
	return a->client;
}


GUIHeader* GUIMainControl_GoToTab(GUIMainControl* w, int i) {
	int len = VEC_LEN(&w->tabs);
	MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
	a->isActive = 0;
	
	w->currentIndex = MAX(0, MIN(len - 1, i));
	
	a = VEC_ITEM(&w->tabs, w->currentIndex);
	a->isActive = 1;
	
	GUIManager_SetMainWindowTitle(w->header.gm, a->title);
	GUIManager_pushFocusedObject(w->header.gm, a->client);
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
			
			GUIManager_pushFocusedObject(w->header.gm, tab->client);
			GUIManager_SetMainWindowTitle(w->header.gm, tab->title);
			tab->isActive = 1;

			return tab->client;
		}
	}
	return NULL;
}


static int mmBeforeClose(MainControlTab* t) {
	
	return 0;
}

static void mmAfterClose(MainControlTab* t) {
	GUIMainMenu* mm = (GUIMainMenu*)t->client;
	
	GUIMainMenu_Destroy(mm);
}

static int fbBeforeClose(MainControlTab* t) {
	
	return 0;
}

static void fbAfterClose(MainControlTab* t) {
	GUIFileBrowser* fb = (GUIFileBrowser*)t->client;
	
	GUIFileBrowser_Destroy(fb);
}

static void fbEveryFrame(MainControlTab* t) {
	GUIFileBrowser* fb = (GUIFileBrowser*)t->client;
	
	if(0 != strcmp(t->title, fb->curDir)) {
		if(t->title) free(t->title);
		t->title = strdup(fb->curDir);
		
// 		GUIManager_SetMainWindowTitle(fb->header.gm, t->title);
	}
}


void GUIMainControl_OpenFileBrowser(GUIMainControl* w, char* path) {
	GUIHeader* o = GUIMainControl_nthTabOfType(w, MCTAB_FILEOPEN, 1);
	if(o != NULL) {
		return;
	}

	GUIFileBrowser* fb = GUIFileBrowser_New(w->header.gm, path);
	fb->gs = w->gs;
	fb->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
	fb->commands = w->commands;

	MainControlTab* tab = GUIMainControl_AddGenericTab(w, &fb->header, path);
	tab->type = MCTAB_FILEOPEN;
	tab->beforeClose = fbBeforeClose;
	tab->afterClose = fbAfterClose;
	// 	tab->everyFrame = fbEveryFrame;
	
	// very important, since normal registration is not used
	fb->header.parent = (GUIHeader*)w;

	GUIMainControl_nthTabOfType(w, MCTAB_FILEOPEN, 1);
}


void GUIMainControl_FuzzyOpener(GUIMainControl* w, char* searchTerm) {
	GUIHeader* o = GUIMainControl_nthTabOfType(w, MCTAB_FUZZYOPEN, 1);
	if(o != NULL) {
		return;
	}

	GUIFuzzyMatchControl* fmc = GUIFuzzyMatchControl_New(w->header.gm, "./", searchTerm);
	fmc->gs = w->gs;
	fmc->commands = w->commands;
	MainControlTab* tab = GUIMainControl_AddGenericTab(w, &fmc->header, "fuzzy matcher");
	tab->type = MCTAB_FUZZYOPEN;
	//tab->beforeClose = gbeBeforeClose;
	//tab->beforeClose = gbeAfterClose;
	//tab->everyFrame = gbeEveryFrame;
	
	fmc->header.parent = (GUIHeader*)w;

	GUIMainControl_nthTabOfType(w, MCTAB_FUZZYOPEN, 1);

	GUIFuzzyMatchControl_Refresh(fmc);
}


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



static int gbeBeforeClose(MainControlTab* t) {
	
	return 0;
}

static int gbeAfterClose(MainControlTab* t) {
	GUIBufferEditor* gbe = (GUIBufferEditor*)t->client;
	
	GlobalSettings_Free(gbe->gs);
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
	
	
	GlobalSettings* lgs = GlobalSettings_Copy(w->gs);
	
	// read local settings files
	if(opt->path) {
		char* dir = strdup(opt->path);
		dirname(dir);
		GlobalSettings_ReadDefaultsAt(lgs, dir);
		free(dir);
	}
	
	// HACK: these structures should be looked up from elsewhere
	EditorParams* ep = pcalloc(ep);
	ep->lineCommentPrefix = "// ";
	ep->selectionCommentPrefix = "/*";
	ep->selectionCommentPostfix= "*/";
	ep->tabWidth = lgs->Buffer_tabWidth;
	
	TextDrawParams* tdp = pcalloc(tdp);
	tdp->font = FontManager_findFont(w->header.gm->fm, lgs->Buffer_font);
	tdp->fontSize = lgs->Buffer_fontSize;
	tdp->charWidth = lgs->Buffer_charWidth;
	tdp->lineHeight = lgs->Buffer_lineHeight;
	tdp->tabWidth = lgs->Buffer_tabWidth;
	
	ThemeDrawParams* theme = pcalloc(theme);
	decodeHexColorNorm(lgs->GUI_GlobalSettings->bgColor, (float*)&(theme->bgColor));
	decodeHexColorNorm(lgs->GUI_GlobalSettings->textColor, (float*)&(theme->textColor));
	decodeHexColorNorm(lgs->GUI_GlobalSettings->cursorColor, (float*)&(theme->cursorColor));
	decodeHexColorNorm(lgs->Theme->lineNumColor, (float*)&(theme->lineNumColor));
	decodeHexColorNorm(lgs->Theme->lineNumBgColor, (float*)&(theme->lineNumBgColor));
	decodeHexColorNorm(lgs->Theme->lineNumBookmarkColor, (float*)&(theme->lineNumBookmarkColor));
	decodeHexColorNorm(lgs->Theme->hl_bgColor, (float*)&(theme->hl_bgColor));
	decodeHexColorNorm(lgs->Theme->hl_textColor, (float*)&(theme->hl_textColor));
	decodeHexColorNorm(lgs->Theme->find_bgColor, (float*)&(theme->find_bgColor));
	decodeHexColorNorm(lgs->Theme->find_textColor, (float*)&(theme->find_textColor));
	decodeHexColorNorm(lgs->Theme->outlineCurrentLineBorderColor, (float*)&(theme->outlineCurrentLineBorderColor));
	
	BufferDrawParams* bdp = pcalloc(bdp);
	bdp->tdp = tdp;
	bdp->theme = theme;
	bdp->showLineNums = lgs->Buffer_showLineNums;
	bdp->lineNumExtraWidth = lgs->Buffer_lineNumExtraWidth;
	
	
	
	// buffer and editor creation
	buf->ep = ep;
	
	GUIBufferEditor* gbe = GUIBufferEditor_New(w->header.gm);
	GUIBufferEditor_UpdateSettings(gbe, lgs);

	gbe->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
// 	gbe->header.size = (Vector2){800, 800}; // doesn't matter
	gbe->ec->font = tdp->font;
	gbe->ec->scrollLines = 0;
	gbe->bdp = bdp;
	gbe->ec->bdp = bdp;
	gbe->hm = &w->hm;
	gbe->header.name = opt->path ? strdup(opt->path) : strdup("<new buffer>");
	gbe->header.parent = (GUIHeader*)w; // important for bubbling
	gbe->sourceFile = opt->path ? strdup(opt->path) : NULL;
	gbe->commands = w->commands;
	gbe->setBreakpoint = (void*)setBreakpoint;
	gbe->setBreakpointData = w;
	GUIStatusBar_SetItems(gbe->statusBar, lgs->MainControl_statusWidgets);
	
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
	
	MainControlTab* tab = GUIMainControl_AddGenericTab(w, &gbe->header, shortname);
	tab->type = MCTAB_EDIT;
	tab->beforeClose = gbeBeforeClose;
	tab->beforeClose = gbeAfterClose;
	tab->everyFrame = gbeEveryFrame;

	if(opt->set_focus) {
		int i = GUIMainControl_FindTabIndexByHeaderP(w, tab->client);
		GUIMainControl_GoToTab(w, i);
	}
	
	BufferLine* bl = Buffer_raw_GetLine(gbe->buffer, opt->line_num);
	if(bl) {
		GBEC_MoveCursorTo(gbe->ec, bl, 0);
		GUIBufferEditControl_SetScroll(gbe->ec, opt->line_num - 11, 0);
	}
}

