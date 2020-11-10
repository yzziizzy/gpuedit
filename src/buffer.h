#ifndef __gpuedit_buffer_h__
#define __gpuedit_buffer_h__

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>


#include "gui.h"
#include "font.h"
#include "highlight.h"
#include "commands.h"

#include "statusBar.h"



#define BL_BOOKMARK_FLAG  (1<<0)

struct hlinfo;

typedef struct BufferLine {
	
	ptrdiff_t lineNum;
	
	size_t allocSz;
	size_t length; // of the text itself
	char* buf;
	
	unsigned int flags;
	
	VEC(TextStyleAtom) style;
	uint8_t* flagBuf; 
	unsigned char indentTabs;
	unsigned char indentSpaces;
	
	struct BufferLine* prev, *next;
	
} BufferLine;


typedef struct BufferRange {
	BufferLine* startLine, *endLine;
	intptr_t startCol, endCol;
	
	intptr_t charLength; // not implemented atm
	int reverse : 1; // DEAD CODE? // active end at: 0 = end, 1 = start
	
	
	// type?
} BufferRange;


typedef struct EditorParams {
// 	char** indentIncreaseTerminals; // {, do, (, [, etc.
	char* lineCommentPrefix; // "// "
	char* selectionCommentPrefix; // "/*"
	char* selectionCommentPostfix; // "*/"
	int tabWidth;
	
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
// 	UndoAction_UnmodifiedFlag,
	UndoAction_SequenceBreak,
	
};


typedef struct BufferUndo {
	enum UndoActions action;
	intptr_t lineNum;
	intptr_t colNum;
// 	intptr_t cursorL;
// 	intptr_t cursorC;
	union {
		int character; // for single-char operations
		
		struct { // for text
			char* text; 
// 			intptr_t length;
		};
		
		struct { // for selection changes
			intptr_t endLine;
			intptr_t endCol;
		};
	};
	intptr_t length;
} BufferUndo;



enum BufferChangeAction {
	BCA_NULL,
	BCA_DeleteChars,
	BCA_DeleteLines,
};

typedef struct BufferChangeNotification {
	Buffer* b;
	BufferRange sel;
	
	int action;
} BufferChangeNotification;

typedef void (*bufferChangeNotifyFn)(BufferChangeNotification* note, void* data);



typedef struct Buffer {
	
	BufferLine* first, *last; 
	
	intptr_t numLines;
	
	char* filePath;
	
	// TODO: also goes to GUIBufferEditControl
	struct hlinfo* hl;
	EditorParams* ep;
	
	
	int undoOldest; // the oldest undo item in the buffer; the end of undo
	int undoCurrent; // the current state of the Buffer; goes backwards with undo, forwards with redo
	int undoNext; // the index after the newest item; the end of redo
	int undoMax; // the size of the ring buffer
	int undoFill; // the number of undo slots used in the ring buffer
	BufferUndo* undoRing;
// 	VEC(BufferUndo) undoStack;
	int undoSaveIndex; // index of the undo position matching the file on disk
// 	char isModified;
	
	char* sourceFile; // should be in GBEditor, but needed here for undo compatibility 
	
	char useDict;
	char* dictCharSet;
	HT(int) dict; // value is reference count
	
	int refs;
	
	// called *before* an intended change, when all pointers are still valid
	VEC(struct {
		bufferChangeNotifyFn fn;
		void* data;
	}) changeListeners;
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
	struct Color4 lineNumColor; 
	struct Color4 lineNumBookmarkColor; 
	struct Color4 lineNumBgColor; 
	struct Color4 hl_bgColor; 
	struct Color4 hl_textColor; 
} ThemeDrawParams;



typedef struct BufferDrawParams {
	char showLineNums;
	float lineNumExtraWidth;
	
	TextDrawParams* tdp;
	ThemeDrawParams* theme;
} BufferDrawParams;






typedef struct GBEFindRange {
	BufferLine* bl;
	intptr_t startChar, endChar;
} GBEFindRange;



