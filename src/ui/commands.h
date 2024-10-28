#ifndef __gputk_commands_h__
#define __gputk_commands_h__


// EOL = End of Line
// SOL = Start of Line


// for application-supplied commands
#include "../commands.h"


#ifndef EXTERN_GUI_VK_LIST
	#define EXTERN_GUI_VK_LIST
#endif

enum {
#define X(a, ...) GUI_CMD_GUISYM_##a,
	EXTERN_GUI_VK_LIST
#undef X
};


#ifndef EXTERN_GUI_ELEMENT_LIST
	#define EXTERN_GUI_ELEMENT_LIST
#endif

#define GUI_ELEMENT_LIST \
	X(SYSTEM) \
	X(Edit) \
	X(FileViewer) \
	\
	X(FileRow) \
	EXTERN_GUI_ELEMENT_LIST
	
	

	
#ifndef EXTERN_GUI_COMMAND_LIST
	#define EXTERN_GUI_COMMAND_LIST
#endif

#define GUI_COMMAND_LIST \
	X(SYSTEM, Nop) \
	X(Edit,   Copy) \
	X(Edit,   Cut) \
	X(Edit,   Paste) \
	X(Edit,   MoveCursorH) \
	X(Edit,   GrowSelectionH) \
	X(Edit,   GoToStart) \
	X(Edit,   GoToEnd) \
	X(Edit,   GrowSelToStart) \
	X(Edit,   GrowSelToEnd) \
	X(Edit,   InsertChar) \
	X(Edit,   Delete) \
	X(Edit,   Backspace) \
	X(Edit,   Clear) \
	X(Edit,   Undo) \
	X(Edit,   Redo) \
	EXTERN_GUI_COMMAND_LIST




#ifndef EXTERN_GUI_COMMAND_FLAG_LIST
	#define EXTERN_GUI_COMMAND_FLAG_LIST
#endif

#define GUI_COMMAND_FLAG_LIST \
	X(noSuppressEvent) \
	EXTERN_GUI_COMMAND_FLAG_LIST
	

#ifndef EXTERN_GUI_COMMAND_MODE_FLAG_LIST
	#define EXTERN_GUI_COMMAND_MODE_FLAG_LIST
#endif

#define GUI_COMMAND_MODE_FLAG_LIST \
	X(isOverlay) \
	X(noSuppressEvent) \
	EXTERN_GUI_COMMAND_MODE_FLAG_LIST
	




#define X(a) GUIELEMENT_##a,
enum GUIElementType {
	GUIELEMENT_NULL = 0,
	GUI_ELEMENT_LIST
};
#undef X


#define X(a, b) GUICMD_##a##_##b,
enum GUICmdType {
	GUICMD_NULL = 0,
	GUICMD_META,
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


#define X(a) GUICMD_MODE_FLAG_ORD_##a,
enum {
	GUI_COMMAND_MODE_FLAG_LIST
	GUICMD_MODE_FLAG_ORD_MAXVALUE,
};
#undef X

#define X(a) GUICMD_MODE_FLAG_##a = 1 << GUICMD_MODE_FLAG_ORD_##a,
enum {
	GUI_COMMAND_MODE_FLAG_LIST
};
#undef X


enum {
	GUI_CMD_SRC_NONE = 0,
	GUI_CMD_SRC_KEY,
	GUI_CMD_SRC_CLICK,
	GUI_CMD_SRC_FOCUS,
	GUI_CMD_SRC_BLUR,
};




typedef struct GUI_Cmd {
	uint16_t src_type; // key, mouse, focus, etc
	uint16_t element;
	uint16_t sub_elem;
	uint16_t mode;
	
	int16_t setMode;
	int16_t clearMode;
	uint32_t mods;
	
	int64_t keysym; // or mouse buttons
	
	uint32_t cmd;
	uint32_t flags;
	
	int16_t paneTargeter;
	
	union {
		long amt;
		char* str;
		char** pstr;
		struct GUI_Cmd* metaCmds; // terminated by an entry with src_type=0
	};
} GUI_Cmd;

typedef struct GUI_CmdModeInfo {
	int id;
	int cascade; // -1 for no cascading
	
	char overlayBitIndex;
	
	char* name;
	uint64_t flags;
} GUI_CmdModeInfo;

typedef struct GUI_CmdList {
	VEC(GUI_Cmd) mods[16]; // ctl, alt, shift, tux
} GUI_CmdList;


typedef struct GUI_CmdElementInfo {
	uint16_t id;
	char* name;
	uint8_t hasDefaultKeystrokeCmd;
	GUI_Cmd defaultKeystrokeCmd;
	HT(uint32_t) nameLookup;
} GUI_CmdElementInfo;

typedef struct GUI_CmdModeState {
	int mode;
	GUI_CmdModeInfo* modeInfo;
	
	uint64_t overlays;
	uint64_t curFlags;
} GUI_CmdModeState;

struct GUIEvent;
typedef struct GUIEvent GUIEvent;
struct GUIManager;
typedef struct GUIManager GUIManager;
struct GUIHeader;
typedef struct GUIHeader GUIHeader;


#define GUI_CMD_RATSYM(btn, reps) ((1 << 30) | ((reps & 0xff) << 15) | (btn & 0xff))
#define GUI_CMD_EXTSYM(a) ((1 << 29) | (a))

GUI_Cmd* Commands_ProbeCommand(GUIManager* gm, int elemType, GUIEvent* gev, GUI_CmdModeState* st, size_t* numCmds);
GUI_Cmd* Commands_ProbeCommandMode(GUIManager* gm, int elemType, GUIEvent* gev, int mode, size_t* numCmds);
GUI_Cmd* Commands_ProbeSubCommand(GUIManager* gm, int sub_elem, GUIEvent* gev);
void Commands_UpdateModes(GUIManager* gm, GUI_CmdModeState* st, GUI_Cmd* cmd, size_t numCmds);
GUI_CmdModeInfo* Commands_GetModeInfo(GUIManager* gm, int id);
GUI_CmdModeInfo* Commands_GetOverlay(GUIManager* gm, int bitIndex);
int Commands_GetModeID(GUIManager* gm, char* name);
int16_t Commands_GetPaneTargeter(GUIManager* gm, char* defStr);

GUI_CmdList* Commands_SeparateCommands(GUI_Cmd* in);


int GUIManager_AddCommand(GUIManager* gm, char* elemname, char* name, uint32_t id);
int GUIManager_AddCommandElement(GUIManager* gm, char* name, uint16_t id);
uint16_t GUIManager_AddCommandMode(GUIManager* gm, char* name);
uint32_t GUIManager_AddCommandFlag(GUIManager* gm, char* name);

void CommandList_loadJSON(GUIManager* gm, json_value_t* root);
void CommandList_loadKeyConfigJSON(GUIManager* gm, json_value_t* root);
void CommandList_loadJSONFile(GUIManager* gm, char* path);


void GUIManager_InitCommands(GUIManager* gm);

#endif //__gputk_commands_h__
