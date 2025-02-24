#ifndef __gpuedit_buffer_h__
#define __gpuedit_buffer_h__

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>


#include "commands.h"
#include "ui/gui.h"
#include "font.h"
#include "msg.h"
#include "highlight.h"

#include "statusBar.h"



#define BL_BOOKMARK_FLAG   (1<<0)
#define BL_BREAKPOINT_FLAG (1<<1)
#define BL_UTF8_FLAG       (1<<2)
#define BL_ANNOTATION_FLAG (1<<3)

struct hlinfo;
struct BufferSettings;
typedef struct BufferSettings BufferSettings;
struct ThemeSettings;
typedef struct ThemeSettings ThemeSettings;


typedef int32_t linenum_t; // should be signed
typedef int32_t colnum_t; // should be signed
typedef uint64_t lineid_t;


typedef struct BufferLine {
	lineid_t id;
	
	linenum_t lineNum;
	
	colnum_t allocSz;
	colnum_t length; // of the text itself
	char* buf;
	
	unsigned int flags;
	
	VEC(TextStyleAtom) style;
	uint8_t* flagBuf; 
	unsigned char indentTabs;
	unsigned char indentSpaces;
	
	struct BufferLine* prev, *next;
	
} BufferLine;


#define CURSOR_LINE(r) ((r)->line[(r)->cursor])
#define CURSOR_COL(r) ((r)->col[(r)->cursor])
#define PIVOT_LINE(r) ((r)->line[!(r)->cursor])
#define PIVOT_COL(r) ((r)->col[!(r)->cursor])
//#define HAS_SELECTION(r) ((r)->line[!(r)->cursor])
#define HAS_SELECTION(r) (!!(r)->selecting)


// BufferRange data is always in order: col[0] <= col[1], line[0] <= line[1]
// EXCLUSIVELY use the macros above to access cursor and pivot positions
// It is possible for either line to be NULL, depending on the history and
//   usage of the particular instance. A NULL line represents a cursor without
//   a selection, ie, no pivot. 
// The order of cursor and pivot in memory depends on whether the user selected
//   from right to left or left to right. This state can change quickly and 
//   drastically; do not cache it or make assumptions about it.
typedef struct BufferRange {
	union {
		BufferLine* line[2];
		lineid_t lineID[2];
	};
	
	colnum_t col[2];
	
	colnum_t colWanted; // Visual column to place the cursor on, if it existed
	
	intptr_t charLength; // not implemented atm
	unsigned int cursor : 1; // cursor at: 0 = start, 1 = end
	unsigned int usesIDs : 1;
	unsigned int selecting : 1;
	unsigned int frozen : 1;
	
	// type?
} BufferRange;


typedef struct BufferRangeSet {
	int changeCounter;
	VEC(BufferRange*) ranges;
} BufferRangeSet;


BufferRange* BufferRange_Copy(BufferRange* r);
BufferRangeSet* BufferRangeSet_Copy(BufferRangeSet* rs);


typedef struct BufferRangeDrawInfo {
	BufferRangeSet* set;
	size_t index;
	
	
	
	// update notifications
} BufferRangeDrawInfo;

typedef struct {
	BufferRange ref;
	int linesNeeded; // how many extra visual lines this annotation needs
	
	char* message;
} BufferAnnotation;



// http://texteditors.org/cgi-bin/wiki.pl?Implementing_Undo_For_Text_Editors

// these represent what was done and must be reversed when using the undo feature

#define UNDO_ACTION_LIST \
	X(InsertText) \
	X(DeleteText) \
	X(InsertChar) \
	X(DeleteChar) \
	X(InsertLineAt) /* the new line will have the given line number */ \
	X(DeleteLine) \
	X(MoveCursorTo) \
	X(SetSelection) \
	X(SequenceBreak) \

// 	UndoAction_UnmodifiedFlag,


enum UndoActions {
#define X(a) UndoAction_##a,
	UNDO_ACTION_LIST
#undef X
};

extern char* undo_actions_names[];


typedef struct BufferUndo {
	enum UndoActions action;
	linenum_t lineNum;
	colnum_t colNum;
// 	intptr_t cursorL;
// 	intptr_t cursorC;
	union {
		int character; // for single-char operations
		
		struct { // for text
			char* text; 
// 			intptr_t length;
		};
		
		struct { // for selection changes
			linenum_t endLine;
			colnum_t endCol;
		};
	};
	union {
		intptr_t length;
		char isReverse; // for selection pivot restoration
	};
} BufferUndo;



enum BufferChangeAction {
	BCA_NULL,
	BCA_DeleteChars,
	BCA_DeleteLines,
	BCA_AddChars,
	BCA_AddLines,
	BCA_AnnotationsChanged,
	BCA_Undo_MoveCursor,
	BCA_Undo_SetSelection,
	BCA_SwapBuffer,
};

typedef struct BufferChangeNotification {
	Buffer* b;
	BufferRange sel;
	char isReverse;
	
	int action;
	
	Buffer* b2; // the new buffer used in BCA_SwapBuffer
} BufferChangeNotification;

typedef void (*bufferChangeNotifyFn)(BufferChangeNotification* note, void* data);


typedef struct BufferPrefixNode {
	int c;
	int64_t refs;
	int64_t trefs;
	uint64_t filter;
	struct BufferPrefixNode* next; // next sibling
	struct BufferPrefixNode* kids;
} BufferPrefixNode;


