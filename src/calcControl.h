#ifndef __gpuedit_calcControl_h__
#define __gpuedit_calcControl_h__



#include "ui/gui.h"
#include "commands.h"



typedef struct GUICalculatorControl {
	GUIHeader header;
	
	GUIEdit* inputBox;
	
	GlobalSettings* gs;
	GUI_Cmd* commands;
	
} GUICalculatorControl;



GUICalculatorControl* GUICalculatorControl_New(GUIManager* gm);

void GUICalculatorControl_ProcessCommand(GUICalculatorControl* w, GUI_Cmd* cmd);


#endif // __gpuedit_calcControl_h__
