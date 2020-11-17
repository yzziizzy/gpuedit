#ifndef __gpuedit_fuzzyMatchControl_h__
#define __gpuedit_fuzzyMatchControl_h__

#include "gui.h"
#include "commands.h"


typedef struct GUIFuzzyMatchControl {
	GUIHeader header;
	
	GUIEdit* searchBox;
	char* searchTerm;
	char** files;
	size_t fileCnt;
	int cursorIndex;
	char* stringBuffer;
	
	float lineHeight;
	float leftMargin;
	
	
	Cmd* commands;
	
} GUIFuzzyMatchControl;


GUIFuzzyMatchControl* GUIFuzzyMatchControl_New(GUIManager* gm, char* path);
void GUIFuzzyMatchControl_Refresh(GUIFuzzyMatchControl* w);

void GUIFuzzyMatchControl_ProcessCommand(GUIFuzzyMatchControl* w, Cmd* cmd);



#endif // __gpuedit_fuzzyMatchControl_h__
