#include <ctype.h>

#include "mainControl.h"
#include "gui.h"
#include "gui_internal.h"

#include "highlighters/c.h"

// temporary, should be separated
#include "window.h"



static void render(GUIMainControl* w, PassFrameParams* pfp);
static void updatePos(GUIMainControl* w, GUIRenderParams* grp, PassFrameParams* pfp);


static void render(GUIMainControl* w, PassFrameParams* pfp) {
	// only render the active tab
	GUIHeader_render(w->activeTab, pfp);
	
	GUIHeader_renderChildren(&w->header, pfp);
}


static void updatePos(GUIMainControl* w, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUIHeader* h = &w->header;
	
		
	Vector2 tl = gui_calcPosGrav(h, grp);
	
	h->absTopLeft = tl;
// 	h->absClip = gui_clipTo(grp->clip, (AABB2){tl, { clientArea.x + tl.x, clientArea.y + tl.y}}); // TODO: clip this to grp->clip
// 	h->absClip = (AABB2){tl, { clientArea.x + tl.x, clientArea.y + tl.y}}; // TODO: clip this to grp->clip
	
	h->absZ = grp->baseZ + h->z;
	
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
}


static GUIObject* hitTest(GUIMainControl* w, Vector2 absTestPos) {
// 	printf("tab tes pos %f,%f %p\n", absTestPos.x, absTestPos.y, w);
	if(w->activeTab) {
		GUIObject* o = gui_defaultHitTest(w->activeTab, absTestPos);
		if(o) return o;
	}
	
	return gui_defaultHitTest(w, absTestPos);
}


static void parentResize(GUIObject* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	gui_default_ParentResize(w_, gev);
}

static void gainedFocus(GUIObject* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	GUIManager_pushFocusedObject(w->header.gm, (GUIObject*)w->activeTab);
}




static void keyDown(GUIObject* w_, GUIEvent* gev) {
	GUIMainControl* w = (GUIMainControl*)w_;
	
	// special commands
	unsigned int S = GUIMODKEY_SHIFT;
	unsigned int C = GUIMODKEY_CTRL;
	unsigned int A = GUIMODKEY_ALT;
	unsigned int T = GUIMODKEY_TUX;
	
// 	unsigned int scrollToCursor   = 1 << 0;
	
	struct {
		unsigned int mods;
		int keysym;
		enum MainCmdType mcmd;
		int n;
		unsigned int flags;
		
	} cmds[] = {
		{A,    XK_Right,     MainCmd_NextTab,         1, 0},
		{A,    XK_Left,      MainCmd_PrevTab,         1, 0},
		{C|S,  'q',          MainCmd_QuitWithoutSave, 0, 0},
		{0,0,0,0,0},
	};
	
	unsigned int ANY = (GUIMODKEY_SHIFT | GUIMODKEY_CTRL | GUIMODKEY_ALT | GUIMODKEY_TUX);
	unsigned int ANY_MASK = ~ANY;
	for(int i = 0; cmds[i].mcmd != 0; i++) {
// 			printf("%d, '%c', %x \n", gev->keycode, gev->keycode, gev->modifiers);
		if(cmds[i].keysym != tolower(gev->keycode)) continue;
		if((cmds[i].mods & ANY) != (gev->modifiers & ANY)) continue;
		// TODO: specific mods
		
		// GUIBufferEditor will pass on commands to the buffer
		GUIMainControl_ProcessCommand(w, &(MainCmd){
			.type = cmds[i].mcmd, 
			.n = cmds[i].n,
			.path = NULL,
		});
		
		
// 		if(cmds[i].flags & scrollToCursor) {
// 			GUIBufferEditor_scrollToCursor(w);
// 		}
		
	}
}



void GUIMainControl_ProcessCommand(GUIMainControl* w, MainCmd* cmd) {
	switch(cmd->type) {
	case MainCmd_None:
		// do nothing
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
		
	case MainCmd_NextTab: GUIMainControl_NextTab(w, cmd->n); break;
	case MainCmd_PrevTab: GUIMainControl_PrevTab(w, cmd->n); break;
	
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
		.ParentResize = parentResize,
		.GainedFocus = gainedFocus,
	};
	
	
	GUIMainControl* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	
	w->tabHeight = 20;
	
	// TODO: resize
	w->bar = GUITabBar_New(gm);
	GUIRegisterObject(w->bar, w);
	GUIResize(w->bar, (Vector2){800, w->tabHeight});
	
	
	return w;
}