typedef struct Buffer {
	lineid_t nextLineID;
	
	BufferLine** idTable;
	size_t idTableFill;
	size_t idTableAlloc;
	uint64_t idTableMask; // used to truncate id's
	
	BufferLine* first, *last; 
	
	int64_t numLines;
	
	int watchDesc;
	int gccWatchDesc;
	
	char* gccRelPath; // path to this file from gccBasePath
	char* gccErrorJSONPath; // full path and file name of the json file
	char* gccErrorJSONFilename; // just file name of the json file
	char* gccErrorJSONDir; // just file name of the json file
	
	HT(linenum_t, BufferAnnotation*) gccErrors;
	
	// TODO: also goes to GUIBufferEditControl
	struct hlinfo* hl;
	
	
	int undoOldest; // the oldest undo item in the buffer; the end of undo
	int undoCurrent; // the current state of the Buffer; goes backwards with undo, forwards with redo
	                 // current is the next index to be ovewritten; the last action 'undone'
	int undoNewest; // the index after the newest item; the end of redo
	
	int undoFill; // the number of undo slots used in any fashion in the ring buffer
	int undoUndoLen; // the number of undo-able actions in the buffer, between undoOldest and undoCurrent
	int undoRedoLen; // the number of redo-able action in the buffer, between undoCurrent and undoNewest;
	
	int undoMax; // the size of the ring buffer
	BufferUndo* undoRing;
// 	VEC(BufferUndo) undoStack;
	int undoSaveIndex; // index of the undo position matching the file on disk
// 	char isModified;
	int changeCounter;
	
	char* sourceFile;
	Buffer* preservedVersion; // for when the file on disk changes
	char changedOnDisk;
	char deletedOnDisk;
	char movedOnDisk;
	char isPreserved; // flag for if this Buffer is in some other buffer's preservedVersion pointer
	
	char reloadGCCErrorFile;
	
	int acMaxSkip;
	
	char useDict;
	char* dictCharSet;
	BufferPrefixNode* dictRoot;
	
	int refs;
	
	// called *before* an intended change, when all pointers are still valid
	VEC(struct {
		bufferChangeNotifyFn fn;
		void* data;
	}) changeListeners;
} Buffer;



typedef struct BufferOpenHistory {
	char* realPath;
	int line;
	int col;
} BufferOpenHistory;


// uniquely identifies a file (not a path, but the underlying file) on a particular system
// inodes are only unique within a filesystem
typedef struct FileID {
	dev_t dev;
	ino_t inode;
} FileID;

typedef struct BufferCache {
	HT(BufferOpenHistory*) openHistory;
	HT(FileID, Buffer*) byFileID; 
	HT(int, Buffer*) byWatchDesc; 
	
	int inotify;
} BufferCache;


BufferCache* BufferCache_New();
Buffer* BufferCache_GetPath(BufferCache* bc, char* path, BufferSettings* bs);
void BufferCache_RemovePath(BufferCache* bc, char* realPath);
void BufferCache_CheckWatches(BufferCache* bc);

BufferOpenHistory* BufferOpenHistory_New();
void BufferOpenHistory_Delete(BufferOpenHistory* boh);
BufferOpenHistory* BufferCache_GetPathHistory(BufferCache* bc, char* realPath);
void BufferCache_RemovePathHistory(BufferCache* bc, char* realPath);
void BufferCache_SetPathHistory(BufferCache* bc, char* realPath, int line, int col);



typedef struct BufferACMatch {
	char* s;
	int len;
	float rank;
} BufferACMatch;

typedef struct BufferACMatchSet {
	BufferACMatch* matches;
	int len;
	int alloc;
	float worst;
	
	int commonPrefixLen;
	int shortestMatchLen;
	int longestMatchLen;
	float maxLineWidth; // cached value for gui
	
	int targetLen;
	char* targetStart;
	BufferRange r; // TODO: line deletion callback
	BufferRange target;
} BufferACMatchSet;



typedef struct BufferEditorMacro {
	VEC(GUI_Cmd) cmds;
} BufferEditorMacro;


// drawing and mouse controls
typedef struct GUIBufferEditControl {

	Buffer* b;
	Highlighter* h;
	
	
	float cursorBlinkTimer;
	float cursorBlinkOnTime;
	float cursorBlinkOffTime;
	
	char outlineCurLine;

	intptr_t scrollLines; // current scroll position in VISUAL lines, 0-based. There may be more visual lines than buffer lines.
	intptr_t scrollCols; // NYI, waiting on next line draw fn iteration
	intptr_t wantedScrollLine; // used before the first render. kind of an ugly hack, for now
	intptr_t wantedScrollCol; // used before the first render. kind of an ugly hack, for now
	
	float textAreaOffsetX; // accounts for line numbers and such

	// TODO: move elsewhere
	GUIFont* font;
	
	                  // this is the real cursor information
	BufferRange* sel; // primary dynamic selection, always a pointer to somewhere in selSet
	
	BufferRangeSet* selSet;
	BufferRangeSet* findSet;
	BufferRangeSet* findSearchSpace;
	long findIndex;
	
	// starting point of a mouse-drag selection
//	BufferLine* selectPivotLine; // BUG: dead pointers on line deletion?
//	intptr_t selectPivotCol;

	float scrollCoastTimer;
	float scrollCoastStrength;
	float scrollCoastMax;
	float scrollDragStartOffset; // offset from the top of the scrollbar to the drag start position
	char isDragSelecting; // normal text selection
	char isDragScrollCoasting; // middle-click-hold scroll coasting
	char isDragScrolling; // dragging the scrollbar
	char scrollCoastDir;
	
	
	char showAutocomplete;
	size_t maxAutocompleteLines; // lines to show in the popup
	intptr_t autocompleteProvokeCol;
	int autocompleteSelectedItem;
	BufferACMatchSet* autocompleteOptions;
	
	// read only
	int linesOnScreen; // number of *full* lines that fit on screen
	int colsOnScreen; // number of *full* columns that fit on screen
	
	
	VEC(uint64_t) lineOffsets; // how far IN PIXELS a line is from the top of the BUFFER.
	                           //   this includes ALL annotations and other visual offsets.
	
	
	// cached during event processing
	Vector2 tl, sz;
	
	char isRecording;
	RING(BufferEditorMacro) macros;
	
	GUI_CmdModeState inputState;
	
	// TODO: padding lines on vscroll

	int linesPerScrollWheel;
	
//	GUIWindow* scrollbar;
	float sbMinHeight;
	char needsRehighlight;

	Settings* s;
	GeneralSettings* gs;
	BufferSettings* bs;
	ThemeSettings* ts;
// 	Cmd* commands;
	
} GUIBufferEditControl;


