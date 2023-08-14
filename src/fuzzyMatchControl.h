#ifndef __gpuedit_fuzzyMatchControl_h__
#define __gpuedit_fuzzyMatchControl_h__


#include "commands.h"
#include "ui/gui.h"
#include "fuzzyMatch.h"
#include "msg.h"


typedef struct GUIFuzzyMatchControl {
	float lineHeight;
	float leftMargin;
	
	GUIString searchTerm; // cannot be first member of struct
//	char* searchTerm;

	fcandidate* matches;
	size_t matchCnt;
	int cursorIndex;
	fcandidate* candidates;
	char** contents;
	char*** stringBuffers;
	float proj_gutter;
	char** projnames;

	
	MessagePipe* upstream;
	GeneralSettings* gs;
	GUIFont* font;
	float fontsize;
//	GUI_Cmd* commands;
	
} GUIFuzzyMatchControl;


GUIFuzzyMatchControl* GUIFuzzyMatchControl_New(GUIManager* gm, Settings* s, MessagePipe* mp, char* path, char* searchTerm);
void GUIFuzzyMatchControl_Refresh(GUIFuzzyMatchControl* w);
void GUIFuzzyMatchControl_Render(GUIFuzzyMatchControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);

int GUIFuzzyMatchControl_ProcessCommand(GUIFuzzyMatchControl* w, GUI_Cmd* cmd);

void GUIFuzzyMatchControl_Destroy(GUIFuzzyMatchControl* w);


#endif // __gpuedit_fuzzyMatchControl_h__