static void switchtab(int index, void* w_) {
	GUIMainControl* w = (GUIMainControl*)w_;
	GUIMainControl_GoToTab(w, index);
	GUIManager_popFocusedObject(w->header.gm);
	GUIManager_pushFocusedObject(w->header.gm, (GUIObject*)w->activeTab);
	
	// HACK
	GUIManager_SetMainWindowTitle(w->header.gm, w->activeTab->h.name);
}


int GUIMainControl_AddGenericTab(GUIMainControl* w, GUIHeader* tab, char* title) {
	if(w->activeTab == NULL) {
		w->activeTab = tab;
		w->currentIndex = 0;
		GUIManager_SetMainWindowTitle(w->header.gm, title);
	}
	
	VEC_PUSH(&w->tabs, tab);
	
	GUITabBar_AddTabEx(w->bar, title, switchtab, w, NULL, NULL);
	
	return VEC_LEN(&w->tabs) - 1;
}


GUIObject* GUIMainControl_NextTab(GUIMainControl* w, char cyclic) {
	int len = VEC_LEN(&w->tabs);
	if(cyclic) {
		w->currentIndex = (w->currentIndex + 1) % len;
	}
	else {
		w->currentIndex = MIN(w->currentIndex + 1, len - 1);
	}
	
	w->activeTab = VEC_ITEM(&w->tabs, w->currentIndex);
	return w->activeTab;
}


GUIObject* GUIMainControl_PrevTab(GUIMainControl* w, char cyclic) {
	int len = VEC_LEN(&w->tabs);
	if(cyclic) {
		w->currentIndex = (w->currentIndex - 1 + len) % len;
	}
	else {
		w->currentIndex = MAX(w->currentIndex + 1, 0);
	}
	
	w->activeTab = VEC_ITEM(&w->tabs, w->currentIndex);
	return w->activeTab;
}


GUIObject* GUIMainControl_GoToTab(GUIMainControl* w, int i) {
	int len = VEC_LEN(&w->tabs);
	w->currentIndex = MAX(0, MIN(len - 1, i));
	
	w->activeTab = VEC_ITEM(&w->tabs, w->currentIndex);
	return w->activeTab;
}


void GUIMainControl_LoadFile(GUIMainControl* w, char* path) {
	
	// HACK: these structures should be looked up from elsewhere
	EditorParams* ep = pcalloc(ep);
	ep->lineCommentPrefix = "// ";
	ep->selectionCommentPrefix = "/*";
	ep->selectionCommentPostfix= "*/";
	ep->tabWidth = 4;
	
	TextDrawParams* tdp = pcalloc(tdp);
	tdp->font = FontManager_findFont(w->header.gm->fm, "Courier New");
	tdp->fontSize = .5;
	tdp->charWidth = 10;
	tdp->lineHeight = 20;
	tdp->tabWidth = 4;
	
	ThemeDrawParams* theme = pcalloc(theme);
	theme->bgColor =      (struct Color4){ 15,  15,  15, 255};
	theme->textColor =    (struct Color4){240, 240, 240, 255};
	theme->cursorColor =  (struct Color4){255,   0, 255, 180};
	theme->hl_bgColor =   (struct Color4){  0, 200, 200, 255};
	theme->hl_textColor = (struct Color4){250,   0,  50, 255};
	
	BufferDrawParams* bdp = pcalloc(bdp);
	bdp->tdp = tdp;
	bdp->theme = theme;
	bdp->showLineNums = 1;
	bdp->lineNumExtraWidth = 10;
	
	
	
	// buffer and editor creation
	Buffer* buf = Buffer_New();
	buf->curCol = 0;
	buf->ep = ep;
	
	GUIBufferEditor* gbe = GUIBufferEditor_New(w->header.gm);
	gbe->header.size = (Vector2){800, 800-20}; // TODO update dynamically
	gbe->buffer = buf;
	gbe->font = tdp->font;
	gbe->scrollLines = 0;
	gbe->bdp = bdp;
	gbe->header.name = strdup(path);
	gbe->header.parent = w; // important for bubbling
	
	gbe->h = pcalloc(gbe->h);
	initCStyles(gbe->h);
	Highlighter_LoadStyles(gbe->h, "config/c_colors.txt");
	
	Buffer_LoadFromFile(buf, path);
	GUIBufferEditor_RefreshHighlight(gbe);
	
	VEC_PUSH(&w->editors, gbe);
	VEC_PUSH(&w->buffers, buf);
	
	GUIMainControl_AddGenericTab(w, gbe, path);
}
