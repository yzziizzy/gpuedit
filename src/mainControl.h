#ifndef __gpuedit_mainControl_h__
#define __gpuedit_mainControl_h__

// this is effectively the root ui element of a gpuedit window (the real root is a dummy window)



#include "ui/gui.h"
#include "buffer.h"
#include "mainMenu.h"
#include "commands.h"
#include "highlight.h"

/*
typedef struct MainCmd {
	int type;
	int n;
	char* path;
} MainCmd;
*/

typedef struct MainControlTab {
	GUIHeader* client;
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
	
	int (*beforeClose)(struct MainControlTab*);
	void (*afterClose)(struct MainControlTab*);
	void (*onActive)(struct MainControlTab*);
	void (*onDeactivate)(struct MainControlTab*);
	void (*onDestroy)(struct MainControlTab*);
	void (*everyFrame)(struct MainControlTab*);
	
} MainControlTab;


struct AppState;
typedef struct AppState AppState;

typedef struct GUIMainControl {
	GUIHeader header;
	
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
	int currentIndex;
	VEC(MainControlTab*) tabs;
	char tabAutoSortDirty;
	
	float editorOffset;
	float editorHeight;
	
	VEC(GUIBufferEditor*) editors;
	VEC(Buffer*) buffers;
	GUIMainMenu* menu; // there is only one main menu
	HighlighterManager hm;
	
	GUI_Cmd* commands;
	
	GlobalSettings* gs;
	
	// TEMP HACK
	char* projectPath;
	HT(char*) breakpoints;
	
} GUIMainControl;



GUIMainControl* GUIMainControl_New(GUIManager* gm, GlobalSettings* gs);
void GUIMainControl_UpdateSettings(GUIMainControl* w, GlobalSettings* s);


MainControlTab* GUIMainControl_AddGenericTab(GUIMainControl* w, GUIHeader* client, char* title);
void GUIMainControl_CloseTab(GUIMainControl* w, int index);

int GUIMainControl_FindTabIndexByBufferPath(GUIMainControl* w, char* path);
int GUIMainControl_FindTabIndexByHeaderP(GUIMainControl* w, GUIHeader* h);

void GUIMainControl_SwapTabs(GUIMainControl* w, int ind_a, int ind_b);
void GUIMainControl_SortTabs(GUIMainControl* w);
GUIHeader* GUIMainControl_NextTab(GUIMainControl* w, char cyclic);
GUIHeader* GUIMainControl_PrevTab(GUIMainControl* w, char cyclic);
GUIHeader* GUIMainControl_GoToTab(GUIMainControl* w, int i);
GUIHeader* GUIMainControl_nthTabOfType(GUIMainControl* w, TabType_t type, int n);



void GUIMainControl_ProcessCommand(GUIMainControl* w, GUI_Cmd* cmd);


void GUIMainControl_OpenMainMenu(GUIMainControl* w);

void GUIMainControl_NewEmptyBuffer(GUIMainControl* w);
void GUIMainControl_LoadFile(GUIMainControl* w, char* path);
void GUIMainControl_LoadFileOpt(GUIMainControl* w, GUIFileOpt* opt);
void GUIMainControl_OpenFileBrowser(GUIMainControl* w, char* path);
void GUIMainControl_FuzzyOpener(GUIMainControl* w, char* searchTerm);
void GUIMainControl_GrepOpen(GUIMainControl* w, char* searchTerm);
void GUIMainControl_Calculator(GUIMainControl* w);
void GUIMainControl_CloseBuffer(GUIMainControl* w, int index);
void GUIMainControl_CloseAllBufferPtr(GUIMainControl* w, Buffer* p);


#endif //__gpuedit_mainControl_h__
