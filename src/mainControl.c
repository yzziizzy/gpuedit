#include <ctype.h>

#include "mainControl.h"
#include "gui.h"
#include "gui_internal.h"

#include "fileBrowser.h"

#include "highlighters/c.h"

// temporary, should be separated
#include "window.h"



static void render(GUIMainControl* w, PassFrameParams* pfp);
static void updatePos(GUIMainControl* w, GUIRenderParams* grp, PassFrameParams* pfp);





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
		AABB2 box;
		box.min.x = tl.x + tabw * i + i + 1;
		box.min.y = tl.y + 1;
		box.max.x = tabw * (i + 1) + i + 1;
		box.max.y = tl.y + w->tabHeight - 1;
		
		gui_drawTextLine(gm, (Vector2){box.min.x, box.min.y}, (Vector2){box.max.x,0}, &w->header.absClip, &gm->defaults.tabTextColor , w->header.absZ + 0.2, tab->title, strlen(tab->title));
		
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
		if(a) GUIHeader_render(a->client, pfp);
	}
	
	GUIHeader_renderChildren(&w->header, pfp);
}


static void updatePos(GUIMainControl* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	
	
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

	/*
	// update all the tabs
	float tabHeight = w->tabHeight;
	if(VEC_LEN(&w->tabs) <= 1) tabHeight = 0;
	
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


static GUIObject* hitTest(GUIMainControl* w, Vector2 absTestPos) {
// 	printf("tab tes pos %f,%f %p\n", absTestPos.x, absTestPos.y, w);
	GUIObject* o;
	
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
		
		if(boxContainsPoint2(&box, &gev->pos)) {
			if(tab->onClick) tab->onClick(i, gev->button, tab->onClickData);
			if(tab->onActivate) tab->onActivate(i, tab->onActivateData);
			
			return NULL;
		}
		
	}
	*/
	
	return gui_defaultHitTest(w, absTestPos);
}


static void parentResize(GUIObject* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	gui_default_ParentResize(w_, gev);
}

static void gainedFocus(GUIObject* w_, GUIEvent* gev) {
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
		
		if(boxContainsPoint2(&box, &gev->pos)) {
			return i;
		}
	}
	
	return -1;
}



static void mouseMove(GUIObject* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	if(VEC_LEN(&w->tabs) == 0) return;
	
	int index = hitTestTabs(w, gev);
// 	if(index < 0) return;
	
	// highlight hovered tab
	VEC_EACH(&w->tabs, i, t) {
		t->isHovered = (i == index); 
	}
}


static void click(GUIObject* w_, GUIEvent* gev) {
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
		
		GUIManager_popFocusedObject(w->header.gm);
		GUIManager_pushFocusedObject(w->header.gm, t->client);
		GUIManager_SetMainWindowTitle(w->header.gm, t->title);
	}
	else if(gev->button == 2) { // middle click
		// close the tab
		GUIMainControl_CloseTab(w, index);
	}
	
}


static void keyDown(GUIObject* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	
	// special commands
	unsigned int S = GUIMODKEY_SHIFT;
	unsigned int C = GUIMODKEY_CTRL;
	unsigned int A = GUIMODKEY_ALT;
	unsigned int T = GUIMODKEY_TUX;
	
// 	unsigned int scrollToCursor   = 1 << 0;
	/*
	Cmd cmds[] = {
		{A,    XK_Right,     MainCmd_NextTab,         1, 0},
		{A,    XK_Left,      MainCmd_PrevTab,         1, 0},
		{C|S,  'q',          MainCmd_QuitWithoutSave, 0, 0},
		{0,0,0,0,0},
	};
	*/
	
	Cmd found;
	unsigned int iter = 0;
	while(Commands_ProbeCommand(gev, w->commands, &found, &iter)) {
		// GUIBufferEditor will pass on commands to the buffer
		GUIMainControl_ProcessCommand(w, &(MainCmd){
			.type = found.cmd, 
			.n = found.amt,
			.path = NULL,
		});
		
		
// 		if(found.flags & scrollToCursor) {
// 			GUIBufferEditor_scrollToCursor(w);
// 		}
		
	}
}


	void* args[4];

