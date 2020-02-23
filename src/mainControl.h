#ifndef __gpuedit_mainControl_h__
#define __gpuedit_mainControl_h__

// this is effectively the root ui element of a gpuedit window (the real root is a dummy window)



#include "gui.h"
#include "buffer.h"
#include "commands.h"





typedef struct MainCmd {
	enum CmdType type;
	int n;
	char* path;
} MainCmd;



typedef struct MainControlTab {
	GUIObject* client;
	char* title;
	
	unsigned int isActive : 1;
	unsigned int isStarred : 1;
	unsigned int isHovered : 1;
	
	int (*beforeClose)(struct MainControlTab*);
	void (*afterClose)(struct MainControlTab*);
	void (*onActive)(struct MainControlTab*);
	void (*onDeactivate)(struct MainControlTab*);
	void (*onDestroy)(struct MainControlTab*);
	void (*everyFrame)(struct MainControlTab*);
	
} MainControlTab;


typedef struct GUIMainControl {
	GUIHeader header;
	
	float tabHeight;
	
	int showFullPathInTitlebar : 1;
	int showFullPathInTab : 1;
	int showRelPathInTitlebar : 1;
	int showRelPathInTab : 1;
	int showTabBarAlways : 1;
	int showEmptyTabBar : 1;
	// tab scrolling
	// multiline tabs
	// extra tab dropdown
	
	int currentIndex;
	VEC(MainControlTab*) tabs;
	
	VEC(GUIBufferEditor*) editors;
	VEC(Buffer*) buffers;
	
	Cmd* commands;
	
	
} GUIMainControl;



GUIMainControl* GUIMainControl_New(GUIManager* gm, GlobalSettings* gs);

MainControlTab* GUIMainControl_AddGenericTab(GUIMainControl* w, GUIHeader* client, char* title);
void GUIMainControl_CloseTab(GUIMainControl* w, int index);
GUIObject* GUIMainControl_NextTab(GUIMainControl* w, char cyclic);
GUIObject* GUIMainControl_PrevTab(GUIMainControl* w, char cyclic);
GUIObject* GUIMainControl_GoToTab(GUIMainControl* w, int i);



void GUIMainControl_ProcessCommand(GUIMainControl* w, MainCmd* cmd);


void GUIMainControl_LoadFile(GUIMainControl* w, char* path);
void GUIMainControl_OpenFileBrowser(GUIMainControl* w, char* path);
void GUIMainControl_CloseBuffer(GUIMainControl* w, int index);
void GUIMainControl_CloseAllBufferPtr(GUIMainControl* w, Buffer* p);


#endif //__gpuedit_mainControl_h__
