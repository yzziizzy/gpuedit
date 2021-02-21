#ifndef __EACSMB_ui_statusBar__
#define __EACSMB_ui_statusBar__


#include "buffer.h"
#include "ui/gui.h"






typedef struct GUIStatusBarItem {
	int order; // l-to-r
	char align; // l/c/r
	
	WidgetType_t type;
//	char* name;
	char line[100];
	
	size_t size;  // width in characters
	float offset; // pixel offset within left/center/right list
	char* format;
} GUIStatusBarItem;


typedef struct GUIBufferEditControl GUIBufferEditControl;

typedef struct GUIStatusBar {
	GUIHeader header;
	
	float spacing;
	AABB2 padding;
	
	GUIBufferEditControl* ec;
	
	VEC(GUIStatusBarItem*) items;
	
	VEC(GUIStatusBarItem*) left;
	VEC(GUIStatusBarItem*) center;
	VEC(GUIStatusBarItem*) right;
	
	
} GUIStatusBar;



GUIStatusBar* GUIStatusBar_New(GUIManager* gm);
GUIStatusBarItem* GUIStatusBar_AddItem(GUIStatusBar* w, WidgetSpec* spec, int order);
GUIStatusBar* GUIStatusBar_SetItems(GUIStatusBar* w, WidgetSpec* widgets);




#endif // __EACSMB_ui_statusBar__

