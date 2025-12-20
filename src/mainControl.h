#ifndef __gpuedit_mainControl_h__
#define __gpuedit_mainControl_h__

// this is effectively the root ui element of a gpuedit window



#include "commands.h"
#include "ui/gui.h"
#include "buffer.h"
#include "mainMenu.h"
#include "highlight.h"
#include "msg.h"
#include "hexedit.h"



struct MainControl;
struct AppState;
typedef struct AppState AppState;


typedef struct MainControlTab {
	char* title;
	TabType_t type;
	int accessIndex;
	
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
	
	void (*loadSessionState)(void* /*client*/, json_value_t* /*out*/);
	void (*saveSessionState)(void* /*client*/, json_value_t* /*out*/);
	
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
	int lastTabAccessIndex;
	VEC(MainControlTab*) tabs;
	MainControlTab* dragTab;
	int dragIndex;
	char tabAutoSortDirty;
	
	int tabsPerRow;
	int numTabRows;
} MainControlPane;



typedef struct MainControl {
	AppState* as; 
	
	BufferCache* bufferCache;
	
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
	int numPanes;
	MainControlPane** paneSet;
	MainControlPane* focusedPane; // always a tabbed pane
	Vector2i focusedPos;
	int xDivisions;
	int yDivisions;
	
	int maxTabRows;
	float minTabWidth; // in pixels, before row splitting
	
	float editorOffset;
	float editorHeight;
	
	VEC(GUIBufferEditor*) editors;
	VEC(Buffer*) buffers;
	GUIMainMenu* menu; // there is only one main menu
	HighlighterManager hm;
	
	MessagePipe rx;
//	GUI_Cmd* commands;
	
	int inputMode;
	GUIManager* gm;
	Settings* s;
	GeneralSettings* gs;
	
	int sessionLoaded;
	
	// TEMP HACK
	HT(char*) breakpoints;
	
} MainControl;


MainControlPane* MainControlPane_New(MainControl* mc);
void MainControlPane_Free(MainControlPane* w, int freeTabContent);

MainControlPane* MainControl_ChoosePane(MainControl* w, int16_t paneTargeter);

MainControlTab* MainControlPane_AddGenericTab(MainControlPane* w, void* client, char* title);
void MainControlPane_CloseTab(MainControlPane* w, int index);
int MainControlPane_FindTabIndexByClient(MainControlPane* w, void* client);
int MainControlPane_FindTabIndexByBufferPath(MainControlPane* w, char* path);

void MainControl_SaveSession(MainControl* w);
void MainControl_OnTabChange(MainControl* w);

MainControl* MainControl_New(GUIManager* gm, Settings* s);
void MainControl_UpdateSettings(MainControl* w, Settings* s);


void MainControl_Render(MainControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);
void MainControlPane_Render(MainControlPane* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);

void MainControl_ExpandPanes(MainControl* w, int newX, int newY);
void MainControl_SetFocusedPane(MainControl* w, MainControlPane* p);
void MainControl_FocusPane(MainControl* w, int x, int y);
MainControlPane* MainControl_GetPane(MainControl* w, int x, int y);
MainControlTab* MainControl_AddGenericTab(MainControl* w, void* client, char* title);
void MainControlPane_afterTabClose(MainControlPane* w);
void MainControl_CloseTab(MainControl* w, int index);
MainControlTab* MainControl_OpenConjugate(MainControl* w, MainControlTab* tab, char** exts, int16_t paneTargeter);
MainControlTab* MainControlPane_OpenConjugate(MainControlPane* w, MainControlTab* tab, char** exts);
MainControlTab* MainControl_OpenSelf(MainControl* w, MainControlTab* tab, int16_t paneTargeter);


int MainControl_FindTabIndexByBufferPath(MainControl* w, char* path);
int MainControl_FindTabIndexByClient(MainControl* w, void* client);

void MainControlPane_MoveTab(MainControlPane* w, int ind_old, int ind_new);
void MainControlPane_SwapTabs(MainControlPane* w, int ind_a, int ind_b);
void MainControlPane_SortTabs(MainControlPane* w);
void MainControl_RepaneTab(MainControlPane* a, MainControlPane* b, int ind_a, int ind_b);
void* MainControlPane_NextTab(MainControlPane* w, char cyclic);
void* MainControlPane_PrevTab(MainControlPane* w, char cyclic);
void* MainControlPane_GoToTab(MainControlPane* w, int i);
MainControlTab* MainControlPane_nthTabOfType(MainControlPane* w, TabType_t type, int n);


void MainControl_ProcessCommand(MainControl* w, GUI_Cmd* cmd);


void MainControl_OpenMainMenu(MainControl* w);

MainControlTab* MainControl_NewEmptyBuffer(MainControl* w);
MainControlTab* MainControl_LoadFile(MainControl* w, char* path);
MainControlTab* MainControlPane_LoadFile(MainControlPane* p, char* path);
MainControlTab* MainControl_LoadFileOpt(MainControl* w, MessageFileOpt* opt);
MainControlTab* MainControlPane_LoadFileOpt(MainControlPane* w, MessageFileOpt* opt);
void MainControl_OpenFileBrowser(MainControl* w, char* path);

MainControlTab* MainControl_FuzzyOpener(MainControl* w, MessageFuzzyOpt* opt);
MainControlTab* MainControlPane_FuzzyOpener(MainControlPane* w, char* searchTerm);

void MainControl_Hexedit(MainControl* w, char* path);


MainControlTab* MainControl_GrepOpen(MainControl* w, MessageGrepOpt* opt);
MainControlTab* MainControlPane_GrepOpen(MainControlPane* w, char* searchTerm);
void MainControlPane_EmptyTab(MainControlPane* w);
void MainControlPane_Calculator(MainControlPane* w);
void MainControl_Terminal(MainControl* w);
void MainControl_CloseBuffer(MainControl* w, int index);
void MainControl_CloseAllBufferPtr(MainControl* w, Buffer* p);

void MainControl_Hexedit(MainControl* w, char* path);



#endif //__gpuedit_mainControl_h__
