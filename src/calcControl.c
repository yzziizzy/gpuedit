
#include "calcControl.h" 
#include "ui/gui_internal.h"

	

static void render(GUIHeader* w_, PassFrameParams* pfp) {
	GUICalculatorControl* w = (GUICalculatorControl*)w_;
	GUIManager* gm = w->header.gm;
	GUIUnifiedVertex* v;



	
	GUIHeader_renderChildren(&w->header, pfp);
}



static void updatePos(GUIHeader* w_, GUIRenderParams* grp, PassFrameParams* pfp) {
	GUICalculatorControl* w = (GUICalculatorControl*)w_;


	gui_defaultUpdatePos(&w->header, grp, pfp);
}






static void gainedFocus(GUIHeader* w_, GUIEvent* gev) {
	GUICalculatorControl* w = (GUICalculatorControl*)w_;
	
	GUIManager_pushFocusedObject(w->header.gm, &w->inputBox->header);
}



static void handleCommand(GUIHeader* w_, GUI_Cmd* cmd) {
	GUICalculatorControl* w = (GUICalculatorControl*)w_;
	GUICalculatorControl_ProcessCommand(w, cmd);
}

void GUICalculatorControl_ProcessCommand(GUICalculatorControl* w, GUI_Cmd* cmd) {
	long amt;

	switch(cmd->cmd) {
		case FuzzyMatcherCmd_Exit:
			GUIManager_BubbleUserEvent(w->header.gm, &w->header, "closeMe");			
			break;
			
		case FuzzyMatcherCmd_CursorMove:
//			if(w->matchCnt == 0) break;
//			w->cursorIndex = (cmd->amt + w->cursorIndex + w->matchCnt) % w->matchCnt;
			break;
			
	}
	
}




static void userEvent(GUIHeader* w_, GUIEvent* gev) {
	GUICalculatorControl* w = (GUICalculatorControl*)w_;
	
	if((GUIEdit*)gev->originalTarget == w->inputBox) {
		if(0 == strcmp(gev->userType, "change")) {
//			w->cursorIndex = 0;
						
//			if(w->searchTerm) free(w->searchTerm);
//			w->searchTerm = strndup(gev->userData, gev->userSize);
			
//			GUICalculatorControl_Refresh(w);
		}
	}
}


GUICalculatorControl* GUICalculatorControl_New(GUIManager* gm) {

	static struct gui_vtbl static_vt = {
		.Render = (void*)render,
		.UpdatePos = (void*)updatePos,
		.HandleCommand = (void*)handleCommand,
	};
	
	static struct GUIEventHandler_vtbl event_vt = {
//		.KeyDown = keyDown,
		.GainedFocus = gainedFocus,
		//.Click = click,
		//.DoubleClick = click,
// 		.ScrollUp = scrollUp,
// 		.ScrollDown = scrollDown,
// 		.DragStart = dragStart,
// 		.DragStop = dragStop,
// 		.DragMove = dragMove,
// 		.ParentResize = parentResize,
		.User = userEvent,
	};
	
	
	GUICalculatorControl* w = pcalloc(w);
	
	gui_headerInit(&w->header, gm, &static_vt, &event_vt);
	w->header.cursor = GUIMOUSECURSOR_ARROW;
	w->header.flags = GUI_MAXIMIZE_X | GUI_MAXIMIZE_Y;
	w->header.cmdElementType = CUSTOM_ELEM_TYPE_Calc;
	
	
	w->inputBox = GUIEdit_New(gm, "");
	
	w->inputBox->header.flags |= GUI_MAXIMIZE_X;
	w->inputBox->header.gravity = GUI_GRAV_BOTTOM_LEFT;
	w->inputBox->header.topleft.y = 0;
	w->inputBox->header.topleft.x = 0;
	w->inputBox->header.size.y = 25;
	
	
	
	GUI_RegisterObject(w, w->inputBox);
	
	return w;
}

