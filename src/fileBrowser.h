#ifndef __gpuedit_fileBrowser_h__
#define __gpuedit_fileBrowser_h__

#include "ui/gui.h"
#include "msg.h"

#include <sys/stat.h>
#include <stdatomic.h>

typedef struct FileBrowserEntry {
	char* name;
	char* ext; // a pointer into above
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
	
} FileBrowserEntry;


typedef struct FileBrowserColumnInfo {
	int type;
	float width;
} FileBrowserColumnInfo;


typedef struct FileBrowserEntryList {
	VEC(FileBrowserEntry) files;
	VEC(FileBrowserColumnInfo) columnInfo;
	
	float lineHeight;
	float leftMargin;
	float headerHeight;
	
	intptr_t cursorIndex;
	intptr_t numSelected;
	intptr_t scrollOffset;
	float scrollPos;
	
	int linesOnScreen;
	
} FileBrowserEntryList;



// the full-functioning dialog contents
typedef struct FileBrowser {
	
	GUIManager* gm;
	MessagePipe* upstream;
	
	GeneralSettings* gs;
	char* acceptButtonLabel;
	
	float lineHeight;
	float leftMargin;
	float headerHeight;
	// iconsize, font params, etc
	
	
	GUIWindow* scrollbar;
	float sbMinHeight;
	
	int linesPerScrollWheel;

	char* curDir;
	
	FileBrowserEntryList entries;
	
	unsigned int isRootDir       : 1;
	unsigned int treeView        : 1;
	unsigned int showPathBar     : 1;
	unsigned int editablePathBar : 1;
	unsigned int multipleSelect  : 1;
	unsigned int onlyExisting    : 1;
	unsigned int allowDirs       : 1;
	unsigned int onlyDirs        : 1;
	unsigned int showNewFolder   : 1;
	unsigned int showNewFile     : 1;
	unsigned int noEmptyAccept   : 1;
	
	// click/dbl/right actions
	
	void* onChooseData;
	void (*onChoose)(void*, char**, intptr_t);
	
	
} FileBrowser;

FileBrowser* FileBrowser_New(GUIManager* gm, Settings* s, MessagePipe* mp, char* path);

void FileBrowser_Render(FileBrowser* w, GUIManager* gm, Vector2 tl, Vector2 sz, PassFrameParams* pfp);

void FileBrowser_Destroy(FileBrowser* w);
void FileBrowser_UnselectAll(FileBrowser* w);

void FileBrowser_ProcessCommand(FileBrowser* w, GUI_Cmd* cmd);

void FileBrowser_Refresh(FileBrowser* w);



void FileBrowser_FreeEntryList(FileBrowserEntry* e, intptr_t sz);
FileBrowserEntry* FileBrowser_CollectSelected(FileBrowser* w, intptr_t* szOut);

void FileBrowser_Autoscroll(FileBrowser* w);


#endif // __gpuedit_fileBrowser_h__
