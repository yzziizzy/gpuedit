#ifndef __gputk_tabBar_h__
#define __gputk_tabBar_h__


typedef struct GUITabBarTab {
	char* title;
	char isActive;
	
	void* userData1;
	void* userData2;
	
	void (*onClick)(int, int, struct GUITabBarTab*);
	void (*onActivate)(int, struct GUITabBarTab*);
	void (*onRemove)(int, struct GUITabBarTab*);
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
	void* userData1,
	void* userData2,
	void (*onClick)(int, int, GUITabBarTab*), 
	void (*onActivate)(int, GUITabBarTab*), 
	void (*onRemove)(int, GUITabBarTab*)
);


void GUITabBar_RemoveTab(GUITabBar* w, int index);


void GUITabBar_SetActive(GUITabBar* w, int index);


#endif // __gputk_tabBar_h__