void GUIMainControl_ProcessCommand(GUIMainControl* w, MainCmd* cmd) {
	GUISimpleWindow* sw;
	GUITextF* textf;
	
	GUIBufferEditor* bb = VEC_ITEM(&w->editors, 0);
	printf("eds: %d, lines: %ld\n", VEC_LEN(&w->editors), bb->buffer->numLines);

	args[0] = &bb->buffer->numLines;

	
	switch(cmd->type) {
	case Cmd_NULL:
		// do nothing
		break;
		
	case MainCmd_SimpleWindowTest:
		
		sw = GUISimpleWindow_New(w->header.gm);
		sw->header.topleft = (Vector2){20, 20};
		sw->header.size = (Vector2){400, 400};
		sw->title = "foobar";
// 		sw->absScrollPos.x = 50;
		GUIRegisterObject(w->header.parent, sw);
		
		
		textf = GUITextF_new(w->header.gm);
		textf->header.topleft = (Vector2){20, 50};
		GUITextF_setString(textf, "----%>ld--", args);
		GUIRegisterObject(w->header.parent, textf);
		
		
		for(int i = 0; i < 1; i++) {
			/*GUIFormControl* ww = GUIFormControl_New(w->header.gm, (i%3) +1, "Foo");
			ww->header.topleft.y = i * 35;
			ww->header.size = (Vector2){370, 35};
			GUIObject_AddClient(sw, ww);*/
			
			GUISelectBox* ww = GUISelectBox_New(w->header.gm);
			ww->header.topleft.y = i * 45;
			ww->header.size = (Vector2){370, 35};
			GUIObject_AddClient(sw, ww);
			
			GUISelectBoxOption opts[] = {
				{.label = "> One"},
				{.label = "> Two"},
				{.label = "> Three"},
				{.label = "> Four"},
				{.label = "> Five"},
			};
			
			GUISelectBox_SetOptions(ww, opts, 5);
			
		}
		
		break;
	
	case MainCmd_OpenFileBrowser:
		GUIMainControl_OpenFileBrowser(w, "./");
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
		GUIMainControl_LoadFile(w, cmd->path);
		break;
		
	case MainCmd_NewEmptyBuffer:
		printf("NYI\n");
		break;
		
	case MainCmd_CloseTab:
		printf("NYI\n");
		break;
		
	case MainCmd_SaveAndCloseTab:
		printf("NYI\n");
		break;
		
	case MainCmd_NextTab: GUIMainControl_NextTab(w, 1/*cmd->n*/); break;
	case MainCmd_PrevTab: GUIMainControl_PrevTab(w, 1/*cmd->n*/); break;
	
	}
}


GUIMainControl* GUIMainControl_New(GUIManager* gm, GlobalSettings* gs) {
	
	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
		.HitTest = (void*)hitTest,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
		.KeyDown = keyDown,
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
	};
	
	
	GUIMainControl* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	w->gs = gs;
	w->tabHeight = gs->MainControl_tabHeight;
	
	VEC_INIT(&w->hm.modules);
	VEC_INIT(&w->hm.plugins);
	Highlighter_LoadModule(&w->hm, "src/highlighters/c.so");
	
	
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
	GUIManager_popFocusedObject(w->header.gm);
	
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
	
	
	MainControlTab* t = pcalloc(t);
	t->client = (GUIObject*)client;
	t->title = strdup(title);
	
	VEC_PUSH(&w->tabs, t);
	
	if(w->currentIndex == -1) {
		w->currentIndex = 0;
		t->isActive = 1;
		GUIManager_SetMainWindowTitle(w->header.gm, title);
	}
	
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
	w->currentIndex %= VEC_LEN(&w->tabs);
	
	t = VEC_ITEM(&w->tabs, w->currentIndex);
	t->isActive = 1;
	
	GUIManager_popFocusedObject(w->header.gm);
	GUIManager_pushFocusedObject(w->header.gm, t->client);
	GUIManager_SetMainWindowTitle(w->header.gm, t->title);
}




GUIObject* GUIMainControl_NextTab(GUIMainControl* w, char cyclic) {
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
	
	GUIManager_popFocusedObject(w->header.gm);
	GUIManager_pushFocusedObject(w->header.gm, a->client);
	return a->client;
}


GUIObject* GUIMainControl_PrevTab(GUIMainControl* w, char cyclic) {
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
	
	GUIManager_pushFocusedObject(w->header.gm, a->client);
	return a->client;
}


