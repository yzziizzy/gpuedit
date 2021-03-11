#ifndef __gpuedit_mainMenu_h__
#define __gpuedit_mainMenu_h__

#include "ui/gui.h"

#include "sti/sti.h"





typedef struct GUIMainMenuItem {
	char* label;
	int type;
	char isSelected;
	
	union {
		int* i;
		float* f;
		double* d;
		char** str;
	} data;
	
	GUIWindow* base; 
	GUIText* gLabel;
	GUIHeader* gControl;
} GUIMainMenuItem;



struct AppState;
typedef struct AppState AppState;

typedef struct GUIMainMenu {
	GUIHeader header;
	
	AppState* as; 
	
	GUIGridLayout* rows; 
	GUIWindow* scrollbar;
	GUIWindow* clientArea;
	GUIButton* saveBtn;
	float sbMinHeight;
	intptr_t scrollOffset;
	
	intptr_t cursorIndex;
	
	VEC(GUIMainMenuItem*) items;
	
	GlobalSettings newGS;
	
	
} GUIMainMenu;



GUIMainMenu* GUIMainMenu_New(GUIManager* gm, AppState* as);
void GUIMainMenu_Destroy(GUIMainMenu* w);

int GUIMainMenu_SaveItem(GUIMainMenu* w, GUIMainMenuItem* item);
int GUIMainMenu_SaveAll(GUIMainMenu* w);

GUIMainMenuItem* GUIMainMenu_AddBaseItem(GUIMainMenu* w, char* name);
GUIMainMenuItem* GUIMainMenu_AddInt(GUIMainMenu* w, char* label, int* i);
GUIMainMenuItem* GUIMainMenu_AddFloat(GUIMainMenu* w, char* label, float* f);
GUIMainMenuItem* GUIMainMenu_AddDouble(GUIMainMenu* w, char* label, double* f);
GUIMainMenuItem* GUIMainMenu_AddString(GUIMainMenu* w, char* label, char** str);




void AppState_UpdateSettings(AppState* as, GlobalSettings* gs);


#endif // __gpuedit_mainMenu_h__
