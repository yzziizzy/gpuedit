#ifndef __gputk_tabControl_h__
#define __gputk_tabControl_h__



typedef struct GUITabControl {
	GUIHeader header;
	
	GUITabBar* bar;
	
	int currentIndex;
	float tabHeight;
	GUIHeader* activeTab;
	
	VEC(GUIHeader*) tabs;
	
} GUITabControl;



GUITabControl* GUITabControl_New(GUIManager* gm);
int GUITabControl_AddTab(GUITabControl* w, GUIHeader* tab, char* title);

// returns the new current tab's contents
GUIHeader* GUITabControl_NextTab(GUITabControl* w, char cyclic);
GUIHeader* GUITabControl_PrevTab(GUITabControl* w, char cyclic);
GUIHeader* GUITabControl_GoToTab(GUITabControl* w, int i);


#endif //__gputk_tabControl_h__
