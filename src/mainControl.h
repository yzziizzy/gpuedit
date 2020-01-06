#ifndef __gpuedit_mainControl_h__
#define __gpuedit_mainControl_h__

// this is effectively the root ui element of a gpuedit window (the real root is a dummy window)



#include "gui.h"
#include "buffer.h"




enum MainCmdType {
	MainCmd_None = 0,
	MainCmd_SaveActiveTab,
	MainCmd_SaveAll,
	MainCmd_Quit,
	MainCmd_SaveQuit,
	MainCmd_QuitWithoutSave,
	MainCmd_LoadFile,
	MainCmd_NewEmptyBuffer,
	MainCmd_ReloadTab,
	MainCmd_CloseTab,
	MainCmd_SaveAndCloseTab,
	MainCmd_NextTab,
	MainCmd_PrevTab,
};


typedef struct MainCmd {
	enum MainCmdType type;
	int n;
	char* path;
} MainCmd;




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
	
	GUITabBar* bar;
	
	int currentIndex;
	GUIObject* activeTab;
	VEC(GUIObject*) tabs;
	
	VEC(GUIBufferEditor*) editors;
	VEC(Buffer*) buffers;
	
	
	
} GUIMainControl;



GUIMainControl* GUIMainControl_New(GUIManager* gm, GlobalSettings* gs);

int GUIMainControl_AddGenericTab(GUIMainControl* w, GUIHeader* tab, char* title);
GUIObject* GUIMainControl_NextTab(GUIMainControl* w, char cyclic);
GUIObject* GUIMainControl_PrevTab(GUIMainControl* w, char cyclic);
GUIObject* GUIMainControl_GoToTab(GUIMainControl* w, int i);



void GUIMainControl_ProcessCommand(GUIMainControl* w, MainCmd* cmd);


void GUIMainControl_LoadFile(GUIMainControl* w, char* path);
void GUIMainControl_CloseBuffer(GUIMainControl* w, int index);
void GUIMainControl_CloseAllBufferPtr(GUIMainControl* w, Buffer* p);


#endif //__gpuedit_mainControl_h__
