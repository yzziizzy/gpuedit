#ifndef __EACSMB_ui_statusBar__
#define __EACSMB_ui_statusBar__


#include "ui/gui.h"






typedef struct StatusBarItem {
	int order; // l-to-r
	char align; // l/c/r
	
	WidgetType_t type;
//	char* name;
	char line[100];
	
	size_t size;  // width in characters
	float offset; // pixel offset within left/center/right list
	char* format;
} StatusBarItem;


struct GUIBufferEditControl;
struct GUIBufferEditor;
typedef struct GUIBufferEditControl GUIBufferEditControl;
typedef struct GUIBufferEditor GUIBufferEditor;

typedef struct StatusBar {
//	GUIHeader header;
	
	float spacing;
	AABB2 padding;
	
	GUIBufferEditControl* ec;
	GUIBufferEditor* ed;
	
	VEC(StatusBarItem*) items;
	
	VEC(StatusBarItem*) left;
	VEC(StatusBarItem*) center;
	VEC(StatusBarItem*) right;
	
	
} StatusBar;



void StatusBar_Render(StatusBar* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);
StatusBar* StatusBar_New(GUIManager* gm, GUIBufferEditor* ec);
StatusBarItem* StatusBar_AddItem(StatusBar* w, WidgetSpec* spec, int order);
StatusBar* StatusBar_SetItems(StatusBar* w, WidgetSpec* widgets);




#endif // __EACSMB_ui_statusBar__

