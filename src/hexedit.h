#ifndef __gpuedit__hexedit_h__
#define __gpuedit__hexedit_h__



#include "ui/gui.h"
#include "buffer.h"

// name sz sign i/f/c
#define HEXEDIT_TYPE_LIST(X) \
	X(u8, 1, 0, i) \
	X(s8, 1, 1, i) \
	X(u16, 2, 0, i) \
	X(s16, 2, 1, i) \
	X(u24, 3, 0, i) \
	X(s24, 3, 1, i) \
	X(u32, 4, 0, i) \
	X(s32, 4, 1, i) \
	X(u64, 8, 0, i) \
	X(s64, 8, 1, i) \
	X(f16, 2, 1, f) \
	X(f32, 4, 1, f) \
	X(f64, 8, 1, f) \


enum HexType {
	HEXEDIT_TYPE_None = 0,
#define X(a, ...) HEXEDIT_TYPE_##a,
	HEXEDIT_TYPE_LIST(X)
#undef X
	HEXEDIT_TYPE_MAX_VALUE
};

typedef struct HexRange {
	ssize_t pos, len;
	enum HexType type;	
	
	ssize_t numLines;
	
	struct HexRange* next, *prev;
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
	
	HexRange* ranges;
	

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

HexRange* Hexedit_FindRangeForLine(Hexedit* w, ssize_t line);
HexRange* Hexedit_FindRange(Hexedit* w, ssize_t pos);

int Hexedit_ProcessCommand(Hexedit* w, GUI_Cmd* cmd);


#endif __gpuedit__hexedit_h__