size_t GBEC_lineFromPos(GUIBufferEditControl* w, Vector2 pos);
size_t GBEC_getColForPos(GUIBufferEditControl* w, BufferLine* bl, float x);

typedef struct GUIStatusBar GUIStatusBar;



#define INPUT_MODE_LIST \
	X(Buffer, 0, NULL,         NONE) \
	X(Find,   1, find_tray,    FIND) \
	X(Replace,2, replace_tray, REPLACE) \
	X(GoTo,   3, goto_tray,    GOTO) \
	X(Save,   4, save_tray,    SAVE) \


#define X(mode, mode_num, template, tray_type) BIM_ ## mode = mode_num,
typedef enum BufferInputMode { 
	INPUT_MODE_LIST
} BufferInputMode_t;
#undef X

//#define X(mode, mode_num, template, tray_type) GBT_ ## tray_type,
//typedef enum TrayType {
//	INPUT_MODE_LIST
//} TrayType_t;
//#undef X
//
//
//typedef struct GUIBufferInputMode {
//	BufferInputMode_t mode;
//	TrayType_t type;
//	char* template;
//} GUIBufferInputMode;
//
//
//typedef struct GUIBufferTray {
//	TrayType_t type;
//} GUIBufferTray;




#define FINDMASK_LIST \
	X(SELECTION) \
	X(SEQUENCE) \
	X(WITHIN_SELECTION) \

enum {
#define X(a, ...) FM_##a##_ORD,
	FINDMASK_LIST
#undef X
};

typedef enum FindMask {
	FM_NONE = 0,
#define X(a, ...) FM_##a = 1 << FM_##a##_ORD,
	FINDMASK_LIST
#undef X
} FindMask_t;



#define MATCHMODE_LIST \
	X(NONE) \
	X(PLAIN) \
	X(PCRE) \
	X(FUZZY) \

	
typedef enum MatchMode {
#define X(a, ...) GFMM_##a,
	MATCHMODE_LIST
#undef X
	FM_MAX_VALUE
} MatchMode_t;

typedef struct GUIFindOpt {
	char case_cmp;
	MatchMode_t match_mode;
} GUIFindOpt;


typedef struct BufferFindState {
	// configurable
	Buffer* b;
	GUIFindOpt opts;
	BufferRangeSet* searchSpace;
	char* pattern;
	char* replaceText;
	
	// read-only
	BufferRangeSet* findSet;
	long findIndex;
	
	// internal
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
	
} BufferFindState;


BufferFindState* BufferFindState_Create(Buffer* b, char* pattern, GUIFindOpt* opt, BufferRange* searchSpace);



// all sorts of fancy stuff, and keyboard controls
typedef struct GUIBufferEditor {
	
	GUIManager* gm;
	
	GUIBufferEditControl* ec;
	
	Buffer* b;
	Highlighter* h;
	
	MessagePipe* tx;
	
	char* sourceFile; // issues with undo-save
	
	char gotoLineTrayOpen;
	long gotoLineNum;
	
	GUIString findQuery;
	GUIString replaceText;
	
	BufferFindState* findState;
	
	void (*setBreakpoint)(char*, intptr_t, void*);
	void* setBreakpointData;
	
	char trayOpen;
	float trayHeight;
//	GUIWindow* trayRoot;
	
	char saveTrayOpen;
	
	// status bar
	char showStatusBar;
	float statusBarHeight;
	StatusBar* statusBar;
	
//	GUI_CmdModeInfo* inputModeInfo;
	
	HighlighterManager* hm;
	
//	GUISimpleWindow* menu;
	
	Settings* s;
	GeneralSettings* gs;
	BufferSettings* bs;
	ThemeSettings* ts;
	
	GUI_CmdModeState inputState;
	
	

} GUIBufferEditor;





struct GUIManager;
typedef struct GUIManager GUIManager;



