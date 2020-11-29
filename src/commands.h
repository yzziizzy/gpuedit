#ifndef __gpuedit_commands_h__
#define __gpuedit_commands_h__

#include "c_json/json.h"

// EOL = End of Line
// SOL = Start of Line

#define COMMANDTYPE_LIST \
	X(Cmd, NULL) \
	X(BufferCmd, Debug) \
	X(BufferCmd, MoveCursorV) \
	X(BufferCmd, MoveCursorH) \
	X(BufferCmd, InsertChar) \
	X(BufferCmd, SplitLine) \
	X(BufferCmd, SplitLineIndent) \
	X(BufferCmd, Backspace) \
	X(BufferCmd, Delete) \
	X(BufferCmd, DeleteCurLine) \
	X(BufferCmd, MovePage) \
	X(BufferCmd, GoToFirstColOfFile) \
	X(BufferCmd, GoToLastColOfFile) \
	X(BufferCmd, GoToFirstCharOfLine) \
	X(BufferCmd, GoToLastCharOfLine) \
	X(BufferCmd, DuplicateLine) \
	X(BufferCmd, SelectNone) \
	X(BufferCmd, SelectAll) \
	X(BufferCmd, SelectLine) \
	X(BufferCmd, SelectToEOL) \
	X(BufferCmd, SelectFromSOL) \
	X(BufferCmd, GoToLine) \
	X(BufferCmd, GoToEOL) \
	X(BufferCmd, GoToSOL) \
	X(BufferCmd, GoToAfterIndent) \
	X(BufferCmd, RehilightWholeBuffer) \
	X(BufferCmd, Cut) \
	X(BufferCmd, Copy) \
	X(BufferCmd, Paste) \
	X(BufferCmd, SetBookmark) \
	X(BufferCmd, RemoveBookmark) \
	X(BufferCmd, ToggleBookmark) \
	X(BufferCmd, GoToNextBookmark) \
	X(BufferCmd, GoToPrevBookmark) \
	X(BufferCmd, GoToFirstBookmark) \
	X(BufferCmd, GoToLastBookmark) \
	X(BufferCmd, Undo) \
	X(BufferCmd, Redo) \
	X(BufferCmd, GrowSelectionH) \
	X(BufferCmd, GrowSelectionV) \
	X(BufferCmd, SelectSequenceUnder) \
	X(BufferCmd, MoveToNextSequence) \
	X(BufferCmd, MoveToPrevSequence) \
	X(BufferCmd, DeleteToNextSequence) \
	X(BufferCmd, DeleteToPrevSequence) \
	X(BufferCmd, Indent) \
	X(BufferCmd, SmartIndent) \
	X(BufferCmd, Unindent) \
	X(BufferCmd, LinePrependText) \
	X(BufferCmd, LineUnprependText) \
	X(BufferCmd, SurroundSelection) \
	X(BufferCmd, UnsurroundSelection) \
	X(BufferCmd, SmartComment) \
	X(BufferCmd, StrictUncomment) \
	X(BufferCmd, SmartUncomment) \
	X(BufferCmd, CollapseWhitespace) \
	X(BufferCmd, FindStartSequenceUnderCursor) \
	X(BufferCmd, FindStart) \
	X(BufferCmd, FindResume) \
	X(BufferCmd, FindNext) \
	X(BufferCmd, ReplaceStart) \
	X(BufferCmd, ReplaceNext) \
	X(BufferCmd, ReplaceAll) \
	X(BufferCmd, Save) \
	X(BufferCmd, Reload) \
	X(BufferCmd, PromptLoad) \
	X(BufferCmd, CloseTray) \
	X(BufferCmd, ToggleMenu) \
	X(BufferCmd, ShowDictComplete) \
	X(BufferCmd, ToggleGDBBreakpoint) \
	X(BufferCmd, ScrollLinesV) \
	X(BufferCmd, ScrollScreenPctV) \
	X(BufferCmd, ScrollColsH) \
	X(BufferCmd, ScrollScreenPctH) \
	X(BufferCmd, NextHighlighter) \
	\
	X(FileBrowserCmd, CursorMove) \
	X(FileBrowserCmd, CursorMoveNoWrap) \
	X(FileBrowserCmd, UpDir) \
	X(FileBrowserCmd, SmartOpen) \
	X(FileBrowserCmd, ToggleSelect) \
	X(FileBrowserCmd, JumpToLetter) \
	\
	X(FuzzyMatcherCmd, CursorMove) \
	X(FuzzyMatcherCmd, Open) \
	\
	X(MainCmd, SimpleWindowTest) \
	X(MainCmd, OpenFileBrowser) \
	X(MainCmd, FuzzyOpener) \
	X(MainCmd, SaveActiveTab) \
	X(MainCmd, SaveAll) \
	X(MainCmd, Quit) \
	X(MainCmd, SaveQuit) \
	X(MainCmd, QuitWithoutSave) \
	X(MainCmd, LoadFile) \
	X(MainCmd, NewEmptyBuffer) \
	X(MainCmd, ReloadTab) \
	X(MainCmd, CloseTab) \
	X(MainCmd, SaveAndCloseTab) \
	X(MainCmd, NextTab) \
	X(MainCmd, PrevTab) \
	X(MainCmd, GoToTab) \
	X(MainCmd, MainMenu) \
	X(MainCmd, ToggleGDBBreakpoint) \
	
	
	/*
	// NYI
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
*/

#define X(a, b) a##_##b,
enum CmdType {
	COMMANDTYPE_LIST
};
#undef X





typedef struct Cmd {
	unsigned int mode;
	unsigned int mods;
	int keysym;
	enum CmdType cmd;
	unsigned int flags;
	union {
		long amt;
		char* str;
		char** pstr;
	};
} Cmd;


typedef struct CmdList {
	Cmd* mods[16]; // ctl, alt, shift, tux
} CmdList;


struct GUIEvent;
typedef struct GUIEvent GUIEvent;


int Commands_ProbeCommand(GUIEvent* gev, Cmd* list, unsigned int mode, Cmd* out, unsigned int* iter);

CmdList* Commands_SeparateCommands(Cmd* in);


Cmd* CommandList_loadJSON(json_value_t* root);
Cmd* CommandList_loadJSONFile(char* path);


#endif //__gpuedit_commands_h__
