#ifndef __EACSMB_ui_simpleWindow_h__
#define __EACSMB_ui_simpleWindow_h__





typedef struct GUISimpleWindow {
	GUIHeader header;
	
	AABB2 border;
	
	GUIWindow* bg;
	GUIWindow* titlebar;
	GUIWindow* closebutton;
	
	GUIWindow* scrollbarY;
	GUIWindow* scrollbarX;
	Vector2 absScrollPos;
	Vector2 clientExtent; // maximum extent of all the windows in the client area
	
	int disableYScroll : 1;
	int disableXScroll : 1;
	int alwaysShowYScroll: 1;
	int alwaysShowXScroll: 1;
	int yScrollIsShown : 1;
	int xScrollIsShown : 1;
	
	GUIHeader clientArea;
	
	char* title;
	
} GUISimpleWindow;




GUISimpleWindow* GUISimpleWindow_New(GUIManager* gm);






#endif // __EACSMB_ui_simpleWindow_h__
