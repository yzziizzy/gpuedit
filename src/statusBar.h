#ifndef __EACSMB_ui_statusBar__
#define __EACSMB_ui_statusBar__


#include "gui.h"






typedef struct GUIStatusBarItem {
	int order; // l-to-r
	char align; // l/c/r
	
	char* name;
	
	float offset;
	
	GUIHeader* item;
	
} GUIStatusBarItem;




typedef struct GUIStatusBar {
	GUIHeader header;
	
	float spacing;
	AABB2 padding;
	Color4 bgColor;
	
	VEC(GUIStatusBarItem*) items;
	
	VEC(GUIStatusBarItem*) left;
	VEC(GUIStatusBarItem*) center;
	VEC(GUIStatusBarItem*) right;
	
	
} GUIStatusBar;



GUIStatusBar* GUIStatusBar_New(GUIManager* gm);
GUIStatusBarItem* GUIStatusBar_AddItem(GUIStatusBar* w, GUIHeader* item, int order, char align, char* name);





#endif // __EACSMB_ui_statusBar__
