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


typedef struct GrepOpenControl {
	GUIString searchTerm;
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
	
} GrepOpenControl;


GrepOpenControl* GrepOpenControl_New(GUIManager* gm, Settings* s, MessagePipe* mp, char* searchTerm);
void GrepOpenControl_Refresh(GrepOpenControl* w);
void GrepOpenControl_Render(GrepOpenControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);

void GrepOpenControl_ProcessCommand(GrepOpenControl* w, GUI_Cmd* cmd);



#endif // __gpuedit_grepOpenControl_h__