// these are raw functions and should not be used directly by Buffer_ operations
// they are internal functions used by Buffer_raw_ operations
BufferLine* BufferLine_New(Buffer* b);
void BufferLine_Delete(Buffer* b, BufferLine* l);
void Buffer_InitEmpty(Buffer* b);
BufferLine* BufferLine_FromStr(Buffer* b, char* text, intptr_t len);
BufferLine* BufferLine_Copy(Buffer* b, BufferLine* orig);
void BufferLine_EnsureAlloc(BufferLine* l, intptr_t len);
void BufferLine_InsertChars(BufferLine* l, char* text, colnum_t col, intptr_t len);
void BufferLine_DeleteChars(BufferLine* l, intptr_t offset, intptr_t len);
void BufferLine_TruncateAfter(BufferLine* l, colnum_t col);
void BufferLine_SetText(BufferLine* l, char* text, intptr_t len);
void BufferLine_AppendText(BufferLine* l, char* text, intptr_t len);
void BufferLine_AppendLine(BufferLine* l, BufferLine* src);
int BufferLine_IsInRange(BufferLine* bl, BufferRange* sel);




// these functions will NOT interact with the undo stack
// they should never use functions below them
// functions below these should only use them and not functions above
BufferLine* Buffer_raw_GetLineByNum(Buffer* b, linenum_t num);
BufferLine* Buffer_raw_GetLineByID(Buffer* b, lineid_t id);
void Buffer_raw_RenumberLines(BufferLine* start, intptr_t num);

// inserts empty lines
BufferLine* Buffer_raw_InsertLineAfter(Buffer* b, BufferLine* before);
BufferLine* Buffer_raw_InsertLineBefore(Buffer* b, BufferLine* after);

void Buffer_raw_DeleteLine(Buffer* b, BufferLine* bl);

void Buffer_raw_InsertChars(Buffer* b, BufferLine* bl, char* txt, intptr_t offset, intptr_t len);
void Buffer_raw_DeleteChars(Buffer* b, BufferLine* bl, intptr_t offset, intptr_t len);



// undo stack processing
void Buffer_UndoInsertText(Buffer* b, linenum_t line, colnum_t col, char* txt, intptr_t len);
void Buffer_UndoDeleteText(Buffer* b, BufferLine* bl, intptr_t offset, intptr_t len);
void Buffer_UndoInsertLineAfter(Buffer* b, BufferLine* before); // safe to just pass in l->prev without checking
void Buffer_UndoDeleteLine(Buffer* b, BufferLine* bl); // saves the text too
void Buffer_UndoSetSelection(Buffer* b, linenum_t startL, colnum_t startC, linenum_t endL, colnum_t endC);
//void Buffer_UndoSequenceBreak(Buffer* b, int saved, intptr_t cursorLine, intptr_t cursorCol);
void Buffer_UndoSequenceBreak(Buffer* b, int saved, 
	linenum_t startL, colnum_t startC, linenum_t endL, colnum_t endC, char isReverse);
void Buffer_UndoReplayToSeqBreak(Buffer* b);
int Buffer_UndoReplayTop(Buffer* b);
void Buffer_UndoTruncateStack(Buffer* b);

// clean up all memory related to the undo system
void Buffer_FreeAllUndo(Buffer* b);

void Buffer_RedoReplayToSeqBreak(Buffer* b);
int Buffer_RedoReplay(Buffer* b, BufferUndo* u);


// debug visualization functions
void BufferUndo_DebugRender(GUIManager* gm, Buffer* w, Vector2 tl, Vector2 sz, PassFrameParams* pfp);
void BufferUndo_DebugRenderItem(GUIManager* gm, BufferUndo* u, Vector2 tl, Vector2 sz, PassFrameParams* pfp);


// -----
// functions below here will add to the undo stack
//    and send buffer notifications
// -----

int Buffer_ProcessCommand(Buffer* b, GUI_Cmd* cmd, int* needRehighlight);
int GBEC_ProcessCommand(GUIBufferEditControl* w, GUI_Cmd* cmd, int* needRehighlight);




// these functions operate independently of the cursor
BufferLine* Buffer_InsertLineBefore(Buffer* b, BufferLine* after, char* text, intptr_t length);
BufferLine* Buffer_InsertLineAfter(Buffer* b, BufferLine* before, char* text, intptr_t length);
BufferLine* Buffer_InsertEmptyLineBefore(Buffer* b, BufferLine* after);
BufferLine* Buffer_InsertEmptyLineAfter(Buffer* b, BufferLine* before);
void Buffer_DeleteLine(Buffer* b, BufferLine* bl);
void Buffer_LineInsertChars(Buffer* b, BufferLine* bl, char* text, intptr_t offset, intptr_t length);
void Buffer_LineAppendText(Buffer* b, BufferLine* bl, char* text, intptr_t length);
void Buffer_LineAppendLine(Buffer* b, BufferLine* target, BufferLine* src);
void Buffer_LineDeleteChars(Buffer* b, BufferLine* bl, colnum_t col, intptr_t length);
void Buffer_LineTruncateAfter(Buffer* b, BufferLine* bl, colnum_t col);
void Buffer_BackspaceAt(Buffer* b, BufferLine* l, colnum_t col);
void Buffer_DeleteAt(Buffer* b, BufferLine* l, colnum_t col);
void Buffer_DuplicateLines(Buffer* b, BufferLine* src, int amt);
void Buffer_LineIndent(Buffer* b, BufferLine* bl);
void Buffer_LineUnindent(Buffer* b, BufferLine* bl);
void Buffer_IndentSelection(Buffer* b, BufferRange* sel);
void Buffer_UnindentSelection(Buffer* b, BufferRange* sel);
void Buffer_LineEnsureEnding(Buffer* b, BufferLine* bl, char* text, intptr_t length);
void Buffer_LinePrependText(Buffer* b, BufferLine* bl, char* text);
void Buffer_LineUnprependText(Buffer* b, BufferLine* bl, char* text);
void Buffer_LineEnsureEndingSelection(Buffer* b, BufferRange* sel, char* text, intptr_t length);
void Buffer_LineAppendTextSelection(Buffer* b, BufferRange* sel, char* text, intptr_t length);
void Buffer_LinePrependTextSelection(Buffer* b, BufferRange* sel, char* text);
void Buffer_LineUnprependTextSelection(Buffer* b, BufferRange* sel, char* text);
void Buffer_SurroundSelection(Buffer* b, BufferRange* sel, char* begin, char* end);
int Buffer_UnsurroundSelection(Buffer* b, BufferRange* sel, char* begin, char* end);
int BufferRange_CompleteLinesOnly(BufferRange* sel);
BufferRange* BufferRange_New(GUIBufferEditControl* w);

