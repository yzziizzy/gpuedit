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
	char* projname;
	int excluded;
} gocandidate;


typedef struct GrepOpenControl {
	GUIString searchTerm;
	gocandidate* matches;
	size_t matchCnt;
	int cursorIndex;
	int scrollLine;
	char** contents;
	char*** stringBuffers;
	float proj_gutter;
	char** projnames;
	
	float lineHeight;
	float leftMargin;
	GUIFont* font;
	float fontsize;
	
	MessagePipe* upstream;
	BufferSettings* bs;
	GeneralSettings* gs;
	
	
} GrepOpenControl;




GrepOpenControl* GrepOpenControl_New(GUIManager* gm, Settings* s, MessagePipe* mp, char* searchTerm);
void GrepOpenControl_Refresh(GrepOpenControl* w);
void GrepOpenControl_Render(GrepOpenControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);

void GrepOpenControl_SaveSessionState(GrepOpenControl* w, json_value_t* out);

int GrepOpenControl_ProcessCommand(GrepOpenControl* w, GUI_Cmd* cmd);

void GrepOpenControl_Destroy(GrepOpenControl* w);



#endif // __gpuedit_grepOpenControl_h__
