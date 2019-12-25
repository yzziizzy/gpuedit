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
	intptr_t startCol, endCol;
	
	struct BufferSelection* next, *prev;
	// type?
} BufferSelection;


typedef struct EditorParams {
// 	char** indentIncreaseTerminals; // {, do, (, [, etc.
	char* lineCommentPrefix; // "// "
	char* selectionCommentPrefix; // "/*"
	char* selectionCommentPostfix; // "*/"
	
} EditorParams;




// http://texteditors.org/cgi-bin/wiki.pl?Implementing_Undo_For_Text_Editors

// these represent what was done and must be reversed when using the undo feature
enum UndoActions {
	UndoAction_InsertText = 0,
	UndoAction_DeleteText,
	UndoAction_InsertChar,
	UndoAction_DeleteChar,
	UndoAction_InsertLineAfter, // 0 inserts a line at the beginning
	UndoAction_DeleteLine,
	UndoAction_MoveCursorTo,
	UndoAction_SetSelection,
	UndoAction_UnmodifiedFlag,
	UndoAction_SequenceBreak,
	
};


typedef struct BufferUndo {
	enum UndoActions action;
	intptr_t lineNum;
	intptr_t colNum;
	intptr_t cursorL;
	intptr_t cursorC;
	union{
		int character; // for single-char operations
		
		struct { // for text
			char* text; 
			intptr_t length;
		};
		
		struct { // for selection changes
			intptr_t endLine;
			intptr_t endCol;
		};
	};
} BufferUndo;

typedef struct Buffer {
	
	BufferLine* first, *last, *current; 
	
	intptr_t numLines;
	intptr_t curCol;
	
	char* filePath;
	
	struct {
		BufferSelection* first, *last;
	} selectionRing;
	
	BufferSelection* sel; // dynamic selection
	
	struct hlinfo* hl;
	EditorParams* ep;
	
	VEC(BufferUndo) undoStack;
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
	
	
	intptr_t scrollLines; // current scroll position, 0-based
	intptr_t scrollCols; // NYI, waiting on next line draw fn iteration
	
	// read only
	int linesOnScreen; // number of *full* lines that fit on screen
	// TODO: padding lines on vscroll
	
	
	int linesPerScrollWheel;
	
	// stating point of a mouse-drag selection
	BufferLine* selectPivotLine; // BUG: dead pointers on line deletion?
	intptr_t selectPivotCol;
	
	
	char lineNumTypingMode; // flag for GoToLine being active
	GUIEdit* lineNumEntryBox;
	
	// TODO: move elsewhere
	GUIFont* font;
	
	
} GUIBufferEditor;




enum BufferCmdType {
	BufferCmd_NULL = 0,
	BufferCmd_Debug,
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
	BufferCmd_Undo,
	BufferCmd_GrowSelectionH,
	BufferCmd_GrowSelectionV,
	BufferCmd_Indent,
	BufferCmd_Unindent,
	
	
	