void GBEC_MoveMarkerH(GUIBufferEditControl* w, BufferRange* r, int c, colnum_t cols);
void GBEC_MoveCursorH(GUIBufferEditControl* w, BufferRange* r, colnum_t cols);
void GBEC_MovePivotH(GUIBufferEditControl* w, BufferRange* r, colnum_t cols);
void GBEC_MoveStartH(GUIBufferEditControl* w, BufferRange* r, colnum_t cols);
void GBEC_MoveEndH(GUIBufferEditControl* w, BufferRange* r, colnum_t cols);


void GBEC_MoveMarkerV(GUIBufferEditControl* w, BufferRange* r, int c, linenum_t lines);
void GBEC_MoveCursorV(GUIBufferEditControl* w, BufferRange* r, linenum_t lines);
void GBEC_MovePivotV(GUIBufferEditControl* w, BufferRange* r, linenum_t lines);
void GBEC_MoveStartV(GUIBufferEditControl* w, BufferRange* r, linenum_t lines);
void GBEC_MoveEndV(GUIBufferEditControl* w, BufferRange* r, linenum_t lines);

void GBEC_MoveRangeCursorTo(GUIBufferEditControl* w, BufferRange* r, BufferLine* bl, colnum_t col);
void GBEC_SetCurrentSelection(GUIBufferEditControl* w, BufferLine* startL, colnum_t startC, BufferLine* endL, colnum_t endC);
void GBEC_SetCurrentSelectionRange(GUIBufferEditControl* w, BufferRange* r);
void GBEC_ClearCurrentSelection(GUIBufferEditControl* w);
void GBEC_ClearAllSelections(GUIBufferEditControl* w);
void Buffer_DeleteSelectionContents(Buffer* b, BufferRange* sel);
void GBEC_SelectSequenceUnder(GUIBufferEditControl* w, BufferLine* l, colnum_t col, char* charSet);
void Buffer_GetSequenceUnder(Buffer* b, BufferLine* l, colnum_t col, char* charSet, BufferRange* out);
void GBEC_PrependToSequence(GUIBufferEditControl* w, BufferLine* line, colnum_t col, char* seq, char* str);
void GBEC_AppendToSequence(GUIBufferEditControl* w, BufferLine* line, colnum_t col, char* seq, char* str);
void GBEC_SurroundSequence(GUIBufferEditControl* w, BufferLine* line, colnum_t col, char* seq, char* left, char* right);

void BufferFindState_FreeAll(BufferFindState* st);

int BufferFindState_FindAll(BufferFindState* st);
int BufferFindState_FindAll_Fuzzy(BufferFindState* st);
int BufferFindState_FindAll_PCRE(BufferFindState* st);
int BufferRangeSet_test(BufferRangeSet* s, BufferLine* bl, colnum_t col);
void BufferRangeSet_FreeAll(BufferRangeSet* s);

char* Buffer_StringFromSelection(Buffer* b, BufferRange* sel, size_t* outLen);

// these functions operate on absolute positions
void Buffer_AppendRawText(Buffer* b, char* source, intptr_t len);
BufferLine* Buffer_AppendLine(Buffer* b, char* text, intptr_t len);
BufferLine* Buffer_PrependLine(Buffer* b, char* text, intptr_t len);
void Buffer_DuplicateSelection(Buffer* b, BufferRange* sel, int amt);
void Buffer_InsertBufferAt(Buffer* target, Buffer* graft, BufferLine* tline, colnum_t tcol, BufferRange* outRange);
void Buffer_CommentLine(Buffer* b, BufferLine* bl);
void Buffer_CommentSelection(Buffer* b, BufferRange* sel);
long BufferRange_FindNextRangeSet(BufferRangeSet* rs, BufferLine* line, colnum_t col);
int Buffer_ReplaceAllString(Buffer* dst, char* needle, Buffer* src);


// returns 0 on success
// finds first match only
// no support for newlines in needle
int Buffer_strstr(Buffer* b, char* needle, BufferRange* out);


void Buffer_SetBookmarkAt(Buffer* b, BufferLine* bl);
void Buffer_RemoveBookmarkAt(Buffer* b, BufferLine* bl);
void Buffer_ToggleBookmarkAt(Buffer* b, BufferLine* bl);

void Buffer_RelPosH(BufferLine* startL, colnum_t startC, colnum_t cols, BufferLine** outL, colnum_t* outC);
void Buffer_RelPosV(BufferLine* startL, colnum_t startC, linenum_t lines, BufferLine** outL, colnum_t* outC);