// drawing and mouse controls
typedef struct GUIBufferEditControl {
	GUIHeader header;

	Buffer* buffer;
	BufferDrawParams* bdp;
	Highlighter* h;
	
	
	float cursorBlinkTimer;
	float cursorBlinkOnTime;
	float cursorBlinkOffTime;
	char cursorBlinkPaused;
	
	char outlineCurLine;

	intptr_t scrollLines; // current scroll position, 0-based
	intptr_t scrollCols; // NYI, waiting on next line draw fn iteration
	
	float textAreaOffsetX; // accounts for line numbers and such

	// TODO: move elsewhere
	GUIFont* font;

	
	BufferLine* current; // line of the cursor
	intptr_t curCol; // characters into the line
	intptr_t curColDisp; // the visible display column, including tabstops, etc.
	intptr_t curColWanted; // the visible display column to use, if it existed.

	BufferRange* sel; // dynamic selection
	
	// starting point of a mouse-drag selection
	BufferLine* selectPivotLine; // BUG: dead pointers on line deletion?
	intptr_t selectPivotCol;

	float scrollCoastTimer;
	float scrollCoastStrength;
	float scrollCoastMax;
	char isDragSelecting;
	char isDragScrollCoasting;
	char scrollCoastDir;
	
	char showAutocomplete;
	int maxAutocompleteLines; // lines to show in the popup
	intptr_t autocompleteProvokeCol;
	VEC(char*) autocompleteOptions;
	
	// read only
	int linesOnScreen; // number of *full* lines that fit on screen
	int colsOnScreen; // number of *full* columns that fit on screen
	// TODO: padding lines on vscroll

	int linesPerScrollWheel;
	
	
	GUIWindow* scrollbar;
	float sbMinHeight;

// 	Cmd* commands;
	
} GUIBufferEditControl;


size_t GBEC_lineFromPos(GUIBufferEditControl* w, Vector2 pos);
size_t GBEC_getColForPos(GUIBufferEditControl* w, BufferLine* bl, float x);

// all sorts of fancy stuff, and keyboard controls
typedef struct GUIBufferEditor {
	GUIHeader header;
	
	
	GUIBufferEditControl* ec;
	
	Buffer* buffer;
	BufferDrawParams* bdp;
	Highlighter* h;
	
	char* sourceFile; // issues with undo-save
	
	unsigned int inputMode;
	
	char findMode; 
	char replaceMode; 
	char lineNumTypingMode; // flag for GoToLine being active
	char loadTypingMode; 
	
	GUIEdit* lineNumEntryBox;
	GUIEdit* findBox;
	GUIEdit* replaceBox;
	GUIEdit* loadBox;
	
	pcre2_code* findRE;
	pcre2_match_data* findMatch;
	BufferLine* findLine;
	intptr_t findCharS;
	intptr_t findCharE;
	intptr_t findLen;
	char* findREError;
	int findREErrorChar;
	BufferLine* nextFindLine;
	intptr_t nextFindChar;
	
	VEC(GBEFindRange) findRanges;
	
	
	char trayOpen;
	GUIWindow* trayRoot;
	
	// status bar
	char showStatusBar;
	float statusBarHeight;
	GUIStatusBar* statusBar;
	
	GUISimpleWindow* menu;
	
	Cmd* commands;

} GUIBufferEditor;





typedef struct BufferCmd {
	enum CmdType type;
	union {
		long amt;
		char* str;
	};
} BufferCmd;



struct GUIManager;
typedef struct GUIManager GUIManager;



// these are raw functions and should not be used directly by Buffer_ operations
// they are internal functions used by Buffer_raw_ operations
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
int BufferLine_IsInRange(BufferLine* bl, BufferRange* sel);




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
void Buffer_UndoSetSelection(Buffer* b, intptr_t startL, intptr_t startC, intptr_t endL, intptr_t endC);
void Buffer_UndoSequenceBreak(Buffer* b, int saved, intptr_t cursorLine, intptr_t cursorCol);
void Buffer_UndoReplayToSeqBreak(Buffer* b);
int Buffer_UndoReplayTop(Buffer* b);
void Buffer_UndoTruncateStack(Buffer* b);

// clean up all memory related to the undo system
void Buffer_FreeAllUndo(Buffer* b);

void Buffer_RedoReplayToSeqBreak(Buffer* b);
int Buffer_RedoReplay(Buffer* b, BufferUndo* u);


// -----
// functions below here will add to the undo stack
//    and send buffer notifications
// -----