	// NYI
	BufferCmd_Redo,
	BufferCmd_Save,
	BufferCmd_MatchPrevIndent,
	BufferCmd_TruncateLine,
	BufferCmd_TruncateLineExceptLeadingWS, // whitespace
	BufferCmd_TruncateLineAfterCursor,
	BufferCmd_TruncateBeforeAfterCursor, // not the meaning of truncate, but it matches the prior
	BufferCmd_CommentLine,
	BufferCmd_CommentSelection,
	BufferCmd_CommentSmart,
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



// these are raw functions and should not be used directly by Buffer_ operations
BufferLine* BufferLine_New();
void BufferLine_Delete(BufferLine* l);
BufferLine* BufferLine_FromStr(char* text, intptr_t len);
BufferLine* BufferLine_Copy(BufferLine* orig);
void BufferLine_EnsureAlloc(BufferLine* l, intptr_t len);
void BufferLine_InsertChars(BufferLine* l, char* text, intptr_t col, intptr_t len);
void BufferLine_DeleteChars(BufferLine* l, intptr_t offset, intptr_t col);
void BufferLine_TruncateAfter(BufferLine* l, intptr_t col);
void BufferLine_SetText(BufferLine* l, char* text, intptr_t len);
void BufferLine_AppendText(BufferLine* l, char* text, intptr_t len);
void BufferLine_AppendLine(BufferLine* l, BufferLine* src);




// these functions will NOT interact with the undo stack
// they should never use functions below them
// functions below these should only use them and not functions above
BufferLine* Buffer_raw_GetLine(Buffer* b, intptr_t lineNum);
void Buffer_raw_RenumberLines(BufferLine* start, intptr_t num);

// inserts empty lines
BufferLine* Buffer_raw_InsertLineAfter(Buffer* b, BufferLine* before);
BufferLine* Buffer_raw_InsertLineBefore(Buffer* b, BufferLine* after);

void Buffer_raw_DeleteLine(Buffer* b, BufferLine* bl);

void Buffer_raw_InsertChars(Buffer* b, BufferLine* bl, char* txt, intptr_t offset, intptr_t len);
void Buffer_raw_DeleteChars(Buffer* b, BufferLine* bl, intptr_t offset, intptr_t len);



// undo stack processing
void Buffer_UndoInsertText(Buffer* b, intptr_t line, intptr_t col, char* txt, intptr_t len);
void Buffer_UndoDeleteText(Buffer* b, BufferLine* bl, intptr_t offset, intptr_t len);
void Buffer_UndoInsertLineAfter(Buffer* b, BufferLine* before); // safe to just pass in l->prev without checking
void Buffer_UndoDeleteLine(Buffer* b, BufferLine* bl); // saves the text too
void Buffer_UndoSequencePoint(Buffer* b);
void Buffer_UndoReplayTop(Buffer* b);

// functions below here will add to the undo stack

void Buffer_ProcessCommand(Buffer* b, BufferCmd* cmd, int* needRehighlight);




// these functions operate independently of the cursor
BufferLine* Buffer_InsertLineBefore(Buffer* b, BufferLine* after, char* text, intptr_t length);
BufferLine* Buffer_InsertLineAfter(Buffer* b, BufferLine* before, char* text, intptr_t length);
BufferLine* Buffer_InsertEmptyLineBefore(Buffer* b, BufferLine* after);
BufferLine* Buffer_InsertEmptyLineAfter(Buffer* b, BufferLine* before);
void Buffer_DeleteLine(Buffer* b, BufferLine* bl);
void Buffer_LineInsertChars(Buffer* b, BufferLine* bl, char* text, intptr_t offset, intptr_t length);
void Buffer_LineAppendText(Buffer* b, BufferLine* bl, char* text, intptr_t length);
void Buffer_LineAppendLine(Buffer* b, BufferLine* target, BufferLine* src);
void Buffer_LineDeleteChars(Buffer* b, BufferLine* bl, intptr_t col, intptr_t length);
void Buffer_LineTruncateAfter(Buffer* b, BufferLine* bl, intptr_t col);
void Buffer_BackspaceAt(Buffer* b, BufferLine* l, intptr_t col);
void Buffer_DeleteAt(Buffer* b, BufferLine* l, intptr_t col);
void Buffer_DuplicateLines(Buffer* b, BufferLine* src, int amt);
void Buffer_LineIndent(Buffer* b, BufferLine* bl);
void Buffer_LineUnindent(Buffer* b, BufferLine* bl);

void Buffer_SetCurrentSelection(Buffer* b, BufferLine* startL, intptr_t startC, BufferLine* endL, intptr_t endC);
void Buffer_ClearCurrentSelection(Buffer* b);
void Buffer_ClearAllSelections(Buffer* b);
void Buffer_DeleteSelectionContents(Buffer* b, BufferSelection* sel);

void Buffer_GrowSelectionH(Buffer* b, intptr_t cols);
void Buffer_GrowSelectionV(Buffer* b, intptr_t cols);

// these functions operate on absolute positions
void Buffer_AppendRawText(Buffer* b, char* source, intptr_t len);
BufferLine* Buffer_AppendLine(Buffer* b, char* text, intptr_t len);
BufferLine* Buffer_PrependLine(Buffer* b, char* text, intptr_t len);
void Buffer_InsertBufferAt(Buffer* target, Buffer* graft, BufferLine* tline, intptr_t tcol);
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
void Buffer_MoveCursorV(Buffer* b, intptr_t lines);
void Buffer_MoveCursorH(Buffer* b, intptr_t cols);
void Buffer_MoveCursor(Buffer* b, intptr_t lines, intptr_t cols);
void Buffer_NextBookmark(Buffer* b);
void Buffer_PrevBookmark(Buffer* b);
void Buffer_FirstBookmark(Buffer* b);
void Buffer_LastBookmark(Buffer* b);
void Buffer_Indent(Buffer* b);
void Buffer_Unindent(Buffer* b);


void Buffer_DebugPrint(Buffer* b);
void Buffer_DebugPrintUndoStack(Buffer* b);




// GUIBufferEditor



void GUIBufferEditor_Draw(GUIBufferEditor* gbe, GUIManager* gm, int lineFrom, int lineTo, int colFrom, int colTo);
static void drawTextLine(GUIManager* gm, TextDrawParams* tdp, struct Color4* textColor, char* txt, int charCount, Vector2 tl);
GUIBufferEditor* GUIBufferEditor_New(GUIManager* gm);

void GUIBufferEditor_scrollToCursor(GUIBufferEditor* gbe);;

void GUIBufferEditor_ProcessCommand(GUIBufferEditor* w, BufferCmd* cmd, int* needRehighlight);






#endif // __gpuedit_buffer_h__
