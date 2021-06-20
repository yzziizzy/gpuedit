#ifndef __gputk_commands_h__
#define __gputk_commands_h__


// EOL = End of Line
// SOL = Start of Line

#define GUI_ELEMENT_LIST \
	X(Edit) \
	X(FileBrowser) \



// The element name will need a _ prepended in the commands.json file
#define GUI_COMMAND_LIST \
	X(Edit, Copy) \
	X(Edit, Cut) \
	X(Edit, Paste) \
	X(Edit, MoveCursorH) \
	X(Edit, GrowSelectionH) \
	X(Edit, GoToStart) \
	X(Edit, GoToEnd) \
	X(Edit, GrowSelToStart) \
	X(Edit, GrowSelToEnd) \
	X(Edit, InsertChar) \
	X(Edit, Delete) \
	X(Edit, Backspace) \
	X(Edit, Clear) \
	X(Edit, Undo) \
	X(Edit, Redo) \
	X(FileBrowser, MoveCursorV) \
	X(FileBrowser, ParentDir) \
	X(FileBrowser, SmartOpen) \
	X(FileBrowser, ToggleSelect) \
	X(FileBrowser, OpenUnderCursor) \
	
	
#define GUI_COMMAND_FLAG_LIST \
	X(noSuppressEvent) \
	
	

#define X(a) GUIELEMENT_##a,
enum GUIElementType {
	GUIELEMENT_NULL = 0,
	GUI_ELEMENT_LIST
};
#undef X


#define X(a, b) GUICMD_##a##_##b,
enum GUICmdType {
	GUICMD_NULL = 0,
	GUI_COMMAND_LIST
	
	GUICMD_MAXVALUE,
};
#undef X


#define X(a) GUICMD_FLAG_ORD_##a,
enum {
	GUI_COMMAND_FLAG_LIST
	GUICMD_FLAG_ORD_MAXVALUE,
};
#undef X

#define X(a) GUICMD_FLAG_##a = 1 << GUICMD_FLAG_ORD_##a,
enum {
	GUI_COMMAND_FLAG_LIST
};
#undef X





typedef struct GUI_Cmd {
	uint16_t element;
	uint16_t mode;
	uint32_t mods;
	int32_t keysym;
	uint32_t cmd;
	uint32_t flags;
	union {
		long amt;
		char* str;
		char** pstr;
	};
} GUI_Cmd;


typedef struct GUI_CmdList {
	VEC(GUI_Cmd) mods[16]; // ctl, alt, shift, tux
} GUI_CmdList;


typedef struct GUI_CmdElementInfo {
	uint16_t id;
	uint8_t hasDefaultKeystrokeCmd;
	GUI_Cmd defaultKeystrokeCmd;
	HT(uint32_t) nameLookup;
} GUI_CmdElementInfo;


struct GUIEvent;
typedef struct GUIEvent GUIEvent;
struct GUIManager;
typedef struct GUIManager GUIManager;
struct GUIHeader;
typedef struct GUIHeader GUIHeader;


GUI_Cmd* Commands_ProbeCommand(GUIHeader* gh, GUIEvent* gev);

GUI_CmdList* Commands_SeparateCommands(GUI_Cmd* in);


int GUIManager_AddCommand(GUIManager* gm, char* elemname, char* name, uint32_t id);
int GUIManager_AddCommandElement(GUIManager* gm, char* name, uint16_t id);
uint16_t GUIManager_AddCommandMode(GUIManager* gm, char* name);
uint32_t GUIManager_AddCommandFlag(GUIManager* gm, char* name);

void CommandList_loadJSON(GUIManager* gm, json_value_t* root);
void CommandList_loadKeyConfigJSON(GUIManager* gm, json_value_t* root);
void CommandList_loadJSONFile(GUIManager* gm, char* path);


#endif //__gputk_commands_h__