void Buffer_ProcessCommand(Buffer* b, BufferCmd* cmd, int* needRehighlight);
void GUIBufferEditControl_ProcessCommand(GUIBufferEditControl* w, BufferCmd* cmd, int* needRehighlight);




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

void GBEC_SetCurrentSelection(GUIBufferEditControl* w, BufferLine* startL, intptr_t startC, BufferLine* endL, intptr_t endC);
void GBEC_ClearCurrentSelection(GUIBufferEditControl* w);
void GBEC_ClearAllSelections(GUIBufferEditControl* w);
void Buffer_DeleteSelectionContents(Buffer* b, BufferRange* sel);
void GBEC_SelectSequenceUnder(GUIBufferEditControl* w, BufferLine* l, intptr_t col, char* charSet);
void Buffer_GetSequenceUnder(Buffer* b, BufferLine* l, intptr_t col, char* charSet, BufferRange* out);

char* Buffer_StringFromSelection(Buffer* b, BufferRange* sel, size_t* outLen);

void GBEC_GrowSelectionH(GUIBufferEditControl* w, intptr_t cols);
void GBEC_GrowSelectionV(GUIBufferEditControl* w, intptr_t cols);

// these functions operate on absolute positions
void Buffer_AppendRawText(Buffer* b, char* source, intptr_t len);
BufferLine* Buffer_AppendLine(Buffer* b, char* text, intptr_t len);
BufferLine* Buffer_PrependLine(Buffer* b, char* text, intptr_t len);
void Buffer_DuplicateSelection(Buffer* b, BufferRange* sel, int amt);
void Buffer_InsertBufferAt(Buffer* target, Buffer* graft, BufferLine* tline, intptr_t tcol, BufferRange* outRange);
void Buffer_CommentLine(Buffer* b, BufferLine* bl);
void Buffer_CommentSelection(Buffer* b, BufferRange* sel);

void Buffer_SetBookmarkAt(Buffer* b, BufferLine* bl);
void Buffer_RemoveBookmarkAt(Buffer* b, BufferLine* bl);
void Buffer_ToggleBookmarkAt(Buffer* b, BufferLine* bl);

void Buffer_RelPosH(Buffer* b, BufferLine* startL, intptr_t startC, intptr_t cols, BufferLine** outL, intptr_t* outC);
void Buffer_RelPosV(Buffer* b, BufferLine* startL, intptr_t startC, intptr_t lines, BufferLine** outL, intptr_t* outC);

void BufferRange_Normalize(BufferRange** pbr);
intptr_t BufferLine_GetIndentCol(BufferLine* l);


// HACK: temporary junk
void GUIBufferEditControl_RefreshHighlight(GUIBufferEditControl* w);



Buffer* Buffer_New();
void Buffer_AddRef(Buffer* b);
void Buffer_Delete(Buffer* b);
Buffer* Buffer_Copy(Buffer* src);
Buffer* Buffer_FromSelection(Buffer* src, BufferRange* sel);
void Buffer_ToRawText(Buffer* b, char** out, size_t* len);
int Buffer_SaveToFile(Buffer* b, char* path);
int Buffer_LoadFromFile(Buffer* b, char* path);
void Buffer_NotifyChanges(BufferChangeNotification* note);
void Buffer_NotifyLineDeletion(Buffer* b, BufferLine* sLine, BufferLine* eLine);



