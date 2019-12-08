#ifndef __gpuedit_buffer_h__
#define __gpuedit_buffer_h__



#include "gui.h"
#include "font.h"
#include "highlight.h"

struct hlinfo;

typedef struct BufferLine {
	
	ptrdiff_t lineNum;
	
	size_t allocSz;
	size_t length; // of the text itself
	char* buf;
	
	VEC(TextStyleAtom) style;
	
	struct BufferLine* prev, *next;
	
} BufferLine;

typedef struct BufferSelection {
	BufferLine* startLine, *endLine;
	size_t startCol, endCol;
	
	// type?
} BufferSelection;

typedef struct Buffer {
	
	BufferLine* first, *last, *current; 
	
	int numLines;
	int curCol;
	
	char* filePath;
	
	BufferSelection* sel;
	
	struct hlinfo* hl;
	
} Buffer;




typedef struct TextDrawParams {
	GUIFont* font;
	float fontSize;
	float lineHeight;
	float charWidth;
	int tabWidth;
	
	// colors
} TextDrawParams;


// TODO: switch to HDR colors
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





// HACK
void hlfn(hlinfo* hl);


typedef struct GUIBufferEditor {
	GUIHeader header;
	
	Buffer* buffer;
	BufferDrawParams* bdp;
	
	ptrdiff_t scrollLines; 
	ptrdiff_t scrollCols; // NYI, waiting on next line draw fn iteration
	
	// read only
	int linesOnScreen; // number of *full* lines that fit on screen
	// TODO: padding lines on vscroll
	
	
	// TODO: move elsewhere
	GUIFont* font;
	
	
} GUIBufferEditor;




enum BufferCmdType {
	BufferCmd_MoveCursorV,
	BufferCmd_MoveCursorH,
	BufferCmd_InsertChar,
	BufferCmd_SplitLine,
	BufferCmd_Backspace,
	BufferCmd_Delete,
};

typedef struct BufferCmd {
	enum BufferCmdType type;
	int amt;
} BufferCmd;



struct GUIManager;
typedef struct GUIManager GUIManager;




BufferLine* BufferLine_New();
void BufferLine_Delete(BufferLine* l);
BufferLine* BufferLine_FromStr(char* text, size_t len);
BufferLine* BufferLine_Copy(BufferLine* orig);
void BufferLine_EnsureAlloc(BufferLine* l, size_t len);
void BufferLine_InsertChar(BufferLine* l, char c, size_t col);
void BufferLine_DeleteChar(BufferLine* l, size_t col);
void BufferLine_SetText(BufferLine* l, char* text, size_t len);
void BufferLine_InsertText(BufferLine* l, char* text, size_t len, size_t col);
void BufferLine_AppendText(BufferLine* l, char* text, size_t len);


void Buffer_RenumberLines(BufferLine* start, size_t num);
void Buffer_ProcessCommand(Buffer* b, BufferCmd* cmd);

// these functions operate independently of the cursor
BufferLine* Buffer_InsertLineBefore(Buffer* b, BufferLine* after);
BufferLine* Buffer_InsertLineAfter(Buffer* b, BufferLine* before);
void Buffer_DeleteLine(Buffer* b, BufferLine* l);
void Buffer_BackspaceAt(Buffer* b, BufferLine* l, size_t col);
void Buffer_DeleteAt(Buffer* b, BufferLine* l, size_t col);


// these functions operate on absolute positions
void Buffer_AppendRawText(Buffer* b, char* source, size_t len);
BufferLine* Buffer_AppendLine(Buffer* b, char* text, size_t len);
BufferLine* Buffer_PrependLine(Buffer* b, char* text, size_t len);
void Buffer_InsertBufferAt(Buffer* target, Buffer* graft, BufferLine* tline, size_t tcol);

// HACK: temporary junk
void Buffer_RefreshHighlight(Buffer* b);



Buffer* Buffer_New();
Buffer* Buffer_Copy(Buffer* src);
Buffer* Buffer_FromSelection(Buffer* src, BufferSelection* sel);
void Buffer_ToRawText(Buffer* b, char** out, size_t* len);
int Buffer_SaveToFile(Buffer* b, char* path);
int Buffer_LoadFromFile(Buffer* b, char* path);



// These functions operate on and with the cursor
BufferLine* Buffer_AdvanceLines(Buffer* b, int n);
void Buffer_InsertLinebreak(Buffer* b);







// GUIBufferEditor



void GUIBufferEditor_Draw(GUIBufferEditor* gbe, GUIManager* gm, int lineFrom, int lineTo, int colFrom, int colTo);
static void drawTextLine(GUIManager* gm, TextDrawParams* tdp, ThemeDrawParams* theme, char* txt, int charCount, Vector2 tl);
GUIBufferEditor* GUIBufferEditor_New(GUIManager* gm);

void GUIBufferEditor_scrollToCursor(GUIBufferEditor* gbe);;


void test(Buffer* b);






#endif // __gpuedit_buffer_h__
