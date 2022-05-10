#ifndef __gpuedit_fuzzyMatchControl_h__
#define __gpuedit_fuzzyMatchControl_h__


#include "commands.h"
#include "ui/gui.h"
#include "fuzzyMatch.h"
#include "msg.h"


typedef struct GUIFuzzyMatchControl {
	
	GUIString searchTerm;
//	char* searchTerm;

	fcandidate* matches;
	size_t matchCnt;
	int cursorIndex;
	fcandidate* candidates;
	char** contents;
	char*** stringBuffers;
	
	float lineHeight;
	float leftMargin;
	
	MessagePipe* upstream;
	GlobalSettings* gs;
//	GUI_Cmd* commands;
	
} GUIFuzzyMatchControl;


GUIFuzzyMatchControl* GUIFuzzyMatchControl_New(GUIManager* gm, MessagePipe* mp, char* path, char* searchTerm);
void GUIFuzzyMatchControl_Refresh(GUIFuzzyMatchControl* w);
void GUIFuzzyMatchControl_Render(GUIFuzzyMatchControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);

void GUIFuzzyMatchControl_ProcessCommand(GUIFuzzyMatchControl* w, GUI_Cmd* cmd);



#endif // __gpuedit_fuzzyMatchControl_h__
