#ifndef __gpuedit_commands_h__
#define __gpuedit_commands_h__








#define EXTERN_GUI_ELEMENT_LIST \
	X(Buffer) \
	X(Main) \
	X(FileBrowser) \
	X(FuzzyMatcher) \
	X(GrepOpen) \
	X(Calc) \
	X(Debug) \




// EOL = End of Line
// SOL = Start of Line

#define EXTERN_GUI_COMMAND_LIST \
	X(Buffer, Debug) \
	X(Buffer, PushMode) \
	X(Buffer, PopMode) \
	X(Buffer, SetMode) \
	X(Buffer, KeyStroke) \
	X(Buffer, MoveCursorV) \
	X(Buffer, MoveCursorH) \
	X(Buffer, MoveCursorHSel) \
	X(Buffer, InsertChar) \
	X(Buffer, InsertString) \
	X(Buffer, SplitLine) \
	X(Buffer, SplitLineIndent) \
	X(Buffer, Backspace) \
	X(Buffer, Delete) \
	X(Buffer, DeleteCurLine) \
	X(Buffer, MovePage) \
	X(Buffer, GoToFirstColOfFile) \
	X(Buffer, GoToLastColOfFile) \
	X(Buffer, GoToFirstCharOrSOL) \
	X(Buffer, GoToFirstCharOfLine) \
	X(Buffer, GoToLastCharOfLine) \
	X(Buffer, DuplicateLine) \
	X(Buffer, SelectNone) \
	X(Buffer, SelectAll) \
	X(Buffer, SelectLine) \
	X(Buffer, SelectToEOL) \
	X(Buffer, SelectFromSOL) \
	X(Buffer, GoToLineLaunch) \
	X(Buffer, GoToLineSubmit) \
	X(Buffer, GoToLineCancel) \
	X(Buffer, GoToEOL) \
	X(Buffer, GoToSOL) \
	X(Buffer, GoToAfterIndent) \
	X(Buffer, RehilightWholeBuffer) \
	X(Buffer, Cut) \
	X(Buffer, SmartCut) \
	X(Buffer, Copy) \
	X(Buffer, Paste) \
	X(Buffer, SetBookmark) \
	X(Buffer, RemoveBookmark) \
	X(Buffer, ToggleBookmark) \
	X(Buffer, GoToNextBookmark) \
	X(Buffer, GoToPrevBookmark) \
	X(Buffer, GoToFirstBookmark) \
	X(Buffer, GoToLastBookmark) \
	X(Buffer, Undo) \
	X(Buffer, Redo) \
	X(Buffer, GrowSelectionH) \
	X(Buffer, GrowSelectionV) \
	X(Buffer, ClearSelection) \
	X(Buffer, SelectSequenceUnder) \
	X(Buffer, MoveToNextSequence) \
	X(Buffer, MoveToPrevSequence) \
	X(Buffer, DeleteToNextSequence) \
	X(Buffer, DeleteToPrevSequence) \
	X(Buffer, GrowSelectionToNextSequence) \
	X(Buffer, GrowSelectionToPrevSequence) \
	X(Buffer, GrowSelectionToSOL) \
	X(Buffer, GrowSelectionToEOL) \
	X(Buffer, Indent) \
	X(Buffer, SmartIndent) \
	X(Buffer, Unindent) \
	X(Buffer, LinePrependText) \
	X(Buffer, LineUnprependText) \
	X(Buffer, LineAppendText) \
	X(Buffer, LineUnappendText) \
	X(Buffer, LineEnsureEnding) \
	X(Buffer, SurroundSelection) \
	X(Buffer, UnsurroundSelection) \
	X(Buffer, ReplaceLineWithSelectionTransform) \
	X(Buffer, SmartComment) \
	X(Buffer, StrictUncomment) \
	X(Buffer, SmartUncomment) \
	X(Buffer, CollapseWhitespace) \
	X(Buffer, FindStartSequenceUnderCursor) \
	X(Buffer, FindStartFromSelection) \
	X(Buffer, FindStart) \
	X(Buffer, SmartFind) \
	X(Buffer, FindResume) \
	X(Buffer, FindNext) \
	X(Buffer, FindPrev) \
	X(Buffer, ReplaceStart) \
	X(Buffer, ReplaceNext) \
	X(Buffer, ReplaceAll) \
	X(Buffer, Save) \
	X(Buffer, SaveAndClose) \
	X(Buffer, PromptAndClose) \
	X(Buffer, Reload) \
	X(Buffer, PromptLoad) \
	X(Buffer, CloseTray) \
	X(Buffer, ToggleMenu) \
	X(Buffer, ShowDictComplete) \
	X(Buffer, ToggleGDBBreakpoint) \
	X(Buffer, ScrollLinesV) \
	X(Buffer, ScrollScreenPctV) \
	X(Buffer, ScrollColsH) \
	X(Buffer, ScrollScreenPctH) \
	X(Buffer, NextHighlighter) \
	X(Buffer, SmartBubbleSelection) \
	X(Buffer, MacroToggleRecording) \
	X(Buffer, MacroReplay) \
	X(Buffer, PushCursor) \
	\
	X(FileBrowser, Exit) \
	X(FileBrowser, CursorMove) \
	X(FileBrowser, CursorMoveNoWrap) \
	X(FileBrowser, UpDir) \
	X(FileBrowser, SmartOpen) \
	X(FileBrowser, ToggleSelect) \
	X(FileBrowser, JumpToLetter) \
	X(FileBrowser, MoveCursorV) \
	X(FileBrowser, ParentDir) \
	X(FileBrowser, OpenUnderCursor) \
	\
	X(FuzzyMatcher, Exit) \
	X(FuzzyMatcher, MoveCursorV) \
	X(FuzzyMatcher, OpenFile) \
	\
	X(GrepOpen, Exit) \
	X(GrepOpen, MoveCursorV) \
	X(GrepOpen, OpenFile) \
	\
	X(Main, EnterLayoutMode) \
	X(Main, ExitLayoutMode) \
	X(Main, ExpandPanesX) \
	X(Main, ExpandPanesY) \
	\
	X(Main, FocusPaneRelX) \
	X(Main, FocusPaneRelY) \
	X(Main, OpenFileBrowser) \
	X(Main, FuzzyOpener) \
	X(Main, GrepOpen) \
	X(Main, Calculator) \
	X(Main, Terminal) \
	X(Main, SaveActiveTab) \
	X(Main, SaveAll) \
	X(Main, Quit) \
	X(Main, SaveQuit) \
	X(Main, QuitWithoutSave) \
	X(Main, LoadFile) \
	X(Main, NewEmptyBuffer) \
	X(Main, ReloadTab) \
	X(Main, CloseTab) \
	X(Main, SaveAndCloseTab) \
	X(Main, NextTab) \
	X(Main, PrevTab) \
	X(Main, MoveTabR) \
	X(Main, MoveTabL) \
	X(Main, SortTabs) \
	X(Main, GoToTab) \
	X(Main, SplitPane) \
	X(Main, MainMenu) \
	X(Main, ToggleGDBBreakpoint) \
	X(Main, FontNudgeLow) \
	X(Main, FontNudgeHigh) \
	X(Main, FontNudgeWidth) \
	X(Main, FontNudgeCenter) \
	
	
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



#define EXTERN_GUI_COMMAND_FLAG_LIST \
	X(scrollToCursor) \
	X(rehighlight) \
	X(resetCursorBlink) \
	X(undoSeqBreak) \
	X(hideMouse) \
	X(centerOnCursor) \



/*
#define X(a, b) a##Cmd_##b,
enum  {
	CUSTOMCMD_STARTVALUE = GUICMD_MAXVALUE,
	COMMANDTYPE_LIST
	CUSTOMCMD_MAXVALUE,
};
#undef X


#define X(a) CUSTOM_ELEM_TYPE_##a,
enum  {
	CUSTOMCMDELEM_STARTVALUE = 10000,
	COMMANDELEMTYPE_LIST
	CUSTOMCMDELEM_MAXVALUE,
};
#undef X
*/



/*
#define X(a) CMD_FLAG_ORD_##a,
enum {
	COMMANDFLAG_FIRSTVALUE = GUICMD_FLAG_ORD_MAXVALUE - 1,
	COMMANDFLAG_LIST
};
#undef X

#define X(a) CMD_FLAG_##a = 1 << CMD_FLAG_ORD_##a,
enum {
	COMMANDFLAG_LIST
};
#undef X
*/



#endif // __gpuedit_commands_h__
