#ifndef __gpuedit_mainMenu_h__
#define __gpuedit_mainMenu_h__

#include "gui.h"
#include "commands.h"

#include "sti/sti.h"





typedef struct GUIMainMenuItem {
	char* label;
	int type;
	char isSelected;
	
	GUIWindow* base; 
	GUIText* gLabel;
	GUIObject* gControl;
} GUIMainMenuItem;


typedef struct GUIMainMenu {
	GUIHeader header;
	
	GUIGridLayout* rows; 
	GUIWindow* scrollbar;
	float sbMinHeight;
	intptr_t scrollOffset;
	
	intptr_t cursorIndex;
	
	VEC(GUIMainMenuItem*) items;
	
	
} GUIMainMenu;



GUIMainMenu* GUIMainMenu_New(GUIManager* gm);
void GUIMainMenu_Destroy(GUIMainMenu* w);

GUIMainMenuItem* GUIMainMenu_AddItem(GUIMainMenu* w, char* name, int type);


#endif // __gpuedit_mainMenu_h__
