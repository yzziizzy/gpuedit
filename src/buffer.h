#ifndef __gpuedit_buffer_h__
#define __gpuedit_buffer_h__



#include "gui.h"
#include "font.h"

typedef struct BufferLine {
	
	int lineNum;
	
	size_t allocSz;
	size_t length; // of the text itself
	char* buf;
	
	char* style;
	
	struct BufferLine* prev, *next;
	
} BufferLine;


typedef struct Buffer {
	GUIHeader header;
	
	BufferLine* first, *last, *current; 
	
	int numLines;
	int curCol;
	
	// TODO: move elsewhere
	GUIFont* font;
	
} Buffer;

typedef struct TextDrawParams {
	GUIFont* font;
	float fontSize;
	float lineHeight;
	float charWidth;
	int tabWidth;
	
	// colors
} TextDrawParams;


typedef struct ThemeDrawParams {
	struct Color4 bgColor; 
	struct Color4 textColor; 
	struct Color4 cursorColor; 
	struct Color4 hl_bgColor; 
	struct Color4 hl_textColor; 
} ThemeDrawParams;


typedef struct BufferDrawParams {
	char showLineNums;
	float lineNumWidth;
	
	TextDrawParams* tdp;
	ThemeDrawParams* theme;
} BufferDrawParams;


enum BufferCmdType {
	BufferCmd_MoveCursorV,
	BufferCmd_MoveCursorH,
	BufferCmd_InsertChar,
	BufferCmd_SplitLine,
};

typedef struct BufferCmd {
	enum BufferCmdType type;
	int amt;
} BufferCmd;



struct GUIManager;
typedef struct GUIManager GUIManager;


void Buffer_processCommand(Buffer* b, BufferCmd* cmd);



BufferLine* Buffer_AddLineBelow(Buffer* b);
BufferLine* Buffer_AdvanceLines(Buffer* b, int n);
void Buffer_Draw(Buffer* b, GUIManager* gm, int lineFrom, int lineTo, int colFrom, int colTo);
static void drawTextLine(GUIManager* gm, TextDrawParams* tdp, char* txt, int charCount, Vector2 tl);


void BufferLine_ensureAlloc(BufferLine* l, size_t len);
void Buffer_insertText(Buffer* b, char* text, size_t len);
void BufferLine_SetText(BufferLine* l, char* text, size_t len);
BufferLine* Buffer_AppendLine(Buffer* b, char* text, size_t len);
void test(Buffer* b);

Buffer* Buffer_New(GUIManager* gm);

void Buffer_loadRawText(Buffer* b, char* source, size_t len);




#endif // __gpuedit_buffer_h__
