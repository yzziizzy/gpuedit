#ifndef __gpuedit_ui_tabControl_h__
#define __gpuedit_ui_tabControl_h__



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
GUIObject* GUITabControl_NextTab(GUITabControl* w, char cyclic);
GUIObject* GUITabControl_PrevTab(GUITabControl* w, char cyclic);
GUIObject* GUITabControl_GoToTab(GUITabControl* w, int i);


#endif //__gpuedit_ui_tabControl_h__
