#ifndef __gpuedit_terminal_h__
#define __gpuedit_terminal_h__


#include "app.h"


typedef struct GUITerminalLine {
	char* text;
	int textAlloc;
	int textLen;
} GUITerminalLine;


typedef struct TermStyle {
	Color4 fg, bg;
	char bold;
	char underline;
} TermStyle;


typedef struct GUITerminal {
	GUIHeader header;
	
	int dirty;
	
	int nrows, ncols;
	GUIFont* font;
	
	char* inputBuffer;
	int ibAlloc;
	int ibLen;
	
	GUIUnifiedVertex* vertexCache;
	int vcAlloc;
	int vcLen;
	
	VEC(GUITerminalLine*) lines;
	
	struct child_pty_info* cpi;
	
	GUI_Cmd* commands;
	GlobalSettings* gs;
} GUITerminal;


GUITerminal* GUITerminal_New(GUIManager* gm);


void GUITerminal_ProcessCommand(GUITerminal* w, GUI_Cmd* cmd);


#endif // __gpuedit_terminal_h__