// These functions operate on and with the cursor
BufferLine* Buffer_AdvanceLines(Buffer* b, int n);
void GBEC_InsertLinebreak(GUIBufferEditControl* b);
void GBEC_MoveCursorV(GUIBufferEditControl* w, intptr_t lines);
void GBEC_MoveCursorH(GUIBufferEditControl* w, intptr_t cols);
void GBEC_MoveCursor(GUIBufferEditControl* w, intptr_t lines, intptr_t cols);
void GBEC_MoveCursorTo(GUIBufferEditControl* w, BufferLine* bl, intptr_t col); // absolute move
void GBEC_NextBookmark(GUIBufferEditControl* w);
void GBEC_PrevBookmark(GUIBufferEditControl* w);
void GBEC_FirstBookmark(GUIBufferEditControl* w);
void GBEC_LastBookmark(GUIBufferEditControl* w);
void Buffer_Indent(Buffer* b);
void Buffer_Unindent(Buffer* b);
intptr_t Buffer_IndentToPrevLine(Buffer* b, BufferLine* bl);
void Buffer_CollapseWhitespace(Buffer* b, BufferLine* l, intptr_t col);
void GBEC_MoveToPrevSequence(GUIBufferEditControl* w, BufferLine* l, intptr_t col, char* charSet);
void GBEC_MoveToNextSequence(GUIBufferEditControl* w, BufferLine* l, intptr_t col, char* charSet);
void GBEC_DeleteToPrevSequence(GUIBufferEditControl* w, BufferLine* l, intptr_t col, char* charSet);
void GBEC_DeleteToNextSequence(GUIBufferEditControl* w, BufferLine* l, intptr_t col, char* charSet);
int Buffer_FindSequenceEdgeForward(Buffer* b, BufferLine** linep, intptr_t* colp, char* charSet);
int Buffer_FindSequenceEdgeBackward(Buffer* b, BufferLine** linep, intptr_t* colp, char* charSet);

void Buffer_DebugPrint(Buffer* b);
void Buffer_DebugPrintUndoStack(Buffer* b);

int Buffer_AddDictWord(Buffer* b, char* word);
int Buffer_RemoveDictWord(Buffer* b, char* word);
void Buffer_RemoveLineFromDict(Buffer* b, BufferLine* l);
void Buffer_AddLineToDict(Buffer* b, BufferLine* l);

// temp
int GUIBufferEditor_FindWord(GUIBufferEditor* w, char* word);


// GUIBufferEditor


void GUIBufferEditControl_UpdateSettings(GUIBufferEditControl* w, GlobalSettings* s);
void GUIBufferEditor_UpdateSettings(GUIBufferEditor* w, GlobalSettings* s);

void GUIBufferEditControl_Draw(GUIBufferEditControl* gbe, GUIManager* gm, int lineFrom, int lineTo, int colFrom, int colTo);
static void drawTextLine(GUIManager* gm, TextDrawParams* tdp, struct Color4* textColor, char* txt, int charCount, Vector2 tl, float z, AABB2* clip);
GUIBufferEditor* GUIBufferEditor_New(GUIManager* gm);
GUIBufferEditControl* GUIBufferEditControl_New(GUIManager* gm);
void GUIBufferEditor_Destroy(GUIBufferEditor* w);

void GUIBufferEditor_SetBuffer(GUIBufferEditor* w, Buffer* b);
void GUIBufferEditControl_SetBuffer(GUIBufferEditControl* w, Buffer* b);


void GUIBufferEditor_scrollToCursor(GUIBufferEditor* gbe);;
void GUIBufferEditControl_scrollToCursor(GUIBufferEditControl* gbe);;

void GUIBufferEditor_ProcessCommand(GUIBufferEditor* w, BufferCmd* cmd, int* needRehighlight);

// set absolute scroll position
void GUIBufferEditControl_SetScroll(GUIBufferEditControl* w, intptr_t line, intptr_t col);

// move the view by this delta
void GUIBufferEditControl_ScrollDir(GUIBufferEditControl* w, intptr_t lines, intptr_t cols);

void GUIBufferEditControl_SetSelectionFromPivot(GUIBufferEditControl* gbe);
void GUIBufferEditControl_MoveCursorTo(GUIBufferEditControl* gbe, intptr_t line, intptr_t col);

void GUIBufferEditor_CloseTray(GUIBufferEditor* w);
void GUIBufferEditor_OpenTray(GUIBufferEditor* w, float height);
void GUIBufferEditor_ToggleTray(GUIBufferEditor* w, float height); 

int GUIBufferEditor_StartFind(GUIBufferEditor* w, char* pattern);
int GUIBufferEditor_NextFindMatch(GUIBufferEditor* w);
void GUIBufferEditor_StopFind(GUIBufferEditor* w);

intptr_t getDisplayColFromWanted(GUIBufferEditControl* w, BufferLine* bl, intptr_t wanted);
intptr_t getActualColFromWanted(GUIBufferEditControl* w, BufferLine* bl, intptr_t wanted);
intptr_t getDisplayColFromActual(GUIBufferEditControl* w, BufferLine* bl, intptr_t col);



#endif // __gpuedit_buffer_h__
