#ifndef __EACSMB_ui_simpleWindow_h__
#define __EACSMB_ui_simpleWindow_h__





typedef struct GUISimpleWindow {
	GUIHeader header;
	
	AABB2 border;
	
	GUIWindow* bg;
	GUIWindow* titlebar;
	GUIWindow* closebutton;
	
	GUIWindow* scrollbar;
	float scrollPos;
	
	
	GUIHeader clientArea;
	
	char* title;
	
} GUISimpleWindow;




GUISimpleWindow* GUISimpleWindow_New(GUIManager* gm);






#endif // __EACSMB_ui_simpleWindow_h__
