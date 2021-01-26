#ifndef __gpuedit_grepOpenControl_h__
#define __gpuedit_grepOpenControl_h__

#include "ui/gui.h"
#include "commands.h"


typedef struct {
	char* basepath;
	char* filepath;
	intptr_t line_num;
	char* line;
	char* render_line;
} gocandidate;


typedef struct GUIGrepOpenControl {
	GUIHeader header;
	
	GUIEdit* searchBox;
	char* searchTerm;
	gocandidate* matches;
	size_t matchCnt;
	int cursorIndex;
	char** contents;
	char*** stringBuffers;
	
	float lineHeight;
	float leftMargin;
	
	GlobalSettings* gs;
	Cmd* commands;
	
} GUIGrepOpenControl;


GUIGrepOpenControl* GUIGrepOpenControl_New(GUIManager* gm, char* path);
void GUIGrepOpenControl_Refresh(GUIGrepOpenControl* w);

void GUIGrepOpenControl_ProcessCommand(GUIGrepOpenControl* w, Cmd* cmd);



#endif // __gpuedit_grepOpenControl_h__
