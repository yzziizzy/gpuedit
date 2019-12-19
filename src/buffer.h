#ifndef __gpuedit_buffer_h__
#define __gpuedit_buffer_h__



#include "gui.h"
#include "font.h"
#include "highlight.h"

#define BL_BOOKMARK_FLAG  (1<<0)

struct hlinfo;

typedef struct BufferLine {
	
	ptrdiff_t lineNum;
	
	size_t allocSz;
	size_t length; // of the text itself
	char* buf;
	
	unsigned int flags;
	
	VEC(TextStyleAtom) style;
	
	struct BufferLine* prev, *next;
	
} BufferLine;


typedef struct BufferSelection {
	BufferLine* startLine, *endLine;
	size_t startCol, endCol;
	
	struct BufferSelection* next, *prev;
	// type?
} BufferSelection;


typedef struct EditorParams {
// 	char** indentIncreaseTerminals; // {, do, (, [, etc.
	char* lineCommentPrefix; // "// "
	char* selectionCommentPrefix; // "/*"
	char* selectionCommentPostfix; // "*/"
	
} EditorParams;


typedef struct Buffer {
	
	BufferLine* first, *last, *current; 
	
	int numLines;
	int curCol;
	
	char* filePath;
	
	struct {
		BufferSelection* first, *last;
	} selectionRing;
	
	BufferSelection* sel; // dynamic selection
	
	struct hlinfo* hl;
	EditorParams* ep;
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
void hlfn(Highlighter* h, hlinfo* hl);




typedef struct GUIBufferEditor {
	GUIHeader header;
	
	Buffer* buffer;
	BufferDrawParams* bdp;
	Highlighter* h;
	
	
	ptrdiff_t scrollLines; // current scroll position, 0-based
	ptrdiff_t scrollCols; // NYI, waiting on next line draw fn iteration
	
	// read only
	int linesOnScreen; // number of *full* lines that fit on screen
	// TODO: padding lines on vscroll
	
	
	int linesPerScrollWheel;
	
	// stating point of a mouse-drag selection
	BufferLine* selectPivotLine; // BUG: dead pointers on line deletion?
	size_t selectPivotCol;
	
	
	char lineNumTypingMode; // flag for GoToLine being active
	GUIEdit* lineNumEntryBox;
	
	// TODO: move elsewhere
	GUIFont* font;
	
	
} GUIBufferEditor;




enum BufferCmdType {
	BufferCmd_NULL = 0,
	BufferCmd_MoveCursorV,
	BufferCmd_MoveCursorH,
	BufferCmd_InsertChar,
	BufferCmd_SplitLine,
	BufferCmd_Backspace,
	BufferCmd_Delete,
	BufferCmd_DeleteCurLine,
	BufferCmd_MovePage,
	BufferCmd_Home,
	BufferCmd_End,
	BufferCmd_DuplicateLine,
	BufferCmd_SelectNone,
	BufferCmd_SelectAll,
	BufferCmd_SelectLine,
	BufferCmd_SelectToEOL, // end of line
	BufferCmd_SelectFromSOL, // start of line
	BufferCmd_GoToLine,
	BufferCmd_RehilightWholeBuffer,
	BufferCmd_Cut,
	BufferCmd_Copy,
	BufferCmd_Paste,
	BufferCmd_SetBookmark,
	BufferCmd_RemoveBookmark,
	BufferCmd_ToggleBookmark,
	BufferCmd_GoToNextBookmark,
	BufferCmd_GoToPrevBookmark,
	BufferCmd_GoToFirstBookmark,
	BufferCmd_GoToLastBookmark,

	
	// NYI
	BufferCmd_Save,
	BufferCmd_Indent,
	BufferCmd_MatchPrevIndent,
	BufferCmd_TruncateLine,
	BufferCmd_TruncateLineExceptLeadingWS, // whitespace
	BufferCmd_TruncateLineAfterCursor,
	BufferCmd_TruncateBeforeAfterCursor, // not the meaning of truncate, but it matches the prior
	BufferCmd_CommentLine,
	BufferCmd_CommentSelection,
	BufferCmd_CommentSmart,
	BufferCmd_GrowSelH,
	BufferCmd_GrowSelV,
	BufferCmd_MoveCursorToEOL,
	BufferCmd_MoveCursorToSOL,
	BufferCmd_MoveCursorToSOLT, // start of line text, ignoring leading whitespace
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
void BufferLine_TruncateAfter(BufferLine* l, size_t col);
void BufferLine_SetText(BufferLine* l, char* text, size_t len);
void BufferLine_InsertText(BufferLine* l, char* text, size_t len, size_t col);
void BufferLine_AppendText(BufferLine* l, char* text, size_t len);
void BufferLine_AppendLine(BufferLine* l, BufferLine* src);


void Buffer_RenumberLines(BufferLine* start, size_t num);
void Buffer_ProcessCommand(Buffer* b, BufferCmd* cmd);

// these functions operate independently of the cursor
BufferLine* Buffer_InsertLineBefore(Buffer* b, BufferLine* after);
void Buffer_InsertLineAfter(Buffer* b, BufferLine* before, BufferLine* after);
BufferLine* Buffer_InsertEmptyLineAfter(Buffer* b, BufferLine* before);
void Buffer_DeleteLine(Buffer* b, BufferLine* l);
void Buffer_BackspaceAt(Buffer* b, BufferLine* l, size_t col);
void Buffer_DeleteAt(Buffer* b, BufferLine* l, size_t col);
void Buffer_SetCurrentSelection(Buffer* b, BufferLine* startL, size_t startC, BufferLine* endL, size_t endC);
void Buffer_ClearCurrentSelection(Buffer* b);
void Buffer_DuplicateLines(Buffer* b, BufferLine* src, int amt);
void Buffer_ClearAllSelections(Buffer* b);
void Buffer_DeleteSelectionContents(Buffer* b, BufferSelection* sel);


// these functions operate on absolute positions
void Buffer_AppendRawText(Buffer* b, char* source, size_t len);
BufferLine* Buffer_AppendLine(Buffer* b, char* text, size_t len);
BufferLine* Buffer_PrependLine(Buffer* b, char* text, size_t len);
void Buffer_InsertBufferAt(Buffer* target, Buffer* graft, BufferLine* tline, size_t tcol);
BufferLine* Buffer_GetLine(Buffer* b, size_t line);
void Buffer_CommentLine(Buffer* b, BufferLine* bl);
void Buffer_CommentSelection(Buffer* b, BufferSelection* sel);

void Buffer_SetBookmarkAt(Buffer* b, BufferLine* bl);
void Buffer_RemoveBookmarkAt(Buffer* b, BufferLine* bl);
void Buffer_ToggleBookmarkAt(Buffer* b, BufferLine* bl);


// HACK: temporary junk
void GUIBufferEditor_RefreshHighlight(GUIBufferEditor* gbe);



Buffer* Buffer_New();
Buffer* Buffer_Copy(Buffer* src);
Buffer* Buffer_FromSelection(Buffer* src, BufferSelection* sel);
void Buffer_ToRawText(Buffer* b, char** out, size_t* len);
int Buffer_SaveToFile(Buffer* b, char* path);
int Buffer_LoadFromFile(Buffer* b, char* path);



// These functions operate on and with the cursor
BufferLine* Buffer_AdvanceLines(Buffer* b, int n);
void Buffer_InsertLinebreak(Buffer* b);
void Buffer_MoveCursorV(Buffer* b, ptrdiff_t lines);
void Buffer_MoveCursorH(Buffer* b, ptrdiff_t cols);
void Buffer_MoveCursor(Buffer* b, ptrdiff_t lines, ptrdiff_t cols);
void Buffer_NextBookmark(Buffer* b);
void Buffer_PrevBookmark(Buffer* b);
void Buffer_FirstBookmark(Buffer* b);
void Buffer_LastBookmark(Buffer* b);



void Buffer_DebugPrint(Buffer* b);




// GUIBufferEditor



void GUIBufferEditor_Draw(GUIBufferEditor* gbe, GUIManager* gm, int lineFrom, int lineTo, int colFrom, int colTo);
static void drawTextLine(GUIManager* gm, TextDrawParams* tdp, struct Color4* textColor, char* txt, int charCount, Vector2 tl);
GUIBufferEditor* GUIBufferEditor_New(GUIManager* gm);

void GUIBufferEditor_scrollToCursor(GUIBufferEditor* gbe);;

void GUIBufferEditor_ProcessCommand(GUIBufferEditor* w, BufferCmd* cmd);






#endif // __gpuedit_buffer_h__
