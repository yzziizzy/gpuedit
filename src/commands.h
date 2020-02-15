#ifndef __gpuedit_commands_h__
#define __gpuedit_commands_h__


// EOL = End of Line
// SOL = Start of Line

#define COMMANDTYPE_LIST \
	X(Cmd, NULL) \
	X(BufferCmd, Debug) \
	X(BufferCmd, MoveCursorV) \
	X(BufferCmd, MoveCursorH) \
	X(BufferCmd, InsertChar) \
	X(BufferCmd, SplitLine) \
	X(BufferCmd, Backspace) \
	X(BufferCmd, Delete) \
	X(BufferCmd, DeleteCurLine) \
	X(BufferCmd, MovePage) \
	X(BufferCmd, Home) \
	X(BufferCmd, End) \
	X(BufferCmd, DuplicateLine) \
	X(BufferCmd, SelectNone) \
	X(BufferCmd, SelectAll) \
	X(BufferCmd, SelectLine) \
	X(BufferCmd, SelectToEOL) \
	X(BufferCmd, SelectFromSOL) \
	X(BufferCmd, GoToLine) \
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
	X(BufferCmd, Indent) \
	X(BufferCmd, Unindent) \
	X(BufferCmd, FindStart) \
	X(BufferCmd, FindNext) \
	X(BufferCmd, Save) \
	X(BufferCmd, Reload) \
	X(BufferCmd, PromptLoad) \
	X(BufferCmd, CloseTray) \
	\
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
	unsigned int mods;
	int keysym;
	enum CmdType cmd;
// 	union {
		int amt;
// 	};
	unsigned int flags;
} Cmd;


typedef struct CmdList {
	Cmd* mods[16]; // ctl, alt, shift, tux
} CmdList;


struct GUIEvent;
typedef struct GUIEvent GUIEvent;


int Commands_ProbeCommand(GUIEvent* gev, Cmd* list, Cmd* out, unsigned int* iter);

CmdList* Commands_SeparateCommands(Cmd* in);

Cmd* CommandList_loadFile(char* path);


#endif //__gpuedit_commands_h__
