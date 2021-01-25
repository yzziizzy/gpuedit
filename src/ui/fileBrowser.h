#ifndef __gputk_fileBrowser_h__
#define __gputk_fileBrowser_h__

#include <sys/stat.h>



typedef struct GUIFileBrowserEntry {
	char* name;
	char* fullPath;
// 	int type;
	mode_t perms;
	
	char* owner;
	uid_t ownerID;
	char* group;
	gid_t groupID;
	
	uint64_t size;
	
	
	
	unsigned int type : 4; // unknown, reg, dir, symlink
	unsigned int hasStats: 1;
	unsigned int isAlreadyOpen : 1;
	unsigned int isSelected : 1;
	unsigned int isRoot: 1; // no parent folder
	
} GUIFileBrowserEntry;


// just the bare file/folder tree.
typedef struct GUIFileBrowserControl {
	GUIHeader header;
	
	float lineHeight;
	float leftMargin;
	// iconsize, font params, etc
	
	GUIWindow* scrollbar;
	float sbMinHeight;
	intptr_t scrollOffset;
	
	intptr_t cursorIndex;
	intptr_t numSelected;
	
	int linesPerScrollWheel;

	char* curDir;
	
	VEC(GUIFileBrowserEntry) entries;
	
	unsigned int treeView       : 1;
	unsigned int onlyDirs       : 1;
	unsigned int multipleSelect : 1;
	// click/dbl/right actions
	
	void* onChooseData;
	void (*onChoose)(void*, char**, intptr_t);
	
} GUIFileBrowserControl;




GUIFileBrowserControl* GUIFileBrowserControl_New(GUIManager* gm, char* path);

void GUIFileBrowserControl_Refresh(GUIFileBrowserControl* w);

void GUIFileBrowserControl_FreeEntryList(GUIFileBrowserEntry* e, intptr_t sz);
GUIFileBrowserEntry* GUIFileBrowserControl_CollectSelected(GUIFileBrowserControl* w, intptr_t* szOut);

void GUIFileBrowserControl_Autoscroll(GUIFileBrowserControl* w);

#endif // __gputk_fileBrowser_h__