int BufferRange_Normalize(BufferRange* r);
int BufferRangeSet_Normalize(BufferRangeSet* rs);
colnum_t BufferLine_GetIndentCol(BufferLine* l);



// HACK: temporary junk
void GUIBufferEditControl_MarkRefreshHighlight(GUIBufferEditControl* w);
void GUIBufferEditControl_RefreshHighlight(GUIBufferEditControl* w);
void GUIBufferEditor_ProbeHighlighter(GUIBufferEditor* w);

void GBEC_RecalcLineOffsets(GUIBufferEditControl* w);


Buffer* Buffer_New(BufferSettings* bs);
void Buffer_AddRef(Buffer* b);
void Buffer_DecRef(Buffer* b);
void Buffer_Delete(Buffer* b);
Buffer* Buffer_Copy(Buffer* src);
Buffer* Buffer_FromSelection(Buffer* src, BufferRange* sel);
void Buffer_ReloadGCCErrorFile(Buffer* b);
void Buffer_ToRawText(Buffer* b, char** out, size_t* len);
int Buffer_SaveToFile(Buffer* b, char* path);
int Buffer_LoadFromFile(Buffer* b, char* path);
int Buffer_Compare(Buffer* a, Buffer* b);
void Buffer_RegisterChangeListener(Buffer* b, bufferChangeNotifyFn fn, void* data);
void Buffer_NotifyChanges(BufferChangeNotification* note);
void Buffer_NotifyLineDeletion(Buffer* b, BufferLine* sLine, BufferLine* eLine);
void Buffer_NotifyLineAddition(Buffer* b, BufferLine* sLine, BufferLine* eLine);
void Buffer_NotifyAnnotationChange(Buffer* b);
void Buffer_NotifyUndoMoveCursor(Buffer* b, BufferLine* line, colnum_t col);
void Buffer_NotifyUndoSetSelection(Buffer* b, BufferLine* startL, colnum_t startC, BufferLine* endL, colnum_t endC, char isReverse);
void BufferRange_DeleteLineNotify(BufferRange* r, BufferRange* dsel);
void BufferRangeSet_DeleteLineNotify(BufferRangeSet* rs, BufferRange* dsel);

// These functions operate on and with the cursor
BufferLine* Buffer_AdvanceLines(Buffer* b, int n);
void GBEC_InsertLinebreak(GUIBufferEditControl* b, int moveCursor);
//void BufferRange_MoveCursorV(GUIBufferEditControl* w, BufferRange* r, linenum_t lines);
//void GBEC_MoveCursorH(GUIBufferEditControl* w, BufferRange* r, colnum_t cols);
//void GBEC_MoveCursorHSel(GUIBufferEditControl* w, colnum_t cols);
void GBEC_MoveCursor(GUIBufferEditControl* w, linenum_t lines, colnum_t cols);
void GBEC_MoveCursorTo(GUIBufferEditControl* w, BufferLine* bl, colnum_t col); // absolute move
void GBEC_MoveCursorToNum(GUIBufferEditControl* w, linenum_t line, colnum_t col); // absolute move
void GBEC_NextBookmark(GUIBufferEditControl* w);
void GBEC_PrevBookmark(GUIBufferEditControl* w);
void GBEC_FirstBookmark(GUIBufferEditControl* w);
void GBEC_LastBookmark(GUIBufferEditControl* w);
void Buffer_Indent(Buffer* b);
void Buffer_Unindent(Buffer* b);
intptr_t Buffer_IndentToPrevLine(Buffer* b, BufferLine* bl);
void Buffer_CollapseWhitespace(Buffer* b, BufferLine* l, colnum_t col);
void GBEC_MoveToPrevSequence(GUIBufferEditControl* w, BufferLine* l, colnum_t col, char* charSet);
void GBEC_MoveToNextSequence(GUIBufferEditControl* w, BufferLine* l, colnum_t col, char* charSet);
void GBEC_DeleteToPrevSequence(GUIBufferEditControl* w, BufferLine* l, colnum_t col, char* charSet);
void GBEC_DeleteToNextSequence(GUIBufferEditControl* w, BufferLine* l, colnum_t col, char* charSet);
int Buffer_FindSequenceEdgeForward(Buffer* b, BufferLine** linep, colnum_t* colp, char* charSet);
int Buffer_FindSequenceEdgeBackward(Buffer* b, BufferLine** linep, colnum_t* colp, char* charSet);
void GBEC_SurroundCurrentSelection(GUIBufferEditControl* w, char* begin, char* end);
void GBEC_UnsurroundCurrentSelection(GUIBufferEditControl* w, char* begin, char* end);
void GBEC_SurroundRange(GUIBufferEditControl* w, BufferRange* r, char* begin, char* end);
void GBEC_UnsurroundRange(GUIBufferEditControl* w, BufferRange* r, char* begin, char* end);
void GBEC_MoveToFirstCharOrSOL(GUIBufferEditControl* w, BufferLine* bl);
void GBEC_MoveToFirstCharOfLine(GUIBufferEditControl* w, BufferLine* bl);
void GBEC_MoveToLastCharOfLine(GUIBufferEditControl* w, BufferLine* bl);

int GBEC_IsBracePair(char left, char right);
int GBEC_IsOpenBrace(char left);
int GBEC_IsCloseBrace(char right);

