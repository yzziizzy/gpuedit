#ifndef __gpuedit_grepOpenControl_h__
#define __gpuedit_grepOpenControl_h__


#include "ui/gui.h"
#include "msg.h"
#include "buffer.h" // for lineNum settings

typedef struct {
	char* basepath;
	char* filepath;
	intptr_t line_num;
	char* line;
	char* render_line;
} gocandidate;


typedef struct GUIGrepOpenControl {
	// GUIHeader header;
	
	// GUIEdit* searchBox;
	GUIString searchTerm;
	// char* searchTerm;
	gocandidate* matches;
	size_t matchCnt;
	int cursorIndex;
	char** contents;
	char*** stringBuffers;
	
	float lineHeight;
	float leftMargin;
	
	MessagePipe* upstream;
	BufferSettings* bs;
	GeneralSettings* gs;
	// GUI_Cmd* commands;
	
} GUIGrepOpenControl;


GUIGrepOpenControl* GUIGrepOpenControl_New(GUIManager* gm, Settings* s, MessagePipe* mp, char* searchTerm);
void GUIGrepOpenControl_Refresh(GUIGrepOpenControl* w);
void GUIGrepOpenControl_Render(GUIGrepOpenControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);

void GUIGrepOpenControl_ProcessCommand(GUIGrepOpenControl* w, GUI_Cmd* cmd);



#endif // __gpuedit_grepOpenControl_h__
