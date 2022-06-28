#ifndef __gpuedit_mainControl_h__
#define __gpuedit_mainControl_h__

// this is effectively the root ui element of a gpuedit window



#include "commands.h"
#include "ui/gui.h"
#include "buffer.h"
#include "mainMenu.h"
#include "highlight.h"
#include "msg.h"



struct MainControl;
struct AppState;
typedef struct AppState AppState;


typedef struct MainControlTab {
	char* title;
	TabType_t type;
	
	unsigned int isActive : 1;
	unsigned int isStarred : 1;
	unsigned int isHovered : 1;
	
	float titleWidth; // updated every frame
	TabScrollType scrollType;
	float lingerStart;
	float lingerEnd;
	float scrollSpeed; // time to move from one side to the other
	
	void* client;
	void (*render)(void* /*client*/, GUIManager*, Vector2 /*tl*/, Vector2 /*sz*/, PassFrameParams*);
	
	int (*beforeClose)(struct MainControlTab*);
	void (*afterClose)(struct MainControlTab*);
	void (*onActive)(struct MainControlTab*);
	void (*onDeactivate)(struct MainControlTab*);
	void (*onDestroy)(struct MainControlTab*);
	void (*everyFrame)(struct MainControlTab*);
	
} MainControlTab;


typedef struct MainControlPane {
	struct MainControl* mc;
	
	int currentIndex;
	VEC(MainControlTab*) tabs;
	char tabAutoSortDirty;
} MainControlPane;



typedef struct MainControl {
	AppState* as; 
	
	int showFullPathInTitlebar : 1;
	int showFullPathInTab : 1;
	int showRelPathInTitlebar : 1;
	int showRelPathInTab : 1;
	int showTabBarAlways : 1;
	int showEmptyTabBar : 1;
	// tab scrolling
	// multiline tabs
	// extra tab dropdown
	
	float tabHeight;
//	int currentIndex;
//	VEC(MainControlTab*) tabs;
//	char tabAutoSortDirty;
	MainControlPane** paneSet;
	MainControlPane* focusedPane; // always a tabbed pane
	int xDivisions;
	int yDivisions;
	
	float editorOffset;
	float editorHeight;
	
	VEC(GUIBufferEditor*) editors;
	VEC(Buffer*) buffers;
	GUIMainMenu* menu; // there is only one main menu
	HighlighterManager hm;
	
	MessagePipe rx;
	GUI_Cmd* commands;
	
	GUIManager* gm;
	Settings* s;
	GeneralSettings* gs;
	
	// TEMP HACK
	char* projectPath;
	HT(char*) breakpoints;
	
} MainControl;


MainControlPane* MainControlPane_New(MainControl* mc);



MainControlTab* MainControlPane_AddGenericTab(MainControlPane* w, void* client, char* title);
void MainControlPane_CloseTab(MainControlPane* w, int index);
int MainControlPane_FindTabIndexByClient(MainControlPane* w, void* client);
int MainControlPane_FindTabIndexByBufferPath(MainControlPane* w, char* path);

MainControl* MainControl_New(GUIManager* gm, Settings* s);
void MainControl_UpdateSettings(MainControl* w, Settings* s);


void MainControl_Render(MainControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);
void MainControlPane_Render(MainControlPane* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);

void MainControl_SetFocusedPane(MainControl* w, MainControlPane* p);
MainControlTab* MainControl_AddGenericTab(MainControl* w, void* client, char* title);
void MainControl_CloseTab(MainControl* w, int index);

int MainControl_FindTabIndexByBufferPath(MainControl* w, char* path);
int MainControl_FindTabIndexByClient(MainControl* w, void* client);

void MainControlPane_SwapTabs(MainControlPane* w, int ind_a, int ind_b);
void MainControlPane_SortTabs(MainControlPane* w);
void* MainControlPane_NextTab(MainControlPane* w, char cyclic);
void* MainControlPane_PrevTab(MainControlPane* w, char cyclic);
void* MainControlPane_GoToTab(MainControlPane* w, int i);
void* MainControlPane_nthTabOfType(MainControlPane* w, TabType_t type, int n);

void MainControl_SplitPane(MainControl* w, int xDivs, int yDivs);


void MainControl_ProcessCommand(MainControl* w, GUI_Cmd* cmd);


void MainControl_OpenMainMenu(MainControl* w);

void MainControl_NewEmptyBuffer(MainControl* w);
void MainControl_LoadFile(MainControl* w, char* path);
void MainControl_LoadFileOpt(MainControl* w, GUIFileOpt* opt);
void MainControl_OpenFileBrowser(MainControl* w, char* path);
void MainControl_FuzzyOpener(MainControl* w, char* searchTerm);
void MainControl_GrepOpen(MainControl* w, char* searchTerm);
void MainControl_Calculator(MainControl* w);
void MainControl_Terminal(MainControl* w);
void MainControl_CloseBuffer(MainControl* w, int index);
void MainControl_CloseAllBufferPtr(MainControl* w, Buffer* p);


#endif //__gpuedit_mainControl_h__