void GBEC_PushCursor(GUIBufferEditControl* w, BufferLine* bl, colnum_t col);

void GBEC_InsertCharsMC(GUIBufferEditControl* w, char* s, size_t cnt);
void GBEC_MoveCursorHMC(GUIBufferEditControl* w, intptr_t cols);

void GBEC_ReplaceLineWithSelectionTransform(
	GUIBufferEditControl* w, 
	char* selectionToken,
	char* cursorToken,
	char* format
);

void Buffer_DebugPrint(Buffer* b);
void Buffer_DebugPrintUndoStack(Buffer* b);

int Buffer_AddDictWord(Buffer* b, char* word, size_t len);
int Buffer_RemoveDictWord(Buffer* b, char* word, size_t len);
void Buffer_RemoveLineFromDict(Buffer* b, BufferLine* l);
void Buffer_AddLineToDict(Buffer* b, BufferLine* l);
void Buffer_PrintDict(Buffer* b);
BufferACMatchSet* Buffer_FindDictMatches(Buffer* b, BufferRange* r);
void GBEC_CancelAutocomplete(GUIBufferEditControl* w);

void GBEC_SmartAlign(GUIBufferEditControl* w, char* separator);

// temp
int GUIBufferEditor_FindWord(GUIBufferEditor* w, char* word);


// GUIBufferEditor


void GUIBufferEditor_Render(GUIBufferEditor* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);
void GBEC_Render(GUIBufferEditControl* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);
void GBEC_Update(GUIBufferEditControl* w, Vector2 tl, Vector2 sz, PassFrameParams* pfp);

void GUIBufferEditor_SaveSessionState(GUIBufferEditor* w, json_value_t* out);
void GUIBufferEditor_LoadSessionState(GUIBufferEditor* w, json_value_t* state);

void GUIBufferEditControl_UpdateSettings(GUIBufferEditControl* w, Settings* s);
void GUIBufferEditor_UpdateSettings(GUIBufferEditor* w, Settings* s);

void GUIBufferEditControl_Draw(GUIBufferEditControl* gbe, GUIManager* gm, Vector2 tl, Vector2 sz, linenum_t lineFrom, linenum_t lineTo, colnum_t colFrom, colnum_t colTo, PassFrameParams* pfp);
GUIBufferEditor* GUIBufferEditor_New(GUIManager* gm, MessagePipe* tx);
GUIBufferEditControl* GUIBufferEditControl_New(GUIManager* gm);
void GUIBufferEditor_Destroy(GUIBufferEditor* w);

void GUIBufferEditor_SetBuffer(GUIBufferEditor* w, Buffer* b);
void GUIBufferEditControl_SetBuffer(GUIBufferEditControl* w, Buffer* b);

// set absolute scroll position
void GUIBufferEditControl_SetScroll(GUIBufferEditControl* w, linenum_t line, colnum_t col);
void GBEC_SetScrollCentered(GUIBufferEditControl* w, linenum_t line, colnum_t col);
void GUIBufferEditor_scrollToCursor(GUIBufferEditor* gbe);
void GBEC_scrollToCursor(GUIBufferEditControl* gbe);
void GBEC_scrollToCursorOpt(GUIBufferEditControl* w, int centered);

int GUIBufferEditor_ProcessCommand(GUIBufferEditor* w, GUI_Cmd* cmd, int* needRehighlight);



// move the view by this delta
void GBEC_ScrollDir(GUIBufferEditControl* w, linenum_t lines, colnum_t cols);

void GBEC_SetSelectionFromPivot(GUIBufferEditControl* gbe);
void GBEC_SelectionChanged(GUIBufferEditControl* gbe);


void GBEC_ReplayMacro(GUIBufferEditControl* w, int index);


// regenerates all PCRE internal structures
int BufferFindState_CompileRE(BufferFindState* st);

// prepares and initializes internal BufferEditor data with the provided findstate struct
// takes ownership of st
int GUIBufferEditor_StartFind(GUIBufferEditor* w, BufferFindState* st);

// cleans up BufferEditor data related to finding
void GUIBufferEditor_StopFind(GUIBufferEditor* w); 

// used to change the search query without changing any other parameters or find state
int GUIBufferEditor_UpdateFindPattern(GUIBufferEditor* w, char* s, ssize_t len);

// activates finding using data near the cursor
// This is the single function to use externally to the find subsystem to start finding in BufferEditor
int GUIBufferEditor_SmartFind(GUIBufferEditor* w, char* charSet, FindMask_t mask);


int GUIBufferEditor_RelativeFindMatch(GUIBufferEditor* w, int offset, int continueFromCursor, BufferFindState* st);


int GUIBufferEditor_ReplaceNext(GUIBufferEditor* w);
int GUIBufferEditor_ReplaceAll(GUIBufferEditor* w, BufferRangeSet* rset, char* text);

// for buffer lines. bad name. meh
void drawTextLine(GUIManager* gm, BufferSettings* bs, GUIFont* f, struct Color4* textColor, char* txt, int charCount, Vector2 tl);



colnum_t GBEC_NominalColFromVisual(GUIBufferEditControl* w, BufferLine* bl, colnum_t wanted);
colnum_t GBEC_VisualColFromNominal(GUIBufferEditControl* w, BufferLine* bl, colnum_t col);


