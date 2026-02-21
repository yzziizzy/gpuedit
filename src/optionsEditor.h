#ifndef __gpuedit__optionsEditor_h__
#define __gpuedit__optionsEditor_h__


#include "ui/gui.h"
#include "buffer.h"






typedef struct OptionsEditor {

	Settings* s;
	GeneralSettings* gs;
	BufferSettings* bs;
	
	GUI_CmdModeState inputState;
	
	long editingIndex; // which item we're currently editing. -1 for none
	long lastEditingIndex; // which item we were previously editing, for saving
	
	int editBool;
	bool* boolTarget;
	float editFloat;
	float* floatTarget;
	double editDouble;
	double* doubleTarget;
	long editLong;
	long* longTarget;	
	GUIString editString;
	char** stringTarget;
	
} OptionsEditor;


void OptionsEditor_Init(OptionsEditor* w, /*GUIManager* gm,*/  Settings* s, MessagePipe* tx);
void OptionsEditor_Destroy(OptionsEditor* oe);
void OptionsEditor_Render(OptionsEditor* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);

int OptionsEditor_ProcessCommand(OptionsEditor* w, GUI_Cmd* cmd);



#endif __gpuedit__optionsEditor_h__
