#ifndef __gpuedit_fuzzyMatchControl_h__
#define __gpuedit_fuzzyMatchControl_h__

#include "ui/gui.h"
#include "fuzzyMatch.h"


typedef struct GUIFuzzyMatchControl {
	GUIHeader header;
	
	GUIEdit* searchBox;
	char* searchTerm;
	fcandidate* matches;
	size_t matchCnt;
	int cursorIndex;
	fcandidate* candidates;
	char** contents;
	char*** stringBuffers;
	
	float lineHeight;
	float leftMargin;
	
	GlobalSettings* gs;
	GUI_Cmd* commands;
	
} GUIFuzzyMatchControl;


GUIFuzzyMatchControl* GUIFuzzyMatchControl_New(GUIManager* gm, char* path, char* searchTerm);
void GUIFuzzyMatchControl_Refresh(GUIFuzzyMatchControl* w);

void GUIFuzzyMatchControl_ProcessCommand(GUIFuzzyMatchControl* w, GUI_Cmd* cmd);



#endif // __gpuedit_fuzzyMatchControl_h__
