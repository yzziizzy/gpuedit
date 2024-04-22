#ifndef __gpuedit_calcControl_h__
#define __gpuedit_calcControl_h__



#include "ui/gui.h"
#include "commands.h"




typedef struct GUICalculatorControl {
	
	GUIManager* gm;

	MessagePipe* upstream;
	GeneralSettings* gs;

	
} GUICalculatorControl;




GUICalculatorControl* GUICalculatorControl_New(GUIManager* gm, Settings* s, MessagePipe* mp);
void GUICalculatorControl_Render(GUICalculatorControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);

void GUICalculatorControl_ProcessCommand(GUICalculatorControl* w, GUI_Cmd* cmd);


void GUICalculatorControl_Refresh(GUICalculatorControl* w);




#endif // __gpuedit_calcControl_h__