// this is the finalizer of MurmurHash3
static inline uint64_t lineIDHash(uint64_t i) {
	i += 1337ULL;
	i ^= i >> 33ULL;
	i *= 0xff51afd7ed558ccdULL;
	i ^= i >> 33ULL;
	i *= 0xc4ceb9fe1a85ec53ULL;
	i ^= i >> 33ULL;
	return i;
}







#define BUFFER_SETTING_LIST \
	SETTING(int,   linesPerScrollWheel,  3,     1,    100) \
	SETTING(bool,  disableScrollbar,     false, NULL, NULL) \
	SETTING(float, scrollbarFadeDistance, 100,  1,    INT_MAX) \
	SETTING(bool,  cursorBlinkEnable,    true,  NULL, NULL) \
	SETTING(float, cursorBlinkOffTime,   0.600, 0,    300000) \
	SETTING(float, cursorBlinkOnTime,    0.600, 0,    300000) \
	SETTING(bool,  highlightCurrentLine, true,  NULL, NULL) \
	SETTING(bool,  outlineCurrentLine,   true,  NULL, NULL) \
	SETTING(float, outlineCurrentLineYOffset,   0,     -999, 999) \
	SETTING(float, lineNumExtraWidth,    10,    0,    1920*16) \
	SETTING(bool,  showLineNums,         true,  NULL, NULL) \
	SETTING(int,   lineNumBase,          10,    2,    36) \
	SETTING(charp, lineNumCharset,       "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", NULL, NULL) \
	SETTING(float, charWidth,            8,     1,    1920*16) \
	SETTING(float, lineHeight,           17,    1,    1920*16) \
	SETTING(int,   tabWidth,             4,     0,    INT_MAX) \
	SETTING(charp, font,                 "Courier New", NULL, NULL) \
	SETTING(float, fontSize,             14,    1,    1920*16) \
	SETTING(bool,  invertSelection,      true,  NULL, NULL) \
	SETTING(int,   maxUndo,              4096,  0,    INT_MAX) \
	SETTING(bool,  useDict,              true,  NULL, NULL) \
	SETTING(charp, dictCharSet,          "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_", NULL, NULL) \
	SETTING(int,   statusBarHeight,      20,    0,    INT_MAX) \
	SETTING(int,   autocompleteMaxSkip,  20,    0,    100) \
	SETTING(charp, gccBasePath,          NULL,  NULL, NULL) \
	SETTING(charp, gccErrorJSONPath,     NULL,  NULL, NULL) \
	SETTING(charp, gccErrorJSONSuffix,   ".gcc.json", NULL, NULL) \




#define THEME_SETTING_LIST \
	SETTING(charp,  name,                          "default dark",NULL, NULL) \
	SETTING(bool,   is_dark,                       true,          NULL, NULL) \
	SETTING(Color4, cursorColor,                   C4H(ffffffff), NULL, NULL) \
	SETTING(Color4, textColor,                     C4H(8f8f8fff), NULL, NULL) \
	SETTING(Color4, bgColor,                       C4H(0f0f0fff), NULL, NULL) \
	SETTING(Color4, lineNumColor,                  C4H(ffffffff), NULL, NULL) \
	SETTING(Color4, lineNumBgColor,                C4H(141414ff), NULL, NULL) \
	SETTING(Color4, lineNumBookmarkColor,          C4H(32ff32ff), NULL, NULL) \
	SETTING(Color4, lineNumBreakpointColor,        C4H(f00f0fff), NULL, NULL) \
	SETTING(Color4, scrollbarColor,                C4H(ffffffff), NULL, NULL) \
	SETTING(Color4, hl_bgColor,                    C4H(00c8c8ff), NULL, NULL) \
	SETTING(Color4, hl_textColor,                  C4H(fa0032ff), NULL, NULL) \
	SETTING(Color4, find_bgColor,                  C4H(115511ff), NULL, NULL) \
	SETTING(Color4, find_textColor,                C4H(660022ff), NULL, NULL) \
	SETTING(Color4, findSpace_bgColor,             C4H(181833ff), NULL, NULL) \
	SETTING(Color4, outlineCurrentLineBorderColor, C4H(323232ff), NULL, NULL) \
	SETTING(Color4, selectedItemTextColor,         C4H(c8c8c8ff), NULL, NULL) \
	SETTING(Color4, selectedItemBgColor,           C4H(505050ff), NULL, NULL) \


#include "settings_macros_on.h"
#define SETTING(type, name, val ,min,max) type name;

typedef struct BufferSettings {
	BUFFER_SETTING_LIST
} BufferSettings;

typedef struct ThemeSettings {
	THEME_SETTING_LIST
} ThemeSettings;

#undef SETTING
#include "settings_macros_off.h"


ThemeSettings* ThemeSettings_Alloc(void* useless);
ThemeSettings* ThemeSettings_Copy(void* useless, ThemeSettings* s);
void ThemeSettings_Free(void* useless, ThemeSettings* s);
void ThemeSettings_LoadDefaults(void* useless, ThemeSettings* s);
void ThemeSettings_LoadJSON(void* useless, ThemeSettings* s, struct json_value* jsv);

BufferSettings* BufferSettings_Alloc(void* useless);
BufferSettings* BufferSettings_Copy(void* useless, BufferSettings* s);
void BufferSettings_Free(void* useless, BufferSettings* s);
void BufferSettings_LoadDefaults(void* useless, BufferSettings* s);
void BufferSettings_LoadJSON(void* useless, BufferSettings* s, struct json_value* jsv);


#endif // __gpuedit_buffer_h__

