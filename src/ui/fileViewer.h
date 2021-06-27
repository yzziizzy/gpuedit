#ifndef __gputk_fileBrowser_h__
#define __gputk_fileBrowser_h__

#include <sys/stat.h>
#include <stdatomic.h>

typedef struct GUIFileBrowserEntry {
	char* name;
	char* fullPath;
// 	int type;
	mode_t perms;
	
	uid_t ownerID;
	gid_t groupID;
	
	int64_t atime; // unix timestamps
	int64_t mtime;
	int64_t ctime;
	
	uint64_t size;
	uint64_t sizeOnDisk;
	
	// ext/mime
	
	// dup'd
	char* humanSize;
	char* humanSizeOnDisk;
	char* atimeStr;
	char* mtimeStr;
	char* ctimeStr;

	// externally owned strings
	char* ownerName;
	char* groupName;

		
	unsigned int type : 4; // target type: unknown, reg, dir
	unsigned int isSymlink : 1;
	unsigned int hasStats: 1;
	unsigned int isAlreadyOpen : 1;
	unsigned int isSelected : 1;
	unsigned int isRoot: 1; // no parent folder
	
} GUIFileBrowserEntry;




typedef struct GUIFileBrowserColumnInfo {
	int type;
	float width;
} GUIFileBrowserColumnInfo;


// just the bare file/folder tree.
typedef struct GUIFileBrowserControl {
	GUIHeader header;
	
	float lineHeight;
	float leftMargin;
	float headerHeight;
	// iconsize, font params, etc
	
	VEC(GUIFileBrowserColumnInfo) columnInfo;
	
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
