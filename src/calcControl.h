#ifndef __gpuedit_calcControl_h__
#define __gpuedit_calcControl_h__



#include "ui/gui.h"
#include "commands.h"


typedef struct CalcHistory {
	
} CalcHistory;


typedef struct GUICalculatorControl {
	GUIHeader header;
	
	char* answer;
	size_t ansalloc;
	
	GUIEdit* inputBox;
	
	GlobalSettings* gs;
	GUI_Cmd* commands;
	
} GUICalculatorControl;



GUICalculatorControl* GUICalculatorControl_New(GUIManager* gm);

void GUICalculatorControl_ProcessCommand(GUICalculatorControl* w, GUI_Cmd* cmd);


void GUICalculatorControl_Refresh(GUICalculatorControl* w);

#endif // __gpuedit_calcControl_h__
