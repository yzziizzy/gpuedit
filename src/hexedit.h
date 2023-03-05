#ifndef __gpuedit__hexedit_h__
#define __gpuedit__hexedit_h__



#include "ui/gui.h"
#include "buffer.h"




typedef struct HexRange {
	ssize_t pos, len;
} HexRange;


typedef struct Hexedit {
	char* filePath;
	int fd;
	
	uint8_t* data;
	ssize_t len;
	
	
	ssize_t bytesPerLine;
	ssize_t linesOnScreen;
	ssize_t scrollPos;
	
	HexRange cursor;
	
	

	GUI_CmdModeState inputState;
	
	Settings* s;
	GeneralSettings* gs;
	BufferSettings* bs;
	
	// gm probably shouldn't be cached
	GUIManager* gm;
	GUIFont* font;

} Hexedit;


void Hexedit_Render(Hexedit* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);
Hexedit* Hexedit_New(GUIManager* gm, Settings* s, char* path);
void Hexedit_LoadFile(Hexedit* w, char* path);

int Hexedit_ProcessCommand(Hexedit* w, GUI_Cmd* cmd);


#endif __gpuedit__hexedit_h__