GUIObject* GUIMainControl_GoToTab(GUIMainControl* w, int i) {
	int len = VEC_LEN(&w->tabs);
	MainControlTab* a = VEC_ITEM(&w->tabs, w->currentIndex);
	a->isActive = 0;
	
	w->currentIndex = MAX(0, MIN(len - 1, i));
	
	a = VEC_ITEM(&w->tabs, w->currentIndex);
	a->isActive = 1;
	
	GUIManager_pushFocusedObject(w->header.gm, a->client);
	return a->client;
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


static void fbOnChoose(void* w_, char** files, intptr_t len) {
	GUIMainControl* w = (GUIMainControl*)w_;
	char** ff = files;
	
	while(*ff) {
// 		printf("files: %s\n", *ff);
		GUIMainControl_LoadFile(w, *ff);
		ff++;
	}
	
}


void GUIMainControl_OpenFileBrowser(GUIMainControl* w, char* path) {
	
	GUIFileBrowser* fb = GUIFileBrowser_New(w->header.gm, path);
	fb->onChooseData = w;
	fb->onChoose = fbOnChoose;
	
	MainControlTab* tab = GUIMainControl_AddGenericTab(w, fb, path);
	tab->beforeClose = fbBeforeClose;
	tab->afterClose = fbAfterClose;
// 	tab->everyFrame = fbEveryFrame;
}


void GUIMainControl_OpenMainMenu(GUIMainControl* w) {
	
	if(w->menu) {
		// TODO set the tab to active
		return;
	}
	
	w->menu = GUIMainMenu_New(w->header.gm, w->as);
	
	MainControlTab* tab = GUIMainControl_AddGenericTab(w, w->menu, "Main Menu");
	tab->beforeClose = mmBeforeClose;
	tab->afterClose = mmAfterClose;
}



static int gbeBeforeClose(MainControlTab* t) {
	
	return 0;
}

static void gbeAfterClose(MainControlTab* t) {
	GUIBufferEditor* gbe = (GUIBufferEditor*)t->client;
	
	GUIBufferEditor_Destroy(gbe);
	
}


static void gbeEveryFrame(MainControlTab* t) {
	GUIBufferEditor* gbe = (GUIBufferEditor*)t->client;
	
	t->isStarred = gbe->buffer->undoSaveIndex != gbe->buffer->undoCurrent;
	
	
}


void GUIMainControl_LoadFile(GUIMainControl* w, char* path) {
	
	// HACK: these structures should be looked up from elsewhere
	EditorParams* ep = pcalloc(ep);
	ep->lineCommentPrefix = "// ";
	ep->selectionCommentPrefix = "/*";
	ep->selectionCommentPostfix= "*/";
	ep->tabWidth = w->gs->Buffer_tabWidth;
	
	TextDrawParams* tdp = pcalloc(tdp);
	tdp->font = FontManager_findFont(w->header.gm->fm, "Courier New");
	tdp->fontSize = .5;
	tdp->charWidth = w->gs->Buffer_charWidth;
	tdp->lineHeight = w->gs->Buffer_lineHeight;
	tdp->tabWidth = w->gs->Buffer_tabWidth;
	
	ThemeDrawParams* theme = pcalloc(theme);
	theme->bgColor =      COLOR4_FROM_HEX( 15,  15,  15, 255);
	theme->textColor =    COLOR4_FROM_HEX(240, 240, 240, 255);
	theme->cursorColor =  COLOR4_FROM_HEX(255,   0, 255, 180);
	theme->hl_bgColor =   COLOR4_FROM_HEX(  0, 200, 200, 255);
	theme->hl_textColor = COLOR4_FROM_HEX(250,   0,  50, 255);
	
	BufferDrawParams* bdp = pcalloc(bdp);
	bdp->tdp = tdp;
	bdp->theme = theme;
	bdp->showLineNums = w->gs->Buffer_showLineNums;
	bdp->lineNumExtraWidth = w->gs->Buffer_lineNumExtraWidth;
	
	
	
	// buffer and editor creation
	Buffer* buf = Buffer_New();
	buf->curCol = 0;
	buf->ep = ep;
	
	GUIBufferEditor* gbe = GUIBufferEditor_New(w->header.gm);
	gbe->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
// 	gbe->header.size = (Vector2){800, 800}; // doesn't matter
	GUIBufferEditor_SetBuffer(gbe, buf);
	gbe->ec->font = tdp->font;
	gbe->ec->scrollLines = w->gs->Buffer_linesPerScrollWheel;
	gbe->bdp = bdp;
	gbe->ec->bdp = bdp;
	gbe->header.name = strdup(path);
	gbe->header.parent = w; // important for bubbling
	gbe->sourceFile = strdup(path);
	gbe->commands = w->commands;
	
	gbe->h = VEC_ITEM(&w->hm.plugins, 0);
	gbe->ec->h = gbe->h;
	// 	initCStyles(gbe->h);
	Highlighter_LoadStyles(gbe->h, "config/c_colors.txt");
	
	Buffer_LoadFromFile(buf, path);
	GUIBufferEditControl_RefreshHighlight(gbe->ec);
	
	VEC_PUSH(&w->editors, gbe);
	VEC_PUSH(&w->buffers, buf);
	
	MainControlTab* tab = GUIMainControl_AddGenericTab(w, gbe, path);
	tab->beforeClose = gbeBeforeClose;
	tab->beforeClose = gbeAfterClose;
	tab->everyFrame = gbeEveryFrame;
}
