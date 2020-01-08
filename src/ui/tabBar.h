#ifndef __gpuedit_ui_tabBar_h__
#define __gpuedit_ui_tabBar_h__


typedef struct GUITabBarTab {
	char* title;
	char isActive;
	
	void (*onClick)(int, void*);
	void* onClickData;
	void (*onActivate)(int, void*);
	void* onActivateData;
} GUITabBarTab;



// just a clickable row of tabs
typedef struct GUITabBar {
	GUIHeader header;
	
	VEC(GUITabBarTab*) tabs;
	
} GUITabBar;



GUITabBar* GUITabBar_New(GUIManager* gm);

int GUITabBar_AddTab(GUITabBar* w, char* title);
int GUITabBar_AddTabEx(
	GUITabBar* w, 
	char* title, 
	void (*onClick)(int, void*), 
	void* onClickData,
	void (*onActivate)(int, void*), 
	void* onActivateData
);


void GUITabBar_SetActive(GUITabBar* w, int index);


#endif // __gpuedit_ui_tabBar_h__
