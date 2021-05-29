#ifndef __gpuedit_terminal_h__
#define __gpuedit_terminal_h__


#include "app.h"




typedef struct GUITerminalControl {
	GUIHeader header;
	
	int nrows, ncols;
	
	
	struct child_pty_info* cpi;
	
	
} GUITerminalControl;


GUITerminalControl* GUITerminalControl_New(GUIManager* gm);



#endif // __gpuedit_terminal_h__
